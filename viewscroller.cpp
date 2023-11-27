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
#define FOCUSLOGLEVEL 7

#include "viewscroller.hpp"
#include "viewstack.hpp"

#if ENABLE_VIEWCONFIG
  #include "viewfactory.hpp"
#endif

using namespace p44;


// MARK: ===== ViewScroller


ViewScroller::ViewScroller() :
  mScrollOffsetX_milli(0),
  mScrollOffsetY_milli(0),
  mScrollStepX_milli(0),
  mScrollStepY_milli(0),
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


void ViewScroller::setOffsetX(double aOffsetX)
{
  mScrollOffsetX_milli = aOffsetX*1000l;
  makeDirty();
}


void ViewScroller::setOffsetY(double aOffsetY)
{
  mScrollOffsetY_milli = aOffsetY*1000l;
  makeDirty();
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
        // perform step
        mScrollOffsetX_milli += mScrollStepX_milli;
        mScrollOffsetY_milli += mScrollStepY_milli;
        makeDirty();
        // limit coordinate increase in wraparound scroll view
        // Note: might need multiple rounds after scrolled view's content size has changed to get back in range
        if (mScrolledView) {
          WrapMode wm = mScrolledView->getWrapMode();
          PixelPoint svfsz = mScrolledView->getFrameSize();
          if (wm&wrapX) {
            long fsx_milli = svfsz.x*1000;
            while ((wm&wrapXmax) && mScrollOffsetX_milli>=fsx_milli && fsx_milli>0)
              mScrollOffsetX_milli-=fsx_milli;
            while ((wm&wrapXmin) && mScrollOffsetX_milli<0 && fsx_milli>0)
              mScrollOffsetX_milli+=fsx_milli;
          }
          if (wm&wrapY) {
            long fsy_milli = svfsz.y*1000;
            while ((wm&wrapYmax) && mScrollOffsetY_milli>=fsy_milli && fsy_milli>0)
              mScrollOffsetY_milli-=fsy_milli;
            while ((wm&wrapYmin) && mScrollOffsetY_milli<0 && fsy_milli>0)
              mScrollOffsetY_milli+=fsy_milli;
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
              (double)mScrollOffsetX_milli/1000, (double)mScrollOffsetY_milli/1000,
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
    WrapMode w = mScrolledView->getWrapMode();
    PixelRect sf = mScrolledView->getFrame();
    if ((w&wrapXmax)==0 && mScrollStepX_milli>0) {
      rem.x = (sf.x+sf.dx) - (int)(mScrollOffsetX_milli/1000+mFrame.dx);
    }
    if ((w&wrapXmin)==0 && mScrollStepX_milli<0) {
      rem.x = (int)(mScrollOffsetX_milli/1000) - sf.x;
    }
    if ((w&wrapYmax)==0 && mScrollStepY_milli>0) {
      rem.y = (sf.y+sf.dy) - (int)(mScrollOffsetY_milli/1000+mFrame.dy);
    }
    if ((w&wrapYmin)==0 && mScrollStepY_milli<0) {
      rem.y = (int)(mScrollOffsetY_milli/1000) - sf.y;
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
  if (mScrollStepX_milli>0) {
    int s = rem.x*1000/mScrollStepX_milli;
    if (s<steps) steps = s;
  }
  if (mScrollStepY_milli>0) {
    int s = rem.y*1000/mScrollStepY_milli;
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
  // Note: implementation aims to be efficient at integer scroll offsets in either or both directions
  int sampleOffsetX = (int)((mScrollOffsetX_milli+(mScrollOffsetX_milli>0 ? 500 : -500))/1000);
  int sampleOffsetY = (int)((mScrollOffsetY_milli+(mScrollOffsetY_milli>0 ? 500 : -500))/1000);
  int subSampleOffsetX = 1;
  int subSampleOffsetY = 1;
  int outsideWeightX = (int)((mScrollOffsetX_milli-(long)sampleOffsetX*1000)*255/1000);
  if (outsideWeightX<0) { outsideWeightX *= -1; subSampleOffsetX = -1; }
  int outsideWeightY = (int)((mScrollOffsetY_milli-(long)sampleOffsetY*1000)*255/1000);
  if (outsideWeightY<0) { outsideWeightY *= -1; subSampleOffsetY = -1; }
  sampleOffsetX += aPt.x;
  sampleOffsetY += aPt.y;
  PixelColor samp = mScrolledView->colorAt({sampleOffsetX, sampleOffsetY});
  if (outsideWeightX!=0) {
    // X Subsampling (and possibly also Y, checked below)
    mixinPixel(samp, mScrolledView->colorAt({sampleOffsetX+subSampleOffsetX, sampleOffsetY}), outsideWeightX);
    // check if ALSO parts from other pixels in Y direction needed
    if (outsideWeightY!=0) {
      // subsample the Y side neigbours
      PixelColor neighbourY = mScrolledView->colorAt({sampleOffsetX, sampleOffsetY+subSampleOffsetY});
      mixinPixel(neighbourY, mScrolledView->colorAt({sampleOffsetX+subSampleOffsetX, sampleOffsetY+subSampleOffsetY}), outsideWeightX);
      // combine with Y main
      mixinPixel(samp, neighbourY, outsideWeightY);
    }
  }
  else if (outsideWeightY!=0) {
    // only Y subsampling
    mixinPixel(samp, mScrolledView->colorAt({sampleOffsetX, sampleOffsetY+subSampleOffsetY}), outsideWeightY);
  }
  return samp;
}


void ViewScroller::startScroll(double aStepX, double aStepY, MLMicroSeconds aInterval, bool aRoundOffsets, long aNumSteps, MLMicroSeconds aStartTime, SimpleCB aCompletedCB)
{
  mScrollStepX_milli = aStepX*1000;
  mScrollStepY_milli = aStepY*1000;
  if (aRoundOffsets) {
    if (mScrollStepX_milli) mScrollOffsetX_milli = (mScrollOffsetX_milli+(mScrollStepX_milli>>1))/mScrollStepX_milli*mScrollStepX_milli;
    if (mScrollStepY_milli) mScrollOffsetY_milli = (mScrollOffsetY_milli+(mScrollStepY_milli>>1))/mScrollStepY_milli*mScrollStepY_milli;
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
    // view
    if (aViewConfig->get("scrolledview", o)) {
      err = p44::createViewFromConfig(o, mScrolledView, this);
      makeDirty();
    }
    // offsets
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
    // scroll
    double stepX = 0;
    double stepY = 0;
    MLMicroSeconds interval = 50*MilliSecond;
    long numSteps = -1;
    bool doStart = false;
    if (aViewConfig->get("stepx", o)) {
      stepX = o->doubleValue();
      doStart = true;
    }
    if (aViewConfig->get("stepy", o)) {
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


P44ViewPtr ViewScroller::getView(const string aLabel)
{
  if (mScrolledView) {
    P44ViewPtr view = mScrolledView->getView(aLabel);
    if (view) return view;
  }
  return inherited::getView(aLabel);
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_VIEWSTATUS

JsonObjectPtr ViewScroller::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  if (mScrolledView) status->add("scrolledview", mScrolledView->viewStatus());
  status->add("offsetx", JsonObject::newDouble(getOffsetX()));
  status->add("offsety", JsonObject::newDouble(getOffsetY()));
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

#endif // ENABLE_VIEWSTATUS

// MARK: - script support

#if P44SCRIPT_FULL_SUPPORT

using namespace P44Script;


ScriptObjPtr ViewScroller::newViewObj()
{
  // base class with standard functionality
  return new ScrollerViewObj(this);
}


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

EventSource *ContentNeededObj::eventSource() const
{
  return static_cast<EventSource*>(mScroller.get());
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
  f->finish(new NumericValue((double)v->scroller()->remainingScrollTime()/Second));
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
static const BuiltInArgDesc alertempty_args[] = { { numeric } };
static const size_t alertempty_numargs = sizeof(alertempty_args)/sizeof(BuiltInArgDesc);
static void alertempty_func(BuiltinFunctionContextPtr f)
{
  ScrollerViewObj* v = dynamic_cast<ScrollerViewObj*>(f->thisObj().get());
  assert(v);
  v->scroller()->setAlertEmpty(f->arg(0)->boolValue());
  f->finish(v);
}

static const BuiltinMemberDescriptor scrollerFunctions[] = {
  { "empty", executable|objectvalue, 0, NULL, &empty_func },
  { "remainingpixels", executable|objectvalue, 0, NULL, &remainingpixels_func },
  { "remainingtime", executable|numeric, 0, NULL, &remainingtime_func },
  { "alertempty", executable|objectvalue, alertempty_numargs, alertempty_args, &alertempty_func },
  { "purge", executable|objectvalue, 0, NULL, &purge_func },
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedScrollerFunctionLookupP = NULL;

ScrollerViewObj::ScrollerViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  if (sharedScrollerFunctionLookupP==NULL) {
    sharedScrollerFunctionLookupP = new BuiltInMemberLookup(scrollerFunctions);
    sharedScrollerFunctionLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedScrollerFunctionLookupP);
}

#endif // P44SCRIPT_FULL_SUPPORT
