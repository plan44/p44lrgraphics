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

#include "lightspotview.hpp"
#include <math.h>

using namespace p44;


// MARK: ===== Light Spot View


LightSpotView::LightSpotView() :
  center({0, 0}),
  extent({5, 5}),
  radial(true),
  briGradient(0),
  hueGradient(0),
  satGradient(0),
  briMode(gradient_none),
  hueMode(gradient_none),
  satMode(gradient_none),
  rotation(0),
  rotCos(1.0),
  rotSin(0.0)
{
}

LightSpotView::~LightSpotView()
{
}


void LightSpotView::clear()
{
}



void LightSpotView::recalculateContentRect()
{
  PixelRect c;
  c.x = center.x;
  c.y = center.y;
  c.dx = extent.x*2+1;
  c.dy = extent.y*2+1;
  setContent(c);
}


void LightSpotView::setQuadrant(PixelRect aQuadrant)
{
  center.x = aQuadrant.x;
  center.y = aQuadrant.y;
  extent.x = aQuadrant.dx;
  extent.y = aQuadrant.dy;
  recalculateContentRect();
}


void LightSpotView::setCenter(PixelCoord aCenter)
{
  center = aCenter;
  recalculateContentRect();
}


void LightSpotView::setExtent(PixelCoord aExtent)
{
  extent = aExtent;
  recalculateContentRect();
}


void LightSpotView::setRotation(double aRotation)
{
  if (aRotation!=rotation) {
    rotation = aRotation;
    double rotPi = rotation*M_PI/180;
    rotSin = sin(rotPi);
    rotCos = cos(rotPi);
    makeDirty();
  }
}


void LightSpotView::setColoringParameters(
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
    recalculateGradients();
  }
}


static double gradientCycles(double aValue, GradientMode aMode)
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


static double gradientCurveLevel(double aProgress, GradientMode aMode)
{
  if ((aMode&gradient_curve_mask)==gradient_none) return 0; // no gradient at all
  bool minus = aProgress<0;
  double samp = gradientCycles(fabs(aProgress), aMode);
  switch (aMode&gradient_curve_mask) {
    case gradient_curve_sin: samp = sin(samp*M_PI/2); break; // sine
    case gradient_curve_log: samp = log(1+(aProgress*(M_E-1))); break; // logarithmic
    case gradient_curve_square: samp = samp>0.5 ? 1 : 0; // square
    default:
    case gradient_curve_lin: break;
  }
  return minus ? -samp : samp;
}


static double gradiated(double aValue, double aProgress, double aGradient, GradientMode aMode, double aMax, bool aWrap)
{
  if (aGradient==0 || aMode==gradient_none) return aValue; // no gradient
  aValue = aValue+gradientCurveLevel(aProgress*aGradient, aMode)*aMax;
  if (aValue>aMax) return aWrap ? aValue-aMax : aMax;
  if (aValue<0) return aWrap ? aValue+aMax : 0;
  return aValue;
}


void LightSpotView::recalculateGradients()
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
  int numGradientPixels = max(frame.dx, frame.dy);
  int extentPixels = max(extent.x, extent.y);
  for (int i=0; i<numGradientPixels; i++) {
    // progress within the extent (0..1)
    double pr = (double)i/extentPixels;
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
  makeDirty();
}



MLMicroSeconds LightSpotView::step(MLMicroSeconds aPriorityUntil)
{
  MLMicroSeconds now = MainLoop::now();
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil);
  // do stuff
  MLMicroSeconds nextstepin = 100*MilliSecond;

  updateNextCall(nextCall, now+nextstepin);
  return nextCall;
}


PixelColor LightSpotView::contentColorAt(PixelCoord aPt)
{
  PixelColor pix = transparent;

  // aPt are coordinates from center (already offset by content frame origin)
  // - rotate
  double x2 = aPt.x*rotCos-aPt.y*rotSin;
  double y2 = aPt.x*rotSin+aPt.y*rotCos;
  // - factor relative to the size (0..1)
  double xf = x2/extent.x;
  double yf = y2/extent.y;
  // - gardient or just clipped
  int numGPixels = (int)gradientPixels.size();
  double progress = radial ? sqrt(xf*xf+yf*yf) : fabs(xf);
  if (progress<1 || (contentWrapMode&clipXY)==0) {
    if (numGPixels>0) {
      int i = progress*numGPixels;
      if (i<0) i = 0; else if (i>=numGPixels) i = numGPixels-1;
      pix = gradientPixels[i];
    }
    else {
      pix = foregroundColor;
    }
  }
  return pix;
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr LightSpotView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
//    if (aViewConfig->get("apropertry", o)) {
//      apropertry  = o->doubleValue()*MilliSecond;
//    }
  }
  return err;
}


#endif // ENABLE_VIEWCONFIG
