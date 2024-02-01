//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2018-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file is part of p44lrgraphics.
//
//  p44lrgraphics is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  p44lrgraphics is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with p44lrgraphics. If not, see <http://www.gnu.org/licenses/>.
//

// File scope debugging options
// - Set ALWAYS_DEBUG to 1 to enable DBGLOG output even in non-DEBUG builds of this file
#define ALWAYS_DEBUG 0
// - set FOCUSLOGLEVEL to non-zero log level (usually, 5,6, or 7==LOG_DEBUG) to get focus (extensive logging) for this file
//   Note: must be before including "logger.hpp" (or anything that includes "logger.hpp")
#define FOCUSLOGLEVEL 0

#include "math.h"

#include "viewscroller.hpp"
#include "viewstack.hpp"

#if ENABLE_VIEWCONFIG
  #include "viewfactory.hpp"
#endif

using namespace p44;


// MARK: ===== ViewScroller


ViewScroller::ViewScroller() :
  mScrollStepX(0),
  mScrollStepY(0),
  mScrollSteps(0),
  mScrollStepInterval(Never),
  mNextScrollStepAt(Never),
  #ifdef __APPLE__
  mSyncScroll(false),
  #else
  mSyncScroll(true),
  #endif
  #if P44SCRIPT_FULL_SUPPORT
  mAlertEmpty(false),
  #endif
  mAutoPurge(false)
{
}


ViewScroller::~ViewScroller()
{
  if (mScrolledView) {
    mScrolledView->setParent(NULL);
  }
  mScrolledView.reset();
}


void ViewScroller::clear()
{
  stopAnimations();
  // just delegate
  if (mScrolledView) mScrolledView->clear();
}


MLMicroSeconds ViewScroller::step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow)
{
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil, aNow);
  if (mScrolledView) {
    updateNextCall(nextCall, mScrolledView->step(aPriorityUntil, aNow));
  }
  // scroll
  if (mScrollSteps!=0 && mScrollStepInterval>0) {
    // scrolling
    MLMicroSeconds next = mNextScrollStepAt-aNow; // time to next step
    if (next>0) {
      updateNextCall(nextCall, mNextScrollStepAt, aPriorityUntil, aNow); // scrolling has priority
    }
    else {
      // execute all step(s) pending
      // Note: will catch up in case step() was not called often enough
      while (next<=0) {
        if (next<-10*MilliSecond) {
          LOG(LOG_DEBUG, "ViewScroller: Warning: precision below 10mS: %lld µS after precise time", -next);
        }
        // perform step (using underlying p44view's scroll)
        mScrollX += FP_FROM_DBL(mScrollStepX);
        mScrollY += FP_FROM_DBL(mScrollStepY);
        recalculateScrollDependencies();
        makeDirty();
        // limit coordinate increase in wraparound scroll view
        // Note: might need multiple rounds after scrolled view's content size has changed to get back in range
        // Note: for this to work it is essential that mScrollX/Y is applied in content coordinates as
        //       the last step of all transforms (which is the case in p44View colorInFrameAt() implementation.
        if (mScrolledView) {
          FramingMode fm = mScrolledView->getFramingMode();
          PixelPoint svfsz = mScrolledView->getFrameSize();
          if (fm&repeatX) {
            if ((fm & repeatXmax) && svfsz.x>0) while (mScrollX>=FP_FROM_INT(svfsz.x)) mScrollX -= FP_FROM_INT(svfsz.x);
            if ((fm & repeatXmin) && svfsz.x>0) while (mScrollX<0) mScrollX += FP_FROM_INT(svfsz.x);
          }
          if (fm&repeatY) {
            if ((fm & repeatYmax) && svfsz.y>0) while (mScrollY>=FP_FROM_INT(svfsz.y)) mScrollY -= FP_FROM_INT(svfsz.y);
            if ((fm & repeatYmin) && svfsz.y>0) while (mScrollY<0) mScrollY += FP_FROM_INT(svfsz.y);
          }
        }
        // check scroll end
        if (mScrollSteps>0) {
          mScrollSteps--;
          if (mScrollSteps==0) {
            // scroll ends here
            if (mScrollCompletedCB) {
              SimpleCB cb = mScrollCompletedCB;
              mScrollCompletedCB = NoOP;
              cb(); // may set up another callback already
            }
            break;
          }
        }
        // advance to next step
        if (mSyncScroll) {
          // try to catch up
          next += mScrollStepInterval;
          mNextScrollStepAt += mScrollStepInterval;
          if (next<0) {
            LOG(LOG_DEBUG, "ViewScroller: needs to catch-up steps -> call step() more often!");
          }
        }
        else {
          // time next from now, even if we are (possibly much) late
          next = mScrollStepInterval;
          mNextScrollStepAt = aNow+mScrollStepInterval;
        }
        updateNextCall(nextCall, mNextScrollStepAt, aPriorityUntil, aNow); // scrolling has priority
      } // while catchup
      if ((
        mNeedContentCB
        #if P44SCRIPT_FULL_SUPPORT
        || (hasSinks() && mAlertEmpty)
        #endif
        ) && mScrolledView
      ) {
        // check if we need more content (i.e. scrolled view does not cover frame of the scroller any more)
        if (needsContent()) {
          if (FOCUSLOGENABLED) {
            PixelRect sf = mScrolledView->getFrame();
            FOCUSLOG("*** Scroller '%s' needs new content: scrollX = %.2f, scrollY=%.2f, frame=(%d,%d,%d,%d) scrolledframe=(%d,%d,%d,%d)",
              getLabel().c_str(),
              FP_DBL_VAL(mScrollX), FP_DBL_VAL(mScrollY),
              mFrame.x, mFrame.y, mFrame.dx, mFrame.dy,
              sf.x, sf.y, sf.dx, sf.dy
            );
          }
          #if P44SCRIPT_FULL_SUPPORT
          if (mAlertEmpty) {
            mAlertEmpty = false; // must re-arm before alerting again
            sendEvent(new P44Script::ContentNeededObj(this)); // one-shot
          }
          #endif
          if (mAutoPurge) {
            // remove views no longer in sight
            purgeScrolledOut();
            // re-adjust scroll offset to prevent getting out of range over time
            // (even if that would take 2.7yrs @ 20mS/Pixel fast scroll)
            // FIXME: do it once we need it having run longer than 2 years ;-)
          }
        }
      }
    }
  }
  return nextCall;
}


PixelPoint ViewScroller::remainingPixelsToScroll()
{
  PixelPoint rem = { INT_MAX, INT_MAX }; // assume forever
  if (mScrolledView) {
    FramingMode fm = mScrolledView->getFramingMode();
    PixelRect sf = mScrolledView->getFrame();
    if ((fm & repeatXmax)==0 && mScrollStepX>0) {
      rem.x = (sf.x+sf.dx) - (int)(FP_INT_VAL(mScrollX)+mFrame.dx);
    }
    if ((fm & repeatXmin)==0 && mScrollStepX<0) {
      rem.x = FP_INT_VAL(mScrollX) - sf.x;
    }
    if ((fm & repeatYmax)==0 && mScrollStepY>0) {
      rem.y = (sf.y+sf.dy) - (int)(FP_INT_VAL(mScrollY)+mFrame.dy);
    }
    if ((fm & repeatYmin)==0 && mScrollStepY<0) {
      rem.y = FP_INT_VAL(mScrollY) - sf.y;
    }
  }
  return rem;
}


bool ViewScroller::needsContent()
{
  PixelPoint rem = remainingPixelsToScroll();
  return rem.x<0 || rem.y<0;
}


MLMicroSeconds ViewScroller::remainingScrollTime()
{
  PixelPoint rem = remainingPixelsToScroll();
  if (rem.x==INT_MAX && rem.y==INT_MAX) return Infinite; // no limit
  int steps = INT_MAX;
  if (mScrollStepX>0) {
    int s = rem.x/mScrollStepX;
    if (s<steps) steps = s;
  }
  if (mScrollStepY>0) {
    int s = rem.y/mScrollStepY;
    if (s<steps) steps = s;
  }
  if (steps==INT_MAX) return Infinite; // we can NOT scroll infinitely ;-)
  return steps*mScrollStepInterval;
}



bool ViewScroller::isDirty()
{
  if (inherited::isDirty()) return true; // dirty anyway
  if (mScrolledView && reportDirtyChilds()) return mScrolledView->isDirty();
  return false;
}


void ViewScroller::updated()
{
  inherited::updated();
  if (mScrolledView) mScrolledView->updated();
}


PixelColor ViewScroller::contentColorAt(PixelPoint aPt)
{
  if (!mScrolledView) return transparent;
  // Note: underlying p44View does the actual scrolling, just return scrolled view as content
  return mScrolledView->colorAt(aPt);
}


void ViewScroller::startScroll(double aStepX, double aStepY, MLMicroSeconds aInterval, bool aRoundOffsets, long aNumSteps, MLMicroSeconds aStartTime, SimpleCB aCompletedCB)
{
  mScrollStepX = aStepX;
  mScrollStepY = aStepY;
  if (aRoundOffsets) {
    if (mScrollStepX) mScrollX = FP_FROM_INT(FP_INT_FLOOR(mScrollX));
    if (mScrollStepY) mScrollY = FP_FROM_INT(FP_INT_FLOOR(mScrollY));
  }
  mScrollStepInterval = aInterval;
  mScrollSteps = aNumSteps;
  MLMicroSeconds now = MainLoop::now();
  // do not allow setting scroll step into the past, as this would cause massive catch-up
  mNextScrollStepAt = aStartTime==Never || aStartTime<now ? now : aStartTime;
  mScrollCompletedCB = aCompletedCB;
}


void ViewScroller::stopScroll()
{
  // no more steps
  mScrollSteps = 0;
}

void ViewScroller::purgeScrolledOut()
{
  ViewStackPtr vs = boost::dynamic_pointer_cast<ViewStack>(mScrolledView);
  if (vs) {
    PixelPoint rem = remainingPixelsToScroll();
    if (rem.x==INT_MAX) rem.x = 0;
    if (rem.y==INT_MAX) rem.y = 0;
    vs->purgeViews(mFrame.dx+rem.x, mFrame.dy+rem.y, false);
  }
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr ViewScroller::configureView(JsonObjectPtr aViewConfig)
{
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    #if !ENABLE_P44SCRIPT
    // Legacy implementation when we have no properties
    // view
    if (aViewConfig->get("offsetx", o)) {
      setOffsetX(o->doubleValue());
    }
    if (aViewConfig->get("offsety", o)) {
      setOffsetY(o->doubleValue());
    }
    if (aViewConfig->get("syncscroll", o)) {
      mSyncScroll = o->boolValue();
    }
    if (aViewConfig->get("autopurge", o)) {
      mAutoPurge = o->boolValue();
    }
    #endif // !ENABLE_P44SCRIPT
    // directly factory-configuring a scrolledview
    if (aViewConfig->get("scrolledview", o)) {
      err = p44::createViewFromConfig(o, mScrolledView, this);
      makeDirty();
    }
    // pseudo "properties" for starting scroll
    double stepX = mScrollStepX;
    double stepY = mScrollStepY;
    MLMicroSeconds interval = mScrollStepInterval>0 ? mScrollStepInterval : 50*MilliSecond;
    long numSteps = -1;
    bool doStart = false;
    if (aViewConfig->get("step_x", o)) {
      stepX = o->doubleValue();
      doStart = true;
    }
    if (aViewConfig->get("step_y", o)) {
      stepY = o->doubleValue();
      doStart = true;
    }
    if (aViewConfig->get("interval", o)) {
      interval = o->doubleValue()*Second;
      doStart = true;
    }
    if (aViewConfig->get("steps", o)) {
      numSteps = o->int32Value();
      if (numSteps==0) stopScroll();
      else doStart = true;
    }
    if (doStart) {
      startScroll(stepX, stepY, interval, true, numSteps);
    }
    if (aViewConfig->get("purgenow", o)) {
      if (o->boolValue()) {
        purgeScrolledOut();
      }
    }
  }
  return err;
}


P44ViewPtr ViewScroller::findView(const string aLabel)
{
  if (mScrolledView) {
    P44ViewPtr view = mScrolledView->findView(aLabel);
    if (view) return view;
  }
  return inherited::findView(aLabel);
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

// legacy implementation

JsonObjectPtr ViewScroller::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  if (mScrolledView) status->add("scrolledview", mScrolledView->viewStatus());
  status->add("syncscroll", JsonObject::newBool(mSyncScroll));
  status->add("autopurge", JsonObject::newBool(mAutoPurge));
  status->add("stepx", JsonObject::newDouble(getStepX()));
  status->add("stepy", JsonObject::newDouble(getStepY()));
  status->add("interval", JsonObject::newDouble((double)mScrollStepInterval/Second));
  status->add("steps", JsonObject::newInt64(mScrollSteps));
  PixelPoint r = remainingPixelsToScroll();
  status->add("remainingx", JsonObject::newInt64(r.x));
  status->add("remainingy", JsonObject::newInt64(r.y));
  status->add("remainingtime", JsonObject::newDouble((double)remainingScrollTime()/Second));
  return status;
}

#endif // ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

// MARK: - script support

#if ENABLE_P44SCRIPT

using namespace P44Script;


ScriptObjPtr ViewScroller::newViewObj()
{
  // base class with standard functionality
  return new ScrollerViewObj(this);
}


#if P44SCRIPT_FULL_SUPPORT

ContentNeededObj::ContentNeededObj(ViewScrollerPtr aScroller) :
  inherited(false),
  mScroller(aScroller)
{
}

void ContentNeededObj::deactivate()
{
  mScroller.reset();
  inherited::deactivate();
}


string ContentNeededObj::getAnnotation() const
{
  return "scroller event";
}

TypeInfo ContentNeededObj::getTypeInfo() const
{
  return inherited::getTypeInfo()|oneshot|freezable;
}


bool ContentNeededObj::isEventSource() const
{
  return mScroller.get(); // yes if it exists
}


void ContentNeededObj::registerForFilteredEvents(EventSink* aEventSink, intptr_t aRegId)
{
  if (mScroller) mScroller->registerForEvents(aEventSink, aRegId); // no filtering
}


double ContentNeededObj::doubleValue() const
{
  return mScroller->needsContent() ? 1 : 0;
}



// empty()
static void empty_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  f->finish(new ContentNeededObj(v->scroller()));
}


// remainingpixels()
static void remainingpixels_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  PixelPoint rem = v->scroller()->remainingPixelsToScroll();
  ObjectValue* r = new ObjectValue();
  r->setMemberByName("x", rem.x==INT_MAX ? new ScriptObj() : new IntegerValue(rem.x));
  r->setMemberByName("y", rem.y==INT_MAX ? new ScriptObj() : new IntegerValue(rem.y));
  f->finish(r);
}


// remainingtime()
static void remainingtime_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  MLMicroSeconds r = v->scroller()->remainingScrollTime();
  f->finish(r==Infinite ? new ScriptObj : new NumericValue((double)r/Second));
}


// purge()
static void purge_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  v->scroller()->purgeScrolledOut();
  f->finish(v);
}


// alertempty(enable)
FUNC_ARG_DEFS(alertempty, { numeric } );
static void alertempty_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  v->scroller()->setAlertEmpty(f->arg(0)->boolValue());
  f->finish(v);
}


// startscroll(stepX, stepY, interval, roundoffsets, numsteps=null, syncstart=0)
FUNC_ARG_DEFS(startscroll, { numeric }, { numeric }, { numeric }, { numeric|optionalarg }, { numeric|optionalarg }, { numeric|optionalarg } );
static void startscroll_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  double stepX = f->arg(0)->doubleValue();
  double stepY = f->arg(1)->doubleValue();
  MLMicroSeconds interval = f->arg(2)->doubleValue()*Second;
  bool roundoffsets = f->arg(3)->undefined() || f->arg(3)->boolValue();
  long numsteps = f->arg(4)->defined() ? f->arg(4)->intValue() : -1; // default to -1 = forever
  MLMicroSeconds starttime = f->arg(5)->doubleValue()*Second; // optional synchronized start time, default to right now==0
  if (starttime>0 && starttime<Day) {
    // roundup interval for synchronized start
    starttime = ((MainLoop::now()+starttime-1)/starttime) * starttime;
  }
  v->scroller()->startScroll(stepX, stepY, interval, roundoffsets, numsteps, starttime);
  f->finish(v); // allow chaining
}


// stopscroll()
static void stopscroll_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  v->scroller()->stopScroll();
  f->finish(v);
}

#endif // P44SCRIPT_FULL_SUPPORT


#define ACCESSOR_CLASS ViewScroller

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  ViewScrollerPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<ScrollerViewObj*>(aParentObj.get())->view().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_BOOL(SyncScroll)
ACC_IMPL_BOOL(AutoPurge)
ACC_IMPL_DBL(StepX)
ACC_IMPL_DBL(StepY)
ACC_IMPL_DBL(ScrollStepIntervalS)
ACC_IMPL_RO_INT(RemainingSteps)

static ScriptObjPtr access_ScrolledView(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite)
{
  if (!aToWrite) return aView.getScrolledView() ? aView.getScrolledView()->newViewObj() : new AnnotatedNullValue("no scrolled view set");
  ErrorPtr err;
  P44ViewPtr scrolledview = P44View::viewFromScriptObj(aToWrite, err);
  if (Error::isOK(err) && aView.isParentOrThis(scrolledview)) {
    err = ScriptError::err(ScriptError::Invalid, "Scrolled view must not be a parent of the scroller");
  }
  if (Error::ok(err)) {
    aView.setScrolledView(scrolledview);
  }
  else {
    aToWrite = new ErrorValue(err);
  }
  return aToWrite; /* reflect back to indicate writable or error */ \
}


static const BuiltinMemberDescriptor scrollerFunctions[] = {
  #if P44SCRIPT_FULL_SUPPORT
  FUNC_DEF_NOARG(empty, executable|objectvalue),
  FUNC_DEF_NOARG(remainingpixels, executable|objectvalue),
  FUNC_DEF_NOARG(remainingtime, executable|numeric),
  FUNC_DEF_W_ARG(alertempty, executable|objectvalue),
  FUNC_DEF_NOARG(purge, executable|objectvalue),
  FUNC_DEF_W_ARG(startscroll, executable|objectvalue),
  FUNC_DEF_NOARG(stopscroll, executable|objectvalue),
  #endif
  // property accessors
  ACC_DECL("scrolledview", objectvalue|lvalue, ScrolledView),
  ACC_DECL("syncscroll", numeric|lvalue, SyncScroll),
  ACC_DECL("autopurge", numeric|lvalue, AutoPurge),
  ACC_DECL("step_x", numeric|lvalue, StepX),
  ACC_DECL("step_y", numeric|lvalue, StepY),
  ACC_DECL("interval", numeric|lvalue, ScrollStepIntervalS),
  ACC_DECL("steps", numeric, RemainingSteps),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedScrollerMemberLookupP = NULL;

ScrollerViewObj::ScrollerViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  if (sharedScrollerMemberLookupP==NULL) {
    sharedScrollerMemberLookupP = new BuiltInMemberLookup(scrollerFunctions);
    sharedScrollerMemberLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedScrollerMemberLookupP);
}

#endif // ENABLE_P44SCRIPT
