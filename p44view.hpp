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

#ifndef _p44lrgraphics_view_hpp__
#define _p44lrgraphics_view_hpp__

#include "p44lrg_common.hpp"
#include "colorutils.hpp"

#if ENABLE_VIEWCONFIG
  #include "jsonobject.hpp"
  #include "application.hpp"
#endif
#if ENABLE_ANIMATION
  #include "valueanimator.hpp"
#endif
#if P44SCRIPT_FULL_SUPPORT
  #include "p44script.hpp"
#endif

#define DEFAULT_MIN_UPDATE_INTERVAL (15*MilliSecond)

#if ENABLE_P44SCRIPT
  // macros for synthesizing property accessors
  #define ACC_DECL(field, types, prop) \
    { field, builtinmember|types, 0, .memberAccessInfo=(void*)&access_##prop, .accessor=&PROPERTY_ACCESSOR }

  #define ACC_IMPL(prop, getter, constructor) \
    static ScriptObjPtr access_##prop(P44View& aView, ScriptObjPtr aToWrite) \
    { \
      if (!aToWrite) return new constructor(aView.get##prop()); \
      aView.set##prop(aToWrite->getter()); \
      return aToWrite; /* reflect back to indicate writable */ \
    }
  #define ACC_IMPL_RO(prop, constructor) \
    static ScriptObjPtr access_##prop(P44View& aView, ScriptObjPtr aToWrite) \
    { \
      if (!aToWrite) return new constructor(aView.get##prop()); \
      return nullptr; /* null to indicate readonly */ \
    }
  #define ACC_IMPL_STR(prop) ACC_IMPL(prop, stringValue, StringValue)
  #define ACC_IMPL_DBL(prop) ACC_IMPL(prop, doubleValue, NumericValue)
  #define ACC_IMPL_INT(prop) ACC_IMPL(prop, intValue, IntegerValue)
  #define ACC_IMPL_BOOL(prop) ACC_IMPL(prop, boolValue, BoolValue)
  #define ACC_IMPL_RO_STR(prop) ACC_IMPL_RO(prop, StringValue)
  #define ACC_IMPL_RO_DBL(prop) ACC_IMPL_RO(prop, NumericValue)
  #define ACC_IMPL_RO_INT(prop) ACC_IMPL_RO(prop, IntegerValue)
#endif // ENABLE_P44SCRIPT

namespace p44 {

  typedef int PixelCoord;

  typedef struct {
    PixelCoord x,y; ///< origin, can be positive or negative
    PixelCoord dx,dy; ///< size, must always be positive (assumption in all code handling rects)
  } PixelRect;

  typedef struct {
    PixelCoord x,y;
  } PixelPoint;



  const PixelRect zeroRect = { .x=0, .y=0, .dx=0, .dy=0 };


  /// Utilities
  /// @{

  /// @return true if child rect is completely contained in parent rect
  bool rectContainsRect(const PixelRect &aParentRect, const PixelRect &aChildRect);

  /// @return true if rectangles intersect
  bool rectIntersectsRect(const PixelRect &aRect1, const PixelRect &aRect2);

  /// @}

  class P44View;
  typedef boost::intrusive_ptr<P44View> P44ViewPtr;

  class P44View : public P44Obj
  {
    friend class ViewStack;

    bool mDirty;
    bool mUpdateRequested; ///< set when needUpdateCB has been called, reset at step() or update()
    TimerCB mNeedUpdateCB; ///< called when dirty check and calling step must occur earlier than what last step() call said
    MLMicroSeconds mMinUpdateInterval; ///< minimum update interval (as a hint from actual display)

    int mGeometryChanging;
    bool mChangedGeometry;
    PixelRect mPreviousFrame;
    PixelRect mPreviousContent;

    #if ENABLE_ANIMATION
    typedef std::list<ValueAnimatorPtr> AnimationsList;
    AnimationsList mAnimations; /// the list of currently running animations
    #endif


  public:

    // Orientation
    enum {
      // basic transformation flags
      xy_swap = 0x01,
      x_flip = 0x02,
      y_flip = 0x04,
      // directions of X axis
      right = 0, /// untransformed X goes left to right, Y goes up
      down = xy_swap+x_flip, /// X goes down, Y goes right
      left = x_flip+y_flip, /// X goes left, Y goes down
      up = xy_swap+y_flip, /// X goes down, Y goes right
    };
    typedef uint8_t Orientation;

    enum {
      noWrap = 0, /// do not wrap
      wrapXmin = 0x01, /// wrap in X direction for X<frame area
      wrapXmax = 0x02, /// wrap in X direction for X>=frame area
      wrapX = wrapXmin|wrapXmax, /// wrap in both X directions
      wrapYmin = 0x04, /// wrap in Y direction for Y<frame area
      wrapYmax = 0x08, /// wrap in Y direction for Y>=frame area
      wrapY = wrapYmin|wrapYmax, /// wrap in both Y directions
      wrapXY = wrapX|wrapY, /// wrap in all directions
      wrapMask = wrapXY, /// mask for wrap bits
      clipXmin = 0x10, /// clip content left of frame area
      clipXmax = 0x20, /// clip content right of frame area
      clipX = clipXmin|clipXmax, // clip content horizontally
      clipYmin = 0x40, /// clip content below frame area
      clipYmax = 0x80, /// clip content above frame area
      clipY = clipYmin|clipYmax, // clip content vertically
      clipXY = clipX|clipY, // clip content
      clipMask = clipXY, // mask for clip bits
      noAdjust = clipXY, // for positioning: do not adjust content rectangle
      appendLeft = wrapXmin, // for positioning: extend to the left
      appendRight = wrapXmax, // for positioning: extend to the right
      fillX = wrapXmin|wrapXmax, // for positioning: set frame size fill parent in X direction
      appendBottom = wrapYmin, // for positioning: extend towards bottom
      appendTop = wrapYmax, // for positioning: extend towards top
      fillY = wrapYmin|wrapYmax, // for positioning: set frame size fill parent in Y direction
      fillXY = fillX|fillY, // for positioning: set frame size fill parent frame
    };
    typedef uint8_t WrapMode;

    /// announce/finish geometry change
    void geometryChange(bool aStart);

  protected:

    // parent view (pointer only, to avoid retain cycles)
    // Containers must make sure their children's parent pointer gets reset before parent goes away
    P44View* mParentView;

    // outer frame
    PixelRect mFrame;
    bool mSizeToContent; ///< if set, frame is automatically resized with content

    // alpha (opacity)
    PixelColorComponent mAlpha;

    // colors
    PixelColor mBackgroundColor; ///< background color
    PixelColor mForegroundColor; ///< foreground color

    // z-order
    int mZOrder;

    // content
    PixelRect mContent; ///< content offset and size relative to frame (but in content coordinates, i.e. possibly orientation translated!)
    Orientation mContentOrientation; ///< orientation of content in frame
    WrapMode mContentWrapMode; ///< content wrap mode in frame area
    bool mContentIsMask; ///< if set, only alpha of content is used on foreground color
    bool mInvertAlpha; ///< invert alpha provided by content
    bool mLocalTimingPriority; ///< if set, this view's timing requirements should be treated with priority over child view's
    MLMicroSeconds mMaskChildDirtyUntil; ///< if>0, child's dirty must not be reported until this time is reached
    double mContentRotation; ///< rotation of content pixels in degree CCW
    // - derived values
    double mRotSin;
    double mRotCos;

    string mLabel; ///< label of the view for addressing it

    /// change rect and trigger geometry change when actually changed
    void changeGeometryRect(PixelRect &aRect, PixelRect aNewRect);

    /// apply flip operations within frame
    void flipCoordInFrame(PixelPoint &aCoord);

    /// rotate coordinate between frame and content (both ways)
    void orientateCoord(PixelPoint &aCoord);

    /// content rectangle in frame coordinates
    void contentRectAsViewCoord(PixelRect &aRect);

    /// transform frame to content coordinates
    /// @note transforming from frame to content coords is: flipCoordInFrame() -> rotateCoord() -> subtract content.x/y
    void inFrameToContentCoord(PixelPoint &aCoord);

    /// transform content to frame coordinates
    /// @note transforming from content to frame coords is: add content.x/y -> rotateCoord() -> flipCoordInFrame()
    void contentToInFrameCoord(PixelPoint &aCoord);


    /// get content pixel color
    /// @param aPt content coordinate
    /// @note aPt is NOT guaranteed to be within actual content as defined by contentSize
    ///   implementation must check this!
    /// @note this default base class implementation shows the foreground color on all pixels within contentSize, background otherwise
    /// @note subclasses should not call inherited normally, but directly return foreground or background colors (or calculated other colors)
    virtual PixelColor contentColorAt(PixelPoint aPt);

    /// helper for implementations: check if aPt within set content size
    bool isInContentSize(PixelPoint aPt);

    /// color effect params have changed
    virtual void recalculateColoring() { /* NOP in the base class */ };

    /// set dirty - to be called by step() and property setters (config, animation) when the view needs to be redisplayed
    void makeDirty();

    /// set color dirty - make dirty and cause coloring update
    void makeColorDirty();

    /// @return if true, dirty childs should be reported
    bool reportDirtyChilds();

    /// helper for determining time of next step call
    /// @param aNextCall time of next call needed known so far, will be updated by candidate if conditions match
    /// @param aCallCandidate time of next call to update aNextCall
    /// @param aCandidatePriorityUntil if set, this means the aCallCandidate must be prioritized when it is before the
    ///   specified time (and the view is enabled for prioritized timing).
    ///   Prioritizing means that reportDirtyChilds() returns false until aCallCandidate has passed, to
    ///   prevent triggering display updates before the prioritized time.
    /// @param aNow referece time for "now" of this step cycle (slightly in the past because taken before calling)
    ///   Only relevant when aCandidatePriorityUntil!=0, if set to Never, current mainloop time will be used
    void updateNextCall(MLMicroSeconds &aNextCall, MLMicroSeconds aCallCandidate, MLMicroSeconds aCandidatePriorityUntil = 0, MLMicroSeconds aNow = Never);

  public :

    /// create view
    P44View();

    virtual ~P44View();

    static const char* staticTypeName() { return "plain"; };
    
    virtual const char* getTypeName() const { return staticTypeName(); }

    /// set the frame within the parent coordinate system
    /// @param aFrame the new frame for the view
    virtual void setFrame(PixelRect aFrame);

    /// @return current frame rect
    PixelRect getFrame() { return mFrame; };

    /// @name property getter/setter
    /// @{
    // frame coords
    PixelCoord getX() { return mFrame.x; };
    void setX(PixelCoord aVal) { mFrame.x = aVal; makeDirty(); mChangedGeometry = true; };
    PixelCoord getY() { return mFrame.y; };
    void setY(PixelCoord aVal) { mFrame.y = aVal; makeDirty(); mChangedGeometry = true; };
    PixelCoord getDx() { return mFrame.dx; };
    void setDx(PixelCoord aVal) { mFrame.dx = aVal; makeDirty(); mChangedGeometry = true; };
    PixelCoord getDy() { return mFrame.dy; };
    void setDy(PixelCoord aVal) { mFrame.dy = aVal; makeDirty(); mChangedGeometry = true; };
    // content coords
    PixelCoord getContentX() { return mContent.x; };
    void setContentX(PixelCoord aVal) { mContent.x = aVal; makeDirty(); mChangedGeometry = true; };
    PixelCoord getContentY() { return mContent.y; };
    void setContentY(PixelCoord aVal) { mContent.y = aVal; makeDirty(); mChangedGeometry = true; };
    PixelCoord getContentDx() { return mContent.dx; };
    void setContentDx(PixelCoord aVal) { mContent.dx = aVal; makeDirty(); mChangedGeometry = true; };
    PixelCoord getContentDy() { return mContent.dy; };
    void setContentDy(PixelCoord aVal) { mContent.dy = aVal; makeDirty(); mChangedGeometry = true; };
    // colors as text
    string getBgcolor() { return pixelToWebColor(getBackgroundColor(), true); };
    void setBgcolor(string aVal) { setBackgroundColor(webColorToPixel(aVal)); };
    string getColor() { return pixelToWebColor(getForegroundColor(), true); };
    void setColor(string aVal) { setForegroundColor(webColorToPixel(aVal)); };
    // flags
    bool getContentIsMask() { return mContentIsMask; };
    void setContentIsMask(bool aVal) { mContentIsMask = aVal; makeDirty(); };
    bool getInvertAlpha() { return mInvertAlpha; };
    void setInvertAlpha(bool aVal) { mInvertAlpha = aVal; makeDirty(); };
    bool getSizeToContent() { return mSizeToContent; };
    void setSizeToContent(bool aVal) { mSizeToContent = aVal; };
    bool getLocalTimingPriority() { return mLocalTimingPriority; };
    void setLocalTimingPriority(bool aVal) { mLocalTimingPriority = aVal; };
    /// @}

    /// @return current content rect
    PixelRect getContent() { return mContent; };

    /// @param aParentView parent view or NULL if none
    void setParent(P44ViewPtr aParentView);

    /// @return get parent
    /// @note parent views should NOT store this, because it creates a retain cycle
    P44ViewPtr getParent();

    /// @param aSubView parent view or NULL if none
    /// @return true if subview could be added
    /// @note this is a NOP in this baseclass
    virtual bool addSubView(P44ViewPtr aSubView) { return false; }

    /// remove specific subview from this view
    /// @param aView the view to remove from the stack
    /// @return true if view actually was a subview and was removed
    /// @note this is a NOP in this baseclass
    virtual bool removeView(P44ViewPtr aView) { return false; }

    /// remove this view from its parent (if any)
    /// @return true if view actually was removed from a parent view
    bool removeFromParent();

    /// set the view's background color
    /// @param aBackgroundColor color of pixels not covered by content
    void setBackgroundColor(PixelColor aBackgroundColor) { mBackgroundColor = aBackgroundColor; makeColorDirty(); };

    /// @return current background color
    PixelColor getBackgroundColor() { return mBackgroundColor; }

    /// set foreground color
    void setForegroundColor(PixelColor aColor) { mForegroundColor = aColor; makeColorDirty(); }

    /// get foreground color
    PixelColor getForegroundColor() const { return mForegroundColor; }

    /// set content wrap mode
    /// @param aWrapMode the new wrap mode
    void setWrapMode(WrapMode aWrapMode) { mContentWrapMode = aWrapMode; makeDirty(); }

    /// get current wrap mode
    /// @return current wrap mode
    WrapMode getWrapMode() { return mContentWrapMode; }

    /// set view label
    /// @note this is for referencing views in reconfigure operations
    /// @param aLabel the new label
    void setLabel(const string aLabel) { mLabel = aLabel; }

    /// @return the label if one if set, or a generated unique default label otherwise
    string getLabel() const;

    /// @return an unique (but volatile) ID
    string getId() const;

    /// set default view label (i.e. set label only if the view does have an empty label so far)
    /// @param aLabel the new label
    void setDefaultLabel(const string aLabel) { if (mLabel.empty()) mLabel = aLabel; };

    /// set view's alpha
    /// @param aAlpha 0=fully transparent, 255=fully opaque
    void setAlpha(PixelColorComponent aAlpha);

    /// get current alpha
    /// @return current alpha value 0=fully transparent=invisible, 255=fully opaque
    PixelColorComponent getAlpha() { return mAlpha; };

    /// hide (set alpha to 0)
    void hide() { setAlpha(0); };

    /// show (set alpha to max)
    void show() { setAlpha(255); };

    /// set visible (hide or show)
    /// @param aVisible true to show, false to hide
    void setVisible(bool aVisible) { if (aVisible) show(); else hide(); };

    /// Check if something of this view is potentially visible (can still consist of transparent pixels only)
    /// @return true if visible
    bool getVisible() { return getAlpha()!=0; }

    /// @return Z-Order (e.g. in a viewstack, highest is frontmost)
    int getZOrder() { return mZOrder; };

    /// @param aZOrder z-order (e.g. in a viewstack, highest is frontmost)
    void setZOrder(int aZOrder);

    /// @param aOrientation the orientation of the content in the frame
    void setOrientation(Orientation aOrientation) { mContentOrientation = aOrientation; makeDirty(); }

    /// @return the orientation of the content in the frame
    Orientation getOrientation() { return mContentOrientation; }

    /// set content rotation around content origin
    /// @param aRotation content rotation in degrees CCW
    void setContentRotation(double aRotation);

    /// @return content rotation around content origin
    double getContentRotation() { return mContentRotation; }

    /// set content size and offset (relative to frame origin, but in content coordinates, i.e. possibly orientation translated!)
    void setContent(PixelRect aContent);

    /// set content size (without changing offset)
    void setContentSize(PixelPoint aSize);

    /// set content origin/center (without changing size)
    void setContentOrigin(PixelPoint aOrigin);

    /// set content origin relative to its own size and frame
    /// @param aRelX relative X position
    /// @param aRelY relative Y position
    /// @param aCentered if set, content origin 0,0 means center of the frame, otherwise the "normal" bottom left of the frame
    /// - centered: 0 = center, -1 = max(framedx,contentdx) to the left/bottom, +1 to the right/top of the center
    /// - not centered: 0 = left/bottom, -1 = max(framesize,contentsize) to the left/bottom, +1 to the right/top
    /// @note for clipped content, -1 or 1 means at least "out of the frame"
    void setRelativeContentOrigin(double aRelX, double aRelY, bool aCentered = true);

    /// set content origin X relative to its own size and the frame size
    /// @param aRelX relative X position
    /// @param aCentered if set, content origin 0 means center of the frame, otherwise the "normal" left of the frame
    /// - centered: 0 = center, -1 = max(framedx,contentdx) to the left/bottom, +1 to the right/top of the center
    /// - not centered: 0 = left, -1 = max(framedx,contentdx) to the left, +1 to the right
    /// @note for clipped content, -1 or 1 means at least "out of the frame"
    void setRelativeContentOriginX(double aRelX, bool aCentered = true);

    /// set content origin Y relative to its own size and the frame size
    /// @param aRelY relative Y position
    /// @param aCentered if set, content origin 0 means center of the frame, otherwise the "normal" bottom of the frame
    /// - centered: 0 = center, -1 = max(framedy,contentdy) to the bottom, +1 to the top of the center
    /// - not centered: 0 = bottom, -1 = max(framedy,contentdy) to the bottom, +1 to the top
    /// @note for clipped content, -1 or 1 means at least "out of the frame"
    void setRelativeContentOriginY(double aRelY, bool aCentered = true);

    /// @return content size
    PixelPoint getContentSize() const { return { mContent.dx, mContent.dy }; }

    /// @return frame size
    PixelPoint getFrameSize() const { return { mFrame.dx, mFrame.dy }; }

    /// set content size to match full frame size, set content position to 0,0
    /// @note if orientation has x/y swapped, content size will also be set swapped
    void setFullFrameContent();

    /// size frame to content (but no move)
    void sizeFrameToContent();

    /// move frame such that its new origin is at the point where the content area starts
    /// @note the pixel that appears at the new frame origin is one corner of the contents, but not necessarily the origin,
    ///   depending on orientation.
    /// @note content does not move relative to view frame origin, but frame does
    /// @param aResize if set, frame is also resized to fit contents
    void moveFrameToContent(bool aResize);

    /// child view has changed geometry (frame, content rect)
    virtual void childGeometryChanged(P44ViewPtr aChildView, PixelRect aOldFrame, PixelRect aOldContent) {};

    /// my own geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) {};

    /// get color at X,Y
    /// @param aPt point to get in frame coordinates
    PixelColor colorAt(PixelPoint aPt);

    /// get the LED rrggbb data string
    /// @param aRawRgb will be filled to contain the raw RGB data as specified by aArea
    /// @param aArea the area to get data for
    /// @return hex string in rrggbb format, no separator, as many as there are LEDs defined
    void ledRGBdata(string& aLedRGB, PixelRect aArea);

    /// clear contents of this view
    /// @note base class just resets content size to zero, subclasses might NOT want to do that
    ///   and thus choose NOT to call inherited.
    virtual void clear();

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @param aNow referece time for "now" of this step cycle (slightly in the past because taken before calling)
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil, MLMicroSeconds aNow);

    /// return if anything changed on the display since last call
    virtual bool isDirty()  { return mDirty; };

    /// call when display is updated
    virtual void updated();

    /// register a callback for when the view (supposedly a root view) and its hierarchy become dirty or needs a step() ASAP
    /// @param aNeedUpdateCB this is called from mainloop, so it's safe to call view methods from it, including step().
    void setNeedUpdateCB(TimerCB aNeedUpdateCB);

    /// set the minimum update interval (aka max framerate)
    /// @note this is a hint for animations to calculate a sensible step size, and for
    ///   other mechanisms that want to call requestUpdate(), to avoid calling it too often and wasting cpu cycles.
    /// @param aMinUpdateInterval minimum interval between updates
    void setMinUpdateInterval(MLMicroSeconds aMinUpdateInterval);

    /// get minimal update interval
    /// @return update interval set in first view towards root, if none set, DEFAULT_MIN_UPDATE_INTERVAL
    MLMicroSeconds getMinUpdateInterval();

    /// stop all animations
    virtual void stopAnimations();

    #if ENABLE_VIEWCONFIG

    /// get orientation from text
    static Orientation textToOrientation(const char *aOrientationText);

    /// get text from orientation
    static string orientationToText(Orientation aOrientation);

    /// get wrap mode from text
    static WrapMode textToWrapMode(const char *aWrapModeText);

    /// get text from wrap mode
    static string wrapModeToText(WrapMode aWrapMode, bool aForPositioning);

    /// get view by label
    /// @param aLabelOrId label or Id of view to find
    /// @return NULL if not found, labelled view otherwise (first one with that label found in case >1 have the same label)
    virtual P44ViewPtr getView(const string aLabelOrId);

    //#if !ENABLE_P44SCRIPT
    virtual // Note: with P44Script, we use property access to generically implement this at the P44View level
    //#endif
    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
    ErrorPtr configureView(JsonObjectPtr aViewConfig);

    #if ENABLE_JSON_APPLICATION

    /// configure view from file or JSON
    /// @param aResourceOrObj if this is a single JSON string ending on ".json", it is treated as a resource file name
    ///    which is loaded and used as view config. All other JSON is used as view config as-is
    /// @param aResourcePrefix will be prepended to aResourceOrObj when it is a filename
    ErrorPtr configureFromResourceOrObj(JsonObjectPtr aResourceOrObj, const string aResourcePrefix = "");

    #endif // ENABLE_JSON_APPLICATION

    #endif // ENABLE_VIEWCONFIG

    #if ENABLE_VIEWSTATUS
    //#if !ENABLE_P44SCRIPT
    virtual // Note: with P44Script, we use property access to generically implement this at the P44View level
    //#endif
    /// @return the current status of the view, in the same format as accepted by configure()
    JsonObjectPtr viewStatus();
    #endif // ENABLE_VIEWSTATUS

  public:

    /// set dirty, additionally request a step ASAP
    void makeDirtyAndUpdate();

    /// call to request an update (in case the display does not update itself with a fixed frame rate)
    virtual void requestUpdate();

    /// call to request an update if needed (i.e. the view is dirty)
    void requestUpdateIfNeeded();

    #if ENABLE_P44SCRIPT

    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj();

    /// @return ArrayValue containing all animators installed on this view
    P44Script::ScriptObjPtr installedAnimators();

    #endif

    #if ENABLE_ANIMATION

    /// get an animator for a property
    /// @param aProperty name of the property to animate
    /// @return an animator
    /// @note even in case the property is not know, a dummy animator will be returned, which cannot be started
    ValueAnimatorPtr animatorFor(const string aProperty);

    /// get a value animation setter for a given property of the view
    /// @param aProperty the name of the property to get a setter for
    /// @param aCurrentValue is assigned the current value of the property
    /// @return the setter to be used by the animator
    virtual ValueSetterCB getPropertySetter(const string aProperty, double& aCurrentValue);

  protected:

    ValueSetterCB getGeometryPropertySetter(PixelCoord &aPixelCoord, double &aCurrentValue);
    ValueSetterCB getCoordPropertySetter(PixelCoord &aPixelCoord, double &aCurrentValue);
    ValueSetterCB getColorComponentSetter(const string aComponent, PixelColor &aPixelColor, double &aCurrentValue);

  private:

    /// generic setter for geometry related values (handles automatic frame resizing etc.)
    void geometryPropertySetter(PixelCoord *aPixelCoordP, double aNewValue);
    /// generic setter for other coordinates that just flag the view dirty
    void coordPropertySetter(PixelCoord *aPixelCoordP, double aNewValue);
    /// generic setter for single R,G,B or A component that just flag the view dirty
    ValueSetterCB getSingleColorComponentSetter(PixelColorComponent &aColorComponent, double &aCurrentValue);
    void singleColorComponentSetter(PixelColorComponent *aColorComponentP, double aNewValue);
    /// setter for hue, saturation, brightness that affect all 3 R,G,B channels at once
    ValueSetterCB getDerivedColorComponentSetter(int aHSBIndex, PixelColor &aPixelColor, double &aCurrentValue);
    void derivedColorComponentSetter(int aHSBIndex, PixelColor *aPixelColorP, double aNewValue);

    void configureAnimation(JsonObjectPtr aAnimationCfg);

    #endif // ENABLE_ANIMATION

  };


  #if ENABLE_P44SCRIPT

  /// property accessor
  /// @param aView the P44View
  /// @param aToWrite if null, this is a read access, otherwise it is the value to write
  /// @return on read, the property value - on write null if the property is not writable, the
  ///   written value (usually aToWrite) when write was successful
  typedef P44Script::ScriptObjPtr (*AccFn)(P44View& aView, P44Script::ScriptObjPtr aToWrite);

  namespace P44Script {

    /// represents a view of a P44lrgraphics view hierarchy
    class P44lrgViewObj : public P44Script::StructuredLookupObject
    {
      typedef P44Script::StructuredLookupObject inherited;
      P44ViewPtr mView;
    public:
      P44lrgViewObj(P44ViewPtr aView);
      virtual string getAnnotation() const P44_OVERRIDE { return "lrgView"; };
      P44ViewPtr view() { return mView; }
    };

    /// represents the global objects related to p44lrgraphics
    class P44lrgLookup : public BuiltInMemberLookup
    {
      typedef BuiltInMemberLookup inherited;
      P44ViewPtr* mRootViewPtrP;
    public:
      P44lrgLookup(P44ViewPtr *aRootViewPtrP);
      P44ViewPtr rootView() { return *mRootViewPtrP; }
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44



#endif /* _p44lrgraphics_view_hpp__ */
