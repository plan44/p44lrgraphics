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

#ifndef _p44lrgraphics_viewsequencer_hpp__
#define _p44lrgraphics_viewsequencer_hpp__

#include "p44lrg_common.hpp"

namespace p44 {


  class AnimationStep
  {
  public:
    P44ViewPtr mView;
    MLMicroSeconds mFadeInTime;
    MLMicroSeconds mShowTime;
    MLMicroSeconds mFadeOutTime;
  };


  
  class ViewSequencer : public P44View
  {
    typedef P44View inherited;

    typedef std::vector<AnimationStep> SequenceVector;

    SequenceVector mSequence; ///< sequence
    bool mRepeating; ///< set if current animation is repeating
    int mCurrentStep; ///< current step in running animation
    SimpleCB mCompletedCB; ///< called when one animation run is done
    P44ViewPtr mCurrentView; ///< current view

    enum {
      as_begin,
      as_show,
      as_fadeout
    } animationState;
    MLMicroSeconds mLastStateChange;

  public :

    /// create view stack
    ViewSequencer();
    virtual ~ViewSequencer();

    static const char* staticTypeName() { return "sequencer"; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// add animation step view to list of animation steps
    /// @param aView the view to add
    void pushStep(P44ViewPtr aView, MLMicroSeconds aShowTime, MLMicroSeconds aFadeInTime=0, MLMicroSeconds aFadeOutTime=0);

    /// start animating the sequence
    /// @param aRepeat if set, animation will repeat
    /// @param aCompletedCB called when animation sequence ends (if repeating, it is called multiple times)
    void startSequence(bool aRepeat, SimpleCB aCompletedCB = NoOP);

    /// stop the sequence
    /// @note: this does not stop the animations of the views in the steps, in particular
    ///    currently running fade-in or fade out animations will run to completion.
    ///    To stop everything at once, use stopAnimations().
    void stopSequence();

    /// stop animation
    /// @note completed callback will not be called
    virtual void stopAnimations() P44_OVERRIDE;

    /// clear all steps
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

    #endif

    #if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif // ENABLE_VIEWSTATUS

    #if ENABLE_P44SCRIPT

    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;

    /// @return ScriptObj representing all sequence steps
    P44Script::ScriptObjPtr stepsList();

    #endif


  protected:

    /// get content pixel color
    /// @param aPt content coordinate
    /// @note aPt is NOT guaranteed to be within actual content as defined by contentSize
    ///   implementation must check this!
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

  private:

    MLMicroSeconds stepAnimation(MLMicroSeconds aNow);

  };
  typedef boost::intrusive_ptr<ViewSequencer> ViewSequencerPtr;

  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a ColorEffectView
    class ViewSequencerObj : public P44lrgViewObj
    {
      typedef P44lrgViewObj inherited;
    public:
      ViewSequencerObj(P44ViewPtr aView);
      ViewSequencerPtr sequencer() { return boost::static_pointer_cast<ViewSequencer>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44



#endif /* _p44lrgraphics_viewsequencer_hpp__ */
