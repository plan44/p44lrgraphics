//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2016-2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#include "p44view.hpp"
#include "viewfactory.hpp" // for registering views

#include <math.h>

using namespace p44;

// MARK: ===== ViewRegistrar

ViewRegistrar::ViewRegistrar(const char* aName, ViewConstructor aConstructor)
{
  p44::registerView(aName, aConstructor);
}


// MARK: ===== View

static ViewRegistrar r(P44View::staticTypeName(), &P44View::newInstance);

P44View::P44View() :
  mParentView(NULL),
  mDirty(false),
  mRequestStepBeforeOrAt(Infinite),
  mMinUpdateInterval(0),
  mStepShowTime(Never),
  mStepRealTime(Never),
  mChangeTrackingLevel(0),
  mChangedGeometry(false),
  mChangedColoring(false),
  mChangedTransform(false),
  mSubsampling(true),
  mSizeToContent(false)
{
  resetTransforms();
  mDirty = false; // reset again, because resetTransforms() sets it
  setFrame(zeroRect);
  // default to normal orientation
  mContentOrientation = right;
  // default to clip to frame, no content repeating
  mFramingMode = clipXY;
  // no automatic adjustment when parent's geometry changes
  mAutoAdjust = noFraming;
  // default content size is same as view's
  setContent(zeroRect);
  mBackgroundColor = { .r=0, .g=0, .b=0, .a=0 }; // transparent background,
  mForegroundColor = { .r=255, .g=255, .b=255, .a=255 }; // fully white foreground...
  mAlpha = 255; // but content pixels passed trough 1:1
  mZOrder = 0; // none in particular
  mContentIsMask = false; // content color will be used
  mInvertAlpha = false; // inverted mask
  mLocalTimingPriority = true;
  mAlignAnimationSteps = true;
  mHaltWhenHidden = true;
  mMaskChildDirtyUntil = Never;
}


P44View::~P44View()
{
  announceChanges(true); // unbalanced, avoid any further dependency updates while finally deleting view
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
  normalizeRect(aArea);
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


void P44View::announceChanges(bool aStart)
{
  if (aStart) {
    if (mChangeTrackingLevel<=0) {
      beginChanges();
    }
    mChangeTrackingLevel++;
  }
  else {
    if (mChangeTrackingLevel>0) {
      mChangeTrackingLevel--;
      if (mChangeTrackingLevel==0) {
        finalizeChanges();
      }
    }
  }
}


void P44View::flagChange(bool &aChangeFlag)
{
  if (mChangeTrackingLevel>0) {
    // just set the flag
    aChangeFlag = true;
    return;
  }
  // No change tracking in progress -> this is a singular change
  // Note: if the change flag is already set, this can only mean we are called
  //   from finalizeChanges(), so DO NOT RECURSE here
  if (!aChangeFlag) {
    aChangeFlag = true;
    // no change bracket open - singular change that needs finalisation right now
    finalizeChanges();
    aChangeFlag = false; // just to make sure, finalizing should reset the flag
  }
}


void P44View::beginChanges()
{
  // start tracking changes
  mChangedGeometry = false;
  mChangedColoring = false;
  mChangedTransform = false;
  mPreviousFrame = mFrame;
  mPreviousContent = mContent;
}


void P44View::finalizeChanges()
{
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
      // Note: as we are passing in the frames, it is safe when the following calls recursively calls geometryChanged again
      //   except that it must not do so unconditionally to prevent endless recursion
      mParentView->childGeometryChanged(this, mPreviousFrame, mPreviousContent);
    }
    mChangedGeometry = false;
  }
  if (mChangedColoring) {
    FOCUSLOG("View '%s' changed coloring", getLabel().c_str());
    makeColorDirty();
    mChangedColoring = false;
  }
  if (mChangedTransform) {
    if (mContentRotation!=0) {
      // Calculate rotation multipliers
      double rotPi = FP_DBL_VAL(mContentRotation)*M_PI/180;
      mRotSin = FP_FROM_DBL(sin(rotPi));
      mRotCos = FP_FROM_DBL(cos(rotPi));
    }
    else {
      // no rotation
      mRotCos = FP_FROM_INT(1);
      mRotSin = FP_FROM_INT(0);
    }
    recalculateScrollDependencies();
    mChangedTransform = false;
    makeDirty();
  }
}


void P44View::recalculateScrollDependencies()
{
  // but non-integer scrolling or scaling might need fractional sampling
  mNeedsFractionalSampling = mContentRotation!=0 || FP_HASFRAC(mScrollX) || FP_HASFRAC(mScrollY) || mShrinkX!=FP_FROM_INT(1) || mShrinkY!=FP_FROM_INT(1);
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
  normalizeRect(aNewRect);
  if (aNewRect.x!=aRect.x) {
    aRect.x = aNewRect.x;
    flagGeometryChange();
  }
  if (aNewRect.y!=aRect.y) {
    aRect.y = aNewRect.y;
    flagGeometryChange();
  }
  if (aNewRect.dx!=aRect.dx) {
    aRect.dx = aNewRect.dx;
    flagGeometryChange();
  }
  if (aNewRect.dy!=aRect.dy) {
    aRect.dy = aNewRect.dy;
    flagGeometryChange();
  }
}



void P44View::setFrame(PixelRect aFrame)
{
  announceChanges(true);
  changeGeometryRect(mFrame, aFrame);
  announceChanges(false);
}


void P44View::setParent(P44ViewPtr aParentView)
{
  mParentView = aParentView.get();
}


P44ViewPtr P44View::getParent()
{
  return P44ViewPtr(mParentView);
}


bool P44View::isParentOrThis(P44ViewPtr aRefView)
{
  if (aRefView==this) return true;
  if (mParentView) return mParentView->isParentOrThis(aRefView);
  return false;
}


void P44View::setContent(PixelRect aContent)
{
  announceChanges(true);
  changeGeometryRect(mContent, aContent);
  if (mSizeToContent) {
    moveFrameToContent(true);
  }
  announceChanges(false);
};


void P44View::setContentSize(PixelPoint aSize)
{
  announceChanges(true);
  changeGeometryRect(mContent, { mContent.x, mContent.y, aSize.x, aSize.y });
  if (mChangedGeometry && mSizeToContent) moveFrameToContent(true);
  announceChanges(false);
};


void P44View::setContentOrigin(PixelPoint aOrigin)
{
  announceChanges(true);
  changeGeometryRect(mContent, { aOrigin.x, aOrigin.y, mContent.dx, mContent.dy });
  announceChanges(false);
};


void P44View::setRelativeContentOrigin(double aRelX, double aRelY, bool aCentered)
{
  announceChanges(true);
  setRelativeContentOriginX(aRelX, aCentered);
  setRelativeContentOriginY(aRelY, aCentered);
  announceChanges(false);
}

void P44View::setRelativeContentOriginX(double aRelX, bool aCentered)
{
  // standard version, content origin is a corner of the relevant area
  announceChanges(true);
  changeGeometryRect(mContent, { (int)(aRelX*max(mContent.dx,mFrame.dx)+(aCentered ? mFrame.dx/2 : 0)), mContent.y, mContent.dx, mContent.dy });
  announceChanges(false);
}


void P44View::setRelativeContentOriginY(double aRelY, bool aCentered)
{
  // standard version, content origin is a corner of the relevant area
  announceChanges(true);
  changeGeometryRect(mContent, { mContent.x, (int)(aRelY*max(mContent.dy,mFrame.dy)+(aCentered ? mFrame.dy/2 : 0)), mContent.dx, mContent.dy });
  announceChanges(false);
}


void P44View::setRelativeContentSize(double aRelDx, double aRelDy, bool aRelativeToLargerFrameDimension)
{
  PixelPoint sz = getFrameSize();
  orientateCoord(sz); // maybe flipped
  if (aRelativeToLargerFrameDimension) {
    sz.x = max(sz.x, sz.y);
    sz.y = sz.x;
  }
  sz.x *= aRelDx*FP_DBL_VAL(mShrinkX); // if content is shrunken, size must be boosted to appear same size again
  sz.y *= aRelDy*FP_DBL_VAL(mShrinkY); // if content is shrunken, size must be boosted to appear same size again
  announceChanges(true);
  // Note: not using setContentSize() because we do not want auto-reframing when we adjust relative to frame
  setContentDx(sz.x);
  setContentDy(sz.y);
  announceChanges(false);
}


void P44View::setContentAppearanceSize(double aRelDx, double aRelDy)
{
  // by default, just 1:1 relative to frame size
  setRelativeContentSize(aRelDx, aRelDy, false);
}


void P44View::setRelativeContentSizeX(double aRelDx)
{
  PixelPoint sz = getFrameSize();
  orientateCoord(sz); // maybe flipped
  sz.x *= aRelDx*FP_DBL_VAL(mShrinkX); // if content is shrunken, size must be boosted to appear same size again
  // Note: not using setContentSize() because we do not want auto-reframing when we adjust relative to frame
  setContentDx(sz.x);
}


void P44View::setRelativeContentSizeY(double aRelDy)
{
  PixelPoint sz = getFrameSize();
  orientateCoord(sz); // maybe flipped
  sz.y *= aRelDy*FP_DBL_VAL(mShrinkY); // if content is shrunken, size must be boosted to appear same size again
  // Note: not using setContentSize() because we do not want auto-reframing when we adjust relative to frame
  setContentDy(sz.y);
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
  announceChanges(true);
  if (aResize) sizeFrameToContent();
  PixelRect f;
  contentRectAsViewCoord(f);
  // move frame to the place where the content rectangle did appear so far...
  changeGeometryRect(mFrame, f);
  // ...which means that no content offset is needed any more (we've compensated it by moving the frame)
  mContent.x = 0; mContent.y = 0;
  announceChanges(false);
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


void P44View::autoAdjustTo(PixelRect aReferenceRect)
{
  normalizeRect(aReferenceRect);
  announceChanges(true);
  if (mAutoAdjust & adjustmentMask) {
    if ((mAutoAdjust & fillX)==fillX) {
      mFrame.dx = aReferenceRect.dx;
    }
    if ((mAutoAdjust & fillY)==fillY) {
      mFrame.dy = aReferenceRect.dy;
    }
    if ((mAutoAdjust & noAdjust)!=noAdjust) {
      setFullFrameContent();
    }
  }
  announceChanges(false);
}



void P44View::clear()
{
  stopAnimations();
  // as the only thing a P44View can display is the content rect in foreground color, reset it here
  // Note: subclasses will not always call inherited::clear() as they might want to retain the content rectangle,
  //       and just remove the actual content data
  setContentSize({0, 0});
}


void P44View::resetTransforms()
{
  announceChanges(true);
  mContentRotation = FP_FROM_INT(0);
  mScrollX = FP_FROM_INT(0);
  mScrollY = FP_FROM_INT(0);
  mShrinkX = FP_FROM_INT(1);
  mShrinkY = FP_FROM_INT(1);
  mChangedTransform = true;
  announceChanges(false);
}


// MARK: ===== updating

void P44View::updated()
{
  mDirty = false;
  mRequestStepBeforeOrAt = Infinite;
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


void P44View::makeDirtyAndUpdate()
{
  // make dirty locally
  mDirty = true;
  // request a step at the root view level.
  // When called from within stepping, ASAP after stepping (request==mStepShowTime), otherwise immediately
  requestUpdate(DEFINED_TIME(mStepShowTime) ? mStepShowTime : 0);
}


void P44View::makeDirty()
{
  if (getVisible()) {
    makeDirtyAndUpdate(); // TODO: do we really need this?
    //mDirty = true; // invisible views
  }
}


void P44View::makeAlphaDirtry()
{
  makeDirtyAndUpdate(); // TODO: do we really need this?
  // mDirty = true; // unconditionally
}


void P44View::makeColorDirty()
{
  recalculateColoring();
  makeDirty();
}


bool P44View::reportDirtyChilds()
{
  if (mAlpha==0) return false; // as long as this is invisible, dirty children are irrelevant
  if (DEFINED_TIME(mMaskChildDirtyUntil)) {
    if (stepShowTime()<mMaskChildDirtyUntil) {
      return false;
    }
    mMaskChildDirtyUntil = 0;
  }
  return true;
}


void P44View::updateNextCall(MLMicroSeconds &aNextCall, MLMicroSeconds aCallCandidate, MLMicroSeconds aCandidatePriorityUntil)
{
  if (mLocalTimingPriority && DEFINED_TIME(aCandidatePriorityUntil) && DEFINED_TIME(aCallCandidate) && aCallCandidate<aCandidatePriorityUntil) {
    // children must not cause "dirty" before candidate time is over
    mMaskChildDirtyUntil = (aCallCandidate-mStepShowTime)*2+mStepShowTime; // duplicate to make sure candidate execution has some time to happen BEFORE dirty is unblocked
  }
  if (!DEFINED_TIME(aNextCall) || (DEFINED_TIME(aCallCandidate) && aCallCandidate<aNextCall)) {
    // candidate wins
    aNextCall = aCallCandidate;
  }
}



void P44View::requestUpdate(MLMicroSeconds aBeforeOrAt)
{
  FOCUSLOG("requestUpdate(%lld) called for view@%p, mStepShowTime=%lld", aBeforeOrAt, this, mStepShowTime);
  P44View *p = this;
  while (p->mParentView) {
    if (DEFINED_TIME(aBeforeOrAt) && p->mRequestStepBeforeOrAt==aBeforeOrAt) return;  // do not request the same time multiple times (except for 0==immediate)
    p->mRequestStepBeforeOrAt = aBeforeOrAt; // mark having requested update all the way down to root, updated() will be called on all views to clear it
    p = p->mParentView;
  }
  // now p = root view
  FOCUSLOG("- root view@%p (from view@%p)", p, this);
  if (DEFINED_TIME(aBeforeOrAt)) {
    // keep already set earlier time (or immediate request == 0), if any
    if (p->mRequestStepBeforeOrAt!=Infinite && aBeforeOrAt>p->mRequestStepBeforeOrAt) {
      FOCUSLOG("- root already has earlier request, keep it: %lld", p->mRequestStepBeforeOrAt);
      aBeforeOrAt = p->mRequestStepBeforeOrAt;
    }
  }
  p->mRequestStepBeforeOrAt = aBeforeOrAt; // only request once
  if (!DEFINED_TIME(aBeforeOrAt) && p->mNeedUpdateCB) {
    FOCUSLOG("- requesting immediate update");
    // there is a needUpdate callback here
    // DO NOT call it directly, but from mainloop, so receiver can safely call
    // back into any view object method without causing recursions
    MainLoop::currentMainLoop().executeNow(p->mNeedUpdateCB);
  }
  else {
    FOCUSLOG("- requesting auxiliary step at %lld", p->mRequestStepBeforeOrAt);
  }
}


MLMicroSeconds P44View::step(MLMicroSeconds aStepShowTime, MLMicroSeconds aPriorityUntil, MLMicroSeconds aStepRealTime)
{
  mRequestStepBeforeOrAt = Infinite; // no step request pending any more
  // this is the entry point, remember those for further use
  mStepShowTime = aStepShowTime; // also serves as marker we are within step calculation
  mStepRealTime = aStepRealTime;
  #if ENABLE_ANIMATION
  if (mHaltWhenHidden && !getVisible()) return Infinite;
  // run local animations first to get the step time these would want
  FOCUSLOG("### starting animations");
  MLMicroSeconds nextAnimationStep = Infinite;
  AnimationsList::iterator pos = mAnimations.begin();
  while (pos != mAnimations.end()) {
    ValueAnimatorPtr animator = (*pos);
    MLMicroSeconds animatorStep = animator->step(mStepShowTime);
    if (!animator->inProgress()) {
      // this animation is done, remove it from the list
      pos = mAnimations.erase(pos);
      continue;
    }
    updateNextCall(nextAnimationStep, animatorStep);
    pos++;
  }
  FOCUSLOG("### done with animations, nextAnimationStep=%lld, mStepShowTime=%lld", nextAnimationStep, mStepShowTime);
  // now call the actual virtual calculation method
  MLMicroSeconds nextStep = stepInternal(aPriorityUntil);
  // animation alignment means we ignore the step time returned from animation steppers
  // when we know there's a step from the view hierarchy within the priority window.
  // This means animations are synced e.g. to the pace of scrollers and thus will not
  // disturb them with extra updates (even when not childs of the scroller, which
  // case is handled by the mMaskChildDirtyUntil mechanism)
  if (mAlignAnimationSteps) {
    // request a global step time in case nobody else sets none
    if (DEFINED_TIME(nextAnimationStep)) requestUpdate(nextAnimationStep);
  }
  else {
    // alignment off, request our step anyway
    updateNextCall(nextStep, nextAnimationStep);
  }
  // now, if we are the root view, maybe request an update when regular hierarchy did not
  if (!mParentView && (!DEFINED_TIME(nextStep) || nextStep>aPriorityUntil) && DEFINED_TIME(mRequestStepBeforeOrAt) && mRequestStepBeforeOrAt<aPriorityUntil) {
    FOCUSLOG("- root: request closer step at %lld, nextStep was %lld, diff=%+lld, priorityuntil=%lld", mRequestStepBeforeOrAt, nextStep, nextStep-mRequestStepBeforeOrAt, aPriorityUntil);
    nextStep = mRequestStepBeforeOrAt;
    mRequestStepBeforeOrAt = Infinite;
  }
  mStepShowTime = Infinite; // no longer within step calculation
  return nextStep;
  #else
  return stepInternal(aPriorityUntil);
  #endif  // ENABLE_ANIMATION
}



MLMicroSeconds P44View::stepInternal(MLMicroSeconds aPriorityUntil)
{
  return Infinite; // No steps (other than property animators that are handled in step())
}


void P44View::setAlpha(PixelColorComponent aAlpha)
{
  if (mAlpha!=aAlpha) {
    mAlpha = aAlpha;
    makeAlphaDirtry();
  }
}


void P44View::setZOrder(int aZOrder)
{
  announceChanges(true);
  if (mZOrder!=aZOrder) {
    mZOrder = aZOrder;
    flagGeometryChange();
  }
  announceChanges(false);
}



#define SHOW_ORIGIN 0


PixelColor P44View::colorAt(PixelPoint aPt)
{
  // default is background color
  if (mAlpha==0) return transparent; // optimisation
  // aPt is parent view coordinates
  aPt.x -= mFrame.x;
  aPt.y -= mFrame.y;
  // aPt is relative to frame origin now
  return colorInFrameAt(aPt);
}


PixelColor P44View::colorInFrameAt(PixelPoint aPt)
{
  if (mAlpha==0 || mShrinkX==0 || mShrinkY==0) return transparent; // optimisation
  PixelColor pc = mBackgroundColor;
  // optionally clip content to frame
  if (mFramingMode&clipXY && (
    ((mFramingMode&clipXmin) && aPt.x<0) ||
    ((mFramingMode&clipXmax) && aPt.x>=mFrame.dx) ||
    ((mFramingMode&clipYmin) && aPt.y<0) ||
    ((mFramingMode&clipYmax) && aPt.y>=mFrame.dy)
  )) {
    // aPt is clipped out
    pc.a = 0; // invisible
  }
  else {
    // aPt is not clipped out, we need to consult content to get the color
    // - optionally repeat frame's contents in selected directions outside the frame
    //   (i.e. wrap outside input coordinates back into x..x+dx and y..y+dy)
    if (mFrame.dx>0) {
      while ((mFramingMode&repeatXmin) && aPt.x<0) aPt.x+=mFrame.dx;
      while ((mFramingMode&repeatXmax) && aPt.x>=mFrame.dx) aPt.x-=mFrame.dx;
    }
    if (mFrame.dy>0) {
      while ((mFramingMode&repeatYmin) && aPt.y<0) aPt.y+=mFrame.dy;
      while ((mFramingMode&repeatYmax) && aPt.y>=mFrame.dy) aPt.y-=mFrame.dy;
    }
    // re-orient in frame and make relative to content origin
    inFrameToContentCoord(aPt);
    // Until here, we are still in the pixel grid of the frame
    if (!mNeedsFractionalSampling) {
      // just apply integer scroll
      aPt.x += FP_INT_VAL(mScrollX);
      aPt.y += FP_INT_VAL(mScrollY);
      // get the pixel
      pc = contentColorAt(aPt);
    }
    else {
      // apply rotation first, then scroll so we can scroll and shrink in any direction
      FracValue rX = FP_FACTOR_FROM_INT(aPt.x);
      FracValue rY = FP_FACTOR_FROM_INT(aPt.y);
      FracValue samplingX = rX*mRotCos-rY*mRotSin;
      FracValue samplingY = rX*mRotSin+rY*mRotCos;
      // apply shrink and scroll (Important: scroll is in content coordinates, after rotation
      samplingX = FP_MUL_CORR(samplingX*mShrinkX) + mScrollX;
      samplingY = FP_MUL_CORR(samplingY*mShrinkY) + mScrollY;
      // Note: subsampling is not centered, but always right/up from the sample point
      // samplingX/Y is now where we must sample from content
      // mShrinkX/Y is the size of the area we need to sample from
      PixelPoint firstPt;
      if (mSubsampling) {
        // - the integer coordinates to start sampling
        firstPt.x = FP_INT_FLOOR(samplingX);
        firstPt.y = FP_INT_FLOOR(samplingY);
        // - the integer coordinates to end sampling
        PixelPoint lastPt;
        lastPt.x = FP_INT_CEIL(samplingX+mShrinkX)-1;
        lastPt.y = FP_INT_CEIL(samplingY+mShrinkY)-1;
        // - the possibly fractional weight at the start
        FracValue firstPixelWeightX = FP_FROM_INT(firstPt.x+1)-samplingX;
        FracValue firstPixelWeightY = FP_FROM_INT(firstPt.y+1)-samplingY;
        // - the possibly fractional weight at the end
        FracValue lastPixelWeightX = samplingX+mShrinkX - FP_FROM_INT(lastPt.x);
        FracValue lastPixelWeightY = samplingY+mShrinkY - FP_FROM_INT(lastPt.y);
        // averaging loop
        // - accumulators
        FracValue r, g, b, a, tw;
        prepareAverage(r, g, b, a, tw);
        FracValue weightY = firstPixelWeightY;
        PixelPoint samplingPt; // sampling point coordinate in content
        for (samplingPt.y = firstPt.y; samplingPt.y<=lastPt.y; samplingPt.y++) {
          FracValue weightX = firstPixelWeightX;
          for (samplingPt.x = firstPt.x; samplingPt.x<=lastPt.x; samplingPt.x++) {
            // the color to sample
            pc = contentColorAt(samplingPt);
            averagePixelPower(r, g, b, a, tw, pc, FP_MUL_CORR(weightY*weightX));
            // adjust the weight
            weightX = samplingPt.x+1==lastPt.x ? lastPixelWeightX : FP_FROM_INT(1); // possibly fractional weight on last pixel
          }
          weightY = samplingPt.y+1==lastPt.y ? lastPixelWeightY : FP_FROM_INT(1); // possibly fractional weight on last pixel
        }
        pc = averagedPixelResult(r, g, b, a, tw);
      }
      else {
        // just get color from integer point
        firstPt.x = FP_INT_ROUND(samplingX);
        firstPt.y = FP_INT_ROUND(samplingY);
        pc = contentColorAt(firstPt);
      }
    }
    // now pc is the color at specified frame coordinate point
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


void p44::normalizeRect(PixelRect &aRect)
{
  if (aRect.dx<0) { aRect.x+=aRect.dx; aRect.dx = -aRect.dx; }
  if (aRect.dy<0) { aRect.y+=aRect.dy; aRect.dy = -aRect.dy; }
}



#if ENABLE_VIEWCONFIG

// MARK: ===== config utilities

typedef struct {
  P44View::Orientation orientation;
  const char *name;
} OrientationDesc;
static const OrientationDesc orientationDescs[] = {
  { P44View::right, "right" }, //  untransformed X goes left to right, Y goes up
  // Note: rest of table should be ordered such that multi-bit combinations come first (for orientationToText)
  { P44View::up, "up" }, //  X goes up, Y goes left
  { P44View::left, "left" }, //  X goes left, Y goes down
  { P44View::down, "down" }, //  X goes down, Y goes right
  { P44View::xy_swap, "swapXY" }, //  swap x and y
  { P44View::x_flip, "flipX" }, //  flip x
  { P44View::y_flip, "flipY" }, //  flip y
  { 0, NULL }
};



P44View::Orientation P44View::textToOrientation(const char *aOrientationText)
{
  Orientation o = P44View::right;
  while (aOrientationText) {
    size_t n = 0;
    while (aOrientationText[n] && aOrientationText[n]!='|') n++;
    for (const OrientationDesc *od = orientationDescs; od->name; od++) {
      if (uequals(aOrientationText, od->name, n)) {
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
  P44View::FramingMode mode;
  const char *name;
  bool forPos;
} FramingModeDesc;
static const FramingModeDesc framingModeDescs[] = {
  { P44View::noFraming, "none", false }, // do not repeat or clip = content spills over frame
  // Note: rest of table should be ordered such that multi-bit combinations come first (for framingModeToText)
  { P44View::repeatXY, "repeatXY", false }, // repeat frame in all directions
  { P44View::repeatX, "repeatX", false }, // repeat frame in both X directions
  { P44View::repeatY, "repeatY", false }, // repeat frame in both Y directions
  { P44View::repeatXmin, "repeatXmin", false }, // repeat frame in X direction for X<frame area
  { P44View::repeatXmax, "repeatXmax", false }, // repeat frame in X direction for X>=frame area
  { P44View::repeatYmin, "repeatYmin", false }, // repeat frame in Y direction for Y<frame area
  { P44View::repeatYmax, "repeatYmax", false }, // repeat frame in Y direction for Y>=frame area
  { P44View::clipXY, "clipXY", false }, // clip content to frame rectangle
  { P44View::clipY, "clipY", false }, // clip content vertically
  { P44View::clipX, "clipX", false }, //  clip content horizontally
  { P44View::clipXmin, "clipXmin", false }, // clip content left of frame area
  { P44View::clipXmax, "clipXmax", false }, // clip content right of frame area
  { P44View::clipYmin, "clipYmin", false }, // clip content below frame area
  { P44View::clipYmax, "clipYmax", false }, // clip content above frame area
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


P44View::FramingMode P44View::textToFramingMode(const char *aFramingModeText)
{
  FramingMode m = P44View::noFraming;
  while (aFramingModeText) {
    size_t n = 0;
    while (aFramingModeText[n] && aFramingModeText[n]!='|') n++;
    for (const FramingModeDesc *wd = framingModeDescs; wd->name && n>0; wd++) {
      if (uequals(aFramingModeText, wd->name, n)) {
        m |= wd->mode;
      }
    }
    aFramingModeText += n;
    if (*aFramingModeText==0) break;
    aFramingModeText++; // skip |
  }
  return m;
}


// MARK: ===== view configuration

#if ENABLE_P44SCRIPT

using namespace P44Script;

ErrorPtr P44View::configureView(JsonObjectPtr aViewConfig)
{
  announceChanges(true);
  string name;
  JsonObjectPtr val;
  ScriptObjPtr vo = newViewObj();
  // these must be postponed to after reading other properties
  bool fullFrameContent = false;
  JsonObjectPtr animationCfg;
  // now
  aViewConfig->resetKeyIteration();
  while (aViewConfig->nextKeyValue(name, val)) {
    // catch special procedural cases
    if (name=="clear" && val->boolValue()) {
      clear();
    }
    else if (name=="fullframe" && val->boolValue()) {
      fullFrameContent = true;
    }
    else if (name=="stopanimations" && val->boolValue()) {
      stopAnimations();
    }
    else if (name=="animate") {
      animationCfg = val;
    }
    // write-onlys for backward compatibility
    else if (name=="rel_content_x") {
      setRelativeContentOriginX(val->doubleValue(), false);
    }
    else if (name=="rel_content_y") {
      setRelativeContentOriginY(val->doubleValue(), false);
    }
    else if (name=="rel_center_x") {
      setRelativeContentOriginX(val->doubleValue(), true);
    }
    else if (name=="rel_center_y") {
      setRelativeContentOriginY(val->doubleValue(), true);
    }
    else {
      // normal member access
      ScriptObjPtr lv = vo->memberByName(name, lvalue);
      if (lv) lv->assignLValue(NoOP, ScriptObj::valueFromJSON(val));
    }
  }
  // now apply postponed ones
  if (fullFrameContent) setFullFrameContent();
  if (animationCfg) configureAnimation(animationCfg);
  if (mChangedGeometry && mSizeToContent) {
    moveFrameToContent(true);
  }
  announceChanges(false);
  return ErrorPtr();
}

#else

// legacy configure

ErrorPtr P44View::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  announceChanges(true);
  if (aViewConfig->get("label", o)) {
    setLabel(o->stringValue());
  }
  if (aViewConfig->get("clear", o)) {
    if(o->boolValue()) clear();
  }
  if (aViewConfig->get("x", o)) {
    setX(o->int32Value());
  }
  if (aViewConfig->get("y", o)) {
    setY(o->int32Value());
  }
  if (aViewConfig->get("dx", o)) {
    setDx(o->int32Value());
  }
  if (aViewConfig->get("dy", o)) {
    setDy(o->int32Value());
  }
  if (aViewConfig->get("bgcolor", o)) {
    setBgcolor(o->stringValue());
  }
  if (aViewConfig->get("color", o)) {
    setColor(o->stringValue());
  }
  if (aViewConfig->get("alpha", o)) {
    setAlpha(o->int32Value());
  }
  if (aViewConfig->get("z_order", o)) {
    setZOrder(o->int32Value());
  }
  if (aViewConfig->get("framing", o)) { // Formerly: "wrapmode"
    if (o->isType(json_type_string)) {
      setFramingMode(textToFramingMode(o->c_strValue()));
    }
    else {
      setFramingMode(o->int32Value());
    }
  }
  if (aViewConfig->get("mask", o)) {
    setContentIsMask(o->boolValue());
  }
  if (aViewConfig->get("invertalpha", o)) {
    setInvertAlpha(o->boolValue());
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
    setContentX(o->int32Value());
  }
  if (aViewConfig->get("content_y", o)) {
    setContentY(o->int32Value());
  }
  if (aViewConfig->get("content_dx", o)) {
    setContentDx(o->int32Value());
  }
  if (aViewConfig->get("content_dy", o)) {
    setContentDy(o->int32Value());
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
  if (aViewConfig->get("scroll_x", o)) {
    setScrollX(o->doubleValue());
  }
  if (aViewConfig->get("scroll_y", o)) {
    setScrollY(o->doubleValue());
  }
  if (aViewConfig->get("zoom_x", o)) {
    setZoomX(o->doubleValue());
  }
  if (aViewConfig->get("zoom_y", o)) {
    setZoomY(o->doubleValue());
  }
  if (aViewConfig->get("timingpriority", o)) {
    setLocalTimingPriority(o->boolValue());
  }
  if (aViewConfig->get("alignanimationsteps", o)) {
    setAlignAnimationSteps(o->boolValue());
  }
  if (aViewConfig->get("haltwhenhidden", o)) {
    setHaltWhenHidden(o->boolValue());
  }
  if (aViewConfig->get("sizetocontent", o)) {
    setSizeToContent(o->boolValue());
  }
  if (mChangedGeometry && mSizeToContent) {
    moveFrameToContent(true);
  }
  if (aViewConfig->get("stopanimations", o)) {
    if(o->boolValue()) stopAnimations();
  }
  #if ENABLE_ANIMATION
  if (aViewConfig->get("animate", o)) {
    configureAnimation(o);
  }
  #endif
  announceChanges(false);
  return ErrorPtr();
}

#endif // legacy configure w/o ENABLE_P44SCRIPT


P44ViewPtr P44View::findView(const string aLabelOrId)
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


string P44View::framingModeToText(P44View::FramingMode aFramingMode, bool aForPositioning)
{
  const FramingModeDesc *wd = framingModeDescs;
  string modes;
  if (aFramingMode==P44View::noFraming) modes = wd->name;
  else while ((++wd)->name && aFramingMode) {
    if (aForPositioning!=wd->forPos) continue;
    if ((aFramingMode & wd->mode) == wd->mode) {
      if (!modes.empty()) modes += "|";
      modes += wd->name;
      aFramingMode &= ~wd->mode;
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
  return string_format("V_%08X", (uint32_t)(intptr_t)this);
}


#if ENABLE_P44SCRIPT

JsonObjectPtr P44View::viewStatus()
{
  JsonObjectPtr status = JsonObject::newObj();
  ScriptObjPtr vo = newViewObj();
  ValueIteratorPtr iter = vo->newIterator(nonebut|jsonrepresentable);
  ScriptObjPtr name;
  while((name = iter->obtainKey(false))) {
    ScriptObjPtr m = iter->obtainValue(nonebut|jsonrepresentable);
    if (m) status->add(name->stringValue().c_str(), m->jsonValue());
    iter->next();
  }
  #if ENABLE_ANIMATION
  // there is no p44script property for this
  status->add("animations", JsonObject::newInt64(mAnimations.size()));
  #endif
  return status;
}

#else

// legacy implementation

JsonObjectPtr P44View::viewStatus()
{
  JsonObjectPtr status = JsonObject::newObj();
  status->add("type", JsonObject::newString(getTypeName()));
  status->add("label", JsonObject::newString(getLabel()));
  status->add("id", JsonObject::newString(getId()));
  status->add("x", JsonObject::newInt32(getX()));
  status->add("y", JsonObject::newInt32(getY()));
  status->add("dx", JsonObject::newInt32(getDx()));
  status->add("dy", JsonObject::newInt32(getDx()));
  status->add("content_x", JsonObject::newInt32(getContentX()));
  status->add("content_y", JsonObject::newInt32(getContentY()));
  status->add("content_dx", JsonObject::newInt32(getContentDx()));
  status->add("content_dy", JsonObject::newInt32(getContentDy()));
  status->add("rotation", JsonObject::newDouble(getContentRotation()));
  status->add("scroll_x", JsonObject::newDouble(getScrollX()));
  status->add("scroll_y", JsonObject::newDouble(getScrollY()));
  status->add("zoom_x", JsonObject::newDouble(getZoomX()));
  status->add("zoom_y", JsonObject::newDouble(getZoomY()));
  status->add("color", JsonObject::newString(getColor()));
  status->add("bgcolor", JsonObject::newString(getBgcolor()));
  status->add("alpha", JsonObject::newInt32(getAlpha()));
  status->add("visible", JsonObject::newBool(getVisible()));
  status->add("z_order", JsonObject::newInt32(getZOrder()));
  status->add("orientation", JsonObject::newString(orientationToText(mContentOrientation)));
  status->add("framing", JsonObject::newString(framingModeToText(getFramingMode(), false)));
  status->add("mask", JsonObject::newBool(getContentIsMask()));
  status->add("invertalpha", JsonObject::newBool(getInvertAlpha()));
  status->add("timingpriority", JsonObject::newBool(getLocalTimingPriority()));
  status->add("alignanimationsteps", JsonObject::newBool(getAlignAnimationSteps()));
  status->add("haltwhenhidden", JsonObject::newBool(getHaltWhenHidden()));
  #if ENABLE_ANIMATION
  status->add("animations", JsonObject::newInt64(mAnimations.size()));
  #endif
  return status;
}

#endif // legacy configure w/o ENABLE_P44SCRIPT

#endif // ENABLE_VIEWSTATUS


#if ENABLE_ANIMATION

// MARK: ===== Animation

void P44View::configureAnimation(JsonObjectPtr aAnimationCfg)
{
  JsonObjectPtr o;
  JsonObjectPtr a = aAnimationCfg;
  if (!a->isType(json_type_array)) {
    a = JsonObject::newArray();
    a->arrayAppend(aAnimationCfg);
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


void P44View::geometryPropertySetter(PixelCoord *aPixelCoordP, double aNewValue)
{
  PixelCoord newValue = aNewValue;
  if (newValue!=*aPixelCoordP) {
    announceChanges(true);
    *aPixelCoordP = newValue;
    flagGeometryChange();
    if (mSizeToContent) moveFrameToContent(true);
    announceChanges(false);
  }
}

ValueSetterCB P44View::getGeometryPropertySetter(PixelCoord &aPixelCoord, double &aCurrentValue)
{
  aCurrentValue = aPixelCoord;
  return boost::bind(&P44View::geometryPropertySetter, this, &aPixelCoord, _1);
}


void P44View::transformPropertySetter(FracValue* aTransformValueP, double aNewValue)
{
  double newValue = aNewValue;
  if (FP_FROM_DBL(newValue)!=*aTransformValueP) {
    announceChanges(true);
    *aTransformValueP = FP_FROM_DBL(newValue);
    flagTransformChange();
    announceChanges(false);
  }
}

ValueSetterCB P44View::getTransformPropertySetter(FracValue& aTransformValue, double &aCurrentValue)
{
  aCurrentValue = FP_DBL_VAL(aTransformValue);
  return boost::bind(&P44View::transformPropertySetter, this, &aTransformValue, _1);
}


void P44View::zoomPropertySetter(FracValue* aShrinkValueP, double aNewZoomValue)
{
  double newValue = aNewZoomValue<=0 ? 0 : 1.0/aNewZoomValue;
  if (FP_FROM_DBL(newValue)!=*aShrinkValueP) {
    announceChanges(true);
    *aShrinkValueP = FP_FROM_DBL(newValue);
    flagTransformChange();
    announceChanges(false);
  }
}

ValueSetterCB P44View::getZoomPropertySetter(FracValue& aShrinkValue, double &aCurrentZoomValue)
{
  aCurrentZoomValue = aShrinkValue<=0 ? 0 : 1/FP_DBL_VAL(aShrinkValue);
  return boost::bind(&P44View::transformPropertySetter, this, &aShrinkValue, _1);
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
  if (uequals(aComponent, "r")) {
    return getSingleColorComponentSetter(aPixelColor.r, aCurrentValue);
  }
  else if (uequals(aComponent, "g")) {
    return getSingleColorComponentSetter(aPixelColor.g, aCurrentValue);
  }
  else if (uequals(aComponent, "b")) {
    return getSingleColorComponentSetter(aPixelColor.b, aCurrentValue);
  }
  else if (uequals(aComponent, "a")) {
    return getSingleColorComponentSetter(aPixelColor.a, aCurrentValue);
  }
  else if (uequals(aComponent, "hue")) {
    return getDerivedColorComponentSetter(0, aPixelColor, aCurrentValue);
  }
  else if (uequals(aComponent, "saturation")) {
    return getDerivedColorComponentSetter(1, aPixelColor, aCurrentValue);
  }
  else if (uequals(aComponent, "brightness")) {
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
  if (uequals(aProperty, "alpha")) {
    aCurrentValue = getAlpha();
    return boost::bind(&P44View::setAlpha, this, _1);
  }
  else if (uequals(aProperty, "x")) {
    return getGeometryPropertySetter(mFrame.x, aCurrentValue);
  }
  else if (uequals(aProperty, "y")) {
    return getGeometryPropertySetter(mFrame.y, aCurrentValue);
  }
  else if (uequals(aProperty, "dx")) {
    return getGeometryPropertySetter(mFrame.dx, aCurrentValue);
  }
  else if (uequals(aProperty, "dy")) {
    return getGeometryPropertySetter(mFrame.dy, aCurrentValue);
  }
  else if (uequals(aProperty, "content_x")) {
    return getGeometryPropertySetter(mContent.x, aCurrentValue);
  }
  else if (uequals(aProperty, "content_y")) {
    return getGeometryPropertySetter(mContent.y, aCurrentValue);
  }
  else if (uequals(aProperty, "rel_content_x")) {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginX, this, _1, false);
  }
  else if (uequals(aProperty, "rel_content_y")) {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginY, this, _1, false);
  }
  else if (uequals(aProperty, "rel_center_x")) {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginX, this, _1, true);
  }
  else if (uequals(aProperty, "rel_center_y")) {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginY, this, _1, true);
  }
  else if (uequals(aProperty, "rel_content_size_x")) {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentSizeX, this, _1);
  }
  else if (uequals(aProperty, "rel_content_size_y")) {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentSizeY, this, _1);
  }
  else if (uequals(aProperty, "scroll_x")) {
    return getTransformPropertySetter(mScrollX, aCurrentValue);
  }
  else if (uequals(aProperty, "scroll_y")) {
    return getTransformPropertySetter(mScrollY, aCurrentValue);
  }
  else if (uequals(aProperty, "zoom_x")) {
    return getZoomPropertySetter(mShrinkX, aCurrentValue);
  }
  else if (uequals(aProperty, "zoom_y")) {
    return getZoomPropertySetter(mShrinkY, aCurrentValue);
  }
  else if (uequals(aProperty, "rotation")) {
    return getTransformPropertySetter(mContentRotation, aCurrentValue);
  }
  else if (uequals(aProperty, "content_dx")) {
    return getGeometryPropertySetter(mContent.dx, aCurrentValue);
  }
  else if (uequals(aProperty, "content_dy")) {
    return getGeometryPropertySetter(mContent.dy, aCurrentValue);
  }
  else if (uequals(aProperty, "color.", 6)) {
    return getColorComponentSetter(aProperty.substr(6), mForegroundColor, aCurrentValue);
  }
  else if (uequals(aProperty, "bgcolor.", 8)) {
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
  #if ENABLE_ANIMATION
  for (AnimationsList::iterator pos = mAnimations.begin(); pos!=mAnimations.end(); ++pos) {
    (*pos)->stop(false); // stop with no callback
  }
  mAnimations.clear();
  #endif
}


// MARK: - script support

#if ENABLE_P44SCRIPT

#include "viewfactory.hpp"

using namespace P44Script;

ScriptObjPtr P44View::newViewObj()
{
  // base class with standard functionality
  return new P44lrgViewObj(this);
}


ScriptObjPtr P44View::installedAnimators()
{
  ArrayValuePtr a = new ArrayValue();
  #if ENABLE_ANIMATION
  for (AnimationsList::iterator pos = mAnimations.begin(); pos!=mAnimations.end(); ++pos) {
    a->appendMember(new ValueAnimatorObj(*pos)); // wrap with access object
  }
  #endif
  return a;
}


#if P44SCRIPT_FULL_SUPPORT


P44ViewPtr P44View::viewFromScriptObj(ScriptObjPtr aArg, ErrorPtr &aErr)
{
  P44ViewPtr view;
  P44lrgViewObj* subview = dynamic_cast<P44lrgViewObj*>(aArg.get());
  if (subview) {
    view = subview->view();
  }
  else {
    // try to create from config
    JsonObjectPtr viewCfgJSON = P44View::viewConfigFromScriptObj(aArg, aErr);
    if (Error::isOK(aErr)) {
      aErr = createViewFromConfig(viewCfgJSON, view, P44ViewPtr());
    }
  }
  return view;
}


JsonObjectPtr P44View::viewConfigFromScriptObj(ScriptObjPtr aArg, ErrorPtr &aErr)
{
  #if SCRIPTING_JSON_SUPPORT
  if (aArg->hasType(structured)) {
    // is already a object value, use its JSON as-is
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


// findview(viewlabel)
FUNC_ARG_DEFS(findview, { text } );
static void findview_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  P44ViewPtr foundView = v->view()->findView(f->arg(0)->stringValue());
  if (foundView) {
    f->finish(foundView->newViewObj());
    return;
  }
  f->finish(new AnnotatedNullValue("no such view"));
}


// addview(view)
FUNC_ARG_DEFS(addview, { structured } );
static void addview_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  ErrorPtr err;
  P44ViewPtr subview = P44View::viewFromScriptObj(f->arg(0), err);
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  if (v->view()->isParentOrThis(subview)) {
    f->finish(new ErrorValue(ScriptError::Invalid, "View cannot be added to itself or to its own children"));
    return;
  }
  if (!v->view()->addSubView(subview)) {
    f->finish(new ErrorValue(ScriptError::Invalid, "cannot add subview here"));
    return;
  }
  f->finish(subview->newViewObj());
}


// configure(jsonconfig|filename)
FUNC_ARG_DEFS(configure, { text|objectvalue } );
static void configure_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  ErrorPtr err;
  JsonObjectPtr viewCfgJSON = P44View::viewConfigFromScriptObj(f->arg(0), err);
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


// content_position(x,y,centered)
FUNC_ARG_DEFS(content_position, { numeric|null }, { numeric|null }, { numeric|optionalarg } );
static void content_position_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  bool centered = f->arg(2)->boolValue();
  v->view()->announceChanges(true);
  if (f->arg(0)->defined()) {
    v->view()->setRelativeContentOriginX(f->arg(0)->doubleValue(), centered);
  }
  if (f->arg(1)->defined()) {
    v->view()->setRelativeContentOriginY(f->arg(1)->doubleValue(), centered);
  }
  v->view()->announceChanges(false);
  f->finish(v); // return view itself to allow chaining
}


// content_size(dx,dy)
FUNC_ARG_DEFS(content_size, { numeric|null }, { numeric|null } );
static void content_size_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->announceChanges(true);
  if (f->arg(0)->defined()) {
    v->view()->setRelativeContentSizeX(f->arg(0)->doubleValue());
  }
  if (f->arg(1)->defined()) {
    v->view()->setRelativeContentSizeY(f->arg(1)->doubleValue());
  }
  v->view()->announceChanges(false);
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
FUNC_ARG_DEFS(set, { text }, { anyvalid } );
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


// animations()   list of running animations
static void animations_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  f->finish(v->view()->installedAnimators());
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
  v->view()->clear();
  f->finish(v); // return view itself to allow chaining
}


// reset()     reset view transformations
static void reset_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->resetTransforms();
  f->finish(v); // return view itself to allow chaining
}



// fullframe()     make content fill frame
static void fullframe_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->setFullFrameContent(); // to make sure changes are applied
  f->finish(v); // return view itself to allow chaining
}


// get(x,y)
FUNC_ARG_DEFS(get, { numeric|undefres }, { numeric|undefres } );
static void get_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  PixelPoint pt;
  pt.x = f->arg(0)->intValue();
  pt.y = f->arg(1)->intValue();
  PixelColor pix = v->view()->colorInFrameAt(pt);
  f->finish(new StringValue(pixelToWebColor(pix, true)));
}


#if ENABLE_ANIMATION

// animator(propertyname)
FUNC_ARG_DEFS(animator, { text } );
static void animator_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  f->finish(new ValueAnimatorObj(v->view()->animatorFor(f->arg(0)->stringValue())));
}

#endif // ENABLE_ANIMATION

#endif // P44SCRIPT_FULL_SUPPORT


#define ACCESSOR_CLASS P44View

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  P44ViewPtr view = reinterpret_cast<P44lrgViewObj*>(aParentObj.get())->view();
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_RO_STR(TypeName)
ACC_IMPL_RO_STR(Id)
ACC_IMPL_STR(Label)
ACC_IMPL_INT(X)
ACC_IMPL_INT(Y)
ACC_IMPL_INT(Dx)
ACC_IMPL_INT(Dy)
ACC_IMPL_INT(ContentX)
ACC_IMPL_INT(ContentY)
ACC_IMPL_INT(ContentDx)
ACC_IMPL_INT(ContentDy)
ACC_IMPL_STR(Color)
ACC_IMPL_STR(Bgcolor)
ACC_IMPL_INT(Alpha)
ACC_IMPL_BOOL(Visible)
ACC_IMPL_INT(ZOrder)
ACC_IMPL_BOOL(SizeToContent)
ACC_IMPL_BOOL(LocalTimingPriority)
ACC_IMPL_BOOL(AlignAnimationSteps)
ACC_IMPL_BOOL(HaltWhenHidden)
ACC_IMPL_BOOL(ContentIsMask)
ACC_IMPL_BOOL(InvertAlpha)
ACC_IMPL_BOOL(Subsampling)
ACC_IMPL_DBL(ContentRotation)
ACC_IMPL_DBL(ScrollX)
ACC_IMPL_DBL(ScrollY)
ACC_IMPL_DBL(ZoomX)
ACC_IMPL_DBL(ZoomY)

static ScriptObjPtr access_FramingMode(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite)
{
  if (!aToWrite) return new StringValue(P44View::framingModeToText(aView.getFramingMode(), false));
  if (aToWrite->hasType(numeric)) aView.setFramingMode(aToWrite->intValue());
  else aView.setFramingMode(P44View::textToFramingMode(aToWrite->stringValue().c_str()));
  return aToWrite; /* reflect back to indicate writable */
}

static ScriptObjPtr access_AutoAdjust(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite)
{
  if (!aToWrite) return new StringValue(P44View::framingModeToText(aView.getAutoAdjust(), true));
  if (aToWrite->hasType(numeric)) aView.setFramingMode(aToWrite->intValue());
  else aView.setAutoAdjust(P44View::textToFramingMode(aToWrite->stringValue().c_str()));
  return aToWrite; /* reflect back to indicate writable */
}


static ScriptObjPtr access_Orientation(P44View& aView, ScriptObjPtr aToWrite)
{
  if (!aToWrite) return new StringValue(P44View::orientationToText(aView.getOrientation()));
  if (aToWrite->hasType(numeric)) aView.setOrientation(aToWrite->intValue());
  else aView.setOrientation(P44View::textToOrientation(aToWrite->stringValue().c_str()));
  return aToWrite; /* reflect back to indicate writable */
}


static const BuiltinMemberDescriptor viewMembers[] = {
  #if P44SCRIPT_FULL_SUPPORT
  FUNC_DEF_W_ARG(findview, executable|objectvalue),
  FUNC_DEF_W_ARG(addview, executable|objectvalue),
  FUNC_DEF_W_ARG(configure, executable|objectvalue),
  FUNC_DEF_W_ARG(set, executable|objectvalue),
  FUNC_DEF_NOARG(render, executable|objectvalue),
  FUNC_DEF_NOARG(remove, executable|numeric),
  FUNC_DEF_NOARG(parent, executable|objectvalue),
  FUNC_DEF_NOARG(clear, executable|objectvalue),
  FUNC_DEF_NOARG(reset, executable|objectvalue),
  FUNC_DEF_NOARG(fullframe, executable|objectvalue),
  FUNC_DEF_W_ARG(content_position, executable|objectvalue),
  FUNC_DEF_W_ARG(content_size, executable|objectvalue),
  FUNC_DEF_W_ARG(get, executable|text),
  #if ENABLE_ANIMATION
  FUNC_DEF_W_ARG(animator, executable|objectvalue),
  FUNC_DEF_NOARG(stopanimations, executable|objectvalue),
  FUNC_DEF_NOARG(animations, executable|arrayvalue),
  #endif // ENABLE_ANIMATION
  #if ENABLE_VIEWSTATUS
  FUNC_DEF_NOARG(status, executable|objectvalue),
  #endif // ENABLE_VIEWSTATUS
  #endif // P44SCRIPT_FULL_SUPPORT
  // property accessors
  ACC_DECL("type", text, TypeName),
  ACC_DECL("id", text, Id),
  ACC_DECL("label", text|lvalue, Label),
  ACC_DECL("x", numeric|lvalue, X),
  ACC_DECL("y", numeric|lvalue, Y),
  ACC_DECL("dx", numeric|lvalue, Dx),
  ACC_DECL("dy", numeric|lvalue, Dy),
  ACC_DECL("content_x", numeric|lvalue, ContentX),
  ACC_DECL("content_y", numeric|lvalue, ContentY),
  ACC_DECL("content_dx", numeric|lvalue, ContentDx),
  ACC_DECL("content_dy", numeric|lvalue, ContentDy),
  ACC_DECL("rotation", numeric|lvalue, ContentRotation),
  ACC_DECL("scroll_x", numeric|lvalue, ScrollX),
  ACC_DECL("scroll_y", numeric|lvalue, ScrollY),
  ACC_DECL("zoom_x", numeric|lvalue, ZoomX),
  ACC_DECL("zoom_y", numeric|lvalue, ZoomY),
  ACC_DECL("color", text|lvalue, Color),
  ACC_DECL("bgcolor", text|lvalue, Bgcolor),
  ACC_DECL("alpha", numeric|lvalue, Alpha),
  ACC_DECL("visible", numeric|lvalue, Visible),
  ACC_DECL("z_order", numeric|lvalue, ZOrder),
  ACC_DECL("framing", text|numeric|lvalue, FramingMode),
  ACC_DECL("autoadjust", text|numeric|lvalue, AutoAdjust),
  ACC_DECL("orientation", text|numeric|lvalue, Orientation),
  ACC_DECL("sizetocontent", numeric|lvalue, SizeToContent),
  ACC_DECL("timingpriority", numeric|lvalue, LocalTimingPriority),
  ACC_DECL("alignanimations", numeric|lvalue, AlignAnimationSteps),
  ACC_DECL("haltwhenhidden", numeric|lvalue, HaltWhenHidden),
  ACC_DECL("contentismask", numeric|lvalue, ContentIsMask),
  ACC_DECL("invertalpha", numeric|lvalue, InvertAlpha),
  ACC_DECL("subsampling", numeric|lvalue, Subsampling),
  BUILTINS_TERMINATOR
};

static BuiltInMemberLookup* sharedViewMemberLookupP = NULL;

P44lrgViewObj::P44lrgViewObj(P44ViewPtr aView) :
  mView(aView)
{
  registerSharedLookup(sharedViewMemberLookupP, viewMembers);
}


#endif // ENABLE_P44SCRIPT
