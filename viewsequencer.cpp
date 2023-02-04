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

#include "viewsequencer.hpp"

#if ENABLE_VIEWCONFIG
  #include "viewfactory.hpp"
#endif

using namespace p44;


// MARK: ===== ViewAnimator

ViewSequencer::ViewSequencer() :
  mRepeating(false),
  mCurrentStep(-1),
  animationState(as_begin)
{
}


ViewSequencer::~ViewSequencer()
{
  for (SequenceVector::iterator pos = mSequence.begin(); pos!=mSequence.end(); ++pos) {
    P44ViewPtr v = pos->mView;
    if (v) v->setParent(NULL);
  }
  mSequence.clear();
}


void ViewSequencer::clear()
{
  stopAnimations();
  mSequence.clear();
  inherited::clear();
}


void ViewSequencer::pushStep(P44ViewPtr aView, MLMicroSeconds aShowTime, MLMicroSeconds aFadeInTime, MLMicroSeconds aFadeOutTime)
{
  AnimationStep s;
  s.mView = aView;
  s.mShowTime = aShowTime;
  s.mFadeInTime = aFadeInTime;
  s.mFadeOutTime = aFadeOutTime;
  mSequence.push_back(s);
  makeDirty();
}


MLMicroSeconds ViewSequencer::step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow)
{
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil, aNow);
  updateNextCall(nextCall, stepAnimation(aNow), aPriorityUntil, aNow); // animation has priority
  if (mCurrentStep<mSequence.size()) {
    updateNextCall(nextCall, mSequence[mCurrentStep].mView->step(aPriorityUntil, aNow));
  }
  return nextCall;
}


void ViewSequencer::stopAnimations()
{
  if (mCurrentView) mCurrentView->stopAnimations();
  animationState = as_begin;
  mCurrentStep = -1;
  inherited::stopAnimations();
}


MLMicroSeconds ViewSequencer::stepAnimation(MLMicroSeconds aNow)
{
  if (mCurrentStep<mSequence.size()) {
    MLMicroSeconds sinceLast = aNow-mLastStateChange;
    AnimationStep as = mSequence[mCurrentStep];
    switch (animationState) {
      case as_begin:
        // initiate animation
        // - set current view
        mCurrentView = as.mView;
        makeDirty();
        if (as.mFadeInTime>0) {
          #if ENABLE_ANIMATION
          mCurrentView->setAlpha(0);
          mCurrentView->animatorFor("alpha")->animate(255, as.mFadeInTime);
          #else
          currentView->setAlpha(255);
          #endif
        }
        animationState = as_show;
        mLastStateChange = aNow;
        // next change we must handle is end of show time
        return aNow+as.mFadeInTime+as.mShowTime;
      case as_show:
        if (sinceLast>as.mFadeInTime+as.mShowTime) {
          // check fadeout
          if (as.mFadeOutTime>0) {
            #if ENABLE_ANIMATION
            as.mView->animatorFor("alpha")->animate(255, as.mFadeOutTime);
            #else
            as.view->setAlpha(255);
            #endif
            animationState = as_fadeout;
          }
          else {
            goto ended;
          }
          mLastStateChange = aNow;
          // next change we must handle is end of fade out time
          return aNow + as.mFadeOutTime;
        }
        else {
          // still waiting for end of show time
          return mLastStateChange+as.mFadeInTime+as.mShowTime;
        }
      case as_fadeout:
        if (sinceLast<as.mFadeOutTime) {
          // next change is end of fade out
          return mLastStateChange+as.mFadeOutTime;
        }
      ended:
        // end of this step
        animationState = as_begin; // begins from start
        mCurrentStep++;
        if (mCurrentStep>=mSequence.size()) {
          // all steps done
          // - call back
          if (mCompletedCB) {
            SimpleCB cb=mCompletedCB;
            if (!mRepeating) mCompletedCB = NoOP;
            cb();
          }
          // - possibly restart
          if (mRepeating) {
            mCurrentStep = 0;
          }
          else {
            stopAnimations();
            return Infinite; // no need to call again for this animation
          }
        }
        return aNow; // call again immediately to initiate next step
    }
  }
  return Infinite;
}


void ViewSequencer::startAnimation(bool aRepeat, SimpleCB aCompletedCB)
{
  mRepeating = aRepeat;
  mCompletedCB = aCompletedCB;
  mCurrentStep = 0;
  animationState = as_begin; // begins from start
  stepAnimation(MainLoop::now());
}



bool ViewSequencer::isDirty()
{
  if (inherited::isDirty()) return true; // dirty anyway
  return mCurrentView && reportDirtyChilds() ? mCurrentView->isDirty() : false;
}


void ViewSequencer::updated()
{
  inherited::updated();
  if (mCurrentView) mCurrentView->updated();
}


PixelColor ViewSequencer::contentColorAt(PixelPoint aPt)
{
  // default is the animator's background color
  if (mAlpha==0 || !mCurrentView) {
    return transparent; // entire viewstack is invisible
  }
  else {
    // consult current step's view
    return mCurrentView->colorAt(aPt);
  }
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr ViewSequencer::configureView(JsonObjectPtr aViewConfig)
{
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    if (aViewConfig->get("steps", o)) {
      for (int i=0; i<o->arrayLength(); ++i) {
        JsonObjectPtr s = o->arrayGet(i);
        JsonObjectPtr o2;
        P44ViewPtr stepView;
        MLMicroSeconds showTime = 500*MilliSecond;
        MLMicroSeconds fadeInTime = 0;
        MLMicroSeconds fadeOutTime = 0;
        if (s->get("view", o2)) {
          err = p44::createViewFromConfig(o2, stepView, this);
          if (Error::isOK(err)) {
            if (s->get("showtime", o2)) {
              showTime = o2->doubleValue()*Second;
            }
            if (s->get("fadeintime", o2)) {
              fadeInTime = o2->doubleValue()*Second;
            }
            if (s->get("fadeouttime", o2)) {
              fadeOutTime = o2->doubleValue()*Second;
            }
            pushStep(stepView, showTime, fadeInTime, fadeOutTime);
          }
        }
      }
    }
    bool doStart = false;
    bool doRepeat = false;
    if (aViewConfig->get("start", o)) {
      doStart = o->boolValue();
    }
    if (aViewConfig->get("repeat", o)) {
      doRepeat = o->boolValue();
    }
    if (doStart) startAnimation(doRepeat);
  }
  return err;
}


P44ViewPtr ViewSequencer::getView(const string aLabel)
{
  for (SequenceVector::iterator pos = mSequence.begin(); pos!=mSequence.end(); ++pos) {
    P44ViewPtr v = pos->mView;
    if (v) {
      P44ViewPtr view = v->getView(aLabel);
      if (view) return view;
    }
  }
  return inherited::getView(aLabel);
}


#endif // ENABLE_VIEWCONFIG


#if ENABLE_VIEWSTATUS

JsonObjectPtr ViewSequencer::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  JsonObjectPtr steps = JsonObject::newArray();
  for (SequenceVector::iterator pos = mSequence.begin(); pos!=mSequence.end(); ++pos) {
    JsonObjectPtr step = JsonObject::newObj();
    step->add("view", pos->mView->viewStatus());
    step->add("showtime", JsonObject::newDouble((double)pos->mShowTime/Second));
    step->add("fadeintime", JsonObject::newDouble((double)pos->mFadeInTime/Second));
    step->add("fadeouttime", JsonObject::newDouble((double)pos->mFadeOutTime/Second));
    steps->arrayAppend(step);
  }
  status->add("steps", steps);
  return status;
}

#endif // ENABLE_VIEWSTATUS
