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
  paletteBuffer(NULL),
  numPaletteEntries(0),
  nextRender(Infinite),
  framesCursor(0),
  remainingLoops(0)
{
}


EpxView::~EpxView()
{
  clear();
}


void EpxView::clear()
{
  inherited::clear();
  clearData();
}


void EpxView::clearData()
{
  // free the buffer if allocated
  if (paletteBuffer) {
    delete[] paletteBuffer;
    paletteBuffer = NULL;
  }
  framesData.clear();
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
    numPaletteEntries = o->int32Value();
    if (numPaletteEntries>256) return TextError::err("Palette cannot exceed 256 entries");
    paletteBuffer = new PixelColor[numPaletteEntries];
    if (aEpxAnimation->get("PaletteHex", o)) {
      string p = hexToBinaryString(o->stringValue().c_str(), false, numPaletteEntries*3);
      if (p.size()!=numPaletteEntries*3) return TextError::err("PaletteHex has wrong size");
      PixelColor pc;
      for (int i=0; i<numPaletteEntries; i++) {
        pc.r = p[i*3];
        pc.g = p[i*3+1];
        pc.b = p[i*3+2];
        pc.a = 255;
        paletteBuffer[i] = pc;
      }
      if (aEpxAnimation->get("FrameCount", o)) {
        frameCount = o->int32Value();
        if (aEpxAnimation->get("FramesHexLength", o)) { // length of the hex string (not of the binary data!)
          // - frames data
          size_t fb = o->int32Value()/2;
          if (fb<MAX_FRAMES_DATA_SZ) {
            if (aEpxAnimation->get("FramesHex", o)) {
              framesData = hexToBinaryString(o->stringValue().c_str(), false, fb);
              if (fb==framesData.size()) {
                // - optional loopcount (default: loop forever)
                loopCount = 0;
                if (aEpxAnimation->get("LoopCount", o)) {
                  loopCount = o->int32Value();
                }
                // - optional framerate (default: 12 FPS)
                frameInterval = Second/12;
                if (aEpxAnimation->get("FrameRate", o)) {
                  if (o->doubleValue()>0) {
                    frameInterval = Second/o->doubleValue();
                  }
                }
                // Epx package read ok
                resetAnimation();
                makeDirty();
                return ErrorPtr();
              }
              return TextError::err("Incorrect FramesHex lenght, expected=%zd, found=%zd", fb, framesData.size());
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


MLMicroSeconds EpxView::step(MLMicroSeconds aPriorityUntil)
{
  MLMicroSeconds now = MainLoop::now();
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil);
  if (nextRender>=0 && now>=nextRender) {
    nextRender = renderFrame();
  }
  updateNextCall(nextCall, nextRender); // Note: make sure nextCall is updated even when render is NOT (YET) called!
  return nextCall;
}


void EpxView::resetAnimation()
{
  remainingLoops = loopCount>0 ? loopCount : 1;
  framesCursor = 0;
  setAlpha(255); // make visible
}


void EpxView::start()
{
  resetAnimation();
  nextRender = 0; // immediately
  makeDirtyAndUpdate();
}


void EpxView::stop()
{
  resetAnimation();
  nextRender = Infinite; // stop running
}



void EpxView::setPalettePixel(uint8_t aPaletteIndex, PixelCoord aPixelIndex)
{
  if (aPaletteIndex>=numPaletteEntries) return; // NOP
  setPixel(paletteBuffer[aPaletteIndex], aPixelIndex);
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


MLMicroSeconds EpxView::renderFrame()
{
  MLMicroSeconds now = MainLoop::now();
  MLMicroSeconds next = Infinite;
  if (framesCursor<framesData.size()) {
    char ft = framesData[framesCursor++];
    switch (ft) {
      case 'I': { // intra-coded frame (just fill in pixels)
        uint16_t pc = getInt16(framesData, framesCursor);
        FOCUSLOG("I-frame with %hd pixels", pc);
        PixelCoord pi = 0;
        while (pc>0 && framesCursor<framesData.size()) {
          setPalettePixel(framesData[framesCursor++], pi++);
          pc--;
        }
        next = now+frameInterval;
        break;
      }
      case 'P': { // prediced picture frame (addressed pixels)
        uint16_t pc = getInt16(framesData, framesCursor);
        FOCUSLOG("P-frame with %hd pixel changes", pc);
        PixelCoord pi = 0;
        if (getNumPixels()>256) {
          // 16 bit pixel addresses
          while (pc>0 && framesCursor+3<=framesData.size()) {
            pi = getInt16(framesData, framesCursor);
            setPalettePixel(framesData[framesCursor++], pi);
            pc--;
          }
        }
        else {
          // 8 bit pixel addresses
          while (pc>0 && framesCursor+2<=framesData.size()) {
            pi = (uint8_t)framesData[framesCursor++];
            setPalettePixel(framesData[framesCursor++], pi);
            pc--;
          }
        }
        next = now+frameInterval;
        break;
      }
      case 'D': { // delay
        uint16_t d = getInt16(framesData, framesCursor);
        FOCUSLOG("D-frame delaying %hd mS", d);
        if (d>0) next = now+(MLMicroSeconds)(d)*MilliSecond;
        break;
      }
      case 'F': { // fade out
        uint16_t d = getInt16(framesData, framesCursor);
        FOCUSLOG("F-frame fading down in %hd mS", d);
        animatorFor("alpha")->from(255)->animate(0, d*MilliSecond);
        if (d>0) next = now+(MLMicroSeconds)(d)*MilliSecond;
        break;
      }
    }
    makeDirty();
  }
  else {
    // end of frames
    if (loopCount>0) remainingLoops--;
    if (remainingLoops>0) {
      // repeat
      setAlpha(255); // make visible (again)
      framesCursor = 0;
      next = now;
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
    if (aViewConfig->get("run", o)) {
      bool r = o->boolValue();
      if (r!=(nextRender>0)) {
        if (r) start();
        else stop();
      }
    }
  }
  return err;
}

#endif // ENABLE_EPX_SUPPORT

#if ENABLE_VIEWSTATUS

JsonObjectPtr EpxView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("run", JsonObject::newBool(nextRender>0));
  return status;
}

#endif // ENABLE_VIEWSTATUS


#endif // ENABLE_IMAGE_SUPPORT


