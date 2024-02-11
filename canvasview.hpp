//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2020-2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
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
#include "coloreffectview.hpp"


namespace p44 {

  class CanvasView : public ColorEffectView
  {
    typedef ColorEffectView inherited;

    PixelColor* mCanvasBuffer;
    size_t mNumPixels;

  public :

    CanvasView();
    virtual ~CanvasView();

    static const char* staticTypeName() { return "canvas"; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// clear the content to all pixels transparent
    virtual void clear() P44_OVERRIDE;

    /// set pixel
    void setPixel(PixelColor aColor, PixelCoord aPixelIndex);
    void setPixel(PixelColor aColor, PixelPoint aPixelPoint);

    /// draw line in foreground color (using gradient if enabled)
    void drawLine(PixelPoint aStart, PixelPoint aEnd);

    /// copy pixels in specified rectangle
    /// @param aSourceView if set to nullptr, copying is from myself. Otherwise, pixels are copied from specified view
    /// @param aFromContent if set, pixels are copied from content, otherwise from frame (including scroll/rotate/zoom transforms)
    /// @param aSrcRect where from to copy the pixels (in aSourceView coordinates)
    /// @param aDestOrigin where to copy the pixels to (bottom left)
    /// @param aNonTransparentOnly if set, only non-fully-transparent pixels are copied
    void copyPixels(P44ViewPtr aSourceView, bool aFromContent, PixelRect aSrcRect, PixelPoint aDestOrigin, bool aNonTransparentOnly);

    /// color effect params have changed
    virtual void recalculateColoring() P44_OVERRIDE;

    /// @name trivial property getters/setters
    /// @{
    size_t getNumPixels() { return mNumPixels; }
    size_t getCanvasBytes() { return mNumPixels*sizeof(PixelColor); }
    /// @}

    #if ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif // ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT

    #if ENABLE_P44SCRIPT
    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;
    #endif

    /// get content color at aPt
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

  protected:

    /// geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

  private:

    void clearData();
    void resize();

  };
  typedef boost::intrusive_ptr<CanvasView> CanvasViewPtr;


  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a CanvasView
    class CanvasViewObj : public ColorEffectViewObj
    {
      typedef ColorEffectViewObj inherited;
    public:
      CanvasViewObj(P44ViewPtr aView);
      CanvasViewPtr canvas() { return boost::static_pointer_cast<CanvasView>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44

#endif /* _p44lrgraphics_canvasview_hpp__ */
