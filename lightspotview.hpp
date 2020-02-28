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

#include "p44lrg_common.hpp"
#include "coloreffectview.hpp"


namespace p44 {


  class LightSpotView : public ColorEffectView
  {
    typedef ColorEffectView inherited;

  public :

    LightSpotView();
    virtual ~LightSpotView();

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    //virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil) P44_OVERRIDE;

    /// set content origin X relative to its own size and frame
    /// @param aRelX relative X position, 0 = center, -1 = max(framedx,contentdx) to the left, +1 to the right
    virtual void setRelativeContentOriginX(double aRelX) P44_OVERRIDE;
    /// set content origin Y relative to its own size and frame
    /// @param aRelY relative Y position, 0 = center, -1 = max(framedy,contentdy) down, +1 up
    virtual void setRelativeContentOriginY(double aRelY) P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG

    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;

    #endif // ENABLE_VIEWCONFIG

    #if ENABLE_ANIMATION

    /// get a value animation setter for a given property of the view
    /// @param aProperty the name of the property to get a setter for
    /// @param aCurrentValue is assigned the current value of the property
    /// @return the setter to be used by the animator
    virtual ValueSetterCB getPropertySetter(const string aProperty, double& aCurrentValue) P44_OVERRIDE;

    #endif // ENABLE_ANIMATION

  protected:

    /// get content pixel color
    /// @param aPt content coordinate
    /// @note aPt is NOT guaranteed to be within actual content as defined by contentSize
    ///   implementation must check this!
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

    /// color effect params have changed
    virtual void recalculateColoring() P44_OVERRIDE;

    /// geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

  private:

    void recalculateContentRect();

  };
  typedef boost::intrusive_ptr<LightSpotView> LightSpotViewPtr;



} // namespace p44


#endif /* __p44lrgraphics_lightspotview_hpp__ */
