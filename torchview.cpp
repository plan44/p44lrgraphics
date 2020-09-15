//
//  Copyright (c) 2017-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
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
  nextCalculation(Never)
{
  flame_min = 100;
  flame_max = 220;
  flame_height = 1;
  spark_probability = 2;
  spark_min = 200;
  spark_max = 255;
  spark_tfr = 40;
  spark_cap = 200;
  up_rad = 40;
  side_rad = 35;
  heat_cap = 0;
  hotspark_min = 250;
  hotsparkColor = { 170, 170, 250, 255 };
  hotsparkColorInc = { 0, 0, 5, 255 };
  foregroundColor = { 180, 145, 0, 255 }; // foreground is "energy" color, which scales up with energy of the spark
  cycleTime = 25*MilliSecond;
  clear();
}


TorchView::~TorchView()
{
}


void TorchView::clear()
{
  // clear and resize to current size
  torchDots.clear();
  torchDots.resize(content.dy*content.dx, { torch_passive, 0, 0 });
}


TorchView::TorchDot& TorchView::dot(PixelPoint aPt)
{
  int dotIdx = aPt.y*content.dx+aPt.x;
  if (dotIdx>=torchDots.size()) clear(); // safety, resize on access outside range
  return torchDots.at(dotIdx);
}



void TorchView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  if (aOldContent.dx!=content.dx || aOldContent.dy!=content.dy) {
    clear(); // size changed: clear and resize dots vector
  }
  inherited::geometryChanged(aOldFrame, aOldContent);
}


void TorchView::recalculateColoring()
{
  calculateGradient(256, extent.y*256*2/content.dy);
  inherited::recalculateColoring();
}



MLMicroSeconds TorchView::step(MLMicroSeconds aPriorityUntil)
{
  MLMicroSeconds now = MainLoop::now();
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil);
  if (cycleTime!=Never && now>=nextCalculation) {
    nextCalculation = now+cycleTime;
    calculateCycle();
  }
  updateNextCall(nextCall, now+cycleTime);
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
  if (alpha==0) return; // don't waste cycles
  PixelPoint dotpos;
  // random flame
  for (dotpos.y=0; dotpos.y<flame_height; dotpos.y++) {
    for (dotpos.x=0; dotpos.x<content.dx; dotpos.x++) {
      TorchDot& d = dot(dotpos);
      d.previous = random(flame_min, flame_max);
      d.mode = torch_nop;
    }
  }
  // random sparks in next row
  dotpos.y = flame_height;
  for (dotpos.x=0; dotpos.x<content.dx; dotpos.x++) {
    TorchDot& d = dot(dotpos);
    if (d.mode!=torch_spark && random(100)<spark_probability) {
      d.previous = random(spark_min, spark_max);
      d.mode = torch_spark;
    }
  }
  // let sparks fly and fade
  for (dotpos.y=0; dotpos.y<content.dy; dotpos.y++) {
    for (dotpos.x=0; dotpos.x<content.dx; dotpos.x++) {
      TorchDot& d = dot(dotpos);
      uint8_t e = d.previous;
      EnergyMode m = d.mode;
      switch (m) {
        case torch_spark: {
          // loose transfer-up-energy as long as there is any
          reduce(e, spark_tfr);
          // cell above is temp spark, sucking up energy from this cell until empty
          if (dotpos.y<content.dy-1) {
            TorchDot& above = dot({ dotpos.x, dotpos.y+1 });
            above.mode = torch_spark_temp;
          }
          break;
        }
        case torch_spark_temp: {
          // just getting some energy from below
          TorchDot& below = dot({ dotpos.x, dotpos.y-1 });
          uint8_t e2 = below.previous;
          if (e2<spark_tfr) {
            // cell below is exhausted, becomes passive
            below.mode = torch_passive;
            // gobble up rest of energy
            increase(e, e2);
            // loose some overall energy
            e = ((int)e*spark_cap)>>8;
            // this cell becomes active spark
            d.mode = torch_spark;
          }
          else {
            increase(e, spark_tfr);
          }
          break;
        }
        case torch_passive: {
          e = ((int)e*heat_cap)>>8;
          increase(e, (
            (((int)dot({dotpos.x>0 ? dotpos.x-1 : content.dx-1, dotpos.y}).previous+(int)dot({dotpos.x<content.dx-1 ? dotpos.x+1 : 0, dotpos.y}).previous)*side_rad)>>9) +
            (((int)dot({dotpos.x, dotpos.y-1}).previous*up_rad)>>8)
          );
        }
        default:
          break;
      }
      d.current = e;
    }
  }
  // transfer new
  for (TorchDotVector::iterator pos=torchDots.begin(); pos!=torchDots.end(); ++pos) {
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
  if (alpha>0 && isInContentSize(aPt)) {
    TorchDot& d = dot(aPt);
    int extraHeat = d.current-hotspark_min;
    if (extraHeat>=0) {
      pix = hotsparkColor;
      addToPixel(pix, dimmedPixel(hotsparkColorInc, 255*(int)extraHeat/(255-hotspark_min)));
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


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr TorchView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    // flame parameters
    if (aViewConfig->get("flame_min", o)) {
      flame_min  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("flame_max", o)) {
      flame_max  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("flame_height", o)) {
      flame_height  = o->int32Value(); makeDirty();
    }
    // spark generation parameters
    if (aViewConfig->get("spark_probability", o)) {
      spark_probability  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("spark_min", o)) {
      spark_min  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("spark_max", o)) {
      spark_max  = o->int32Value(); makeDirty();
    }
    // spark development parameters
    if (aViewConfig->get("spark_tfr", o)) {
      spark_tfr  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("spark_cap", o)) {
      spark_cap  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("up_rad", o)) {
      up_rad  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("side_rad", o)) {
      side_rad  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("heat_cap", o)) {
      heat_cap  = o->int32Value(); makeDirty();
    }
    // hot spark coloring parameters
    if (aViewConfig->get("hotspark_min", o)) {
      hotspark_min  = o->int32Value(); makeDirty();
    }
    if (aViewConfig->get("hotsparkcolor", o)) {
      hotsparkColor = webColorToPixel(o->stringValue()); makeDirty();
    }
    if (aViewConfig->get("hotsparkinc", o)) {
      hotsparkColorInc = webColorToPixel(o->stringValue()); makeDirty();
    }
    // timing parameter
    if (aViewConfig->get("cycletime", o)) {
      cycleTime  = o->doubleValue()*Second;
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG


#if ENABLE_VIEWSTATUS

JsonObjectPtr TorchView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("flame_min", JsonObject::newInt32(flame_min));
  status->add("flame_max", JsonObject::newInt32(flame_max));
  status->add("flame_height", JsonObject::newInt32(flame_height));
  status->add("spark_probability", JsonObject::newInt32(spark_probability));
  status->add("spark_min", JsonObject::newInt32(spark_min));
  status->add("spark_max", JsonObject::newInt32(spark_max));
  status->add("spark_tfr", JsonObject::newInt32(spark_tfr));
  status->add("spark_cap", JsonObject::newInt32(spark_cap));
  status->add("up_rad", JsonObject::newInt32(up_rad));
  status->add("side_rad", JsonObject::newInt32(side_rad));
  status->add("heat_cap", JsonObject::newInt32(heat_cap));
  status->add("hotspark_min", JsonObject::newInt32(hotspark_min));
  status->add("hotsparkinc", JsonObject::newString(pixelToWebColor(hotsparkColorInc)));
  status->add("hotsparkcolor", JsonObject::newString(pixelToWebColor(hotsparkColor)));
  status->add("cycletime", JsonObject::newDouble((double)cycleTime/Second));

  return status;
}

#endif // ENABLE_VIEWSTATUS
