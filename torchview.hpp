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

#ifndef _p44lrgraphics_torchview_hpp__
#define _p44lrgraphics_torchview_hpp__

#include "p44lrg_common.hpp"
#include "coloreffectview.hpp"

namespace p44 {

  class TorchView : public ColorEffectView
  {
    typedef ColorEffectView inherited;

    // flame parameters
    uint8_t flame_min; ///< 0..255, minimal flame pixel energy
    uint8_t flame_max; ///< 0..255, maximal flame pixel energy
    uint8_t flame_height; ///< number of flame (random fire) rows at the bottom
    // spark generation parameters
    uint8_t spark_probability; // 0..100, probability for generating a spark at all
    uint8_t spark_min; ///< 0..255, minimal initial spark energy
    uint8_t spark_max; ///< 0..255, maximal initial spark energy
    // spark development parameters
    uint8_t spark_tfr; ///< 0..256 how much energy is transferred up for a spark per cycle
    uint16_t spark_cap; ///< 0..255: how much energy is retained from previous cycle
    uint16_t up_rad; ///< up radiation
    uint16_t side_rad; ///< sidewards radiation
    uint16_t heat_cap; ///< 0..255: passive cells: how much energy is retained from previous cycle
    // hot spark coloring parameters
    uint8_t hotspark_min; ///< 0..255 energy level for showing a spark in hotsparkColor
    PixelColor hotsparkColor; ///< color of lowest temp extra hot spark
    PixelColor hotsparkColorInc; ///< color increment for highest temp extra hot spark
//    PixelColor biasColor; ///< color bias for flame and spark pixels
    // @note main color for flame and spark pixels that is scaled by energy is foregroundColor
    // timing parameter
    MLMicroSeconds cycleTime; ///< spark update cycle time

    MLMicroSeconds nextCalculation; ///< next time animation must be calculated

    typedef enum {
      torch_passive = 0, // just environment, glow from nearby radiation
      torch_nop = 1, // no processing
      torch_spark = 2, // slowly looses energy, moves up
      torch_spark_temp = 3, // a spark still getting energy from the level below
    } EnergyMode;

    typedef struct {
      EnergyMode mode;
      uint8_t current; // calculation output
      uint8_t previous; // calculation input
    } TorchDot;

    typedef std::vector<TorchDot> TorchDotVector;

    TorchDotVector torchDots;

  public :

    TorchView();
    virtual ~TorchView();

    /// clear
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
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

    /// color effect params have changed
    virtual void recalculateColoring() P44_OVERRIDE;

    /// geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

  private:

    TorchDot& dot(PixelPoint aPt);

    void calculateCycle();

  };
  typedef boost::intrusive_ptr<TorchView> TorchViewPtr;



} // namespace p44



#endif /* _p44lrgraphics_torchview_hpp__ */
