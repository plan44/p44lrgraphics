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

#ifndef _p44lrgraphics_epxview_hpp__
#define _p44lrgraphics_epxview_hpp__

#include "p44lrg_common.hpp"
#include "canvasview.hpp"

#if ENABLE_EPX_SUPPORT

namespace p44 {

  class EpxView : public CanvasView
  {
    typedef CanvasView inherited;

    PixelColor* paletteBuffer;
    size_t numPaletteEntries;

    string framesData;
    size_t frameCount;

    int loopCount; ///< number of loops per animation run, 0 = forever
    MLMicroSeconds frameInterval; ///< interval between frames (1/framerate)

    MLMicroSeconds nextRender; ///< time when next rendering should occur, Infinite when stopped
    size_t framesCursor; ///< scanning position within framesData
    int remainingLoops;

  public :

    EpxView();

    virtual ~EpxView();

    /// clear the content
    virtual void clear() P44_OVERRIDE;

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil) P44_OVERRIDE;

    /// load epx data
    ErrorPtr loadEpxAnimationJSON(JsonObjectPtr aEpxAnimation);

    void start();
    void stop();

    #if ENABLE_VIEWCONFIG
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

  private:

    void clearData();
    MLMicroSeconds renderFrame();
    void resetAnimation();
    void setPalettePixel(uint8_t aPaletteIndex, PixelCoord aPixelIndex);

  };
  typedef boost::intrusive_ptr<EpxView> EpxViewPtr;


} // namespace p44

#endif // ENABLE_EPX_SUPPORT

#endif /* _p44lrgraphics_epxview_hpp__ */
