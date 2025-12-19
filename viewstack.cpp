//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2016-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
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


#include "viewstack.hpp"

#if ENABLE_VIEWCONFIG
  #include "viewfactory.hpp"
#endif


using namespace p44;


// MARK: ===== ViewStack

static ViewRegistrar r(ViewStack::staticTypeName(), &ViewStack::newInstance);

ViewStack::ViewStack()
{
  mPositioningMode = noFraming+noAdjust;
}


ViewStack::~ViewStack()
{
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    (*pos)->setParent(NULL);
  }
  mViewStack.clear();
}


void ViewStack::pushView(P44ViewPtr aView, int aSpacing, bool aFullFrame)
{
  announceChanges(true);
  // clip/noAdjust bits set means we don't want the content rect to get recalculated
  bool adjust = (mPositioningMode&noAdjust)==0;
  bool fill = false;
  // auto-positioning?
  if (mPositioningMode & repeatXY) {
    // repeat bits determine in which direction to position the view relative to those already present
    // (note, sensible alias constants exist for appending: appendLeft/Right/Top/Bottom)
    PixelRect r;
    getEnclosingContentRect(r);
    // X auto positioning/sizing
    if ((mPositioningMode&fillX)==fillX) {
      aView->mFrame.dx = mContent.dx;
      fill = true;
    }
    else if (mPositioningMode&appendRight) {
      aView->mFrame.x = r.x+r.dx+aSpacing;
    }
    else if (mPositioningMode&appendLeft) {
      aView->mFrame.x = r.x-aView->mFrame.dx-aSpacing;
    }
    // Y auto positioning/sizing
    if ((mPositioningMode&fillY)==fillY) {
      aView->mFrame.dy = mContent.dy;
      fill = true;
    }
    else if (mPositioningMode&appendTop) {
      aView->mFrame.y = r.y+r.dy+aSpacing;
    }
    else if (mPositioningMode&appendBottom) {
      aView->mFrame.y = r.y-aView->mFrame.dy-aSpacing;
    }
    if ((aView->getFramingMode()&clipXY)==0) {
      LOG(LOG_WARNING, "ViewStack '%s', pushed view '%s' is not clipped, probably will obscure neigbours!", getLabel().c_str(), aView->getLabel().c_str());
    }
    if (!mSizeToContent && !fill) {
      LOG(LOG_WARNING, "ViewStack '%s' does not size to content, autopositioned view '%s' might be outside frame and thus invisible!", getLabel().c_str(), aView->getLabel().c_str());
    }
  }
  mViewStack.push_back(aView);
  FOCUSLOG("+++ ViewStack '%s' pushes subview #%zu with frame=(%d,%d,%d,%d) - frame coords are relative to content origin",
    getLabel().c_str(),
    mViewStack.size(),
    aView->mFrame.x, aView->mFrame.y, aView->mFrame.dx, aView->mFrame.dy
  );
  if (adjust) {
    recalculateContentArea();
  }
  sortZOrder();
  aView->setParent(this);
  makeDirty();
  announceChanges(false);
  // re-apply fullframe ON the pushed view (in case frame was adjusted during push)
  if (aFullFrame && ((mPositioningMode&fillX)==fillX || (mPositioningMode&fillY)==fillY)) {
    aView->setFullFrameContent();
  }
  // allow the view to auto-adjust to the containing frame
  aView->autoAdjustTo(mFrame);
}


void ViewStack::purgeViews(int aKeepDx, int aKeepDy, bool aCompletely)
{
  if ((mPositioningMode&repeatXY)==0) return; // no positioning -> NOP
  // calculate content bounds where to keep views
  PixelRect r;
  getEnclosingContentRect(r);
  if (mPositioningMode&repeatXmax) {
    r.x = r.x+r.dx-aKeepDx;
    r.dx = aKeepDx;
  }
  else if (mPositioningMode&repeatXmin) {
    r.dx = aKeepDx;
  }
  if (mPositioningMode&repeatYmax) {
    r.y = r.y+r.dy-aKeepDy;
    r.dy = aKeepDy;
  }
  else if (mPositioningMode&repeatYmin) {
    r.dy = aKeepDy;
  }
  // now purge views
  FOCUSLOG("ViewStack '%s' purges with keep rectangle=(%d,%d,%d,%d)",
    getLabel().c_str(),
    r.x, r.y, r.dx, r.dy
  );
  ViewsList::iterator pos = mViewStack.begin();
  announceChanges(true);
  while (pos!=mViewStack.end()) {
    P44ViewPtr v = *pos;
    if (
      !rectIntersectsRect(r, v->mFrame) ||
      (aCompletely && !rectContainsRect(r, v->mFrame))
    ) {
      if (mViewStack.size()<=1) break; // do not purge last view in stack
      // remove this view
      FOCUSLOG("--- purges subview #%zu with frame=(%d,%d,%d,%d)",
        mViewStack.size(),
        v->mFrame.x, v->mFrame.y, v->mFrame.dx, v->mFrame.dy
      );
      pos = mViewStack.erase(pos);
      flagGeometryChange();
    }
    else {
      // test next
      ++pos;
    }
  }
  if (mChangedGeometry && (mPositioningMode&clipXY)==0) {
    recalculateContentArea();
  }
  announceChanges(false);
}


void ViewStack::recalculateContentArea()
{
  // where is the actual content relative to the content origin?
  PixelRect r;
  getEnclosingContentRect(r);
  // move actual content back to content origin
  offsetSubviews({-r.x, -r.y});
  // to avoid content moving seen from the outside, now modify content rectangle
  setContent(r);
}


static bool compare_zorder(P44ViewPtr& aFirst, P44ViewPtr& aSecond)
{
  return aFirst->getZOrder() < aSecond->getZOrder();
}

void ViewStack::sortZOrder()
{
  mViewStack.sort(compare_zorder);
}


void ViewStack::offsetSubviews(PixelPoint aOffset)
{
  announceChanges(true);
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    P44ViewPtr v = *pos;
    PixelRect f = v->mFrame;
    f.x += aOffset.x;
    f.y += aOffset.y;
    v->setFrame(f);
  }
  announceChanges(false);
}


void ViewStack::getEnclosingContentRect(PixelRect &aBounds)
{
  // get enclosing rect of all layers
  if (mViewStack.empty()) {
    // no content, just a point at the origin
    aBounds = zeroRect;
    return;
  }
  int minX = INT_MAX;
  int maxX = INT_MIN;
  int minY = INT_MAX;
  int maxY = INT_MIN;
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    P44ViewPtr v = *pos;
    if (v->mFrame.x<minX) minX = v->mFrame.x;
    if (v->mFrame.y<minY) minY = v->mFrame.y;
    if (v->mFrame.x+v->mFrame.dx>maxX) maxX = v->mFrame.x+v->mFrame.dx;
    if (v->mFrame.y+v->mFrame.dy>maxY) maxY = v->mFrame.y+v->mFrame.dy;
  }
  aBounds.x = minX;
  aBounds.y = minY;
  aBounds.dx = maxX-minX; if (aBounds.dx<0) aBounds.dx = 0;
  aBounds.dy = maxY-minY; if (aBounds.dy<0) aBounds.dy = 0;
  FOCUSLOG("ViewStack '%s': enclosingContentRect=(%d,%d,%d,%d)", getLabel().c_str(), aBounds.x, aBounds.y, aBounds.dx, aBounds.dy);
}


bool ViewStack::addSubView(P44ViewPtr aSubView)
{
  // just push w/o spacing or fullframe
  pushView(aSubView, 0, false);
  return true;
}



void ViewStack::popView()
{
  announceChanges(true);
  if (!mViewStack.empty()) {
    mViewStack.back()->setParent(NULL);
    mViewStack.pop_back();
    flagGeometryChange();
  }
  announceChanges(false);
}


bool ViewStack::removeView(P44ViewPtr aView)
{
  bool removed = false;
  announceChanges(true);
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    if ((*pos)==aView) {
      (*pos)->setParent(NULL);
      mViewStack.erase(pos);
      flagGeometryChange();
      removed = true;
      break;
    }
  }
  announceChanges(false);
  return removed;
}


void ViewStack::clear()
{
  announceChanges(true);
  while (true) {
    ViewsList::iterator pos = mViewStack.begin();
    if (pos==mViewStack.end()) break;
    (*pos)->setParent(NULL);
    mViewStack.erase(pos);
    flagGeometryChange();
  }
  announceChanges(false);
  inherited::clear();
}


MLMicroSeconds ViewStack::stepInternal(MLMicroSeconds aPriorityUntil)
{
  MLMicroSeconds nextCall = inherited::stepInternal(aPriorityUntil);
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    updateNextCall(nextCall, (*pos)->step(stepShowTime(), aPriorityUntil, stepRealTime())); // no local view priorisation
  }
  return nextCall;
}


bool ViewStack::isDirty()
{
  if (inherited::isDirty()) return true; // dirty anyway (such as changing alpha)
  if (mAlpha==0) return false; // as long as the stack is invisible, dirty childs are irrelevant
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    if ((*pos)->isDirty())
      return true; // subview is dirty -> stack is dirty
  }
  return false;
}


void ViewStack::updated()
{
  inherited::updated();
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    (*pos)->updated();
  }
}


void ViewStack::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  inherited::geometryChanged(aOldFrame, aOldContent);
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    (*pos)->autoAdjustTo(mFrame);
  }
}


void ViewStack::childGeometryChanged(P44ViewPtr aChildView, PixelRect aOldFrame, PixelRect aOldContent)
{
  if (mChangeTrackingLevel==0) {
    // only if not already in process of changing
    //if ((positioningMode&clipXY)==0) {
    if (mSizeToContent) {
      // current content bounds should not clip -> auto-adjust
      recalculateContentArea();
      moveFrameToContent(true);
    }
    // re-sort z_order
    sortZOrder();
  }
}



PixelColor ViewStack::contentColorAt(PixelPoint aPt)
{
  // default is the viewstack's background color
  if (mAlpha==0) {
    return transparent; // entire viewstack is invisible
  }
  else {
    // consult views in stack
    PixelColor pc = black; // start with black, i.e. no color (NOT background!)
    PixelColor lc;
    uint8_t seethrough = 255; // first layer is directly visible, not yet obscured
    for (ViewsList::reverse_iterator pos = mViewStack.rbegin(); pos!=mViewStack.rend(); ++pos) {
      P44ViewPtr layer = *pos;
      if (layer->mAlpha==0) continue; // shortcut: skip fully transparent layers
      lc = layer->colorAt(aPt);
      if (lc.a==0) continue; // skip layer with fully transparent pixel
      // not-fully-transparent pixel
      pc.a = 255; // the composite cannot be transparent
      // - scale down to current budget left
      lc.a = dimVal(lc.a, seethrough);
      lc = dimmedPixel(lc, lc.a);
      addToPixel(pc, lc);
      seethrough -= lc.a;
      if (seethrough<=0) break; // nothing more to see though
    } // collect from all layers
    if (seethrough>0) {
      // rest is background or overall transparency
      lc.a = dimVal(mBackgroundColor.a, seethrough);
      seethrough -= lc.a;
      lc = dimmedPixel(mBackgroundColor, lc.a);
      addToPixel(pc, lc);
    }
    if(seethrough>0) {
      pc.a = 255-seethrough; // overall transparency is what is left of seethrough
      if (pc.a>0) dimPixel(pc, 65025/pc.a); // but intensity of stack result must be amplified accordingly
    }
    // factor in alpha of entire viewstack
    if (mAlpha!=255) {
      pc.a = dimVal(pc.a, mAlpha);
    }
    return pc;
  }
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr ViewStack::configureView(JsonObjectPtr aViewConfig)
{
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    #if !ENABLE_P44SCRIPT
    if (aViewConfig->get("positioningmode", o)) {
      if (o->isType(json_type_string)) {
        setPositioningMode(textToFramingMode(o->c_strValue()));
      }
      else {
        setPositioningMode((FramingMode)o->int32Value());
      }
    }
    #endif
    if (aViewConfig->get("layers", o)) {
      for (int i=0; i<o->arrayLength(); ++i) {
        JsonObjectPtr l = o->arrayGet(i);
        JsonObjectPtr o2;
        JsonObjectPtr lv;
        P44ViewPtr layerView;
        int spacing = 0;
        bool fullframe = false;
        // Note extra object level containing single "view" is legacy, supported only for backwards compatibility
        if (l->get("view", lv)) {
          // legacy view config nested in a special layer object, only for setting positioning options
          if (l->get("spacing", o2)) spacing = o2->int32Value();
          if (l->get("fullframe", o2)) fullframe = o2->boolValue();
          if (l->get("positioning", o2)) {
            LOG(LOG_WARNING, "Warning: legacy 'positioning' in stack subview -> use stack-global 'positioningmode' instead");
            setPositioningMode((FramingMode)o2->int32Value());
          }
        }
        else {
          // new style: no in-between object
          lv = l; // the layer is the view config
        }
        if (!lv) {
          err = TextError::err("missing view in layers");
        }
        else {
          err = p44::createViewFromConfig(lv, layerView, this);
          if (Error::isOK(err)) {
            // the options that were formerly in the in-between object can be specified as layer_xxx here
            if (lv->get("layer_spacing", o2)) spacing = o2->int32Value();
            if (lv->get("layer_fullframe", o2)) spacing = o2->int32Value();
            // push the layer
            pushView(layerView, spacing, fullframe);
          }
        }
      }
    }
    if (aViewConfig->get("pop", o)) {
      if(o->boolValue()) popView();
    }
  }
  return err;
}


P44ViewPtr ViewStack::findLayer(const string aLabel, bool aRecursive)
{
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    if (*pos) {
      P44ViewPtr view;
      if (aRecursive) view = (*pos)->findView(aLabel); // derived class
      else view = (*pos)->P44View::findView(aLabel); // base class only checks label/id
      if (view) return view;
    }
  }
  return P44ViewPtr();
}


P44ViewPtr ViewStack::findView(const string aLabel)
{
  P44ViewPtr view = findLayer(aLabel, true);
  if (view) return view;
  return inherited::findView(aLabel);
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

JsonObjectPtr ViewStack::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  JsonObjectPtr layers = JsonObject::newArray();
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    layers->arrayAppend((*pos)->viewStatus());
  }
  status->add("layers", layers);
  status->add("positioningmode", JsonObject::newString(framingModeToText(getPositioningMode(), true)));
  return status;
}

#endif // ENABLE_VIEWSTATUS

#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr ViewStack::newViewObj()
{
  // base class with standard functionality
  return new ViewStackObj(this);
}


ScriptObjPtr ViewStack::layersList()
{
  ArrayValuePtr layers = new ArrayValue();
  for (ViewsList::iterator pos = mViewStack.begin(); pos!=mViewStack.end(); ++pos) {
    layers->appendMember((*pos)->newViewObj());
  }
  return layers;
}


#if P44SCRIPT_FULL_SUPPORT


// pushview(view [,spacing [, fullframe]])
FUNC_ARG_DEFS(pushview, { structured }, { numeric|optionalarg }, { numeric|optionalarg } );
static void pushview_func(BuiltinFunctionContextPtr f)
{
  ViewStackObj* v = dynamic_cast<ViewStackObj*>(f->thisObj().get());
  assert(v);
  ErrorPtr err;
  P44ViewPtr pushedview = P44View::viewFromScriptObj(f->arg(0), err);
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  int spacing = f->arg(1)->intValue();
  bool fullframe = f->arg(2)->boolValue();
  v->stack()->pushView(pushedview, spacing, fullframe);
  // return the pushed view for chaining
  f->finish(pushedview->newViewObj());
}


// purge(dx, dy, completely)
FUNC_ARG_DEFS(purge, { numeric }, { numeric }, { numeric|optionalarg } );
static void purge_func(BuiltinFunctionContextPtr f)
{
  ViewStackObj* v = dynamic_cast<ViewStackObj*>(f->thisObj().get());
  assert(v);
  PixelCoord dx = f->arg(0)->intValue();
  PixelCoord dy = f->arg(1)->intValue();
  bool completely = f->arg(2)->boolValue();
  v->stack()->purgeViews(dx, dy, completely);
}


// popview()
static void popview_func(BuiltinFunctionContextPtr f)
{
  ViewStackObj* v = dynamic_cast<ViewStackObj*>(f->thisObj().get());
  assert(v);
  v->stack()->popView();
  f->finish();
}


#endif // P44SCRIPT_FULL_SUPPORT

#define ACCESSOR_CLASS ViewStack

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  ViewStackPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<ViewStackObj*>(aParentObj.get())->stack().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

//ACC_IMPL_BOOL(Run);

static ScriptObjPtr access_PositioningMode(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite)
{
  if (!aToWrite) return new StringValue(P44View::framingModeToText(aView.getPositioningMode(), true));
  if (aToWrite->hasType(numeric)) aView.setPositioningMode(aToWrite->intValue());
  else aView.setPositioningMode(P44View::textToFramingMode(aToWrite->stringValue().c_str()));
  return aToWrite; /* reflect back to indicate writable */
}

static ScriptObjPtr access_Layers(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite)
{
  ScriptObjPtr ret;
  if (!aToWrite) { // not writable
    ret = aView.layersList();
  }
  return ret;
}



static const BuiltinMemberDescriptor viewStackMembers[] = {
  #if P44SCRIPT_FULL_SUPPORT
  FUNC_DEF_W_ARG(pushview, executable|null|error),
  FUNC_DEF_NOARG(popview, executable|null|error),
  FUNC_DEF_W_ARG(purge, executable|null|error),
  #endif
  // property accessors
  ACC_DECL("positioningmode", numeric|lvalue, PositioningMode),
  ACC_DECL("layers", arrayvalue, Layers),
  BUILTINS_TERMINATOR
};

static BuiltInMemberLookup* sharedViewStackMemberLookupP = NULL;

ViewStackObj::ViewStackObj(P44ViewPtr aView) :
  inherited(aView)
{
  registerSharedLookup(sharedViewStackMemberLookupP, viewStackMembers);
}

#endif // ENABLE_P44SCRIPT
