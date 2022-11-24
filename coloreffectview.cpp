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
  mRadial(true),
  mBriGradient(0),
  mHueGradient(0),
  mSatGradient(0),
  mBriMode(gradient_none),
  mHueMode(gradient_none),
  mSatMode(gradient_none)
{
  // make sure we start dark!
  setForegroundColor(black);
  setBackgroundColor(transparent);
  mExtent.x = 10;
  mExtent.y = 10;
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
    aBaseColor.r!=mForegroundColor.r || aBaseColor.b!=mForegroundColor.g || aBaseColor.g!=mForegroundColor.b || aBaseColor.a!=mForegroundColor.a ||
    mBriGradient!=aBri || mHueGradient!=aHue || mSatGradient!=aSat ||
    mBriMode!=aBriMode || mHueMode!=aHueMode || mSatMode!=aSatMode ||
    aRadial!=mRadial
  ) {
    mRadial = aRadial;
    mForegroundColor = aBaseColor;
    mBriGradient = aBri; mBriMode = aBriMode;
    mHueGradient = aHue; mHueMode = aHueMode;
    mSatGradient = aSat; mSatMode = aSatMode;
    makeColorDirty();
  }
}


void ColorEffectView::setExtent(PixelPoint aExtent)
{
  mExtent = aExtent;
  makeColorDirty(); // because it also affects gradient
}


void ColorEffectView::setRelativeExtent(double aRelativeExtent)
{
  mExtent.x = aRelativeExtent*mContent.dx/2;
  mExtent.y = aRelativeExtent*mContent.dy/2;
  makeColorDirty(); // because it also affects gradient
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


#define CURVE_EXP 4.0

double ColorEffectView::gradientCurveLevel(double aProgress, GradientMode aMode)
{
  if ((aMode&gradient_curve_mask)==gradient_none) return 0; // no gradient at all
  bool minus = aProgress<0;
  double samp = gradientCycles(fabs(aProgress), aMode);
  switch (aMode&gradient_curve_mask) {
    case gradient_curve_square: samp = samp>0.5 ? 1 : 0; break; // square
    case gradient_curve_sin: samp = sin(samp*M_PI/2); break; // sine
    case gradient_curve_cos: samp = 1-cos(samp*M_PI/2); break; // cosine
    case gradient_curve_log: samp = 1.0/CURVE_EXP*log(samp*(exp(CURVE_EXP)-1)+1); break; // logarithmic
    case gradient_curve_exp: samp = (exp(samp*CURVE_EXP)-1)/(exp(CURVE_EXP)-1); break; // exponential
    default:
    case gradient_curve_lin: break; // linear/triangle
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
  mGradientPixels.clear();
  if (mBriGradient==0 && mHueGradient==0 && mSatGradient==0) return; // optimized
  // initial HSV
  double base_h,base_s,base_b;
  pixelToHsb(mForegroundColor, base_h, base_s, base_b, true);
  double h,s,b;
  // now create gradient pixels covering extent dimension
  for (int i=0; i<aNumGradientPixels; i++) {
    // progress within the extent (0..1)
    double pr = aExtentPixels>0 ? (double)i/aExtentPixels : 0;
    // - hue
    h = gradiated(base_h, pr, mHueGradient, mHueMode, 360, true);
    // - saturation
    s = gradiated(base_s, pr, mSatGradient, mSatMode, 1, false);
    // - brightness
    b = gradiated(base_b, pr, mBriGradient, mBriMode, 1, false);
    // store the pixel
    PixelColor gpix = hsbToPixel(h,s,b, true);
    mGradientPixels.push_back(gpix);
  }
}


PixelColor ColorEffectView::gradientPixel(int aPixelIndex)
{
  int numGPixels = (int)mGradientPixels.size();
  if (numGPixels==0) return mForegroundColor;
  if (aPixelIndex<0) aPixelIndex = 0; else if (aPixelIndex>=numGPixels) aPixelIndex = numGPixels-1;
  return mGradientPixels[aPixelIndex];
}



#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr ColorEffectView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    bool colsChanged = false;
    if (aViewConfig->get("brightness_gradient", o)) {
      mBriGradient = o->doubleValue();
      colsChanged = true;
    }
    if (aViewConfig->get("hue_gradient", o)) {
      mHueGradient = o->doubleValue();
      colsChanged = true;
    }
    if (aViewConfig->get("saturation_gradient", o)) {
      mSatGradient = o->doubleValue();
      colsChanged = true;
    }
    if (aViewConfig->get("brightness_mode", o)) {
      mBriMode = o->int32Value();
      colsChanged = true;
    }
    if (aViewConfig->get("hue_mode", o)) {
      mHueMode = o->int32Value();
      colsChanged = true;
    }
    if (aViewConfig->get("saturation_mode", o)) {
      mSatMode = o->int32Value();
      colsChanged = true;
    }
    if (aViewConfig->get("radial", o)) {
      mRadial = o->boolValue();
      colsChanged = true;
    }
    if (colsChanged) {
      makeColorDirty();
    }
    if (aViewConfig->get("extent_x", o)) {
      mExtent.x = o->doubleValue();
      makeDirty();
    }
    if (aViewConfig->get("extent_y", o)) {
      mExtent.y = o->doubleValue();
      makeDirty();
    }
    if (aViewConfig->get("rel_extent", o)) {
      setRelativeExtent(o->doubleValue());
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_VIEWSTATUS

JsonObjectPtr ColorEffectView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("brightness_gradient", JsonObject::newDouble(mBriGradient));
  status->add("hue_gradient", JsonObject::newDouble(mHueGradient));
  status->add("saturation_gradient", JsonObject::newDouble(mSatGradient));
  status->add("brightness_mode", JsonObject::newInt32(mBriMode));
  status->add("hu_mode", JsonObject::newInt32(mHueMode));
  status->add("saturation_mode", JsonObject::newInt32(mSatMode));
  status->add("radial", JsonObject::newBool(mRadial));
  status->add("extent_x", JsonObject::newDouble(mExtent.x));
  status->add("extent_y", JsonObject::newDouble(mExtent.y));
  return status;
}

#endif // ENABLE_VIEWSTATUS


#if ENABLE_ANIMATION

void ColorEffectView::coloringPropertySetter(double &aColoringParam, double aNewValue)
{
  if (aNewValue!=aColoringParam) {
    aColoringParam = aNewValue;
    makeColorDirty();
  }
}


ValueSetterCB ColorEffectView::getPropertySetter(const string aProperty, double& aCurrentValue)
{
  if (aProperty=="hue_gradient") {
    aCurrentValue = mHueGradient;
    return boost::bind(&ColorEffectView::coloringPropertySetter, this, mHueGradient, _1);
  }
  else if (aProperty=="brightness_gradient") {
    aCurrentValue = mHueGradient;
    return boost::bind(&ColorEffectView::coloringPropertySetter, this, mBriGradient, _1);
  }
  else if (aProperty=="saturation_gradient") {
    aCurrentValue = mSatGradient;
    return boost::bind(&ColorEffectView::coloringPropertySetter, this, mSatGradient, _1);
  }
  else if (aProperty=="extent_x") {
    return getCoordPropertySetter(mExtent.x, aCurrentValue);
  }
  else if (aProperty=="extent_y") {
    return getCoordPropertySetter(mExtent.y, aCurrentValue);
  }
  if (aProperty=="rel_extent") {
    aCurrentValue = 0; // dummy
    return boost::bind(&ColorEffectView::setRelativeExtent, this, _1);
  }

  // unknown at this level
  return inherited::getPropertySetter(aProperty, aCurrentValue);
}



#endif // ENABLE_ANIMATION
