//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2016-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#ifndef _p44lrgraphics_viewstack_hpp__
#define _p44lrgraphics_viewstack_hpp__

#include "p44lrg_common.hpp"

namespace p44 {

  class ViewStack : public P44View
  {
    typedef P44View inherited;

    typedef std::list<P44ViewPtr> ViewsList;

    ViewsList mViewStack;
    FramingMode mPositioningMode; ///< mode for positioning views with pushView, purging with purgeView, and autoresizing on child changes

  public :

    /// create view stack
    ViewStack();
    virtual ~ViewStack();

    static const char* staticTypeName() { return "stack"; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// set the positioning, purging and autosizing mode
    /// @param aPositioningMode where to append or purge views. Using FramingMode constants as follows:
    /// - for pushView: appendRight means appending in positive X direction, appendLeft means in negative X direction, etc.
    /// - for pushView, purgeView and change of child view sizes: any clip bits set (noAdjust) means *NO* automatic change of content bounds
    /// - for purgeViews: repeatXmax means measuring new size from max X coordinate in negative X direction,
    ///   repeatXmin means from min X coordinate in positive X direction, etc.
    void setPositioningMode(FramingMode aPositioningMode) { mPositioningMode = aPositioningMode; }

    /// get the current positioning mode as set by setPositioningMode()
    FramingMode getPositioningMode() { return mPositioningMode; }

    /// push view onto top of stack
    /// @param aView the view to push in front of all other views
    /// @param aSpacing extra pixels between appended views
    /// @param aFullFrame if set, content area is auto-adjusted after pushing (because depending on positioning mode,
    ///   the frame might have changed)
    void pushView(P44ViewPtr aView, int aSpacing = 0, bool aFullFrame = false);

    /// purge views that are outside the specified content size in the specified direction
    /// @param aKeepDx keep views with frame completely or partially within this new size (measured according to positioning mode)
    /// @param aKeepDy keep views with frame completely or partially within this new size (measured according to positioning mode)
    /// @param aCompletely if set, keep only views which are completely within the new size
    /// @note views will be purged bottom-most first
    /// @note the last view remaining in the stack will always be kept even when it is out of the keepDx/y range
    void purgeViews(int aKeepDx, int aKeepDy, bool aCompletely);

    /// remove topmost view
    void popView();

    /// @param aSubView parent view or NULL if none
    /// @return true if subview could be added
    virtual bool addSubView(P44ViewPtr aSubView) P44_OVERRIDE;

    /// remove specific view
    /// @param aView the view to remove from the stack
    /// @return true if view actually was a subview and was removed
    virtual bool removeView(P44ViewPtr aView) P44_OVERRIDE;

    /// offset all subviews
    /// @param aOffset will be added to all subview frame's x/y
    void offsetSubviews(PixelPoint aOffset);

    /// @return number of views in the stack
    size_t numViews() { return mViewStack.size(); }

    /// @param aBounds will be set to rect (in content coords) enclosing all subviews
    void getEnclosingContentRect(PixelRect &aBounds);

    /// clear stack, means remove all views
    virtual void clear() P44_OVERRIDE;

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @param aNow referece time for "now" of this step cycle (slightly in the past because taken before calling)
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow) P44_OVERRIDE;

    /// return if anything changed on the display since last call
    virtual bool isDirty() P44_OVERRIDE;

    /// call when display is updated
    virtual void updated() P44_OVERRIDE;

    /// child view has changed geometry (frame, content rect)
    virtual void childGeometryChanged(P44ViewPtr aChildView, PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

    /// my own geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG

    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;

    /// find view by label or id
    /// @param aLabel label of view to find
    /// @return NULL if not found, labelled view otherwise (first one with that label found in case >1 have the same label)
    virtual P44ViewPtr findView(const string aLabel) P44_OVERRIDE;

    /// find layer
    /// @param aLabel label of layer to find
    /// @return NULL if not found, labelled view otherwise (first one with that label found in case >1 have the same label)
    P44ViewPtr findLayer(const string aLabel, bool aRecursive);


    #endif

    #if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif // ENABLE_VIEWSTATUS

    #if ENABLE_P44SCRIPT

    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;

    /// @return ScriptObj representing all stack layers
    P44Script::ScriptObjPtr layersList();

    #endif

  protected:

    /// get content pixel color
    /// @param aPt content coordinate
    /// @note aPt is NOT guaranteed to be within actual content as defined by contentSize
    ///   implementation must check this!
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

  private:

    void recalculateContentArea();
    void sortZOrder();

  };
  typedef boost::intrusive_ptr<ViewStack> ViewStackPtr;

  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a ColorEffectView
    class ViewStackObj : public P44lrgViewObj
    {
      typedef P44lrgViewObj inherited;
    public:
      ViewStackObj(P44ViewPtr aView);
      ViewStackPtr stack() { return boost::static_pointer_cast<ViewStack>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44


#endif /* _p44lrgraphics_viewstack_hpp__ */
