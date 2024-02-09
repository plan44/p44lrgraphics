//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2020 plan44.ch / Lukas Zeller, Zurich, Switzerland
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


#include "epxview.hpp"

#if ENABLE_EPX_SUPPORT

#if ENABLE_VIEWCONFIG
  #include "application.hpp"
#endif

using namespace p44;

// MARK: ===== EpxView

EpxView::EpxView() :
  mPaletteBuffer(NULL),
  mNumPaletteEntries(0),
  mNextRender(Infinite),
  mFramesCursor(0),
  mRemainingLoops(0)
{
}


EpxView::~EpxView()
{
  clearData();
}


void EpxView::clear()
{
  inherited::clear(); // reset canvas to all transparent pixels
  clearData(); // remove EXP animation
}


void EpxView::clearData()
{
  // free the buffer if allocated
  if (mPaletteBuffer) {
    delete[] mPaletteBuffer;
    mPaletteBuffer = NULL;
  }
  mFramesData.clear();
}


/*

 Sample JSON as available from Expressive Pixels Windows App via copy&paste:

 {
   "Command": "UPLOAD_ANIMATION8",
   "ID": "xxx",
   "Name": "yyy",
   "PaletteSize": 256,
   "PaletteHexLength": 1536,
   "PaletteHex": "000000597e8721374397d2e49b8b664159681a1914e...", // rrggbbrrggbb
   "FrameCount": 7,
   "FrameRate": 8,
   "LoopCount": 4,
   "FramesHexLength": 3626,
   "FramesHex": "4901000e0e0e0e0e18aa840984162e0e0e0e0e0e0e0eb352486..."
 }

 Animation format specs: https://github.com/microsoft/ExpressivePixels/wiki/Animation-Format

 */

#define MAX_FRAMES_DATA_SZ 0x40000

ErrorPtr EpxView::loadEpxAnimationJSON(JsonObjectPtr aEpxAnimation)
{
  // clear any previous pattern (and make dirty)
  clearData();
  // get the animation
  JsonObjectPtr o;
  if (aEpxAnimation->get("PaletteSize", o)) {
    // - palette
    mNumPaletteEntries = o->int32Value();
    if (mNumPaletteEntries>256) return TextError::err("Palette cannot exceed 256 entries");
    mPaletteBuffer = new PixelColor[mNumPaletteEntries];
    if (aEpxAnimation->get("PaletteHex", o)) {
      string p = hexToBinaryString(o->stringValue().c_str(), false, mNumPaletteEntries*3);
      if (p.size()!=mNumPaletteEntries*3) return TextError::err("PaletteHex has wrong size");
      PixelColor pc;
      for (int i=0; i<mNumPaletteEntries; i++) {
        pc.r = p[i*3];
        pc.g = p[i*3+1];
        pc.b = p[i*3+2];
        pc.a = 255;
        mPaletteBuffer[i] = pc;
      }
      if (aEpxAnimation->get("FrameCount", o)) {
        mFrameCount = o->int32Value();
        if (aEpxAnimation->get("FramesHexLength", o)) { // length of the hex string (not of the binary data!)
          // - frames data
          size_t fb = o->int32Value()/2;
          if (fb<MAX_FRAMES_DATA_SZ) {
            if (aEpxAnimation->get("FramesHex", o)) {
              mFramesData = hexToBinaryString(o->stringValue().c_str(), false, fb);
              if (fb==mFramesData.size()) {
                // - optional loopcount (default: loop forever)
                mLoopCount = 0;
                if (aEpxAnimation->get("LoopCount", o)) {
                  mLoopCount = o->int32Value();
                }
                // - optional framerate (default: 12 FPS)
                mFrameInterval = Second/12;
                if (aEpxAnimation->get("FrameRate", o)) {
                  if (o->doubleValue()>0) {
                    mFrameInterval = Second/o->doubleValue();
                  }
                }
                // Epx package read ok
                resetAnimation();
                makeDirty();
                return ErrorPtr();
              }
              return TextError::err("Incorrect FramesHex lenght, expected=%zd, found=%zd", fb, mFramesData.size());
            }
            return TextError::err("Missing FramesHex");
          }
          return TextError::err("FramesHex too large: %zd (max allowed: %zd)", fb, (size_t)MAX_FRAMES_DATA_SZ);
        }
        return TextError::err("Missing FramesHexLength");
      }
      return TextError::err("Missing FrameCount");
    }
    return TextError::err("Missing PaletteHex");
  }
  return TextError::err("Missing PaletteSize");
}


MLMicroSeconds EpxView::step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow)
{
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil, aNow);
  if (mNextRender>=0 && aNow>=mNextRender) {
    mNextRender = renderFrame(aNow);
  }
  updateNextCall(nextCall, mNextRender); // Note: make sure nextCall is updated even when render is NOT (YET) called!
  return nextCall;
}


void EpxView::resetAnimation()
{
  mRemainingLoops = mLoopCount>0 ? mLoopCount : 1;
  mFramesCursor = 0;
  setAlpha(255); // make visible
}


void EpxView::start()
{
  resetAnimation();
  mNextRender = 0; // immediately
  makeDirtyAndUpdate();
}


void EpxView::stop()
{
  resetAnimation();
  mNextRender = Infinite; // stop running
}



void EpxView::setPalettePixel(uint8_t aPaletteIndex, PixelCoord aPixelIndex)
{
  if (aPaletteIndex>=mNumPaletteEntries) return; // NOP
  setPixel(mPaletteBuffer[aPaletteIndex], aPixelIndex);
}


inline static uint16_t getInt16(const string &aData, size_t &aCursor)
{
  if (aCursor+2<=aData.size()) {
    uint16_t r = (aData[aCursor]<<8)+(aData[aCursor+1]);
    aCursor+=2;
    return r;
  }
  return 0;
}


MLMicroSeconds EpxView::renderFrame(MLMicroSeconds aNow)
{
  MLMicroSeconds next = Infinite;
  if (mFramesCursor<mFramesData.size()) {
    char ft = mFramesData[mFramesCursor++];
    switch (ft) {
      case 'I': { // intra-coded frame (just fill in pixels)
        uint16_t pc = getInt16(mFramesData, mFramesCursor);
        FOCUSLOG("I-frame with %hd pixels", pc);
        PixelCoord pi = 0;
        while (pc>0 && mFramesCursor<mFramesData.size()) {
          setPalettePixel(mFramesData[mFramesCursor++], pi++);
          pc--;
        }
        next = aNow+mFrameInterval;
        break;
      }
      case 'P': { // prediced picture frame (addressed pixels)
        uint16_t pc = getInt16(mFramesData, mFramesCursor);
        FOCUSLOG("P-frame with %hd pixel changes", pc);
        PixelCoord pi = 0;
        if (getNumPixels()>256) {
          // 16 bit pixel addresses
          while (pc>0 && mFramesCursor+3<=mFramesData.size()) {
            pi = getInt16(mFramesData, mFramesCursor);
            setPalettePixel(mFramesData[mFramesCursor++], pi);
            pc--;
          }
        }
        else {
          // 8 bit pixel addresses
          while (pc>0 && mFramesCursor+2<=mFramesData.size()) {
            pi = (uint8_t)mFramesData[mFramesCursor++];
            setPalettePixel(mFramesData[mFramesCursor++], pi);
            pc--;
          }
        }
        next = aNow+mFrameInterval;
        break;
      }
      case 'D': { // delay
        uint16_t d = getInt16(mFramesData, mFramesCursor);
        FOCUSLOG("D-frame delaying %hd mS", d);
        if (d>0) next = aNow+(MLMicroSeconds)(d)*MilliSecond;
        break;
      }
      case 'F': { // fade out
        uint16_t d = getInt16(mFramesData, mFramesCursor);
        FOCUSLOG("F-frame fading down in %hd mS", d);
        animatorFor("alpha")->from(255)->animate(0, d*MilliSecond);
        if (d>0) next = aNow+(MLMicroSeconds)(d)*MilliSecond;
        break;
      }
    }
    makeDirty();
  }
  else {
    // end of frames
    if (mLoopCount>0) mRemainingLoops--;
    if (mRemainingLoops>0) {
      // repeat
      setAlpha(255); // make visible (again)
      mFramesCursor = 0;
      next = aNow;
    }
  }
  FOCUSLOG("next frame due at %s", MainLoop::string_mltime(next,3).c_str());
  return next;
}




#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr EpxView::configureView(JsonObjectPtr aViewConfig)
{
  // will possibly set dx, dy and/or sizetocontent first
  ErrorPtr err = inherited::configureView(aViewConfig);
  // now check for loading actual image
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    if (aViewConfig->get("epx", o)) {
      err = loadEpxAnimationJSON(o);
    }
    #if !ENABLE_P44SCRIPT
    if (aViewConfig->get("run", o)) {
      setRun(o->boolValue());
    }
    #endif
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

JsonObjectPtr EpxView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("run", JsonObject::newBool(getRun()));
  return status;
}

#endif // ENABLE_VIEWSTATUS


#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr EpxView::newViewObj()
{
  // base class with standard functionality
  return new EpxViewObj(this);
}

#if P44SCRIPT_FULL_SUPPORT

// loadepx(object|filepath)
FUNC_ARG_DEFS(loadepx, { text|objectvalue|undefres } );
static void loadepx_func(BuiltinFunctionContextPtr f)
{
  EpxViewObj* v = dynamic_cast<EpxViewObj*>(f->thisObj().get());
  assert(v);
  JsonObjectPtr json;
  ErrorPtr err;
  if (f->arg(0)->hasType(objectvalue)) {
    // use object as JSON directly
    json = f->arg(0)->jsonValue();
  }
  else {
    // assum filepath
    json = Application::jsonResource(f->arg(0)->stringValue(), &err);
  }
  if (Error::isOK(err)) {
    err = v->epx()->loadEpxAnimationJSON(json);
  }
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  f->finish();
}

#endif // P44SCRIPT_FULL_SUPPORT

#define ACCESSOR_CLASS EpxView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  EpxViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<EpxViewObj*>(aParentObj.get())->epx().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_BOOL(Run);

static const BuiltinMemberDescriptor epxMembers[] = {
  #if P44SCRIPT_FULL_SUPPORT
  FUNC_DEF_W_ARG(loadepx, executable|null|error),
  #endif
  // property accessors
  ACC_DECL("run", numeric|lvalue, Run),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedEpxMemberLookupP = NULL;

EpxViewObj::EpxViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  registerSharedLookup(sharedEpxMemberLookupP, epxMembers);
}

#endif // ENABLE_P44SCRIPT

#endif // ENABLE_EPX_SUPPORT

