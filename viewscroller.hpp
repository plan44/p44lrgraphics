//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2018-2025 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#ifndef _p44lrgraphics_viewscroller_hpp__
#define _p44lrgraphics_viewscroller_hpp__

#include "p44lrg_common.hpp"

#if P44SCRIPT_FULL_SUPPORT
  #include "p44script.hpp"
#endif

namespace p44 {

  /// Content reload handler
  /// @return true if scrolling should continue
  typedef boost::function<bool ()> NeedContentCB;

  /// Smooth scrolling (subpixel resolution)
  class ViewScroller :
    public P44View
    #if P44SCRIPT_FULL_SUPPORT
    , public P44Script::EventSource
    #endif
  {
    typedef P44View inherited;

  private:

    P44ViewPtr mScrolledView;

    double mScrollStepX; ///< X distance to scroll per scrollStepInterval
    double mScrollStepY; ///< Y distance to scroll per scrollStepInterval
    long mScrollSteps; ///< >0: number of remaining scroll steps, <0 = scroll forever, 0=scroll stopped
    bool mSyncScroll; ///< if set, scroll timing has absolute priority (and multi-step jumps can occur to catch up delays)
    MLMicroSeconds mScrollStepInterval; ///< interval between scroll steps
    MLMicroSeconds mNextScrollStepAt; ///< exact time when next step should occur
    bool mHalted; ///< set when halted (due to being hidden
    SimpleCB mScrollCompletedCB; ///< called when one scroll is done
    NeedContentCB mNeedContentCB; ///< called when we need new scroll content
    bool mAutoPurge; ///< when set, and needContentCB is set, will try to purge completely scrolled off views
    #if P44SCRIPT_FULL_SUPPORT
    bool mAlertEmpty; ///< alert scroller getting empty as event
    #endif

  public:

    /// get content pixel color
    /// @param aPt content coordinate
    /// @note aPt is NOT guaranteed to be within actual content as defined by contentSize
    ///   implementation must check this!
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

    /// create view
    ViewScroller();
    virtual ~ViewScroller();

    static const char* staticTypeName() { return "scroller"; };
    static P44View* newInstance() { return new ViewScroller; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// set the to be scrolled view
    /// @param aScrolledView the view of which a part should be shown in this view.
    void setScrolledView(P44ViewPtr aScrolledView);

    /// @return the view being scrolled
    P44ViewPtr getScrolledView() { return mScrolledView; }

    /// @name trivial property getters/setters
    /// @{
    bool getSyncScroll() { return mSyncScroll; };
    void setSyncScroll(bool aVal) { mSyncScroll = aVal; };
    bool getAutoPurge() { return mAutoPurge; };
    void setAutoPurge(bool aVal) { mAutoPurge = aVal; };
    double getStepX() const { return mScrollStepX; }
    void setStepX(double aVal) { mScrollStepX = aVal; }
    double getStepY() const { return mScrollStepY; }
    void setStepY(double aVal) { mScrollStepY = aVal; }
    double getScrollStepIntervalS() const { return (double)mScrollStepInterval/Second; }
    void setScrollStepIntervalS(double aVal) { mScrollStepInterval = aVal*Second; }

    /// @}

    /// start scrolling
    /// @param aStepX scroll step in X direction
    /// @param aStepY scroll step in Y direction
    /// @param aInterval interval between scrolling steps
    /// @param aRoundOffsets if set (default), current scroll offsets will be rounded to next aStepX,Y boundary first
    /// @param aNumSteps number of scroll steps, <0 = forever (until stopScroll() is called)
    /// @param aStartTime time of first step in MainLoop::now() timescale. If ==Never, then now() is used
    /// @param aCompletedCB called when scroll ends because aNumSteps have been executed (but not when aborted via stopScroll())
    /// @note MainLoop::now() time is monotonic (CLOCK_MONOTONIC under Linux, but is adjusted by adjtime() from NTP
    ///   so it should remain in sync over multiple devices as long as these have NTP synced time.
    /// @note MainLoop::now() time is not absolute, but has a unspecified starting point.
    ///   Use MainLoop::unixTimeToMainLoopTime() to convert a absolute starting point into now() time.
    void startScroll(double aStepX, double aStepY, MLMicroSeconds aInterval, bool aRoundOffsets = true, long aNumSteps = -1, MLMicroSeconds aStartTime = Never, SimpleCB aCompletedCB = NoOP);

    /// stop scrolling
    /// @note completed callback will not be called
    void stopScroll();

    /// @return the remaining scroll steps
    long getRemainingSteps() const { return mScrollSteps; }

    /// set handler to be called when new content is needed (current scrolledView does no longer provide content for scrolling further)
    /// @param aNeedContentCB handler to be called when content has scrolled so far that it no longer fills the frame
    /// @param aAutoPurge if set (default), subviews of scrolled view (if it is a ViewStack) that are no longer in the frame will be purged
    ///   every time after aNeedContentCB is called.
    /// @note when the scrolled view does not run out of content, no auto-purging will happen. So make sure to call purgeScrolledOut()
    ///   once in a while when content is added other than via the aNeedContentCB callback.
    void setNeedContentHandler(NeedContentCB aNeedContentCB, bool aAutoPurge = true) { mNeedContentCB = aNeedContentCB; mAutoPurge = aAutoPurge; };

    /// purge unneeded (scrolled off) views in content view (if it is a ViewStack)
    void purgeScrolledOut();

    /// @return number of pixels in either direction to scroll until current content is exhausted.
    ///   x/y are set to INT_MAX to signal "infinite" (if scrolled view's framing mode is set in the scroll direction)
    PixelPoint remainingPixelsToScroll();

    /// @return true if remaining pixels in either direction are <0
    bool needsContent();

    void setAlertEmpty(bool aAlertEmpty) { mAlertEmpty = aAlertEmpty; };

    /// @return time until scroll will need more content
    MLMicroSeconds remainingScrollTime();

    /// clear contents of this view
    virtual void clear() P44_OVERRIDE;

    /// calculate changes on the display for a given time, return time of when the NEXT step should be shown
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @return Infinite if there is no immediate need to call step again, otherwise time of when we need to SHOW
    ///   the next result, i.e. should have called step() and display rendering already.
    /// @note all stepping calculations will be exclusively based on aStepShowTime, and never real time, so
    ///   we can calculate results in advance
    virtual MLMicroSeconds stepInternal(MLMicroSeconds aPriorityUntil) P44_OVERRIDE;

    /// called when alpha changes
    virtual void makeAlphaDirtry() P44_OVERRIDE;

    /// return if anything changed on this view or on subviews in the hierarchy
    virtual bool isDirty() P44_OVERRIDE;

    /// call when display is updated
    virtual void updated() P44_OVERRIDE;

    /// my own geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG

    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;

    /// get view by label
    /// @param aLabel label of view to find
    /// @return NULL if not found, labelled view otherwise (first one with that label found in case >1 have the same label)
    virtual P44ViewPtr findView(const string aLabel) P44_OVERRIDE;

    #endif // ENABLE_VIEWCONFIG

    #if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif // ENABLE_VIEWSTATUS

    #if ENABLE_P44SCRIPT
    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;
    #endif

  };
  typedef boost::intrusive_ptr<ViewScroller> ViewScrollerPtr;


  #if ENABLE_P44SCRIPT

  namespace P44Script {

    #if P44SCRIPT_FULL_SUPPORT

    /// represents scroller out-of-content event/flag
    class ContentNeededObj : public NumericValue
    {
      typedef NumericValue inherited;
      ViewScrollerPtr mScroller;
    public:
      ContentNeededObj(ViewScrollerPtr aScroller);
      virtual void deactivate() P44_OVERRIDE;
      virtual string getAnnotation() const P44_OVERRIDE;
      virtual TypeInfo getTypeInfo() const P44_OVERRIDE;
      virtual bool isEventSource() const P44_OVERRIDE;
      virtual void registerForFilteredEvents(EventSink* aEventSink, intptr_t aRegId = 0) P44_OVERRIDE;
      virtual double doubleValue() const P44_OVERRIDE;
    };

    #endif // P44SCRIPT_FULL_SUPPORT

    /// represents a ViewScroller
    class ScrollerViewObj : public P44lrgViewObj
    {
      typedef P44lrgViewObj inherited;
    public:
      ScrollerViewObj(P44ViewPtr aView);
      ViewScrollerPtr scroller() { return boost::static_pointer_cast<ViewScroller>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44



#endif /* _p44lrgraphics_viewscroller_hpp__ */
