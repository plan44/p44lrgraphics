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

#include "p44view.hpp"
#include "ledchaincomm.hpp" // for brightnessToPwm and pwmToBrightness

#include <math.h>

using namespace p44;

// MARK: ===== View

P44View::P44View() :
  mParentView(NULL),
  mDirty(false),
  mUpdateRequested(false),
  mMinUpdateInterval(0),
  mGeometryChanging(0),
  mChangedGeometry(false),
  mSizeToContent(false),
  mContentRotation(0),
  mRotCos(1.0),
  mRotSin(0.0)
{
  setFrame(zeroRect);
  // default to normal orientation
  mContentOrientation = right;
  // default to clip, no content wrap
  mContentWrapMode = clipXY;
  // default content size is same as view's
  setContent(zeroRect);
  mBackgroundColor = { .r=0, .g=0, .b=0, .a=0 }; // transparent background,
  mForegroundColor = { .r=255, .g=255, .b=255, .a=255 }; // fully white foreground...
  mAlpha = 255; // but content pixels passed trough 1:1
  mZOrder = 0; // none in particular
  mContentIsMask = false; // content color will be used
  mInvertAlpha = false; // inverted mask
  mLocalTimingPriority = true;
  mMaskChildDirtyUntil = Never;
}


P44View::~P44View()
{
  geometryChange(true); // unbalanced, avoid any further dependency updates while finally deleting view
  clear();
  removeFromParent();
}


// MARK: ===== frame and content

bool P44View::isInContentSize(PixelPoint aPt)
{
  return aPt.x>=0 && aPt.y>=0 && aPt.x<mContent.dx && aPt.y<mContent.dy;
}


PixelColor P44View::contentColorAt(PixelPoint aPt)
{
  // for plain views, show content rect in foreground
  if (isInContentSize(aPt))
    return mForegroundColor;
  else
    return mBackgroundColor;
}


void P44View::ledRGBdata(string& aLedRGB, PixelRect aArea)
{
  aLedRGB.reserve(aArea.dx*aArea.dy*6+1); // one extra for a message terminator
  // pixel data row by row
  for (int y=0; y<aArea.dy; ++y) {
    for (int x=0; x<aArea.dx; ++x) {
      PixelColor pix = colorAt({
        aArea.x+x,
        aArea.y+y
      });
      dimPixel(pix, pix.a);
      string_format_append(aLedRGB, "%02X%02X%02X", pix.r, pix.g, pix.b);
    }
  }
}


void P44View::geometryChange(bool aStart)
{
  if (aStart){
    if (mGeometryChanging<=0) {
      // start tracking changes
      mChangedGeometry = false;
      mPreviousFrame = mFrame;
      mPreviousContent = mContent;
    }
    mGeometryChanging++;
  }
  else {
    if (mGeometryChanging>0) {
      mGeometryChanging--;
      if (mGeometryChanging==0) {
        if (mChangedGeometry) {
          FOCUSLOG("View '%s' changed geometry: frame=(%d,%d,%d,%d)->(%d,%d,%d,%d), content=(%d,%d,%d,%d)->(%d,%d,%d,%d)",
            getLabel().c_str(),
            mPreviousFrame.x, mPreviousFrame.y, mPreviousFrame.dx, mPreviousFrame.dy,
            mFrame.x, mFrame.y, mFrame.dx, mFrame.dy,
            mPreviousContent.x, mPreviousContent.y, mPreviousContent.dx, mPreviousContent.dy,
            mContent.x, mContent.y, mContent.dx, mContent.dy
          );
          makeDirty();
          geometryChanged(mPreviousFrame, mPreviousContent); // let subclasses know
          if (mParentView) {
            // Note: as we are passing in the frames, it is safe when the following calls recursively calls geometryChange again
            //   except that it must not do so unconditionally to prevent endless recursion
            mParentView->childGeometryChanged(this, mPreviousFrame, mPreviousContent);
          }
        }
      }
    }
  }
}



void P44View::orientateCoord(PixelPoint &aCoord)
{
  if (mContentOrientation & xy_swap) {
    swap(aCoord.x, aCoord.y);
  }
}


void P44View::flipCoordInFrame(PixelPoint &aCoord)
{
  // flip within frame if not zero sized
  if ((mContentOrientation & x_flip) && mFrame.dx>0) {
    aCoord.x = mFrame.dx-aCoord.x-1;
  }
  if ((mContentOrientation & y_flip) && mFrame.dy>0) {
    aCoord.y = mFrame.dy-aCoord.y-1;
  }
}


void P44View::inFrameToContentCoord(PixelPoint &aCoord)
{
  flipCoordInFrame(aCoord);
  orientateCoord(aCoord);
  aCoord.x -= mContent.x;
  aCoord.y -= mContent.y;
}


void P44View::contentToInFrameCoord(PixelPoint &aCoord)
{
  aCoord.x += mContent.x;
  aCoord.y += mContent.y;
  orientateCoord(aCoord);
  flipCoordInFrame(aCoord);
}


/// change rect and trigger geometry change when actually changed
void P44View::changeGeometryRect(PixelRect &aRect, PixelRect aNewRect)
{
  if (aNewRect.x!=aRect.x) {
    mChangedGeometry = true;
    aRect.x = aNewRect.x;
  }
  if (aNewRect.y!=aRect.y) {
    mChangedGeometry = true;
    aRect.y = aNewRect.y;
  }
  if (aNewRect.dx!=aRect.dx) {
    mChangedGeometry = true;
    aRect.dx = aNewRect.dx;
  }
  if (aNewRect.dy!=aRect.dy) {
    mChangedGeometry = true;
    aRect.dy = aNewRect.dy;
  }
}



void P44View::setFrame(PixelRect aFrame)
{
  geometryChange(true);
  changeGeometryRect(mFrame, aFrame);
  geometryChange(false);
}


void P44View::setParent(P44ViewPtr aParentView)
{
  mParentView = aParentView.get();
}


P44ViewPtr P44View::getParent()
{
  return P44ViewPtr(mParentView);
}



void P44View::setContent(PixelRect aContent)
{
  geometryChange(true);
  changeGeometryRect(mContent, aContent);
  if (mSizeToContent) {
    moveFrameToContent(true);
  }
  geometryChange(false);
};


void P44View::setContentSize(PixelPoint aSize)
{
  geometryChange(true);
  changeGeometryRect(mContent, { mContent.x, mContent.y, aSize.x, aSize.y });
  if (mChangedGeometry && mSizeToContent) moveFrameToContent(true);
  geometryChange(false);
};


void P44View::setContentOrigin(PixelPoint aOrigin)
{
  geometryChange(true);
  changeGeometryRect(mContent, { aOrigin.x, aOrigin.y, mContent.dx, mContent.dy });
  geometryChange(false);
};


void P44View::setRelativeContentOrigin(double aRelX, double aRelY, bool aCentered)
{
  geometryChange(true);
  setRelativeContentOriginX(aRelX, aCentered);
  setRelativeContentOriginY(aRelY, aCentered);
  geometryChange(false);
}

void P44View::setRelativeContentOriginX(double aRelX, bool aCentered)
{
  // standard version, content origin is a corner of the relevant area
  geometryChange(true);
  changeGeometryRect(mContent, { (int)(aRelX*max(mContent.dx,mFrame.dx)+(aCentered ? mFrame.dx/2 : 0)), mContent.y, mContent.dx, mContent.dy });
  geometryChange(false);
}


void P44View::setRelativeContentOriginY(double aRelY, bool aCentered)
{
  // standard version, content origin is a corner of the relevant area
  geometryChange(true);
  changeGeometryRect(mContent, { mContent.x, (int)(aRelY*max(mContent.dy,mFrame.dy)+(aCentered ? mFrame.dy/2 : 0)), mContent.dx, mContent.dy });
  geometryChange(false);
}


void P44View::setContentRotation(double aRotation)
{
  if (aRotation!=mContentRotation) {
    mContentRotation = aRotation;
    double rotPi = mContentRotation*M_PI/180;
    mRotSin = sin(rotPi);
    mRotCos = cos(rotPi);
    makeDirty();
  }
}


void P44View::setFullFrameContent()
{
  PixelPoint sz = getFrameSize();
  orientateCoord(sz);
  setContent({ 0, 0, sz.x, sz.y });
}



void P44View::contentRectAsViewCoord(PixelRect &aRect)
{
  // get opposite content rect corners
  PixelPoint c1 = { 0, 0 };
  contentToInFrameCoord(c1);
  PixelPoint inset = { mContent.dx>0 ? 1 : 0, mContent.dy>0 ? 1 : 0 };
  PixelPoint c2 = { mContent.dx-inset.x, mContent.dy-inset.y };
  // transform into coords relative to frame origin
  contentToInFrameCoord(c2);
  // make c2 the non-origin corner
  if (c1.x>c2.x) swap(c1.x, c2.x);
  if (c1.y>c2.y) swap(c1.y, c2.y);
  // create view coord rectangle around current contents
  aRect.x = c1.x + mFrame.x;
  aRect.dx = c2.x-c1.x+inset.x;
  aRect.y = c1.y + mFrame.y;
  aRect.dy = c2.y-c1.y+inset.y;
  FOCUSLOG("View '%s' frame=(%d,%d,%d,%d), content rect as view coords=(%d,%d,%d,%d)",
    getLabel().c_str(),
    mFrame.x, mFrame.y, mFrame.dx, mFrame.dy,
    aRect.x, aRect.y, aRect.dx, aRect.dy
  );
}


/// move frame such that its origin is at the actual content's origin
/// @note content does not move relative to view frame origin, but frame does
void P44View::moveFrameToContent(bool aResize)
{
  geometryChange(true);
  if (aResize) sizeFrameToContent();
  PixelRect f;
  contentRectAsViewCoord(f);
  // move frame to the place where the content rectangle did appear so far...
  changeGeometryRect(mFrame, f);
  // ...which means that no content offset is needed any more (we've compensated it by moving the frame)
  mContent.x = 0; mContent.y = 0;
  geometryChange(false);
}


void P44View::sizeFrameToContent()
{
  PixelPoint sz = { mContent.dx, mContent.dy };
  orientateCoord(sz);
  PixelRect f = mFrame;
  f.dx = sz.x;
  f.dy = sz.y;
  changeGeometryRect(mFrame, f);
}



void P44View::clear()
{
  stopAnimations();
  // as the only thing a P44View can display is the content rect in foreground color, reset it here
  // Note: subclasses will not always call inherited::clear() as they might want to retain the content rectangle,
  //       and just remove the actual content data
  setContentSize({0, 0});
}


// MARK: ===== updating

void P44View::makeDirtyAndUpdate()
{
  // make dirty locally
  makeDirty();
  // request a step at the root view level
  requestUpdate();
}


void P44View::requestUpdate()
{
  FOCUSLOG("requestUpdate() called for view@%p", this);
  P44View *p = this;
  while (p->mParentView) {
    if (p->mUpdateRequested) return;  // already requested, no need to descend to root
    p->mUpdateRequested = true; // mark having requested update all the way down to root, update() will be called on all views to clear it
    p = p->mParentView;
  }
  // now p = root view
  if (!p->mUpdateRequested && p->mNeedUpdateCB) {
    p->mUpdateRequested = true; // only request once
    FOCUSLOG("actually requesting update from root view@%p (from view@%p)", p, this);
    // there is a needUpdate callback here
    // DO NOT call it directly, but from mainloop, so receiver can safely call
    // back into any view object method without causing recursions
    MainLoop::currentMainLoop().executeNow(p->mNeedUpdateCB);
  }
}


void P44View::requestUpdateIfNeeded()
{
  if (!mUpdateRequested && isDirty()) {
    requestUpdate();
  }
}


void P44View::updated()
{
  mDirty = false;
  mUpdateRequested = false;
}


void P44View::setNeedUpdateCB(TimerCB aNeedUpdateCB)
{
  mNeedUpdateCB = aNeedUpdateCB;
}


void P44View::setMinUpdateInterval(MLMicroSeconds aMinUpdateInterval)
{
  mMinUpdateInterval = aMinUpdateInterval;
}


MLMicroSeconds P44View::getMinUpdateInterval()
{
  P44View *p = this;
  do {
    if (p->mMinUpdateInterval>0) return p->mMinUpdateInterval;
    p = p->mParentView;
  } while (p);
  return DEFAULT_MIN_UPDATE_INTERVAL;
}


bool P44View::removeFromParent()
{
  if (mParentView) {
    return mParentView->removeView(this); // should always return true...
  }
  return false;
}


void P44View::makeDirty()
{
  mDirty = true;
}


void P44View::makeColorDirty()
{
  recalculateColoring();
  makeDirty();
}


bool P44View::reportDirtyChilds()
{
  if (mMaskChildDirtyUntil) {
    if (MainLoop::now()<mMaskChildDirtyUntil) {
      return false;
    }
    mMaskChildDirtyUntil = 0;
  }
  return true;
}


void P44View::updateNextCall(MLMicroSeconds &aNextCall, MLMicroSeconds aCallCandidate, MLMicroSeconds aCandidatePriorityUntil, MLMicroSeconds aNow)
{
  if (mLocalTimingPriority && aCandidatePriorityUntil>0 && aCallCandidate>=0 && aCallCandidate<aCandidatePriorityUntil) {
    // children must not cause "dirty" before candidate time is over
    if (aNow==Never) aNow = MainLoop::now();
    mMaskChildDirtyUntil = (aCallCandidate-aNow)*2+aNow; // duplicate to make sure candidate execution has some time to happen BEFORE dirty is unblocked
  }
  if (aNextCall<=0 || (aCallCandidate>0 && aCallCandidate<aNextCall)) {
    // candidate wins
    aNextCall = aCallCandidate;
  }
}


MLMicroSeconds P44View::step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow)
{
  mUpdateRequested = false; // no step request pending any more
  // check animations
  MLMicroSeconds nextCall = Infinite;
  #if ENABLE_ANIMATION
  AnimationsList::iterator pos = mAnimations.begin();
  while (pos != mAnimations.end()) {
    ValueAnimatorPtr animator = (*pos);
    MLMicroSeconds nextStep = animator->step(aNow);
    if (!animator->inProgress()) {
      // this animation is done, remove it from the list
      pos = mAnimations.erase(pos);
      continue;
    }
    updateNextCall(nextCall, nextStep);
    pos++;
  }
  #endif // ENABLE_ANIMATION
  return nextCall;
}


void P44View::setAlpha(PixelColorComponent aAlpha)
{
  if (mAlpha!=aAlpha) {
    mAlpha = aAlpha;
    makeDirty();
  }
}


void P44View::setZOrder(int aZOrder)
{
  geometryChange(true);
  if (mZOrder!=aZOrder) {
    mZOrder = aZOrder;
    mChangedGeometry = true;
  }
  geometryChange(false);
}



#define SHOW_ORIGIN 0


PixelColor P44View::colorAt(PixelPoint aPt)
{
  // default is background color
  PixelColor pc = mBackgroundColor;
  if (mAlpha==0) {
    pc.a = 0; // entire view is invisible
  }
  else {
    // calculate coordinate relative to the frame's origin
    aPt.x -= mFrame.x;
    aPt.y -= mFrame.y;
    // optionally clip content to frame
    if (mContentWrapMode&clipXY && (
      ((mContentWrapMode&clipXmin) && aPt.x<0) ||
      ((mContentWrapMode&clipXmax) && aPt.x>=mFrame.dx) ||
      ((mContentWrapMode&clipYmin) && aPt.y<0) ||
      ((mContentWrapMode&clipYmax) && aPt.y>=mFrame.dy)
    )) {
      // clip
      pc.a = 0; // invisible
    }
    else {
      // not clipped
      // optionally wrap content (repeat frame contents in selected directions)
      if (mFrame.dx>0) {
        while ((mContentWrapMode&wrapXmin) && aPt.x<0) aPt.x+=mFrame.dx;
        while ((mContentWrapMode&wrapXmax) && aPt.x>=mFrame.dx) aPt.x-=mFrame.dx;
      }
      if (mFrame.dy>0) {
        while ((mContentWrapMode&wrapYmin) && aPt.y<0) aPt.y+=mFrame.dy;
        while ((mContentWrapMode&wrapYmax) && aPt.y>=mFrame.dy) aPt.y-=mFrame.dy;
      }
      // translate into content coordinates
      inFrameToContentCoord(aPt);
      // now get content pixel in content coordinates
      if (mContentRotation==0) {
        // optimization: no rotation, get pixel
        pc = contentColorAt(aPt);
      }
      else {
        // - rotate first
        PixelPoint rpt;
        rpt.x = aPt.x*mRotCos-aPt.y*mRotSin;
        rpt.y = aPt.x*mRotSin+aPt.y*mRotCos;
        pc = contentColorAt(rpt);
      }
      if (mInvertAlpha) {
        pc.a = 255-pc.a;
      }
      if (mContentIsMask) {
        // use only (possibly inverted) alpha of content, color comes from foregroundColor
        pc.r = mForegroundColor.r;
        pc.g = mForegroundColor.g;
        pc.b = mForegroundColor.b;
      }
      #if SHOW_ORIGIN
      if (aPt.x==0 && aPt.y==0) {
        return { .r=255, .g=0, .b=0, .a=255 };
      }
      else if (aPt.x==1 && aPt.y==0) {
        return { .r=0, .g=255, .b=0, .a=255 };
      }
      else if (aPt.x==0 && aPt.y==1) {
        return { .r=0, .g=0, .b=255, .a=255 };
      }
      #endif
      if (pc.a==0) {
        // background is where content is fully transparent
        pc = mBackgroundColor;
        // Note: view background does NOT shine through semi-transparent content pixels!
        //   Rather, non-fully-transparent content pixels directly are view pixels!
      }
      // factor in layer alpha
      if (mAlpha!=255) {
        pc.a = dimVal(pc.a, mAlpha);
      }
    }
  }
  return pc;
}


// MARK: ===== Utilities

bool p44::rectContainsRect(const PixelRect &aParentRect, const PixelRect &aChildRect)
{
  return
    aChildRect.x>=aParentRect.x &&
    aChildRect.x+aChildRect.dx<=aParentRect.x+aParentRect.dx &&
    aChildRect.y>=aParentRect.y &&
    aChildRect.y+aChildRect.dy<=aParentRect.y+aParentRect.dy;
}

bool p44::rectIntersectsRect(const PixelRect &aRect1, const PixelRect &aRect2)
{
  return
    aRect1.x+aRect1.dx>aRect2.x &&
    aRect1.x<aRect2.x+aRect2.dx &&
    aRect1.y+aRect1.dy>aRect2.y &&
    aRect1.y<aRect2.y+aRect2.dy;
}





#if ENABLE_VIEWCONFIG

// MARK: ===== config utilities

typedef struct {
  P44View::Orientation orientation;
  const char *name;
} OrientationDesc;
static const OrientationDesc orientationDescs[] = {
  { P44View::right, "right" }, //  untransformed X goes left to right, Y goes up
  // Note: rest of table should be ordered such that multi-bit combinations come first (for wrapModeToText)
  { P44View::down, "down" }, //  X goes down, Y goes right
  { P44View::left, "left" }, //  X goes left, Y goes down
  { P44View::up, "up" }, //  X goes down, Y goes right
  { P44View::xy_swap, "xy_swap" }, //  swap x and y
  { P44View::x_flip, "x_flip" }, //  flip x
  { P44View::y_flip, "y_flip" }, //  flip y
  { 0, NULL }
};



P44View::WrapMode P44View::textToOrientation(const char *aOrientationText)
{
  Orientation o = P44View::right;
  while (aOrientationText) {
    size_t n = 0;
    while (aOrientationText[n] && aOrientationText[n]!='|') n++;
    for (const OrientationDesc *od = orientationDescs; od->name; od++) {
      if (strucmp(aOrientationText, od->name, n)==0) {
        o |= od->orientation;
      }
    }
    aOrientationText += n;
    if (*aOrientationText==0) break;
    aOrientationText++; // skip |
  }
  return o;
}



typedef struct {
  P44View::WrapMode mode;
  const char *name;
  bool forPos;
} WrapModeDesc;
static const WrapModeDesc wrapModeDescs[] = {
  { P44View::noWrap, "none", false }, // do not wrap or clip
  // Note: rest of table should be ordered such that multi-bit combinations come first (for wrapModeToText)
  { P44View::wrapXY, "wrapXY", false }, //  wrap in all directions
  { P44View::wrapX, "wrapX", false }, //  wrap in both X directions
  { P44View::wrapY, "wrapY", false }, //  wrap in both Y directions
  { P44View::wrapXmin, "wrapXmin", false }, //  wrap in X direction for X<frame area
  { P44View::wrapXmax, "wrapXmax", false }, //  wrap in X direction for X>=frame area
  { P44View::wrapYmin, "wrapYmin", false }, //  wrap in Y direction for Y<frame area
  { P44View::wrapYmax, "wrapYmax", false }, //  wrap in Y direction for Y>=frame area
  { P44View::clipXY, "clipXY", false }, //  clip content
  { P44View::clipY, "clipY", false }, //  clip content vertically
  { P44View::clipX, "clipX", false }, //  clip content horizontally
  { P44View::clipXmin, "clipXmin", false }, //  clip content left of frame area
  { P44View::clipXmax, "clipXmax", false }, //  clip content right of frame area
  { P44View::clipYmin, "clipYmin", false }, //  clip content below frame area
  { P44View::clipYmax, "clipYmax", false }, //  clip content above frame area
  { P44View::noAdjust, "noAdjust", true }, //  for positioning: do not adjust content rectangle
  { P44View::fillXY, "fillXY", true }, //  for positioning: set frame size fill parent frame
  { P44View::fillX, "fillX", true }, //  for positioning: set frame size fill parent in X direction
  { P44View::fillY, "fillY", true }, //  for positioning: set frame size fill parent in Y direction
  { P44View::appendLeft, "appendLeft", true }, //  for positioning: extend to the left
  { P44View::appendRight, "appendRight", true }, //  for positioning: extend to the right
  { P44View::appendBottom, "appendBottom", true }, //  for positioning: extend towards bottom
  { P44View::appendTop, "appendTop", true }, //  for positioning: extend towards top
  { 0, NULL }
};


P44View::WrapMode P44View::textToWrapMode(const char *aWrapModeText)
{
  WrapMode m = P44View::noWrap;
  while (aWrapModeText) {
    size_t n = 0;
    while (aWrapModeText[n] && aWrapModeText[n]!='|') n++;
    for (const WrapModeDesc *wd = wrapModeDescs; wd->name && n>0; wd++) {
      if (strucmp(aWrapModeText, wd->name, n)==0) {
        m |= wd->mode;
      }
    }
    aWrapModeText += n;
    if (*aWrapModeText==0) break;
    aWrapModeText++; // skip |
  }
  return m;
}





// MARK: ===== view configuration

ErrorPtr P44View::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  geometryChange(true);
  if (aViewConfig->get("label", o)) {
    mLabel = o->stringValue();
  }
  if (aViewConfig->get("clear", o)) {
    if(o->boolValue()) clear();
  }
  if (aViewConfig->get("x", o)) {
    mFrame.x = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("y", o)) {
    mFrame.y = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("dx", o)) {
    mFrame.dx = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("dy", o)) {
    mFrame.dy = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("bgcolor", o)) {
    mBackgroundColor = webColorToPixel(o->stringValue()); makeColorDirty();
  }
  if (aViewConfig->get("color", o)) {
    mForegroundColor = webColorToPixel(o->stringValue()); makeColorDirty();
  }
  if (aViewConfig->get("alpha", o)) {
    setAlpha(o->int32Value());
  }
  if (aViewConfig->get("z_order", o)) {
    setZOrder(o->int32Value());
  }
  if (aViewConfig->get("wrapmode", o)) {
    if (o->isType(json_type_string)) {
      setWrapMode(textToWrapMode(o->c_strValue()));
    }
    else {
      setWrapMode(o->int32Value());
    }
  }
  if (aViewConfig->get("mask", o)) {
    mContentIsMask = o->boolValue();
    makeDirty();
  }
  if (aViewConfig->get("invertalpha", o)) {
    mInvertAlpha = o->boolValue();
    makeDirty();
  }
  // frame rect should be defined here (unless we'll use sizetocontent below), so we can check the content related props now
  if (aViewConfig->get("orientation", o)) {
    if (o->isType(json_type_string)) {
      setOrientation(textToOrientation(o->c_strValue()));
    }
    else {
      setOrientation(o->int32Value());
    }
  }
  if (aViewConfig->get("fullframe", o)) {
    if(o->boolValue()) setFullFrameContent();
  }
  // modification of content rect
  if (aViewConfig->get("content_x", o)) {
    mContent.x = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("content_y", o)) {
    mContent.y = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("content_dx", o)) {
    mContent.dx = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("content_dy", o)) {
    mContent.dy = o->int32Value(); makeDirty();
    mChangedGeometry = true;
  }
  if (aViewConfig->get("rel_content_x", o)) {
    setRelativeContentOriginX(o->doubleValue(), false);
  }
  if (aViewConfig->get("rel_content_y", o)) {
    setRelativeContentOriginY(o->doubleValue(), false);
  }
  if (aViewConfig->get("rel_center_x", o)) {
    setRelativeContentOriginX(o->doubleValue(), true);
  }
  if (aViewConfig->get("rel_center_y", o)) {
    setRelativeContentOriginY(o->doubleValue(), true);
  }
  if (aViewConfig->get("rotation", o)) {
    setContentRotation(o->doubleValue());
  }
  if (aViewConfig->get("timingpriority", o)) {
    mLocalTimingPriority = o->boolValue();
  }
  if (aViewConfig->get("sizetocontent", o)) {
    mSizeToContent = o->boolValue();
  }
  if (mChangedGeometry && mSizeToContent) {
    moveFrameToContent(true);
  }
  if (aViewConfig->get("stopanimations", o)) {
    if(o->boolValue()) stopAnimations();
  }
  #if ENABLE_ANIMATION
  if (aViewConfig->get("animate", o)) {
    JsonObjectPtr a = o;
    if (!a->isType(json_type_array)) {
      a = JsonObject::newArray();
      a->arrayAppend(o);
    }
    ValueAnimatorPtr referenceAnimation;
    for (int i=0; i<a->arrayLength(); i++) {
      o = a->arrayGet(i);
      JsonObjectPtr p;
      ValueAnimatorPtr animator;
      if (o->get("property", p)) {
        animator = animatorFor(p->stringValue());
        if (o->get("to", p)) {
          double to = p->doubleValue();
          if (o->get("time", p)) {
            MLMicroSeconds duration = p->doubleValue()*Second;
            // optional params
            bool autoreverse = false;
            int cycles = 1;
            MLMicroSeconds minsteptime = 0;
            double stepsize = 0;
            animator->function(ValueAnimator::easeInOut)->param(3);
            if (o->get("minsteptime", p)) minsteptime = p->doubleValue()*Second;
            if (o->get("stepsize", p)) stepsize = p->doubleValue();
            if (o->get("autoreverse", p)) autoreverse = p->boolValue();
            if (o->get("cycles", p)) cycles = p->int32Value();
            if (o->get("from", p)) animator->from(p->doubleValue());
            if (o->get("function", p)) animator->function(p->stringValue());
            if (o->get("param", p)) animator->param(p->doubleValue());
            if (o->get("delay", p)) animator->startDelay(p->doubleValue()*Second);
            if (o->get("afteranchor", p) && p->boolValue()) animator->runAfter(referenceAnimation);
            if (o->get("makeanchor", p) && p->boolValue()) referenceAnimation = animator;
            animator->repeat(autoreverse, cycles)->stepParams(minsteptime, stepsize)->animate(to, duration, NoOP);
          }
        }
      }
    }
  }
  #endif
  geometryChange(false);
  return ErrorPtr();
}


P44ViewPtr P44View::getView(const string aLabelOrId)
{
  if (aLabelOrId==mLabel || aLabelOrId==getId()) {
    return P44ViewPtr(this); // that's me
  }
  return P44ViewPtr(); // not found
}


#if ENABLE_JSON_APPLICATION

ErrorPtr P44View::configureFromResourceOrObj(JsonObjectPtr aResourceOrObj, const string aResourcePrefix)
{
  ErrorPtr err;
  JsonObjectPtr cfg = Application::jsonObjOrResource(aResourceOrObj, &err, aResourcePrefix);
  if (Error::isOK(err)) {
    err = configureView(cfg);
  }
  return err;
}

#endif // ENABLE_JSON_APPLICATION

#endif // ENABLE_VIEWCONFIG


#if ENABLE_VIEWSTATUS


string P44View::wrapModeToText(P44View::WrapMode aWrapMode, bool aForPositioning)
{
  const WrapModeDesc *wd = wrapModeDescs;
  string modes;
  if (aWrapMode==P44View::noWrap) modes = wd->name;
  else while ((++wd)->name && aWrapMode) {
    if (aForPositioning!=wd->forPos) continue;
    if ((aWrapMode & wd->mode) == wd->mode) {
      if (!modes.empty()) modes += "|";
      modes += wd->name;
      aWrapMode &= ~wd->mode;
    }
  }
  return modes;
}


string P44View::orientationToText(P44View::Orientation aOrientation)
{
  const OrientationDesc *od = orientationDescs;
  string ori;
  if (aOrientation==P44View::right) ori = od->name;
  else while ((++od)->name && aOrientation) {
    if ((aOrientation & od->orientation) == od->orientation) {
      if (!ori.empty()) ori += "|";
      ori += od->name;
      aOrientation &= ~od->orientation;
    }
  }
  return ori;
}


string P44View::getLabel() const
{
  if (!mLabel.empty()) return mLabel;
  return "untitled";
}


string P44View::getId() const
{
  if (!mLabel.empty()) return mLabel;
  return string_format("V_%08lx", (uint32_t)(intptr_t)this);
}


JsonObjectPtr P44View::viewStatus()
{
  JsonObjectPtr status = JsonObject::newObj();
  status->add("type", JsonObject::newString(viewTypeName()));
  status->add("label", JsonObject::newString(getLabel()));
  status->add("id", JsonObject::newString(getId()));
  status->add("x", JsonObject::newInt32(mFrame.x));
  status->add("y", JsonObject::newInt32(mFrame.y));
  status->add("dx", JsonObject::newInt32(mFrame.dx));
  status->add("dy", JsonObject::newInt32(mFrame.dy));
  status->add("content_x", JsonObject::newInt32(mContent.x));
  status->add("content_y", JsonObject::newInt32(mContent.y));
  status->add("content_dx", JsonObject::newInt32(mContent.dx));
  status->add("content_dy", JsonObject::newInt32(mContent.dy));
  status->add("rotation", JsonObject::newDouble(mContentRotation));
  status->add("color", JsonObject::newString(pixelToWebColor(mForegroundColor, true)));
  status->add("bgcolor", JsonObject::newString(pixelToWebColor(mBackgroundColor, true)));
  status->add("alpha", JsonObject::newInt32(getAlpha()));
  status->add("z_order", JsonObject::newInt32(getZOrder()));
  status->add("orientation", JsonObject::newString(orientationToText(mContentOrientation)));
  status->add("wrapmode", JsonObject::newString(wrapModeToText(getWrapMode(), false)));
  status->add("mask", JsonObject::newBool(mContentIsMask));
  status->add("invertalpha", JsonObject::newBool(mInvertAlpha));
  status->add("timingpriority", JsonObject::newBool(mLocalTimingPriority));
  #if ENABLE_ANIMATION
  status->add("animations", JsonObject::newInt64(mAnimations.size()));
  #endif
  return status;
}

#endif // ENABLE_VIEWSTATUS


#if ENABLE_ANIMATION


void P44View::geometryPropertySetter(PixelCoord *aPixelCoordP, double aNewValue)
{
  PixelCoord newValue = aNewValue;
  if (newValue!=*aPixelCoordP) {
    geometryChange(true);
    *aPixelCoordP = newValue;
    mChangedGeometry = true;
    if (mSizeToContent) moveFrameToContent(true);
    geometryChange(false);
  }
}

ValueSetterCB P44View::getGeometryPropertySetter(PixelCoord &aPixelCoord, double &aCurrentValue)
{
  aCurrentValue = aPixelCoord;
  return boost::bind(&P44View::geometryPropertySetter, this, &aPixelCoord, _1);
}


void P44View::coordPropertySetter(PixelCoord *aPixelCoordP, double aNewValue)
{
  PixelCoord newValue = aNewValue;
  if (newValue!=*aPixelCoordP) {
    *aPixelCoordP = newValue;
    makeDirty();
  }
}

ValueSetterCB P44View::getCoordPropertySetter(PixelCoord &aPixelCoord, double &aCurrentValue)
{
  aCurrentValue = aPixelCoord;
  return boost::bind(&P44View::coordPropertySetter, this, &aPixelCoord, _1);
}


ValueSetterCB P44View::getColorComponentSetter(const string aComponent, PixelColor &aPixelColor, double &aCurrentValue)
{
  if (aComponent=="r") {
    return getSingleColorComponentSetter(aPixelColor.r, aCurrentValue);
  }
  else if (aComponent=="g") {
    return getSingleColorComponentSetter(aPixelColor.g, aCurrentValue);
  }
  else if (aComponent=="b") {
    return getSingleColorComponentSetter(aPixelColor.b, aCurrentValue);
  }
  else if (aComponent=="a") {
    return getSingleColorComponentSetter(aPixelColor.a, aCurrentValue);
  }
  else if (aComponent=="hue") {
    return getDerivedColorComponentSetter(0, aPixelColor, aCurrentValue);
  }
  else if (aComponent=="saturation") {
    return getDerivedColorComponentSetter(1, aPixelColor, aCurrentValue);
  }
  else if (aComponent=="brightness") {
    return getDerivedColorComponentSetter(2, aPixelColor, aCurrentValue);
  }
  return NoOP;
}


ValueSetterCB P44View::getSingleColorComponentSetter(PixelColorComponent &aColorComponent, double &aCurrentValue)
{
  aCurrentValue = aColorComponent;
  return boost::bind(&P44View::singleColorComponentSetter, this, &aColorComponent, _1);
}

void P44View::singleColorComponentSetter(PixelColorComponent *aColorComponentP, double aNewValue)
{
  PixelColorComponent c = aNewValue;
  if (*aColorComponentP!=c) {
    *aColorComponentP = c;
    makeColorDirty();
  }
}


ValueSetterCB P44View::getDerivedColorComponentSetter(int aHSBIndex, PixelColor &aPixelColor, double &aCurrentValue)
{
  double hsb[3];
  pixelToHsb(aPixelColor, hsb[0], hsb[1], hsb[2]);
  aCurrentValue = hsb[aHSBIndex];
  return boost::bind(&P44View::derivedColorComponentSetter, this, aHSBIndex, &aPixelColor, _1);
}


void P44View::derivedColorComponentSetter(int aHSBIndex, PixelColor *aPixelColorP, double aNewValue)
{
  double hsb[3];
  pixelToHsb(*aPixelColorP, hsb[0], hsb[1], hsb[2]);
  //LOG(LOG_DEBUG, "--> Pixel = %3d, %3d, %3d -> HSB = %3.2f, %1.3f, %1.3f", aPixelColorP->r, aPixelColorP->g, aPixelColorP->b, hsb[0], hsb[1], hsb[2]);
  if (hsb[aHSBIndex]!=aNewValue) {
    hsb[aHSBIndex] = aNewValue;
    PixelColor px = hsbToPixel(hsb[0], hsb[1], hsb[2]);
    //LOG(LOG_DEBUG, "<-- Pixel = %3d, %3d, %3d <- HSB = %3.2f, %1.3f, %1.3f", px.r, px.g, px.b, hsb[0], hsb[1], hsb[2]);
    aPixelColorP->r = px.r;
    aPixelColorP->g = px.g;
    aPixelColorP->b = px.b;
    makeColorDirty();
  }
}


ValueSetterCB P44View::getPropertySetter(const string aProperty, double& aCurrentValue)
{
  if (aProperty=="alpha") {
    aCurrentValue = getAlpha();
    return boost::bind(&P44View::setAlpha, this, _1);
  }
  else if (aProperty=="rotation") {
    aCurrentValue = mContentRotation;
    return boost::bind(&P44View::setContentRotation, this, _1);
  }
  else if (aProperty=="x") {
    return getGeometryPropertySetter(mFrame.x, aCurrentValue);
  }
  else if (aProperty=="y") {
    return getGeometryPropertySetter(mFrame.y, aCurrentValue);
  }
  else if (aProperty=="dx") {
    return getGeometryPropertySetter(mFrame.dx, aCurrentValue);
  }
  else if (aProperty=="dy") {
    return getGeometryPropertySetter(mFrame.dy, aCurrentValue);
  }
  else if (aProperty=="content_x") {
    return getGeometryPropertySetter(mContent.x, aCurrentValue);
  }
  else if (aProperty=="content_y") {
    return getGeometryPropertySetter(mContent.y, aCurrentValue);
  }
  else if (aProperty=="rel_content_x") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginX, this, _1, false);
  }
  else if (aProperty=="rel_content_y") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginY, this, _1, false);
  }
  else if (aProperty=="rel_center_x") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginX, this, _1, true);
  }
  else if (aProperty=="rel_center_y") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginY, this, _1, true);
  }
  else if (aProperty=="content_dx") {
    return getGeometryPropertySetter(mContent.dx, aCurrentValue);
  }
  else if (aProperty=="content_dy") {
    return getGeometryPropertySetter(mContent.dy, aCurrentValue);
  }
  else if (aProperty.substr(0,6)=="color.") {
    return getColorComponentSetter(aProperty.substr(6), mForegroundColor, aCurrentValue);
  }
  else if (aProperty.substr(0,8)=="bgcolor.") {
    return getColorComponentSetter(aProperty.substr(8), mBackgroundColor, aCurrentValue);
  }
  // unknown
  return NoOP;
}


ValueAnimatorPtr P44View::animatorFor(const string aProperty)
{
  double startValue;
  ValueSetterCB valueSetter = getPropertySetter(aProperty, startValue);
  ValueAnimatorPtr animator = ValueAnimatorPtr(new ValueAnimator(valueSetter, false, getMinUpdateInterval())); // not self-timed
  if (animator->valid()) {
    mAnimations.push_back(animator);
    makeDirtyAndUpdate(); // to make sure animation sequence starts
  }
  return animator->from(startValue);
}


#endif // ENABLE_ANIMATION


void P44View::stopAnimations()
{
  // always available as base implementation for other animations (such as in viewseqencer)
  #if ENABLE_ANIMATION
  for (AnimationsList::iterator pos = mAnimations.begin(); pos!=mAnimations.end(); ++pos) {
    (*pos)->stop(false); // stop with no callback
  }
  mAnimations.clear();
  #endif
}


// MARK: - script support

#if P44SCRIPT_FULL_SUPPORT

#include "viewfactory.hpp"

using namespace P44Script;

ScriptObjPtr P44View::newViewObj()
{
  // base class with standard functionality
  return new P44lrgViewObj(this);
}

// findview(viewlabel)
static const BuiltInArgDesc findview_args[] = { { text } };
static const size_t findview_numargs = sizeof(findview_args)/sizeof(BuiltInArgDesc);
static void findview_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  P44ViewPtr foundView = v->view()->getView(f->arg(0)->stringValue());
  if (foundView) {
    f->finish(foundView->newViewObj());
    return;
  }
  f->finish(new AnnotatedNullValue("no such view"));
}


// addview(view)
static const BuiltInArgDesc addview_args[] = { { object } };
static const size_t addview_numargs = sizeof(addview_args)/sizeof(BuiltInArgDesc);
static void addview_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  P44lrgViewObj* subview = dynamic_cast<P44lrgViewObj*>(f->arg(0).get());
  if (!subview) {
    f->finish(new ErrorValue(ScriptError::Invalid, "argument must be a view"));
    return;
  }
  if (!v->view()->addSubView(subview->view())) {
    f->finish(new ErrorValue(ScriptError::Invalid, "cannot add subview here"));
    return;
  }
  f->finish(subview);
}


static JsonObjectPtr viewConfigFromArg(ScriptObjPtr aArg, ErrorPtr &aErr)
{
  #if SCRIPTING_JSON_SUPPORT
  if (aArg->hasType(structured)) {
    // is already a JSON value, use it as-is
    return aArg->jsonValue();
  }
  else
  #endif
  {
    // JSON from string (or file if we have a JSON app)
    string viewConfig = aArg->stringValue();
    #if ENABLE_JSON_APPLICATION
    return Application::jsonObjOrResource(viewConfig, &aErr);
    #else
    return JsonObject::objFromText(viewConfig.c_str(), -1, &aErr);
    #endif
  }
}


// configure(jsonconfig|filename)
static const BuiltInArgDesc configure_args[] = { { text|object } };
static const size_t configure_numargs = sizeof(configure_args)/sizeof(BuiltInArgDesc);
static void configure_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  ErrorPtr err;
  JsonObjectPtr viewCfgJSON = viewConfigFromArg(f->arg(0), err);
  if (Error::isOK(err)) {
    err = v->view()->configureView(viewCfgJSON);
    if (Error::isOK(err)) {
      v->view()->requestUpdate(); // to make sure changes are applied
    }
  }
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  f->finish(v); // return view itself to allow chaining
}


#if ENABLE_VIEWSTATUS

// status()
static void status_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  f->finish(ScriptObj::valueFromJSON(v->view()->viewStatus()));
}

#endif

// set(propertyname, newvalue)   convenience function to set a single property
static const BuiltInArgDesc set_args[] = { { text }, { anyvalid } };
static const size_t set_numargs = sizeof(set_args)/sizeof(BuiltInArgDesc);
static void set_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  JsonObjectPtr viewCfgJSON = JsonObject::newObj();
  viewCfgJSON->add(f->arg(0)->stringValue().c_str(), f->arg(1)->jsonValue());
  if (Error::ok(v->view()->configureView(viewCfgJSON))) {
    v->view()->requestUpdate(); // to make sure changes are applied
  }
  f->finish(v); // return view itself to allow chaining
}


// stopanimations()     stop animations
static void stopanimations_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->stopAnimations();
  f->finish(v); // return view itself to allow chaining
}


// remove()     remove from subview
static void remove_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  bool removed = v->view()->removeFromParent();
  f->finish(new BoolValue(removed)); // true if actually removed
}



// render()     trigger rendering
static void render_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->requestUpdate(); // to make sure changes are applied
  f->finish(v); // return view itself to allow chaining
}


// parent()     return parent view
static void parent_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  P44ViewPtr parent = v->view()->getParent();
  if (!parent) {
    f->finish(new AnnotatedNullValue("no parent view"));
    return;
  }
  f->finish(parent->newViewObj());
}


// clear()     clear view
static void clear_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->clear(); // to make sure changes are applied
  f->finish(v); // return view itself to allow chaining
}



#if ENABLE_ANIMATION

// animator(propertyname)
static const BuiltInArgDesc animator_args[] = { { text } };
static const size_t animator_numargs = sizeof(animator_args)/sizeof(BuiltInArgDesc);
static void animator_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  f->finish(new ValueAnimatorObj(v->view()->animatorFor(f->arg(0)->stringValue())));
}

#endif


static const BuiltinMemberDescriptor viewFunctions[] = {
  { "findview", executable|object, findview_numargs, findview_args, &findview_func },
  { "addview", executable|object, addview_numargs, addview_args, &addview_func },
  { "configure", executable|object, configure_numargs, configure_args, &configure_func },
  { "set", executable|object, set_numargs, set_args, &set_func },
  { "render", executable|object, 0, NULL, &render_func },
  { "remove", executable|numeric, 0, NULL, &remove_func },
  { "parent", executable|object, 0, NULL, &parent_func },
  { "clear", executable|object, 0, NULL, &clear_func },
  #if ENABLE_ANIMATION
  { "animator", executable|object, animator_numargs, animator_args, &animator_func },
  { "stopanimations", executable|object, 0, NULL, &stopanimations_func },
  #endif
  #if ENABLE_VIEWSTATUS
  { "status", executable|object, 0, NULL, &status_func },
  #endif
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedViewFunctionLookupP = NULL;

P44lrgViewObj::P44lrgViewObj(P44ViewPtr aView) :
  mView(aView)
{
  if (sharedViewFunctionLookupP==NULL) {
    sharedViewFunctionLookupP = new BuiltInMemberLookup(viewFunctions);
    sharedViewFunctionLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedViewFunctionLookupP);
}


// hsv(hue, sat, bri) // convert to webcolor string
static const BuiltInArgDesc hsv_args[] = { { numeric }, { numeric+optionalarg }, { numeric+optionalarg } };
static const size_t hsv_numargs = sizeof(hsv_args)/sizeof(BuiltInArgDesc);
static void hsv_func(BuiltinFunctionContextPtr f)
{
  // hsv(hue, sat, bri) // convert to webcolor string
  double h = f->arg(0)->doubleValue();
  double s = 1.0;
  double b = 1.0;
  if (f->numArgs()>1) {
    s = f->arg(1)->doubleValue();
    if (f->numArgs()>2) {
      b = f->arg(2)->doubleValue();
    }
  }
  PixelColor p = hsbToPixel(h, s, b);
  f->finish(new StringValue(pixelToWebColor(p, false)));
}


// makeview(jsonconfig|filename)
static const BuiltInArgDesc makeview_args[] = { { text|object } };
static const size_t makeview_numargs = sizeof(makeview_args)/sizeof(BuiltInArgDesc);
static void makeview_func(BuiltinFunctionContextPtr f)
{
  P44ViewPtr newView;
  ErrorPtr err;
  JsonObjectPtr viewCfgJSON = viewConfigFromArg(f->arg(0), err);
  if (Error::isOK(err)) {
    err = createViewFromConfig(viewCfgJSON, newView, P44ViewPtr());
  }
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  f->finish(newView->newViewObj());
}



static ScriptObjPtr lrg_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite)
{
  P44lrgLookup* l = dynamic_cast<P44lrgLookup*>(&aMemberLookup);
  P44ViewPtr rv = l->rootView();
  if (!rv) return new AnnotatedNullValue("no root view");
  return rv->newViewObj();
}


static const BuiltinMemberDescriptor lrgGlobals[] = {
  { "makeview", executable|object, makeview_numargs, makeview_args, &makeview_func },
  { "lrg", builtinmember, 0, NULL, (BuiltinFunctionImplementation)&lrg_accessor }, // Note: correct '.accessor=&lrg_accessor' form does not work with OpenWrt g++, so need ugly cast here
  { "hsv", executable|text, hsv_numargs, hsv_args, &hsv_func },
  { NULL } // terminator
};


P44lrgLookup::P44lrgLookup(P44ViewPtr *aRootViewPtrP) :
  inherited(lrgGlobals),
  mRootViewPtrP(aRootViewPtrP)
{
}

#endif // P44SCRIPT_FULL_SUPPORT
