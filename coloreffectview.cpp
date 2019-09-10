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

#include "coloreffectview.hpp"
#include <math.h>

using namespace p44;


// MARK: ===== Color Effect View


ColorEffectView::ColorEffectView() :
  radial(true),
  briGradient(0),
  hueGradient(0),
  satGradient(0),
  briMode(gradient_none),
  hueMode(gradient_none),
  satMode(gradient_none)
{
  // make sure we start dark!
  setForegroundColor(black);
  setBackgroundColor(transparent);
}

ColorEffectView::~ColorEffectView()
{
}


void ColorEffectView::setColoringParameters(
  PixelColor aBaseColor,
  double aBri, GradientMode aBriMode,
  double aHue, GradientMode aHueMode,
  double aSat, GradientMode aSatMode,
  bool aRadial
) {
  if (
    aBaseColor.r!=foregroundColor.r || aBaseColor.b!=foregroundColor.g || aBaseColor.g!=foregroundColor.b || aBaseColor.a!=foregroundColor.a ||
    briGradient!=aBri || hueGradient!=aHue || satGradient!=aSat ||
    briMode!=aBriMode || hueMode!=aHueMode || satMode!=aSatMode ||
    aRadial!=radial
  ) {
    radial = aRadial;
    setForegroundColor(aBaseColor);
    briGradient = aBri; briMode = aBriMode;
    hueGradient = aHue; hueMode = aHueMode;
    satGradient = aSat; satMode = aSatMode;
    recalculateColoring();
  }
}


void ColorEffectView::setExtent(PixelCoord aExtent)
{
  extent = aExtent;
  makeDirty();
}


double ColorEffectView::gradientCycles(double aValue, GradientMode aMode)
{
  aMode &= gradient_repeat_mask;
  switch (aMode) {
    case gradient_repeat_cyclic:
      return aValue-floor(aValue);
    case gradient_repeat_oscillating: {
      double pr = aValue-floor(aValue);
      if (((int)aValue & 1)==1) return 1-pr;
      return pr;
    }
    case gradient_unlimited:
      return aValue; // unlimited
    default:
    case gradient_repeat_none:
      return aValue>1 ? 1 : aValue;
  }
}


double ColorEffectView::gradientCurveLevel(double aProgress, GradientMode aMode)
{
  if ((aMode&gradient_curve_mask)==gradient_none) return 0; // no gradient at all
  bool minus = aProgress<0;
  double samp = gradientCycles(fabs(aProgress), aMode);
  switch (aMode&gradient_curve_mask) {
    case gradient_curve_sin: samp = sin(samp*M_PI/2); break; // sine
    case gradient_curve_square: samp = samp>0.5 ? 1 : 0; // square
    default:
    case gradient_curve_lin: break;
  }
  return minus ? -samp : samp;
}


double ColorEffectView::gradiated(double aValue, double aProgress, double aGradient, GradientMode aMode, double aMax, bool aWrap)
{
  if (aGradient==0 || aMode==gradient_none) return aValue; // no gradient
  aValue = aValue+gradientCurveLevel(aProgress*aGradient, aMode)*aMax;
  if (aValue>aMax) return aWrap ? aValue-aMax : aMax;
  if (aValue<0) return aWrap ? aValue+aMax : 0;
  return aValue;
}



void ColorEffectView::calculateGradient(int aNumGradientPixels, int aExtentPixels)
{
  gradientPixels.clear();
  if (briGradient==0 && hueGradient==0 && satGradient==0) return; // optimized
  // initial HSV
  Row3 rgb = { (double)foregroundColor.r/255, (double)foregroundColor.g/255, (double)foregroundColor.b/255 };
  Row3 hsb;
  RGBtoHSV(rgb, hsb);
  hsb[2] = (double)foregroundColor.a/255;
  Row3 resHsb;
  // now create gradient pixels covering larger extent dimension
  for (int i=0; i<aNumGradientPixels; i++) {
    // progress within the extent (0..1)
    double pr = (double)i/aExtentPixels;
    // - brightness
    resHsb[2] = gradiated(hsb[2], pr, briGradient, briMode, 1, false);
    // - hue
    resHsb[0] = gradiated(hsb[0], pr, hueGradient, hueMode, 360, true);
    // - saturation
    resHsb[1] = gradiated(hsb[1], pr, satGradient, satMode, 1, false);
    // store the pixel
    PixelColor gpix = hsbToPixel(resHsb[0], resHsb[1], resHsb[2], true);
    gradientPixels.push_back(gpix);
  }
}


PixelColor ColorEffectView::gradientPixel(int aPixelIndex)
{
  int numGPixels = (int)gradientPixels.size();
  if (aPixelIndex<0) aPixelIndex = 0; else if (aPixelIndex>=numGPixels) aPixelIndex = numGPixels-1;
  return gradientPixels[aPixelIndex];
}



#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr ColorEffectView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    bool colsChanged = false;
    if (aViewConfig->get("color", o)) {
      // parent handles setting the (base)color, but this is a coloring param change
      colsChanged = true;
    }
    if (aViewConfig->get("brightness_gradient", o)) {
      briGradient = o->doubleValue();
      colsChanged = true;
    }
    if (aViewConfig->get("hue_gradient", o)) {
      hueGradient = o->doubleValue();
      colsChanged = true;
    }
    if (aViewConfig->get("saturation_gradient", o)) {
      satGradient = o->doubleValue();
      colsChanged = true;
    }
    if (aViewConfig->get("brightness_mode", o)) {
      briMode = o->int32Value();
      colsChanged = true;
    }
    if (aViewConfig->get("hue_mode", o)) {
      hueMode = o->int32Value();
      colsChanged = true;
    }
    if (aViewConfig->get("saturation_mode", o)) {
      satMode = o->int32Value();
      colsChanged = true;
    }
    if (aViewConfig->get("radial", o)) {
      radial = o->boolValue();
      colsChanged = true;
    }
    if (colsChanged) {
      recalculateColoring();
    }
  }
  return err;
}


#endif // ENABLE_VIEWCONFIG
