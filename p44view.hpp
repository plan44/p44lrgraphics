//
//  Copyright (c) 2016-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#define DEFAULT_MIN_UPDATE_INTERVAL (15*MilliSecond)

namespace p44 {

  typedef uint8_t PixelColorComponent;

  typedef struct {
    PixelColorComponent r;
    PixelColorComponent g;
    PixelColorComponent b;
    PixelColorComponent a; // alpha
  } PixelColor;

  const PixelColor transparent = { .r=0, .g=0, .b=0, .a=0 };
  const PixelColor black = { .r=0, .g=0, .b=0, .a=255 };
  const PixelColor white = { .r=255, .g=255, .b=255, .a=255 };

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

  /// dim down (or light up) value
  /// @param aVal 0..255 value to dim up or down
  /// @param aDim 0..255: dim, >255: light up (255=100%)
  /// @return dimmed value, limited to max==255
  PixelColorComponent dimVal(PixelColorComponent aVal, uint16_t aDim);

  /// dim  r,g,b values of a pixel (alpha unaffected)
  /// @param aPix the pixel
  /// @param aDim 0..255: dim, >255: light up (255=100%)
  void dimPixel(PixelColor &aPix, uint16_t aDim);

  /// return dimmed pixel (alpha same as input)
  /// @param aPix the pixel
  /// @param aDim 0..255: dim, >255: light up (255=100%)
  /// @return dimmed pixel
  PixelColor dimmedPixel(const PixelColor aPix, uint16_t aDim);

  /// dim pixel r,g,b down by its alpha value, but alpha itself is not changed!
  /// @param aPix the pixel
  void alpahDimPixel(PixelColor &aPix);

  /// reduce a value by given amount, but not below minimum
  /// @param aByte value to be reduced
  /// @param aAmount amount to reduce
  /// @param aMin minimum value (default=0)
  void reduce(uint8_t &aByte, uint8_t aAmount, uint8_t aMin = 0);

  /// increase a value by given amount, but not above maximum
  /// @param aByte value to be increased
  /// @param aAmount amount to increase
  /// @param aMax maximum value (default=255)
  void increase(uint8_t &aByte, uint8_t aAmount, uint8_t aMax = 255);

  /// add color of one pixel to another
  /// @note does not check for color component overflow/wraparound!
  /// @param aPixel the pixel to add to
  /// @param aPixelToAdd the pixel to add
  void addToPixel(PixelColor &aPixel, PixelColor aPixelToAdd);

  /// overlay a pixel on top of a pixel (based on alpha values)
  /// @param aPixel the original pixel to add an ovelay to
  /// @param aOverlay the pixel to be laid on top
  void overlayPixel(PixelColor &aPixel, PixelColor aOverlay);

  /// mix two pixels
  /// @param aMainPixel the original pixel which will be modified to contain the mix
  /// @param aOutsidePixel the pixel to mix in
  /// @param aAmountOutside 0..255 (= 0..100%) value to determine how much weight the outside pixel should get in the result
  void mixinPixel(PixelColor &aMainPixel, PixelColor aOutsidePixel, PixelColorComponent aAmountOutside);

  /// get RGB pixel from HSB
  /// @param aHue hue, 0..360 degrees
  /// @param aSaturation saturation, 0..1
  /// @param aBrightness brightness, 0..1
  /// @param aBrightnessAsAlpha if set, brightness is returned in alpha, while RGB will be set for full brightness
  PixelColor hsbToPixel(double aHue, double aSaturation = 1.0, double aBrightness = 1.0, bool aBrightnessAsAlpha = false);

  /// get HSB from pixel
  /// @param aPixelColor the pixel color
  /// @param aHue receives hue, 0..360 degrees
  /// @param aSaturation receives saturation, 0..1
  /// @param aBrightness receives brightness, 0..1
  /// @param aIncludeAlphaIntoBrightness if set, alpha is included in aBrightness returned
  void pixelToHsb(PixelColor aPixelColor, double &aHue, double &aSaturation, double &aBrightness, bool aIncludeAlphaIntoBrightness = false);

  #if ENABLE_VIEWCONFIG

  /// convert Web color to pixel color
  /// @param aWebColor web style #ARGB or #AARRGGBB color, alpha (A, AA) is optional, "#" is also optional
  /// @return pixel color. If Alpha is not specified, it is set to fully opaque = 255.
  PixelColor webColorToPixel(const string aWebColor);

  /// convert pixel color to web color
  /// @param aPixelColor pixel color
  /// @return web color in RRGGBB style or AARRGGBB when alpha is not fully opaque (==255)
  string pixelToWebColor(const PixelColor aPixelColor);

  #endif // ENABLE_VIEWCONFIG

  /// @}

  class P44View;
  typedef boost::intrusive_ptr<P44View> P44ViewPtr;

  class P44View : public P44Obj
  {
    friend class ViewStack;

    bool dirty;
    bool updateRequested; ///< set when needUpdateCB has been called, reset at step() or update()
    TimerCB needUpdateCB; ///< called when dirty check and calling step must occur earlier than what last step() call said
    MLMicroSeconds minUpdateInterval; ///< minimum update interval (as a hint from actual display)

    int geometryChanging;
    bool changedGeometry;
    PixelRect previousFrame;
    PixelRect previousContent;

    #if ENABLE_ANIMATION
    typedef std::list<ValueAnimatorPtr> AnimationsList;
    AnimationsList animations; /// the list of currently running animations
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


  protected:

    // parent view (pointer only, to avoid retain cycles)
    // Containers must make sure their children's parent pointer gets reset before parent goes away
    P44View* parentView;

    // outer frame
    PixelRect frame;
    bool sizeToContent; ///< if set, frame is automatically resized with content

    // alpha (opacity)
    PixelColorComponent alpha;

    // colors
    PixelColor backgroundColor; ///< background color
    PixelColor foregroundColor; ///< foreground color

    // z-order
    int z_order;

    // content
    PixelRect content; ///< content offset and size relative to frame (but in content coordinates, i.e. possibly orientation translated!)
    Orientation contentOrientation; ///< orientation of content in frame
    WrapMode contentWrapMode; ///< content wrap mode in frame area
    bool contentIsMask; ///< if set, only alpha of content is used on foreground color
    bool invertAlpha; ///< invert alpha provided by content
    bool localTimingPriority; ///< if set, this view's timing requirements should be treated with priority over child view's
    MLMicroSeconds maskChildDirtyUntil; ///< if>0, child's dirty must not be reported until this time is reached
    double contentRotation; ///< rotation of content pixels in degree CCW
    // - derived values
    double rotSin;
    double rotCos;



    string label; ///< label of the view for addressing it

    /// announce/finish geometry change
    void geometryChange(bool aStart);

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
    virtual PixelColor contentColorAt(PixelPoint aPt);

    /// helper for implementations: check if aPt within set content size
    bool isInContentSize(PixelPoint aPt);

    /// color effect params have changed
    virtual void recalculateColoring() { /* NOP in the base class */ };

    /// set dirty, additionally request a step ASAP
    void makeDirtyAndUpdate();

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
    void updateNextCall(MLMicroSeconds &aNextCall, MLMicroSeconds aCallCandidate, MLMicroSeconds aCandidatePriorityUntil = 0);

  public :

    /// create view
    P44View();

    virtual ~P44View();

    static const char* staticTypeName() { return "plain"; };
    virtual const char* viewTypeName() { return staticTypeName(); }

    /// set the frame within the parent coordinate system
    /// @param aFrame the new frame for the view
    virtual void setFrame(PixelRect aFrame);

    /// @return current frame rect
    PixelRect getFrame() { return frame; };

    /// @return current content rect
    PixelRect getContent() { return content; };

    /// @param aParentView parent view or NULL if none
    void setParent(P44ViewPtr aParentView);

    /// set the view's background color
    /// @param aBackgroundColor color of pixels not covered by content
    void setBackgroundColor(PixelColor aBackgroundColor) { backgroundColor = aBackgroundColor; makeColorDirty(); };

    /// @return current background color
    PixelColor getBackgroundColor() { return backgroundColor; }

    /// set foreground color
    void setForegroundColor(PixelColor aColor) { foregroundColor = aColor; makeColorDirty(); }

    /// get foreground color
    PixelColor getForegroundColor() const { return foregroundColor; }

    /// set content wrap mode
    /// @param aWrapMode the new wrap mode
    void setWrapMode(WrapMode aWrapMode) { contentWrapMode = aWrapMode; makeDirty(); }

    /// get current wrap mode
    /// @return current wrap mode
    WrapMode getWrapMode() { return contentWrapMode; }

    /// set view label
    /// @note this is for referencing views in reconfigure operations
    /// @param aLabel the new label
    void setLabel(const string aLabel) { label = aLabel; }

    /// set default view label (i.e. set label only if the view does have an empty label so far)
    /// @param aLabel the new label
    void setDefaultLabel(const string aLabel) { if (label.empty()) label = aLabel; };

    /// set view's alpha
    /// @param aAlpha 0=fully transparent, 255=fully opaque
    void setAlpha(PixelColorComponent aAlpha);

    /// get current alpha
    /// @return current alpha value 0=fully transparent=invisible, 255=fully opaque
    PixelColorComponent getAlpha() { return alpha; };

    /// hide (set alpha to 0)
    void hide() { setAlpha(0); };

    /// show (set alpha to max)
    void show() { setAlpha(255); };

    /// set visible (hide or show)
    /// @param aVisible true to show, false to hide
    void setVisible(bool aVisible) { if (aVisible) show(); else hide(); };

    /// @return Z-Order (e.g. in a viewstack, highest is frontmost)
    int getZOrder() { return z_order; };

    /// @param aZOrder z-order (e.g. in a viewstack, highest is frontmost)
    void setZOrder(int aZOrder);

    /// @param aOrientation the orientation of the content in the frame
    void setOrientation(Orientation aOrientation) { contentOrientation = aOrientation; makeDirty(); }

    /// set content rotation around content origin
    /// @param aRotation content rotation in degrees CCW
    void setContentRotation(double aRotation);

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

    /// set content origin X relative to its own size and frame
    /// @param aRelX relative X position
    /// @param aCentered if set, content origin 0 means center of the frame, otherwise the "normal" left of the frame
    /// - centered: 0 = center, -1 = max(framedx,contentdx) to the left/bottom, +1 to the right/top of the center
    /// - not centered: 0 = left, -1 = max(framedx,contentdx) to the left, +1 to the right
    /// @note for clipped content, -1 or 1 means at least "out of the frame"
    void setRelativeContentOriginX(double aRelX, bool aCentered = true);
    /// set content origin Y relative to its own size and frame
    /// @param aRelY relative Y position
    /// @param aCentered if set, content origin 0 means center of the frame, otherwise the "normal" bottom of the frame
    /// - centered: 0 = center, -1 = max(framedy,contentdy) to the bottom, +1 to the top of the center
    /// - not centered: 0 = bottom, -1 = max(framedy,contentdy) to the bottom, +1 to the top
    /// @note for clipped content, -1 or 1 means at least "out of the frame"
    void setRelativeContentOriginY(double aRelY, bool aCentered = true);

    /// @return content size
    PixelPoint getContentSize() const { return { content.dx, content.dy }; }

    /// @return frame size
    PixelPoint getFrameSize() const { return { frame.dx, frame.dy }; }

    /// set content size to full frame content with same origin and orientation
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

    /// clear contents of this view
    /// @note base class just resets content size to zero, subclasses might NOT want to do that
    ///   and thus choose NOT to call inherited.
    virtual void clear();

    /// calculate changes on the display, return time of next change
    /// @param aPriorityUntil for views with local priority flag set, priority is valid until this time is reached
    /// @return Infinite if there is no immediate need to call step again, otherwise mainloop time of when to call again latest
    /// @note this must be called as demanded by return value, and after making changes to the view
    virtual MLMicroSeconds step(MLMicroSeconds aPriorityUntil);

    /// return if anything changed on the display since last call
    virtual bool isDirty()  { return dirty; };

    /// call when display is updated
    virtual void updated();

    /// remove this view from its parent (if any)
    /// @return true if view actually was removed from a parent view
    bool removeFromParent();

    /// remove specific subview from this view
    /// @param aView the view to remove from the stack
    /// @return true if view actually was a subview and was removed
    /// @note this is a NOP in this baseclass
    virtual bool removeView(P44ViewPtr aView) { return false; }


    /// register a callback for when the view (supposedly a root view) and its hierarchy become dirty or needs a step() ASAP
    /// @param aNeedUpdateCB this is called from mainloop, so it's safe to call view methods from it, including step().
    void setNeedUpdateCB(TimerCB aNeedUpdateCB, MLMicroSeconds aMinUpdateInterval);

    /// get minimal update interval
    /// @return update interval set in first view towards root, if none set, DEFAULT_MIN_UPDATE_INTERVAL
    MLMicroSeconds getMinUpdateInterval();

    /// stop all animations
    virtual void stopAnimations();

    #if ENABLE_VIEWCONFIG

    /// get orientation from text
    static Orientation textToOrientation(const char *aOrientationText);

    /// get wrap mode from text
    static WrapMode textToWrapMode(const char *aWrapModeText);

    /// configure view from JSON
    /// @param aViewConfig JSON for configuring view and subviews
    /// @return ok or error in case of real errors (image not found etc., but minor
    ///   issues like unknown properties usually don't cause error)
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig);

    /// get view by label
    /// @param aLabel label of view to find
    /// @return NULL if not found, labelled view otherwise (first one with that label found in case >1 have the same label)
    virtual P44ViewPtr getView(const string aLabel);

    #if ENABLE_JSON_APPLICATION

    /// configure view from file or JSON
    /// @param aResourceOrObj if this is a single JSON string ending on ".json", it is treated as a resource file name
    ///    which is loaded and used as view config. All other JSON is used as view config as-is
    /// @param aResourcePrefix will be prepended to aResourceOrObj when it is a filename
    ErrorPtr configureFromResourceOrObj(JsonObjectPtr aResourceOrObj, const string aResourcePrefix = "");

    #endif // ENABLE_JSON_APPLICATION

    #endif // ENABLE_VIEWCONFIG

    #if ENABLE_VIEWSTATUS
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus();
    #endif // ENABLE_VIEWSTATUS

    #if ENABLE_ANIMATION

  public:

    /// call to request an update (in case the display does not update itself with a fixed frame rate)
    virtual void requestUpdate();

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

    #endif // ENABLE_ANIMATION

  };

} // namespace p44



#endif /* _p44lrgraphics_view_hpp__ */
