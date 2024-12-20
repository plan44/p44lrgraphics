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

#ifndef __p44lrgraphics_coloreffectview_hpp__
#define __p44lrgraphics_coloreffectview_hpp__

#include "p44lrg_common.hpp"

namespace p44 {

  typedef enum : uint8_t {
    gradient_none = 0,
    // curves
    gradient_curve_mask = 0x07,
    gradient_curve_square = 0x01,
    gradient_curve_lin = 0x02,
    gradient_curve_sin = 0x03,
    gradient_curve_cos = 0x04,
    gradient_curve_log = 0x05,
    gradient_curve_exp = 0x06,
    // invert
    gradient_curve_inverted = 0x08,
    // modes
    gradient_repeat_mask = 0xF0,
    gradient_repeat_none = 0x00,
    gradient_repeat_cyclic = 0x10,
    gradient_repeat_oscillating = 0x20,
    gradient_unlimited = 0x30,
  } GradientMode;


  class ColorEffectView : public P44View
  {
    typedef P44View inherited;

  protected:
    
    typedef std::vector<PixelColor> PixelVector;
    PixelVector mGradientPixels; ///< precalculated gradient pixels

    /// coloring effect parameters (usually some kind of gradient)
    bool mRadial; ///< if set, gradient is applied radially from a center, otherwise along an axis
    double mBriGradient;
    double mHueGradient;
    double mSatGradient;
    GradientMode mBriMode;
    GradientMode mHueMode;
    GradientMode mSatMode;
    bool mTransparentFade; ///< if set, brightness gradient controls alpha, otherwise color itself

    double mEffectCycles; ///< how many overall effect cycles to run within the effect - depends on effect itself what it actually means
    double mEffectZoom; ///< zoom for effect (relative to content size), 1=standard - depends on effect itself what it actually means, negative/Infinite -> unlimited
    bool mEffectWrap; ///< repeat/wrap effect - depends on effect itself what it actually means

  public :

    ColorEffectView();
    virtual ~ColorEffectView();

    static const char* staticTypeName() { return "coloreffect"; };
    static P44View* newInstance() { return new ColorEffectView; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// set coloring parameters
    void setColoringParameters(
      PixelColor aBaseColor,
      double aBri, GradientMode aBriMode,
      double aHue, GradientMode aHueMode,
      double aSat, GradientMode aSatMode,
      bool aRadial
    );

    /// @name trivial property getters/setters
    /// @{
    // - gradients
    double getBriGradient() { return mBriGradient; };
    void setBriGradient(double aVal) { mBriGradient = aVal; flagColorChange(); };
    double getHueGradient() { return mHueGradient; };
    void setHueGradient(double aVal) { mHueGradient = aVal; flagColorChange(); };
    double getSatGradient() { return mSatGradient; };
    void setSatGradient(double aVal) { mSatGradient = aVal; flagColorChange(); };
    // - gradient modes
    GradientMode getBriMode() { return mBriMode; };
    void setBriMode(GradientMode aVal) { mBriMode = aVal; flagColorChange(); };
    GradientMode getHueMode() { return mHueMode; };
    void setHueMode(GradientMode aVal) { mHueMode = aVal; flagColorChange(); };
    GradientMode getSatMode() { return mSatMode; };
    void setSatMode(GradientMode aVal) { mSatMode = aVal; flagColorChange(); };
    // - effect cycles
    double getEffectCycles() { return mEffectCycles; };
    void setEffectCycles(double aVal) { mEffectCycles = aVal; flagColorChange(); };
    // - effect scaling
    double getEffectZoom() { return mEffectZoom; };
    void setEffectZoom(double aVal) { mEffectZoom = aVal; makeDirty(); };
    bool getEffectWrap() { return mEffectWrap; };
    void setEffectWrap(bool aVal) { mEffectWrap = aVal; makeDirty(); };
    // - flags
    bool getRadial() { return mRadial; };
    void setRadial(bool aVal) { mRadial = aVal; makeColorDirty(); };
    bool getTransparentFade() { return mTransparentFade; };
    void setTransparentFade(bool aVal) { mTransparentFade = aVal; makeColorDirty(); };
    /// @}

    /// gradient utilities
    static double gradientCycles(double aValue, GradientMode aMode);
    static double gradientCurveLevel(double aProgress, GradientMode aMode);
    static double gradiated(double aValue, double aProgress, double aGradient, GradientMode aMode, double aMax, bool aWrap);

    void calculateGradient(int aNumGradientPixels);

    #if ENABLE_VIEWCONFIG

    /// convert from text to gradient mode
    static GradientMode textToGradientMode(const char *aGradientText);

    #if !ENABLE_P44SCRIPT
    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

    #endif

    #if ENABLE_VIEWSTATUS

    /// convert from gradient mode to text
    static string gradientModeToText(GradientMode aMode);

    #if !ENABLE_P44SCRIPT
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif

    #endif // ENABLE_VIEWSTATUS

    #if ENABLE_ANIMATION

    /// get a value animation setter for a given property of the view
    /// @param aProperty the name of the property to get a setter for
    /// @param aCurrentValue is assigned the current value of the property
    /// @return the setter to be used by the animator
    virtual ValueSetterCB getPropertySetter(const string aProperty, double& aCurrentValue) P44_OVERRIDE;

    #if ENABLE_P44SCRIPT
    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;
    #endif

  private:

    // generic coloring param (e.g. gradient) setter
    void coloringPropertySetter(double &aColoringParam, double aNewValue);

    #endif // ENABLE_ANIMATION

  protected:

    /// @param aPixelIndex the index into the gradient. Any index can be passed, indices>numGradientPixels
    ///   are wrapped or last pixel repeated
    /// @param aWrap if set, indices outside the available number of gradient pixels will wrap around
    ///   if not set, all indices outside the available number of gradient pixels return the last pixel
    /// @return the pixel as specified by aPixelIndex if in range, otherwise first or last gradient pixel, resp
    PixelColor gradientPixel(int aPixelIndex, bool aWrap);

  };
  typedef boost::intrusive_ptr<ColorEffectView> ColorEffectViewPtr;


  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a ColorEffectView
    class ColorEffectViewObj : public P44lrgViewObj
    {
      typedef P44lrgViewObj inherited;
    public:
      ColorEffectViewObj(P44ViewPtr aView);
      ColorEffectViewPtr colorEffect() { return boost::static_pointer_cast<ColorEffectView>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT


} // namespace p44


#endif /* __p44lrgraphics_coloreffectview_hpp__ */
