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


P44ViewPtr ViewSequencer::findView(const string aLabel)
{
  for (SequenceVector::iterator pos = mSequence.begin(); pos!=mSequence.end(); ++pos) {
    P44ViewPtr v = pos->mView;
    if (v) {
      P44ViewPtr view = v->findView(aLabel);
      if (view) return view;
    }
  }
  return inherited::findView(aLabel);
}

#endif // ENABLE_VIEWCONFIG


#if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

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


#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr ViewSequencer::newViewObj()
{
  // base class with standard functionality
  return new ViewSequencerObj(this);
}


ScriptObjPtr ViewSequencer::stepsList()
{
  ArrayValuePtr steps = new ArrayValue();
  for (SequenceVector::iterator pos = mSequence.begin(); pos!=mSequence.end(); ++pos) {
    ObjectValuePtr step = new ObjectValue();
    step->setMemberByName("view", pos->mView->newViewObj());
    step->setMemberByName("showtime", new NumericValue(pos->mShowTime/Second));
    step->setMemberByName("fadeintime", new NumericValue(pos->mFadeInTime/Second));
    step->setMemberByName("fadeouttime", new NumericValue(pos->mFadeOutTime/Second));
    steps->appendMember(step);
  }
  return steps;
}


#if P44SCRIPT_FULL_SUPPORT

// pushstep(view, showtime [, fadeintime [, fadeouttime]])
static const BuiltInArgDesc pushstep_args[] = { { structured }, { numeric }, { numeric|optionalarg }, { numeric|optionalarg } };
static const size_t pushstep_numargs = sizeof(pushstep_args)/sizeof(BuiltInArgDesc);
static void pushstep_func(BuiltinFunctionContextPtr f)
{
  ViewSequencerObj* v = dynamic_cast<ViewSequencerObj*>(f->thisObj().get());
  assert(v);
  P44lrgViewObj* subview = dynamic_cast<P44lrgViewObj*>(f->arg(0).get());
  if (!subview) {
    f->finish(new ErrorValue(ScriptError::Invalid, "first argument must be a view"));
    return;
  }
  MLMicroSeconds showtime = f->arg(1)->doubleValue()*Second;
  MLMicroSeconds fadein = f->arg(2)->doubleValue()*Second;
  MLMicroSeconds fadeout = f->arg(3)->doubleValue()*Second;
  v->sequencer()->pushStep(subview->view(), showtime, fadein, fadeout);
  f->finish();
}


// start(repeat)
static const BuiltInArgDesc start_args[] = { { numeric|optionalarg } };
static const size_t start_numargs = sizeof(start_args)/sizeof(BuiltInArgDesc);
static void start_func(BuiltinFunctionContextPtr f)
{
  ViewSequencerObj* v = dynamic_cast<ViewSequencerObj*>(f->thisObj().get());
  assert(v);
  bool repeat = f->arg(0)->boolValue();
  v->sequencer()->startAnimation(repeat);
  // TODO: maybe make awaitable
  f->finish();
}


#endif // P44SCRIPT_FULL_SUPPORT

#define ACCESSOR_CLASS ViewSequencer

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  ViewSequencerPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<ViewSequencerObj*>(aParentObj.get())->sequencer().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}


static ScriptObjPtr access_Steps(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite)
{
  ScriptObjPtr ret;
  if (!aToWrite) { // not writable
    ret = aView.stepsList();
  }
  return ret;
}


static const BuiltinMemberDescriptor viewSequencerMembers[] = {
  #if P44SCRIPT_FULL_SUPPORT
  { "pushstep", executable|null|error, pushstep_numargs, pushstep_args, &pushstep_func },
  { "start", executable|null|error, start_numargs, start_args, &start_func },
  #endif
  // property accessors
  ACC_DECL("steps", objectvalue, Steps),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedViewSequencerMemberLookupP = NULL;

ViewSequencerObj::ViewSequencerObj(P44ViewPtr aView) :
  inherited(aView)
{
  if (sharedViewSequencerMemberLookupP==NULL) {
    sharedViewSequencerMemberLookupP = new BuiltInMemberLookup(viewSequencerMembers);
    sharedViewSequencerMemberLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedViewSequencerMemberLookupP);
}

#endif // ENABLE_P44SCRIPT
