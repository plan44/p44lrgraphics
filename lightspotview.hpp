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

    static const char* staticTypeName() { return "lightspot"; };
    virtual const char* viewTypeName() const P44_OVERRIDE { return staticTypeName(); }

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
