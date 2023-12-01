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

#ifndef _p44lrgraphics_torchview_hpp__
#define _p44lrgraphics_torchview_hpp__

#include "p44lrg_common.hpp"
#include "coloreffectview.hpp"

namespace p44 {

  class TorchView : public ColorEffectView
  {
    typedef ColorEffectView inherited;

    // flame parameters
    uint8_t mFlameMin; ///< 0..255, minimal flame pixel energy
    uint8_t mFlameMax; ///< 0..255, maximal flame pixel energy
    uint8_t mFlameHeight; ///< number of flame (random fire) rows at the bottom
    // spark generation parameters
    uint8_t mSparkProbability; // 0..100, probability for generating a spark at all
    uint8_t mSparkMin; ///< 0..255, minimal initial spark energy
    uint8_t mSparkMax; ///< 0..255, maximal initial spark energy
    // spark development parameters
    uint8_t mSparkTfr; ///< 0..256 how much energy is transferred up for a spark per cycle
    uint16_t mSparkCap; ///< 0..255: how much energy is retained from previous cycle
    uint16_t mUpRad; ///< up radiation
    uint16_t mSideRad; ///< sidewards radiation
    uint16_t mHeatCap; ///< 0..255: passive cells: how much energy is retained from previous cycle
    // hot spark coloring parameters
    uint8_t mHotsparkMin; ///< 0..255 energy level for showing a spark in hotsparkColor
    PixelColor mHotsparkColor; ///< color of lowest temp extra hot spark
    PixelColor mHotsparkColorInc; ///< color increment for highest temp extra hot spark
    // @note main color for flame and spark pixels that is scaled by energy is foregroundColor
    // timing parameter
    MLMicroSeconds mCycleTime; ///< spark update cycle time

    MLMicroSeconds mNextCalculation; ///< next time animation must be calculated

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

    TorchDotVector mTorchDots;

  public :

    TorchView();
    virtual ~TorchView();

    static const char* staticTypeName() { return "torch"; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// clear
    virtual void clear() P44_OVERRIDE;

    /// @name trivial property getters/setters
    /// @{
    // params
    int getFlameMin() { return mFlameMin; }
    void setFlameMin(int aVal) { mFlameMin = aVal; makeDirty(); }
    int getFlameMax() { return mFlameMax; }
    void setFlameMax(int aVal) { mFlameMax = aVal; makeDirty(); }
    int getFlameHeight() { return mFlameHeight; }
    void setFlameHeight(int aVal) { mFlameHeight = aVal; makeDirty(); }
    int getSparkProbability() { return mSparkProbability; }
    void setSparkProbability(int aVal) { mSparkProbability = aVal; makeDirty(); }
    int getSparkMin() { return mSparkMin; }
    void setSparkMin(int aVal) { mSparkMin = aVal; makeDirty(); }
    int getSparkMax() { return mSparkMax; }
    void setSparkMax(int aVal) { mSparkMax = aVal; makeDirty(); }
    int getSparkTfr() { return mSparkTfr; }
    void setSparkTfr(int aVal) { mSparkTfr = aVal; makeDirty(); }
    int getSparkCap() { return mSparkCap; }
    void setSparkCap(int aVal) { mSparkCap = aVal; makeDirty(); }
    int getUpRad() { return mUpRad; }
    void setUpRad(int aVal) { mUpRad = aVal; makeDirty(); }
    int getSideRad() { return mSideRad; }
    void setSideRad(int aVal) { mSideRad = aVal; makeDirty(); }
    int getHeatCap() { return mHeatCap; }
    void setHeatCap(int aVal) { mHeatCap = aVal; makeDirty(); }
    int getHotsparkMin() { return mHotsparkMin; }
    void setHotsparkMin(int aVal) { mHotsparkMin = aVal; makeDirty(); }
    // colors as text
    string getHotsparkColor() { return pixelToWebColor(mHotsparkColor, true); };
    void setHotsparkColor(string aVal) { mHotsparkColor = webColorToPixel(aVal); makeDirty(); };
    string getHotsparkColorInc() { return pixelToWebColor(mHotsparkColorInc, true); };
    void setHotsparkColorInc(string aVal) { mHotsparkColorInc = webColorToPixel(aVal); makeDirty(); };
    // timing
    double getCycleTimeS() const { return (double)mCycleTime/Second; }
    void setCycleTimeS(double aVal) { mCycleTime = aVal*Second; }
    /// @}

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @param aNow referece time for "now" of this step cycle (slightly in the past because taken before calling)
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow) P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT
    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
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

  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a TorchView
    class TorchViewObj : public P44lrgViewObj
    {
      typedef P44lrgViewObj inherited;
    public:
      TorchViewObj(P44ViewPtr aView);
      TorchViewPtr torch() { return boost::static_pointer_cast<TorchView>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44



#endif /* _p44lrgraphics_torchview_hpp__ */
