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

  /// EpxView implements a player for Microsoft Research's "expressive pixel" IoT animation format
  /// See https://www.microsoft.com/en-us/research/project/microsoft-expressive-pixels/ for general information,
  /// see https://github.com/microsoft/ExpressivePixels/wiki/Animation-Format for description of the
  /// animation format, for which EpxView contains a independently developed decoder/player
  class EpxView : public CanvasView
  {
    typedef CanvasView inherited;

    PixelColor* mPaletteBuffer;
    size_t mNumPaletteEntries;

    string mFramesData;
    size_t mFrameCount;

    int mLoopCount; ///< number of loops per animation run, 0 = forever
    MLMicroSeconds mFrameInterval; ///< interval between frames (1/framerate)

    MLMicroSeconds mNextRender; ///< time when next rendering should occur, Infinite when stopped
    size_t mFramesCursor; ///< scanning position within framesData
    int mRemainingLoops;

  public :

    EpxView();
    virtual ~EpxView();

    static const char* staticTypeName() { return "epx"; };
    virtual const char* viewTypeName() P44_OVERRIDE { return staticTypeName(); }

    /// clear the content
    virtual void clear() P44_OVERRIDE;

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @param aNow referece time for "now" of this step cycle (slightly in the past because taken before calling)
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow) P44_OVERRIDE;

    /// load epx data
    ErrorPtr loadEpxAnimationJSON(JsonObjectPtr aEpxAnimation);

    void start();
    void stop();

    #if ENABLE_VIEWCONFIG
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

    #if ENABLE_VIEWSTATUS
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif // ENABLE_VIEWSTATUS

  private:

    void clearData();
    MLMicroSeconds renderFrame(MLMicroSeconds aNow);
    void resetAnimation();
    void setPalettePixel(uint8_t aPaletteIndex, PixelCoord aPixelIndex);

  };
  typedef boost::intrusive_ptr<EpxView> EpxViewPtr;


} // namespace p44

#endif // ENABLE_EPX_SUPPORT

#endif /* _p44lrgraphics_epxview_hpp__ */
