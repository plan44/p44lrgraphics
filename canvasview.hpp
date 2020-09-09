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

#ifndef _p44lrgraphics_canvasview_hpp__
#define _p44lrgraphics_canvasview_hpp__

#include "p44lrg_common.hpp"


namespace p44 {

  class CanvasView : public P44View
  {
    typedef P44View inherited;

    PixelColor* canvasBuffer;
    size_t numPixels;

  public :

    CanvasView();

    virtual ~CanvasView();

    /// clear the content
    virtual void clear() P44_OVERRIDE;

    /// set pixel
    void setPixel(PixelColor aColor, PixelCoord aPixelIndex);
    void setPixel(PixelColor aColor, PixelPoint aPixelPoint);

    PixelCoord getNumPixels() { return numPixels; }

  protected:

    /// get content color at aPt
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

    /// geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

  private:

    void clearData();
    void resize();

  };
  typedef boost::intrusive_ptr<CanvasView> CanvasViewPtr;


} // namespace p44

#endif /* _p44lrgraphics_canvasview_hpp__ */
