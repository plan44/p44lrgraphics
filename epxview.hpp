//  SPDX-License-Identifier: GPL-3.0-or-later
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
    static P44View* newInstance() { return new EpxView; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// clear the content
    virtual void clear() P44_OVERRIDE;

    /// calculate changes on the display for a given time, return time of when the NEXT step should be shown
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @return Infinite if there is no immediate need to call step again, otherwise time of when we need to SHOW
    ///   the next result, i.e. should have called step() and display rendering already.
    /// @note all stepping calculations will be exclusively based on aStepShowTime, and never real time, so
    ///   we can calculate results in advance
    virtual MLMicroSeconds stepInternal(MLMicroSeconds aPriorityUntil) P44_OVERRIDE;

    /// load epx data
    ErrorPtr loadEpxAnimationJSON(JsonObjectPtr aEpxAnimation);

    void start();
    void stop();

    /// @name trivial property getters/setters
    /// @{
    bool getRun() { return mNextRender>0; }
    void setRun(bool aVal) { if (aVal!=getRun()) { if (aVal) start(); else stop(); } }
    /// @}

    #if ENABLE_VIEWCONFIG
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

    #if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif // ENABLE_VIEWSTATUS

    #if ENABLE_P44SCRIPT
    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;
    #endif

  private:

    void clearData();
    MLMicroSeconds renderFrame(MLMicroSeconds aNow);
    void resetAnimation();
    void setPalettePixel(uint8_t aPaletteIndex, PixelCoord aPixelIndex);

  };
  typedef boost::intrusive_ptr<EpxView> EpxViewPtr;

  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a EpxView, but is also a Canvas
    class EpxViewObj : public CanvasViewObj
    {
      typedef CanvasViewObj inherited;
    public:
      EpxViewObj(P44ViewPtr aView);
      EpxViewPtr epx() { return boost::static_pointer_cast<EpxView>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44

#endif // ENABLE_EPX_SUPPORT

#endif /* _p44lrgraphics_epxview_hpp__ */
