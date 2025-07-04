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

#include "coloreffectview.hpp"
#include <math.h>

using namespace p44;


// MARK: ===== Color Effect View

// Note: is a base class, not an independent view, so not registered to the factory

ColorEffectView::ColorEffectView() :
  mRadial(true),
  mTransparentFade(true),
  mEffectWrap(false),
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
  mEffectCycles = 1; // single cycle (gradient = 1 -> covers full gradient)
  mEffectZoom = 1; // standard effect size (relative to content size)
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


double ColorEffectView::gradientCycles(double aValue, GradientMode aMode)
{
  aMode = (GradientMode)(aMode & gradient_repeat_mask);
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
  double samp = gradientCycles(aProgress, aMode);
  switch (aMode&gradient_curve_mask) {
    case gradient_curve_square: return samp>0.5 ? 1 : 0; // square
    case gradient_curve_sin: return (1+sin(samp*M_PI))/2; // one sine period per progress unit
    case gradient_curve_cos: return (1-cos(samp*M_PI))/2; // one cosine period per progress unit
    case gradient_curve_log: return log(samp*(exp(CURVE_EXP)-1)+1) / CURVE_EXP; // logarithmic
    case gradient_curve_exp: return (exp(samp*CURVE_EXP)-1)/(exp(CURVE_EXP)-1); // exponential
    default:
    case gradient_curve_lin: break; // linear/triangle
  }
  return samp;
}


double ColorEffectView::gradiated(double aValue, double aProgress, double aGradient, GradientMode aMode, double aMax, bool aWrap)
{
  if (aGradient==0 || (aMode&gradient_curve_mask)==gradient_none) return aValue; // no gradient
  // add gradient for the value
  double componentprogress = aProgress*aGradient;
  bool minus = componentprogress<0;
  double change = gradientCurveLevel(fabs(componentprogress), aMode)*aMax;
  if (minus) aValue -= change;
  else aValue += change;
  // bring back into range
  if (aWrap) {
    while (aValue>aMax) aValue-=aMax;
    while (aValue<0) aValue+=aMax;
  }
  else {
    if (aValue>aMax) aValue = aMax;
    else if (aValue<0) aValue = 0;
  }
  // possibly invert the value: start at maximum instead of minimum
  if (aMode & gradient_curve_inverted) aValue = aMax - aValue;
  return aValue;
}



/// @param aNumGradientPixels the number of gradient pixels we need (at most, should be enough to span
///   the entire view)
void ColorEffectView::calculateGradient(int aNumGradientPixels)
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
    double pr = (double)i*mEffectCycles/aNumGradientPixels;
    // - hue
    h = gradiated(base_h, pr, mHueGradient, mHueMode, 360, true);
    // - saturation
    s = gradiated(base_s, pr, mSatGradient, mSatMode, 1, false);
    // - brightness
    b = gradiated(base_b, pr, mBriGradient, mBriMode, 1, false);
    // store the pixel
    PixelColor gpix = hsbToPixel(h,s,b, mTransparentFade);
    mGradientPixels.push_back(gpix);
  }
}


PixelColor ColorEffectView::gradientPixel(int aPixelIndex, bool aWrap)
{
  int numGPixels = (int)mGradientPixels.size();
  if (numGPixels==0) return mForegroundColor;
  if (aPixelIndex<0) aPixelIndex = 0; // for negative indices: value of first pixel
  else if (aPixelIndex>=numGPixels) {
    if (aWrap) {
      aPixelIndex = aPixelIndex % numGPixels;
    }
    else {
      aPixelIndex = numGPixels-1; // for indices larger than gradient we have: value of the last pixel
    }
  }
  return mGradientPixels[aPixelIndex];
}

// MARK: ===== view configuration

#if ENABLE_VIEWCONFIG

#if !ENABLE_P44SCRIPT

// legacy implementation
ErrorPtr ColorEffectView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  announceChanges(true);
  if (Error::isOK(err)) {
    bool colsChanged = false;
    if (aViewConfig->get("brightness_gradient", o)) {
      setBriGradient(o->doubleValue());
    }
    if (aViewConfig->get("hue_gradient", o)) {
      setHueGradient(o->doubleValue());
    }
    if (aViewConfig->get("saturation_gradient", o)) {
      setSatGradient(o->doubleValue());
    }
    if (aViewConfig->get("brightness_mode", o)) {
      if (o->isType(json_type_string)) {
        setBriMode(textToGradientMode(o->c_strValue()));
      }
      else {
        setBriMode((GradientMode)(o->int32Value()));
      }
    }
    if (aViewConfig->get("hue_mode", o)) {
      if (o->isType(json_type_string)) {
        setHueMode(textToGradientMode(o->c_strValue()));
      }
      else {
        setHueMode((GradientMode)(o->int32Value()));
      }
    }
    if (aViewConfig->get("saturation_mode", o)) {
      if (o->isType(json_type_string)) {
        setSatMode(textToGradientMode(o->c_strValue()));
      }
      else {
        setSatMode((GradientMode)(o->int32Value()));
      }
    }
    if (aViewConfig->get("radial", o)) {
      setRadial(o->boolValue());
    }
    if (aViewConfig->get("extent_x", o)) {
      setExtentX(o->doubleValue());
    }
    if (aViewConfig->get("extent_y", o)) {
      setExtentY(o->doubleValue());
    }
    if (aViewConfig->get("rel_extent", o)) {
      setRelativeExtent(o->doubleValue());
    }
  }
  announceChanges(false);
  return err;
}

#endif

typedef struct {
  GradientMode mode;
  GradientMode mask;
  const char *name;
} GradientModeDesc;
static const GradientModeDesc gradientModeDesc[] = {
  { gradient_none, (GradientMode)(~0), "none" },
  { gradient_curve_square, gradient_curve_mask, "square" },
  { gradient_curve_lin, gradient_curve_mask, "linear" },
  { gradient_curve_sin, gradient_curve_mask, "sin" },
  { gradient_curve_cos, gradient_curve_mask, "cos" },
  { gradient_curve_log, gradient_curve_mask, "log" },
  { gradient_curve_exp, gradient_curve_mask, "exp" },
  { gradient_curve_inverted, gradient_curve_inverted, "inverted" },
  { gradient_repeat_none, gradient_repeat_mask, "norepeat" },
  { gradient_repeat_cyclic, gradient_repeat_mask, "cyclic" },
  { gradient_repeat_oscillating, gradient_repeat_mask, "oscillating" },
  { gradient_unlimited, gradient_repeat_mask, "unlimited" },
  { (GradientMode)0, (GradientMode)0, nullptr }
};


GradientMode ColorEffectView::textToGradientMode(const char *aGradientText)
{
  GradientMode g = gradient_none;
  while (aGradientText) {
    size_t n = 0;
    while (aGradientText[n] && aGradientText[n]!='|') n++;
    for (const GradientModeDesc *gd = gradientModeDesc; gd->name; gd++) {
      if (uequals(aGradientText, gd->name, n)) {
        g = (GradientMode)(g | gd->mode);
        break;
      }
    }
    aGradientText += n;
    if (*aGradientText==0) break;
    aGradientText++; // skip |
  }
  return g;
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_VIEWSTATUS

string ColorEffectView::gradientModeToText(GradientMode aMode)
{
  const GradientModeDesc *gd = gradientModeDesc;
  string modes;
  while (gd->name) {
    if ((aMode & gd->mask) == gd->mode) {
      if (!modes.empty()) modes += "|";
      modes += gd->name;
      if (aMode==0) break; // no gradient at all - do not consult curves and repeat modes
    }
    gd++;
  }
  return modes;
}

#if !ENABLE_P44SCRIPT

JsonObjectPtr ColorEffectView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("brightness_gradient", JsonObject::newDouble(getBriGradient()));
  status->add("hue_gradient", JsonObject::newDouble(getHueGradient()));
  status->add("saturation_gradient", JsonObject::newDouble(getSatGradient()));
  status->add("brightness_mode", JsonObject::newString(gradientModeToText(mBriMode)));
  status->add("hue_mode", JsonObject::newString(gradientModeToText(mHueMode)));
  status->add("saturation_mode", JsonObject::newString(gradientModeToText(mSatMode)));
  status->add("radial", JsonObject::newBool(getRadial()));
  status->add("extent_x", JsonObject::newDouble(getExtentX()));
  status->add("extent_y", JsonObject::newDouble(getExtentY()));
  return status;
}

#endif // !ENABLE_P44SCRIPT

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
  if (uequals(aProperty, "hue_gradient")) {
    aCurrentValue = mHueGradient;
    return boost::bind(&ColorEffectView::coloringPropertySetter, this, mHueGradient, _1);
  }
  else if (uequals(aProperty, "brightness_gradient")) {
    aCurrentValue = mHueGradient;
    return boost::bind(&ColorEffectView::coloringPropertySetter, this, mBriGradient, _1);
  }
  else if (uequals(aProperty, "saturation_gradient")) {
    aCurrentValue = mSatGradient;
    return boost::bind(&ColorEffectView::coloringPropertySetter, this, mSatGradient, _1);
  }
  else if (uequals(aProperty, "effect_cycles")) {
    aCurrentValue = mEffectCycles;
    return boost::bind(&ColorEffectView::setEffectCycles, this, _1);
  }
  else if (uequals(aProperty, "effect_zoom")) {
    aCurrentValue = mEffectZoom;
    return boost::bind(&ColorEffectView::setEffectZoom, this, _1);
  }
  // unknown at this level
  return inherited::getPropertySetter(aProperty, aCurrentValue);
}

#endif // ENABLE_ANIMATION


#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr ColorEffectView::newViewObj()
{
  // base class with standard functionality
  return new ColorEffectViewObj(this);
}


#define ACCESSOR_CLASS ColorEffectView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  ColorEffectViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<ColorEffectViewObj*>(aParentObj.get())->colorEffect().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

#define ACC_IMPL_GMODE(prop) \
  static ScriptObjPtr access_##prop(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite) \
  { \
    if (!aToWrite) return new StringValue(ACCESSOR_CLASS::gradientModeToText(aView.get##prop())); \
    if (aToWrite->hasType(numeric)) aView.set##prop((GradientMode)(aToWrite->intValue())); \
    else aView.set##prop(ACCESSOR_CLASS::textToGradientMode(aToWrite->stringValue().c_str())); \
    return aToWrite; /* reflect back to indicate writable */ \
  }

ACC_IMPL_DBL(BriGradient);
ACC_IMPL_DBL(HueGradient);
ACC_IMPL_DBL(SatGradient);
ACC_IMPL_GMODE(BriMode);
ACC_IMPL_GMODE(HueMode);
ACC_IMPL_GMODE(SatMode);
ACC_IMPL_DBL(EffectCycles);
ACC_IMPL_DBL(EffectZoom);
ACC_IMPL_BOOL(EffectWrap);
ACC_IMPL_BOOL(Radial);
ACC_IMPL_BOOL(TransparentFade);


static const BuiltinMemberDescriptor colorEffectMembers[] = {
  // property accessors
  ACC_DECL("brightness_gradient", numeric|lvalue, BriGradient),
  ACC_DECL("hue_gradient", numeric|lvalue, HueGradient),
  ACC_DECL("saturation_gradient", numeric|lvalue, SatGradient),
  ACC_DECL("brightness_mode", numeric|lvalue, BriMode),
  ACC_DECL("hue_mode", numeric|lvalue, HueMode),
  ACC_DECL("saturation_mode", numeric|lvalue, SatMode),
  ACC_DECL("effect_cycles", numeric|lvalue, EffectCycles),
  ACC_DECL("effect_zoom", numeric|lvalue, EffectZoom),
  ACC_DECL("effect_wrap", numeric|lvalue, EffectWrap),
  ACC_DECL("radial", numeric|lvalue, Radial),
  ACC_DECL("transparent_fade", numeric|lvalue, TransparentFade),
  BUILTINS_TERMINATOR
};

static BuiltInMemberLookup* sharedColorEffectMemberLookupP = NULL;

ColorEffectViewObj::ColorEffectViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  registerSharedLookup(sharedColorEffectMemberLookupP, colorEffectMembers);
}

#endif // ENABLE_P44SCRIPT
