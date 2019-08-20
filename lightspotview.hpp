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

#ifndef __p44lrgraphics_lightspotview_hpp__
#define __p44lrgraphics_lightspotview_hpp__

#include "p44view.hpp"


namespace p44 {

  typedef enum {
    gradient_none = 0,
    // curves
    gradient_curve_mask = 0x0F,
    gradient_curve_square = 0x01,
    gradient_curve_lin = 0x02,
    gradient_curve_log = 0x03,
    gradient_curve_sin = 0x04,
    // modes
    gradient_repeat_mask = 0xF0,
    gradient_repeat_none = 0x00,
    gradient_repeat_cyclic = 0x10,
    gradient_repeat_oscillating = 0x20,
    gradient_unlimited = 0x30,
  } GradientModeEnum;
  typedef uint8_t GradientMode;


  class LightSpotView : public P44View
  {
    typedef P44View inherited;

    /// parameters
    PixelCoord center; ///< center coordinate
    PixelCoord extent; ///< extent of main light field, depends on effects how far it actually goes
    double rotation; ///< rotation of main light field in degree CCW

    /// derived values
    double rotSin;
    double rotCos;

    /// gradients
    bool radial; ///< if set, gradient is applied radially from the center, otherwise along the x axis
    double briGradient;
    double hueGradient;
    double satGradient;
    GradientMode briMode;
    GradientMode hueMode;
    GradientMode satMode;

    typedef std::vector<PixelColor> PixelVector;
    PixelVector gradientPixels; ///< precalculated gradient pixels

  public :

    LightSpotView();
    virtual ~LightSpotView();


    /// set center + extent combined as the size of the right upper quadrant of the light field
    void setQuadrant(PixelRect aQuadrant);

    /// set center
    /// @param aExtent the center of the light
    void setCenter(PixelCoord aCenter);

    /// set extent (how many pixels the light field reaches out around the center)
    /// @param aExtent the extent radii of the light in x and y direction
    /// @note extent(0,0) means single pixel at the center
    void setExtent(PixelCoord aExtent);

    /// set rotation
    /// @param aRotation rotation in degrees CCW
    void setRotation(double aRotation);

    /// set center color
    void setCenterColor(PixelColor aColor);

    /// set coloring parameters
    /// @note gradient value determines the change of the base value over the extent range
    void setColoringParameters(
      PixelColor aBaseColor,
      double aBri, GradientMode aBriMode,
      double aHue, GradientMode aHueMode,
      double aSat, GradientMode aSatMode,
      bool aRadial
    );

    /// clear and resize
    virtual void clear() P44_OVERRIDE;

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil) P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG

    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;

    #endif

  protected:

    /// get content pixel color
    /// @param aPt content coordinate
    /// @note aPt is NOT guaranteed to be within actual content as defined by contentSize
    ///   implementation must check this!
    virtual PixelColor contentColorAt(PixelCoord aPt) P44_OVERRIDE;

  private:

    void recalculateContentRect();
    void recalculateGradients();

  };
  typedef boost::intrusive_ptr<LightSpotView> LightSpotViewPtr;



} // namespace p44


#endif /* __p44lrgraphics_lightspotview_hpp__ */
