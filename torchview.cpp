//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2017-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#include "torchview.hpp"

using namespace p44;


// MARK: ===== TorchView


TorchView::TorchView() :
  mNextCalculation(Never)
{
  mFlameMin = 100;
  mFlameMax = 220;
  mFlameHeight = 1;
  mSparkProbability = 2;
  mSparkMin = 200;
  mSparkMax = 255;
  mSparkTfr = 40;
  mSparkCap = 200;
  mUpRad = 40;
  mSideRad = 35;
  mHeatCap = 0;
  mHotsparkMin = 250;
  mHotsparkColor = { 170, 170, 250, 255 };
  mHotsparkColorInc = { 0, 0, 5, 255 };
  mForegroundColor = { 180, 145, 0, 255 }; // foreground is "energy" color, which scales up with energy of the spark
  mCycleTime = 25*MilliSecond;
  clear();
}


TorchView::~TorchView()
{
}


void TorchView::clear()
{
  stopAnimations();
  // clear and resize to current size
  mTorchDots.clear();
  mTorchDots.resize(mContent.dy*mContent.dx, { torch_passive, 0, 0 });
}


TorchView::TorchDot& TorchView::dot(PixelPoint aPt)
{
  int dotIdx = aPt.y*mContent.dx+aPt.x;
  if (dotIdx>=mTorchDots.size()) clear(); // safety, resize on access outside range
  return mTorchDots.at(dotIdx);
}



void TorchView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  if (aOldContent.dx!=mContent.dx || aOldContent.dy!=mContent.dy) {
    clear(); // size changed: clear and resize dots vector
  }
  inherited::geometryChanged(aOldFrame, aOldContent);
}


void TorchView::recalculateColoring()
{
  #if NEW_COLORING
  calculateGradient(256);
  #else
  calculateGradient(256, mExtent.y*256*2/mContent.dy);
  #endif
  inherited::recalculateColoring();
}



MLMicroSeconds TorchView::step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow)
{
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil, aNow);
  if (mCycleTime!=Never && aNow>=mNextCalculation) {
    mNextCalculation = aNow+mCycleTime;
    calculateCycle();
  }
  updateNextCall(nextCall, aNow+mCycleTime);
  return nextCall;
}


static uint16_t random(uint16_t aMinOrMax, uint16_t aMax = 0)
{
  if (aMax==0) {
    aMax = aMinOrMax;
    aMinOrMax = 0;
  }
  uint32_t r = aMinOrMax;
  aMax = aMax - aMinOrMax + 1;
  r += rand() % aMax;
  return r;
}


void TorchView::calculateCycle()
{
  if (mAlpha==0) return; // don't waste cycles
  PixelPoint dotpos;
  // random flame
  for (dotpos.y=0; dotpos.y<mFlameHeight; dotpos.y++) {
    for (dotpos.x=0; dotpos.x<mContent.dx; dotpos.x++) {
      TorchDot& d = dot(dotpos);
      d.previous = random(mFlameMin, mFlameMax);
      d.mode = torch_nop;
    }
  }
  // random sparks in next row
  dotpos.y = mFlameHeight;
  for (dotpos.x=0; dotpos.x<mContent.dx; dotpos.x++) {
    TorchDot& d = dot(dotpos);
    if (d.mode!=torch_spark && random(100)<mSparkProbability) {
      d.previous = random(mSparkMin, mSparkMax);
      d.mode = torch_spark;
    }
  }
  // let sparks fly and fade
  for (dotpos.y=0; dotpos.y<mContent.dy; dotpos.y++) {
    for (dotpos.x=0; dotpos.x<mContent.dx; dotpos.x++) {
      TorchDot& d = dot(dotpos);
      uint8_t e = d.previous;
      EnergyMode m = d.mode;
      switch (m) {
        case torch_spark: {
          // loose transfer-up-energy as long as there is any
          reduce(e, mSparkTfr);
          // cell above is temp spark, sucking up energy from this cell until empty
          if (dotpos.y<mContent.dy-1) {
            TorchDot& above = dot({ dotpos.x, dotpos.y+1 });
            above.mode = torch_spark_temp;
          }
          break;
        }
        case torch_spark_temp: {
          // just getting some energy from below
          TorchDot& below = dot({ dotpos.x, dotpos.y-1 });
          uint8_t e2 = below.previous;
          if (e2<mSparkTfr) {
            // cell below is exhausted, becomes passive
            below.mode = torch_passive;
            // gobble up rest of energy
            increase(e, e2);
            // loose some overall energy
            e = ((int)e*mSparkCap)>>8;
            // this cell becomes active spark
            d.mode = torch_spark;
          }
          else {
            increase(e, mSparkTfr);
          }
          break;
        }
        case torch_passive: {
          e = ((int)e*mHeatCap)>>8;
          increase(e, (
            (((int)dot({dotpos.x>0 ? dotpos.x-1 : mContent.dx-1, dotpos.y}).previous+(int)dot({dotpos.x<mContent.dx-1 ? dotpos.x+1 : 0, dotpos.y}).previous)*mSideRad)>>9) +
            (((int)dot({dotpos.x, dotpos.y-1}).previous*mUpRad)>>8)
          );
        }
        default:
          break;
      }
      d.current = e;
    }
  }
  // transfer new
  for (TorchDotVector::iterator pos=mTorchDots.begin(); pos!=mTorchDots.end(); ++pos) {
    pos->previous = pos->current;
  }
  makeDirty();
}

#define NEWCOLORING 1

#if !NEWCOLORING
static const uint8_t energymap[32] = {0, 64, 96, 112, 128, 144, 152, 160, 168, 176, 184, 184, 192, 200, 200, 208, 208, 216, 216, 224, 224, 224, 232, 232, 232, 240, 240, 240, 240, 248, 248, 248};
#endif

PixelColor TorchView::contentColorAt(PixelPoint aPt)
{
  PixelColor pix = transparent;
  if (mAlpha>0 && isInContentSize(aPt)) {
    TorchDot& d = dot(aPt);
    int extraHeat = d.current-mHotsparkMin;
    if (extraHeat>=0) {
      pix = mHotsparkColor;
      addToPixel(pix, dimmedPixel(mHotsparkColorInc, 255*(int)extraHeat/(255-mHotsparkMin)));
    }
    else {
      if (d.current>0) {
        #if NEWCOLORING
        pix = gradientPixel(255-d.current);
        #else
        // energy to brightness is non-linear
        pix.a = 255; // opaque
        uint8_t eb = energymap[d.current>>3];
        pix = { 10, 0, 0, 255 };
        addToPixel(pix, dimmedPixel(foregroundColor, eb));
        #endif
      }
    }
  }
  return pix;
}


#if ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT

// MARK: ===== view configuration

ErrorPtr TorchView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    // flame parameters
    if (aViewConfig->get("flame_min", o)) setFlameMin(o->int32Value());
    if (aViewConfig->get("flame_max", o)) setFlameMax(o->int32Value());
    if (aViewConfig->get("flame_height", o)) setFlameHeight(o->int32Value());
    // spark generation parameters
    if (aViewConfig->get("spark_probability", o)) setSparkProbability(o->int32Value());
    if (aViewConfig->get("spark_min", o)) setSparkMin(o->int32Value());
    if (aViewConfig->get("spark_max", o)) setSparkMax(o->int32Value());
    // spark development parameters
    if (aViewConfig->get("spark_tfr", o)) setSparkTfr(o->int32Value());
    if (aViewConfig->get("spark_cap", o)) setSparkCap(o->int32Value());
    if (aViewConfig->get("up_rad", o)) setUpRad(o->int32Value());
    if (aViewConfig->get("side_rad", o)) setSideRad(o->int32Value());
    if (aViewConfig->get("heat_cap", o)) setHeatCap(o->int32Value());
    // hot spark coloring parameters
    if (aViewConfig->get("hotspark_min", o)) setHotsparkMin(o->int32Value());
    if (aViewConfig->get("hotsparkcolor", o)) setHotsparkColor(o->stringValue());
    if (aViewConfig->get("hotsparkinc", o)) setHotsparkColorInc(o->stringValue());
    // timing parameter
    if (aViewConfig->get("cycletime", o)) setCycleTimeS(o->doubleValue());
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT


#if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

JsonObjectPtr TorchView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("flame_min", JsonObject::newInt32(getFlameMin()));
  status->add("flame_max", JsonObject::newInt32(getFlameMax()));
  status->add("flame_height", JsonObject::newInt32(getFlameHeight()));
  status->add("spark_probability", JsonObject::newInt32(getSparkProbability()));
  status->add("spark_min", JsonObject::newInt32(getSparkMin()));
  status->add("spark_max", JsonObject::newInt32(getSparkMax()));
  status->add("spark_tfr", JsonObject::newInt32(getSparkTfr()));
  status->add("spark_cap", JsonObject::newInt32(getSparkCap()));
  status->add("up_rad", JsonObject::newInt32(getUpRad()));
  status->add("side_rad", JsonObject::newInt32(getSideRad()));
  status->add("heat_cap", JsonObject::newInt32(getHeatCap()));
  status->add("hotspark_min", JsonObject::newInt32(getHotsparkMin()));
  status->add("hotsparkcolor", JsonObject::newString(getHotsparkColor()));
  status->add("hotsparkinc", JsonObject::newString(getHotsparkColorInc()));
  status->add("cycletime", JsonObject::newDouble(getCycleTimeS()));
  return status;
}

#endif // ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr TorchView::newViewObj()
{
  // base class with standard functionality
  return new TorchViewObj(this);
}


#define ACCESSOR_CLASS TorchView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  TorchViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<TorchViewObj*>(aParentObj.get())->torch().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_INT(FlameMin)
ACC_IMPL_INT(FlameMax)
ACC_IMPL_INT(FlameHeight)
ACC_IMPL_INT(SparkProbability)
ACC_IMPL_INT(SparkMin)
ACC_IMPL_INT(SparkMax)
ACC_IMPL_INT(SparkTfr)
ACC_IMPL_INT(SparkCap)
ACC_IMPL_INT(UpRad)
ACC_IMPL_INT(SideRad)
ACC_IMPL_INT(HeatCap)
ACC_IMPL_INT(HotsparkMin)
ACC_IMPL_STR(HotsparkColor)
ACC_IMPL_STR(HotsparkColorInc)
ACC_IMPL_INT(CycleTimeS)

static const BuiltinMemberDescriptor torchMembers[] = {
  // property accessors
  ACC_DECL("flame_min", numeric|lvalue, FlameMin),
  ACC_DECL("flame_max", numeric|lvalue, FlameMax),
  ACC_DECL("flame_height", numeric|lvalue, FlameHeight),
  ACC_DECL("spark_probability", numeric|lvalue, SparkProbability),
  ACC_DECL("spark_min", numeric|lvalue, SparkMin),
  ACC_DECL("spark_max", numeric|lvalue, SparkMax),
  ACC_DECL("spark_tfr", numeric|lvalue, SparkTfr),
  ACC_DECL("spark_cap", numeric|lvalue, SparkCap),
  ACC_DECL("up_rad", numeric|lvalue, UpRad),
  ACC_DECL("side_rad", numeric|lvalue, SideRad),
  ACC_DECL("heat_cap", numeric|lvalue, HeatCap),
  ACC_DECL("hotspark_min", numeric|lvalue, HotsparkMin),
  ACC_DECL("hotsparkcolor", numeric|lvalue, HotsparkColor),
  ACC_DECL("hotsparkinc", numeric|lvalue, HotsparkColorInc),
  ACC_DECL("cycletime", numeric|lvalue, CycleTimeS),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedTorchMemberLookupP = NULL;

TorchViewObj::TorchViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  if (sharedTorchMemberLookupP==NULL) {
    sharedTorchMemberLookupP = new BuiltInMemberLookup(torchMembers);
    sharedTorchMemberLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedTorchMemberLookupP);
}

#endif // ENABLE_P44SCRIPT
