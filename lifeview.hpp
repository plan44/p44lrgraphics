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

#ifndef _p44lrgraphics_lifeview_hpp__
#define _p44lrgraphics_lifeview_hpp__

#include "p44lrg_common.hpp"
#include "coloreffectview.hpp"

namespace p44 {

  class LifeView : public ColorEffectView
  {
    typedef ColorEffectView inherited;

    std::vector<uint32_t> mCells; ///< internal representation

    int mDynamics;
    int mPopulation;

    int mStaticcount;
    int mMaxStatic;
    int mMinPopulation;
    int mMinStatic;

    MLMicroSeconds mGenerationInterval;
    MLMicroSeconds mLastGeneration;

    double mHue;
    double mSaturation;
    double mBrightness;

  public :

    LifeView();
    virtual ~LifeView();

    static const char* staticTypeName() { return "life"; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// set generation interval
    /// @param aInterval time between generations, Never = stopped
    virtual void setGenerationInterval(MLMicroSeconds aInterval);

    /// clear and resize to new
    virtual void clear() P44_OVERRIDE;

    /// @name trivial property getters/setters
    /// @{
    double getGenerationIntervalS() { return (double)mGenerationInterval/Second; }
    void setGenerationIntervalS(double aVal) { mGenerationInterval = aVal*Second; }
    int getMaxStatic() { return mMaxStatic; }
    void setMaxStatic(int aVal) { mMaxStatic = aVal; }
    int getMinStatic() { return mMinStatic; }
    void setMinStatic(int aVal) { mMinStatic = aVal; }
    int getMinPopulation() { return mMinPopulation; }
    void setMinPopulation(int aVal) { mMinPopulation = aVal; }
    /// @}


    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @param aNow referece time for "now" of this step cycle (slightly in the past because taken before calling)
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow) P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG
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

    /// randomly create some cells to get (re)started
    /// @param aMinCells minimum number of cells to create
    /// @param aMaxCells maximum number of cells to create
    void createRandomCells(int aMinCells, int aMaxCells);

    /// revive, i.e. call createRandomCells() with appropriate params depending on field size
    void revive();

    /// place one of the predefined patterns
    void placePattern(uint16_t aPatternNo, bool aWrap=true, int aCenterX=-1, int aCenterY=-1, int aOrientation=-1);

  protected:

    /// get content pixel color
    /// @param aPt content coordinate
    /// @note aPt is NOT guaranteed to be within actual content as defined by contentSize
    ///   implementation must check this!
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

    bool prepareCells();
    void recalculateColoring() P44_OVERRIDE;
    void nextGeneration();
    void timeNext();
    int cellindex(int aX, int aY, bool aWrap);
    void calculateGeneration();
  };
  typedef boost::intrusive_ptr<LifeView> LifeViewPtr;

  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a LifeView, but is also a Canvas
    class LifeViewObj : public ColorEffectViewObj
    {
      typedef ColorEffectViewObj inherited;
    public:
      LifeViewObj(P44ViewPtr aView);
      LifeViewPtr life() { return boost::static_pointer_cast<LifeView>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44



#endif /* _p44lrgraphics_lifeview_hpp__ */
