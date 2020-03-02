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

// File scope debugging options
// - Set ALWAYS_DEBUG to 1 to enable DBGLOG output even in non-DEBUG builds of this file
#define ALWAYS_DEBUG 0
// - set FOCUSLOGLEVEL to non-zero log level (usually, 5,6, or 7==LOG_DEBUG) to get focus (extensive logging) for this file
//   Note: must be before including "logger.hpp" (or anything that includes "logger.hpp")
#define FOCUSLOGLEVEL 7

#include "p44view.hpp"
#include "ledchaincomm.hpp" // for brightnessToPwm and pwmToBrightness

using namespace p44;

// MARK: ===== View

P44View::P44View() :
  parentView(NULL),
  dirty(false),
  updateRequested(false),
  geometryChanging(0),
  changedGeometry(false),
  sizeToContent(false),
  contentRotation(0),
  rotCos(1.0),
  rotSin(0.0)
{
  setFrame(zeroRect);
  // default to normal orientation
  contentOrientation = right;
  // default to clip, no content wrap
  contentWrapMode = clipXY;
  // default content size is same as view's
  setContent(zeroRect);
  backgroundColor = { .r=0, .g=0, .b=0, .a=0 }; // transparent background,
  foregroundColor = { .r=255, .g=255, .b=255, .a=255 }; // fully white foreground...
  alpha = 255; // but content pixels passed trough 1:1
  z_order = 0; // none in particular
  contentIsMask = false; // content color will be used
  invertAlpha = false; // inverted mask
  localTimingPriority = true;
  maskChildDirtyUntil = Never;
}


P44View::~P44View()
{
  clear();
}


// MARK: ===== frame and content

bool P44View::isInContentSize(PixelPoint aPt)
{
  return aPt.x>=0 && aPt.y>=0 && aPt.x<content.dx && aPt.y<content.dy;
}


PixelColor P44View::contentColorAt(PixelPoint aPt)
{
  if (isInContentSize(aPt))
    return foregroundColor;
  else
    return backgroundColor;
}


void P44View::geometryChange(bool aStart)
{
  if (aStart){
    if (geometryChanging<=0) {
      // start tracking changes
      changedGeometry = false;
      previousFrame = frame;
      previousContent = content;
    }
    geometryChanging++;
  }
  else {
    if (geometryChanging>0) {
      geometryChanging--;
      if (geometryChanging==0) {
        if (changedGeometry) {
          FOCUSLOG("View '%s' changed geometry: frame=(%d,%d,%d,%d)->(%d,%d,%d,%d), content=(%d,%d,%d,%d)->(%d,%d,%d,%d)",
            label.c_str(),
            previousFrame.x, previousFrame.y, previousFrame.dx, previousFrame.dy,
            frame.x, frame.y, frame.dx, frame.dy,
            previousContent.x, previousContent.y, previousContent.dx, previousContent.dy,
            content.x, content.y, content.dx, content.dy
          );
          makeDirty();
          geometryChanged(previousFrame, previousContent); // let subclasses know
          if (parentView) {
            // Note: as we are passing in the frames, it is safe when the following calls recursively calls geometryChange again
            //   except that it must not do so unconditionally to prevent endless recursion
            parentView->childGeometryChanged(this, previousFrame, previousContent);
          }
        }
      }
    }
  }
}



void P44View::orientateCoord(PixelPoint &aCoord)
{
  if (contentOrientation & xy_swap) {
    swap(aCoord.x, aCoord.y);
  }
}


void P44View::flipCoordInFrame(PixelPoint &aCoord)
{
  // flip within frame if not zero sized
  if ((contentOrientation & x_flip) && frame.dx>0) {
    aCoord.x = frame.dx-aCoord.x-1;
  }
  if ((contentOrientation & y_flip) && frame.dy>0) {
    aCoord.y = frame.dy-aCoord.y-1;
  }
}


void P44View::inFrameToContentCoord(PixelPoint &aCoord)
{
  flipCoordInFrame(aCoord);
  orientateCoord(aCoord);
  aCoord.x -= content.x;
  aCoord.y -= content.y;
}


void P44View::contentToInFrameCoord(PixelPoint &aCoord)
{
  aCoord.x += content.x;
  aCoord.y += content.y;
  orientateCoord(aCoord);
  flipCoordInFrame(aCoord);
}


/// change rect and trigger geometry change when actually changed
void P44View::changeGeometryRect(PixelRect &aRect, PixelRect aNewRect)
{
  if (aNewRect.x!=aRect.x) {
    changedGeometry = true;
    aRect.x = aNewRect.x;
  }
  if (aNewRect.y!=aRect.y) {
    changedGeometry = true;
    aRect.y = aNewRect.y;
  }
  if (aNewRect.dx!=aRect.dx) {
    changedGeometry = true;
    aRect.dx = aNewRect.dx;
  }
  if (aNewRect.dy!=aRect.dy) {
    changedGeometry = true;
    aRect.dy = aNewRect.dy;
  }
}



void P44View::setFrame(PixelRect aFrame)
{
  geometryChange(true);
  changeGeometryRect(frame, aFrame);
  geometryChange(false);
}


void P44View::setParent(P44ViewPtr aParentView)
{
  parentView = aParentView.get();
}


void P44View::setContent(PixelRect aContent)
{
  geometryChange(true);
  changeGeometryRect(content, aContent);
  if (sizeToContent) {
    moveFrameToContent(true);
  }
  geometryChange(false);
};


void P44View::setContentSize(PixelPoint aSize)
{
  geometryChange(true);
  changeGeometryRect(content, { content.x, content.y, aSize.x, aSize.y });
  if (changedGeometry && sizeToContent) moveFrameToContent(true);
  geometryChange(false);
};


void P44View::setContentOrigin(PixelPoint aOrigin)
{
  geometryChange(true);
  changeGeometryRect(content, { aOrigin.x, aOrigin.y, content.dx, content.dy });
  geometryChange(false);
};


void P44View::setRelativeContentOrigin(double aRelX, double aRelY, bool aCentered)
{
  geometryChange(true);
  setRelativeContentOriginX(aRelX, aCentered);
  setRelativeContentOriginY(aRelY, aCentered);
  geometryChange(false);
}

void P44View::setRelativeContentOriginX(double aRelX, bool aCentered)
{
  // standard version, content origin is a corner of the relevant area
  geometryChange(true);
  changeGeometryRect(content, { (int)(aRelX*max(content.dx,frame.dx)+(aCentered ? frame.dx/2 : 0)), content.y, content.dx, content.dy });
  geometryChange(false);
}


void P44View::setRelativeContentOriginY(double aRelY, bool aCentered)
{
  // standard version, content origin is a corner of the relevant area
  geometryChange(true);
  changeGeometryRect(content, { content.x, (int)(aRelY*max(content.dy,frame.dy)+(aCentered ? frame.dy/2 : 0)), content.dx, content.dy });
  geometryChange(false);
}


void P44View::setContentRotation(double aRotation)
{
  if (aRotation!=contentRotation) {
    contentRotation = aRotation;
    double rotPi = contentRotation*M_PI/180;
    rotSin = sin(rotPi);
    rotCos = cos(rotPi);
    makeDirty();
  }
}


void P44View::setFullFrameContent()
{
  PixelPoint sz = getFrameSize();
  setOrientation(P44View::right);
  orientateCoord(sz);
  setContent({ 0, 0, sz.x, sz.y });
}



void P44View::contentRectAsViewCoord(PixelRect &aRect)
{
  // get opposite content rect corners
  PixelPoint c1 = { 0, 0 };
  contentToInFrameCoord(c1);
  PixelPoint inset = { content.dx>0 ? 1 : 0, content.dy>0 ? 1 : 0 };
  PixelPoint c2 = { content.dx-inset.x, content.dy-inset.y };
  // transform into coords relative to frame origin
  contentToInFrameCoord(c2);
  // make c2 the non-origin corner
  if (c1.x>c2.x) swap(c1.x, c2.x);
  if (c1.y>c2.y) swap(c1.y, c2.y);
  // create view coord rectangle around current contents
  aRect.x = c1.x + frame.x;
  aRect.dx = c2.x-c1.x+inset.x;
  aRect.y = c1.y + frame.y;
  aRect.dy = c2.y-c1.y+inset.y;
  FOCUSLOG("View '%s' frame=(%d,%d,%d,%d), content rect as view coords=(%d,%d,%d,%d)",
    label.c_str(),
    frame.x, frame.y, frame.dx, frame.dy,
    aRect.x, aRect.y, aRect.dx, aRect.dy
  );
}


/// move frame such that its origin is at the actual content's origin
/// @note content does not move relative to view frame origin, but frame does
void P44View::moveFrameToContent(bool aResize)
{
  geometryChange(true);
  if (aResize) sizeFrameToContent();
  PixelRect f;
  contentRectAsViewCoord(f);
  // move frame to the place where the content rectangle did appear so far...
  changeGeometryRect(frame, f);
  // ...which means that no content offset is needed any more (we've compensated it by moving the frame)
  content.x = 0; content.y = 0;
  geometryChange(false);
}


void P44View::sizeFrameToContent()
{
  PixelPoint sz = { content.dx, content.dy };
  orientateCoord(sz);
  PixelRect f = frame;
  f.dx = sz.x;
  f.dy = sz.y;
  changeGeometryRect(frame, f);
}



void P44View::clear()
{
  #if ENABLE_ANIMATION
  stopAnimations();
  #endif
  setContentSize({0, 0});
}


// MARK: ===== updating

void P44View::makeDirtyAndUpdate()
{
  // make dirty locally
  makeDirty();
  // request a step at the root view level
  requestUpdate();
}


void P44View::requestUpdate()
{
  P44View *p = this;
  while (p->parentView) {
    p = p->parentView;
  }
  if (!p->updateRequested && p->needUpdateCB) {
    updateRequested = true; // only request once
    // there is a needUpdate callback here
    // DO NOT call it directly, but from mainloop, so receiver can safely call
    // back into any view object method without causing recursions
    MainLoop::currentMainLoop().executeNow(p->needUpdateCB);
  }
}

void P44View::updated()
{
  dirty = false;
  updateRequested = false;
}



void P44View::makeDirty()
{
  dirty = true;
}


void P44View::makeColorDirty()
{
  recalculateColoring();
  makeDirty();
}


bool P44View::reportDirtyChilds()
{
  if (maskChildDirtyUntil) {
    if (MainLoop::now()<maskChildDirtyUntil) {
      return false;
    }
    maskChildDirtyUntil = 0;
  }
  return true;
}


void P44View::updateNextCall(MLMicroSeconds &aNextCall, MLMicroSeconds aCallCandidate, MLMicroSeconds aCandidatePriorityUntil)
{
  if (localTimingPriority && aCandidatePriorityUntil>0 && aCallCandidate>=0 && aCallCandidate<aCandidatePriorityUntil) {
    // children must not cause "dirty" before candidate time is over
    MLMicroSeconds now = MainLoop::now();
    maskChildDirtyUntil = (aCallCandidate-now)*2+now; // duplicate to make sure candidate execution has some time to happen BEFORE dirty is unblocked
  }
  if (aNextCall<=0 || (aCallCandidate>0 && aCallCandidate<aNextCall)) {
    // candidate wins
    aNextCall = aCallCandidate;
  }
}


MLMicroSeconds P44View::step(MLMicroSeconds aPriorityUntil)
{
  updateRequested = false; // no step request pending any more
  // check animations
  MLMicroSeconds nextCall = Infinite;
  #if ENABLE_ANIMATION
  AnimationsList::iterator pos = animations.begin();
  while (pos != animations.end()) {
    ValueAnimatorPtr animator = (*pos);
    MLMicroSeconds nextStep = animator->step();
    if (nextStep==Infinite) {
      // this animation is done, remove it from the list
      pos = animations.erase(pos);
      continue;
    }
    updateNextCall(nextCall, nextStep);
    pos++;
  }
  #endif // ENABLE_ANIMATION
  return nextCall;
}


void P44View::setAlpha(PixelColorComponent aAlpha)
{
  if (alpha!=aAlpha) {
    alpha = aAlpha;
    makeDirty();
  }
}


void P44View::setZOrder(int aZOrder)
{
  geometryChange(true);
  if (z_order!=aZOrder) {
    z_order = aZOrder;
    changedGeometry = true;
  }
  geometryChange(false);
}



#define SHOW_ORIGIN 0


PixelColor P44View::colorAt(PixelPoint aPt)
{
  // default is background color
  PixelColor pc = backgroundColor;
  if (alpha==0) {
    pc.a = 0; // entire view is invisible
  }
  else {
    // calculate coordinate relative to the frame's origin
    aPt.x -= frame.x;
    aPt.y -= frame.y;
    // optionally clip content to frame
    if (contentWrapMode&clipXY && (
      ((contentWrapMode&clipXmin) && aPt.x<0) ||
      ((contentWrapMode&clipXmax) && aPt.x>=frame.dx) ||
      ((contentWrapMode&clipYmin) && aPt.y<0) ||
      ((contentWrapMode&clipYmax) && aPt.y>=frame.dy)
    )) {
      // clip
      pc.a = 0; // invisible
    }
    else {
      // not clipped
      // optionally wrap content (repeat frame contents in selected directions)
      if (frame.dx>0) {
        while ((contentWrapMode&wrapXmin) && aPt.x<0) aPt.x+=frame.dx;
        while ((contentWrapMode&wrapXmax) && aPt.x>=frame.dx) aPt.x-=frame.dx;
      }
      if (frame.dy>0) {
        while ((contentWrapMode&wrapYmin) && aPt.y<0) aPt.y+=frame.dy;
        while ((contentWrapMode&wrapYmax) && aPt.y>=frame.dy) aPt.y-=frame.dy;
      }
      // translate into content coordinates
      inFrameToContentCoord(aPt);
      // now get content pixel in content coordinates
      if (contentRotation==0) {
        // optimization: no rotation, get pixel
        pc = contentColorAt(aPt);
      }
      else {
        // - rotate first
        PixelPoint rpt;
        rpt.x = aPt.x*rotCos-aPt.y*rotSin;
        rpt.y = aPt.x*rotSin+aPt.y*rotCos;
        pc = contentColorAt(rpt);
      }
      if (invertAlpha) {
        pc.a = 255-pc.a;
      }
      if (contentIsMask) {
        // use only (possibly inverted) alpha of content, color comes from foregroundColor
        pc.r = foregroundColor.r;
        pc.g = foregroundColor.g;
        pc.b = foregroundColor.b;
      }
      #if SHOW_ORIGIN
      if (aPt.x==0 && aPt.y==0) {
        return { .r=255, .g=0, .b=0, .a=255 };
      }
      else if (aPt.x==1 && aPt.y==0) {
        return { .r=0, .g=255, .b=0, .a=255 };
      }
      else if (aPt.x==0 && aPt.y==1) {
        return { .r=0, .g=0, .b=255, .a=255 };
      }
      #endif
      if (pc.a==0) {
        // background is where content is fully transparent
        pc = backgroundColor;
        // Note: view background does NOT shine through semi-transparent content pixels!
        //   Rather, non-fully-transparent content pixels directly are view pixels!
      }
      // factor in layer alpha
      if (alpha!=255) {
        pc.a = dimVal(pc.a, alpha);
      }
    }
  }
  return pc;
}


// MARK: ===== Utilities

bool p44::rectContainsRect(const PixelRect &aParentRect, const PixelRect &aChildRect)
{
  return
    aChildRect.x>=aParentRect.x &&
    aChildRect.x+aChildRect.dx<=aParentRect.x+aParentRect.dx &&
    aChildRect.y>=aParentRect.y &&
    aChildRect.y+aChildRect.dy<=aParentRect.y+aParentRect.dy;
}

bool p44::rectIntersectsRect(const PixelRect &aRect1, const PixelRect &aRect2)
{
  return
    aRect1.x+aRect1.dx>aRect2.x &&
    aRect1.x<aRect2.x+aRect2.dx &&
    aRect1.y+aRect1.dy>aRect2.y &&
    aRect1.y<aRect2.y+aRect2.dy;
}



PixelColorComponent p44::dimVal(PixelColorComponent aVal, uint16_t aDim)
{
  uint32_t d = (aDim+1)*aVal;
  if (d>0xFFFF) return 0xFF;
  return d>>8;
}


void p44::dimPixel(PixelColor &aPix, uint16_t aDim)
{
  if (aDim==255) return; // 100% -> NOP
  aPix.r = dimVal(aPix.r, aDim);
  aPix.g = dimVal(aPix.g, aDim);
  aPix.b = dimVal(aPix.b, aDim);
}


PixelColor p44::dimmedPixel(const PixelColor aPix, uint16_t aDim)
{
  PixelColor pix = aPix;
  dimPixel(pix, aDim);
  return pix;
}


void p44::alpahDimPixel(PixelColor &aPix)
{
  if (aPix.a!=255) {
    dimPixel(aPix, aPix.a);
  }
}


void p44::reduce(uint8_t &aByte, uint8_t aAmount, uint8_t aMin)
{
  int r = aByte-aAmount;
  if (r<aMin)
    aByte = aMin;
  else
    aByte = (uint8_t)r;
}


void p44::increase(uint8_t &aByte, uint8_t aAmount, uint8_t aMax)
{
  int r = aByte+aAmount;
  if (r>aMax)
    aByte = aMax;
  else
    aByte = (uint8_t)r;
}


void p44::overlayPixel(PixelColor &aPixel, PixelColor aOverlay)
{
  if (aOverlay.a==255) {
    aPixel = aOverlay;
  }
  else {
    // mix in
    // - reduce original by alpha of overlay
    aPixel = dimmedPixel(aPixel, 255-aOverlay.a);
    // - reduce overlay by its own alpha
    aOverlay = dimmedPixel(aOverlay, aOverlay.a);
    // - add in
    addToPixel(aPixel, aOverlay);
  }
  aPixel.a = 255; // result is never transparent
}


void p44::mixinPixel(PixelColor &aMainPixel, PixelColor aOutsidePixel, PixelColorComponent aAmountOutside)
{
  if (aAmountOutside>0) {
    if (aMainPixel.a!=255 || aOutsidePixel.a!=255) {
      // mixed transparency
      PixelColorComponent alpha = dimVal(aMainPixel.a, pwmToBrightness(255-aAmountOutside)) + dimVal(aOutsidePixel.a, pwmToBrightness(aAmountOutside));
      if (alpha>0) {
        // calculation only needed for non-transparent result
        // - alpha boost compensates for energy
        uint16_t ab = 65025/alpha;
        // Note: aAmountOutside is on the energy scale, not brightness, so need to add in PWM scale!
        uint16_t r_e = ( (((uint16_t)brightnessToPwm(dimVal(aMainPixel.r, aMainPixel.a))+1)*(255-aAmountOutside)) + (((uint16_t)brightnessToPwm(dimVal(aOutsidePixel.r, aOutsidePixel.a))+1)*(aAmountOutside)) )>>8;
        uint16_t g_e = ( (((uint16_t)brightnessToPwm(dimVal(aMainPixel.g, aMainPixel.a))+1)*(255-aAmountOutside)) + (((uint16_t)brightnessToPwm(dimVal(aOutsidePixel.g, aOutsidePixel.a))+1)*(aAmountOutside)) )>>8;
        uint16_t b_e = ( (((uint16_t)brightnessToPwm(dimVal(aMainPixel.b, aMainPixel.a))+1)*(255-aAmountOutside)) + (((uint16_t)brightnessToPwm(dimVal(aOutsidePixel.b, aOutsidePixel.a))+1)*(aAmountOutside)) )>>8;
        // - back to brightness, add alpha boost
        uint16_t r = (((uint16_t)pwmToBrightness(r_e)+1)*ab)>>8;
        uint16_t g = (((uint16_t)pwmToBrightness(g_e)+1)*ab)>>8;
        uint16_t b = (((uint16_t)pwmToBrightness(b_e)+1)*ab)>>8;
        // - check max brightness
        uint16_t m = r; if (g>m) m = g; if (b>m) m = b;
        if (m>255) {
          // more brightness requested than we have
          // - scale down to make max=255
          uint16_t cr = 65025/m;
          r = (r*cr)>>8;
          g = (g*cr)>>8;
          b = (b*cr)>>8;
          // - increase alpha by reduction of components
          alpha = (((uint16_t)alpha+1)*m)>>8;
          aMainPixel.r = r>255 ? 255 : r;
          aMainPixel.g = g>255 ? 255 : g;
          aMainPixel.b = b>255 ? 255 : b;
          aMainPixel.a = alpha>255 ? 255 : alpha;
        }
        else {
          // brightness below max, just convert back
          aMainPixel.r = r;
          aMainPixel.g = g;
          aMainPixel.b = b;
          aMainPixel.a = alpha;
        }
      }
      else {
        // resulting alpha is 0, fully transparent pixel
        aMainPixel = transparent;
      }
    }
    else {
      // no transparency on either side, simplified case
      uint16_t r_e = ( (((uint16_t)brightnessToPwm(aMainPixel.r)+1)*(255-aAmountOutside)) + (((uint16_t)brightnessToPwm(aOutsidePixel.r)+1)*(aAmountOutside)) )>>8;
      uint16_t g_e = ( (((uint16_t)brightnessToPwm(aMainPixel.g)+1)*(255-aAmountOutside)) + (((uint16_t)brightnessToPwm(aOutsidePixel.g)+1)*(aAmountOutside)) )>>8;
      uint16_t b_e = ( (((uint16_t)brightnessToPwm(aMainPixel.b)+1)*(255-aAmountOutside)) + (((uint16_t)brightnessToPwm(aOutsidePixel.b)+1)*(aAmountOutside)) )>>8;
      aMainPixel.r = r_e>255 ? 255 : pwmToBrightness(r_e);
      aMainPixel.g = g_e>255 ? 255 : pwmToBrightness(g_e);
      aMainPixel.b = b_e>255 ? 255 : pwmToBrightness(b_e);
      aMainPixel.a = 255;
    }
  }
}


void p44::addToPixel(PixelColor &aPixel, PixelColor aPixelToAdd)
{
  increase(aPixel.r, aPixelToAdd.r);
  increase(aPixel.g, aPixelToAdd.g);
  increase(aPixel.b, aPixelToAdd.b);
}


PixelColor p44::webColorToPixel(const string aWebColor)
{
  PixelColor res = transparent;
  size_t i = 0;
  size_t n = aWebColor.size();
  if (n>0 && aWebColor[0]=='#') { i++; n--; } // skip optional #
  uint32_t h;
  if (sscanf(aWebColor.c_str()+i, "%x", &h)==1) {
    if (n<=4) {
      // short form RGB or ARGB
      res.a = 255;
      if (n==4) { res.a = (h>>12)&0xF; res.a |= res.a<<4; }
      res.r = (h>>8)&0xF; res.r |= res.r<<4;
      res.g = (h>>4)&0xF; res.g |= res.g<<4;
      res.b = (h>>0)&0xF; res.b |= res.b<<4;
    }
    else {
      // long form RRGGBB or AARRGGBB
      res.a = 255;
      if (n==8) { res.a = (h>>24)&0xFF; }
      res.r = (h>>16)&0xFF;
      res.g = (h>>8)&0xFF;
      res.b = (h>>0)&0xFF;
    }
  }
  return res;
}


string p44::pixelToWebColor(const PixelColor aPixelColor)
{
  string w;
  if (aPixelColor.a!=255) string_format_append(w, "%02X", aPixelColor.a);
  string_format_append(w, "%02X%02X%02X", aPixelColor.r, aPixelColor.g, aPixelColor.b);
  return w;
}



PixelColor p44::hsbToPixel(double aHue, double aSaturation, double aBrightness, bool aBrightnessAsAlpha)
{
  PixelColor p;
  Row3 RGB, HSV = { aHue, aSaturation, aBrightnessAsAlpha ? 1.0 : aBrightness };
  HSVtoRGB(HSV, RGB);
  p.r = RGB[0]*255;
  p.g = RGB[1]*255;
  p.b = RGB[2]*255;
  p.a = aBrightnessAsAlpha ? aBrightness*255: 255;
  return p;
}


void p44::pixelToHsb(PixelColor aPixelColor, double &aHue, double &aSaturation, double &aBrightness, bool aIncludeAlphaIntoBrightness)
{
  Row3 HSV, RGB = { (double)aPixelColor.r/255, (double)aPixelColor.g/255, (double)aPixelColor.b/255 };
  RGBtoHSV(RGB, HSV);
  aHue = HSV[0];
  aSaturation = HSV[1];
  if (aIncludeAlphaIntoBrightness)
    aBrightness = HSV[2]*aPixelColor.a/255;
  else
    aBrightness = HSV[2];
}



#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr P44View::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  geometryChange(true);
  if (aViewConfig->get("label", o)) {
    label = o->stringValue();
  }
  if (aViewConfig->get("clear", o)) {
    if(o->boolValue()) clear();
  }
  if (aViewConfig->get("x", o)) {
    frame.x = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("y", o)) {
    frame.y = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("dx", o)) {
    frame.dx = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("dy", o)) {
    frame.dy = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("bgcolor", o)) {
    backgroundColor = webColorToPixel(o->stringValue()); makeColorDirty();
  }
  if (aViewConfig->get("color", o)) {
    foregroundColor = webColorToPixel(o->stringValue()); makeColorDirty();
  }
  if (aViewConfig->get("alpha", o)) {
    setAlpha(o->int32Value());
  }
  if (aViewConfig->get("z_order", o)) {
    setZOrder(o->int32Value());
  }
  if (aViewConfig->get("wrapmode", o)) {
    setWrapMode(o->int32Value());
  }
  if (aViewConfig->get("mask", o)) {
    contentIsMask = o->boolValue();
    makeDirty();
  }
  if (aViewConfig->get("invertalpha", o)) {
    invertAlpha = o->boolValue();
    makeDirty();
  }
  // frame rect should be defined here (unless we'll use sizetocontent below), so we can check the content related propes now
  if (aViewConfig->get("fullframe", o)) {
    if(o->boolValue()) setFullFrameContent();
  }
  // modification of content rect
  if (aViewConfig->get("content_x", o)) {
    content.x = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("content_y", o)) {
    content.y = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("content_dx", o)) {
    content.dx = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("content_dy", o)) {
    content.dy = o->int32Value(); makeDirty();
    changedGeometry = true;
  }
  if (aViewConfig->get("rel_content_x", o)) {
    setRelativeContentOriginX(o->doubleValue(), false);
  }
  if (aViewConfig->get("rel_content_y", o)) {
    setRelativeContentOriginY(o->doubleValue(), false);
  }
  if (aViewConfig->get("rel_center_x", o)) {
    setRelativeContentOriginX(o->doubleValue(), true);
  }
  if (aViewConfig->get("rel_center_y", o)) {
    setRelativeContentOriginY(o->doubleValue(), true);
  }
  if (aViewConfig->get("rotation", o)) {
    setContentRotation(o->doubleValue());
  }
  if (aViewConfig->get("orientation", o)) {
    // check this after fullframe, because fullframe resets orientation
    setOrientation(o->int32Value());
  }
  if (aViewConfig->get("timingpriority", o)) {
    localTimingPriority = o->boolValue();
  }
  if (aViewConfig->get("sizetocontent", o)) {
    sizeToContent = o->boolValue();
  }
  if (changedGeometry && sizeToContent) {
    moveFrameToContent(true);
  }
  if (aViewConfig->get("stopanimations", o)) {
    if(o->boolValue()) stopAnimations();
  }
  #if ENABLE_ANIMATION
  if (aViewConfig->get("animate", o)) {
    JsonObjectPtr a = o;
    if (!a->isType(json_type_array)) {
      a = JsonObject::newArray();
      a->arrayAppend(o);
    }
    for (int i=0; i<a->arrayLength(); i++) {
      o = a->arrayGet(i);
      JsonObjectPtr p;
      ValueAnimatorPtr animator;
      if (o->get("property", p)) {
        animator = animatorFor(p->stringValue());
        if (o->get("to", p)) {
          double to = p->doubleValue();
          if (o->get("time", p)) {
            MLMicroSeconds duration = p->doubleValue()*Second;
            // optional params
            bool autoreverse = false;
            int cycles = 1;
            MLMicroSeconds minsteptime = 0;
            double stepsize = 0;
            animator->function(ValueAnimator::easeInOut)->param(3);
            if (o->get("minsteptime", p)) minsteptime = p->doubleValue()*Second;
            if (o->get("stepsize", p)) stepsize = p->doubleValue();
            if (o->get("autoreverse", p)) autoreverse = p->boolValue();
            if (o->get("cycles", p)) cycles = p->int32Value();
            if (o->get("from", p)) animator->from(p->doubleValue());
            if (o->get("function", p)) animator->function(p->stringValue());
            if (o->get("param", p)) animator->param(p->doubleValue());
            animator->repeat(autoreverse, cycles)->animate(to, duration, NULL, minsteptime, stepsize);
          }
        }
      }
    }
  }
  #endif
  geometryChange(false);
  return ErrorPtr();
}


P44ViewPtr P44View::getView(const string aLabel)
{
  if (aLabel==label) {
    return P44ViewPtr(this); // that's me
  }
  return P44ViewPtr(); // not found
}


ErrorPtr P44View::configureFromResourceOrObj(JsonObjectPtr aResourceOrObj, const string aResourcePrefix)
{
  ErrorPtr err;
  JsonObjectPtr cfg = Application::jsonObjOrResource(aResourceOrObj, &err, aResourcePrefix);
  if (Error::isOK(err)) {
    err = configureView(cfg);
  }
  return err;
}


#endif // ENABLE_VIEWCONFIG


#if ENABLE_ANIMATION


void P44View::geometryPropertySetter(PixelCoord *aPixelCoordP, double aNewValue)
{
  PixelCoord newValue = aNewValue;
  if (newValue!=*aPixelCoordP) {
    geometryChange(true);
    *aPixelCoordP = newValue;
    changedGeometry = true;
    if (sizeToContent) moveFrameToContent(true);
    geometryChange(false);
  }
}

ValueSetterCB P44View::getGeometryPropertySetter(PixelCoord &aPixelCoord, double &aCurrentValue)
{
  aCurrentValue = aPixelCoord;
  return boost::bind(&P44View::geometryPropertySetter, this, &aPixelCoord, _1);
}


void P44View::coordPropertySetter(PixelCoord *aPixelCoordP, double aNewValue)
{
  PixelCoord newValue = aNewValue;
  if (newValue!=*aPixelCoordP) {
    *aPixelCoordP = newValue;
    makeDirty();
  }
}

ValueSetterCB P44View::getCoordPropertySetter(PixelCoord &aPixelCoord, double &aCurrentValue)
{
  aCurrentValue = aPixelCoord;
  return boost::bind(&P44View::coordPropertySetter, this, &aPixelCoord, _1);
}


ValueSetterCB P44View::getColorComponentSetter(const string aComponent, PixelColor &aPixelColor, double &aCurrentValue)
{
  if (aComponent=="r") {
    return getSingleColorComponentSetter(aPixelColor.r, aCurrentValue);
  }
  else if (aComponent=="g") {
    return getSingleColorComponentSetter(aPixelColor.g, aCurrentValue);
  }
  else if (aComponent=="b") {
    return getSingleColorComponentSetter(aPixelColor.b, aCurrentValue);
  }
  else if (aComponent=="a") {
    return getSingleColorComponentSetter(aPixelColor.a, aCurrentValue);
  }
  else if (aComponent=="hue") {
    return getDerivedColorComponentSetter(0, aPixelColor, aCurrentValue);
  }
  else if (aComponent=="saturation") {
    return getDerivedColorComponentSetter(1, aPixelColor, aCurrentValue);
  }
  else if (aComponent=="brightness") {
    return getDerivedColorComponentSetter(2, aPixelColor, aCurrentValue);
  }
  return NULL;
}


ValueSetterCB P44View::getSingleColorComponentSetter(PixelColorComponent &aColorComponent, double &aCurrentValue)
{
  aCurrentValue = aColorComponent;
  return boost::bind(&P44View::singleColorComponentSetter, this, &aColorComponent, _1);
}

void P44View::singleColorComponentSetter(PixelColorComponent *aColorComponentP, double aNewValue)
{
  PixelColorComponent c = aNewValue;
  if (*aColorComponentP!=c) {
    *aColorComponentP = c;
    makeColorDirty();
  }
}


ValueSetterCB P44View::getDerivedColorComponentSetter(int aHSBIndex, PixelColor &aPixelColor, double &aCurrentValue)
{
  double hsb[3];
  pixelToHsb(aPixelColor, hsb[0], hsb[1], hsb[2]);
  aCurrentValue = hsb[aHSBIndex];
  return boost::bind(&P44View::derivedColorComponentSetter, this, aHSBIndex, &aPixelColor, _1);
}


void P44View::derivedColorComponentSetter(int aHSBIndex, PixelColor *aPixelColorP, double aNewValue)
{
  double hsb[3];
  pixelToHsb(*aPixelColorP, hsb[0], hsb[1], hsb[2]);
  //LOG(LOG_DEBUG, "--> Pixel = %3d, %3d, %3d -> HSB = %3.2f, %1.3f, %1.3f", aPixelColorP->r, aPixelColorP->g, aPixelColorP->b, hsb[0], hsb[1], hsb[2]);
  if (hsb[aHSBIndex]!=aNewValue) {
    hsb[aHSBIndex] = aNewValue;
    PixelColor px = hsbToPixel(hsb[0], hsb[1], hsb[2]);
    //LOG(LOG_DEBUG, "<-- Pixel = %3d, %3d, %3d <- HSB = %3.2f, %1.3f, %1.3f", px.r, px.g, px.b, hsb[0], hsb[1], hsb[2]);
    aPixelColorP->r = px.r;
    aPixelColorP->g = px.g;
    aPixelColorP->b = px.b;
    makeColorDirty();
  }
}


ValueSetterCB P44View::getPropertySetter(const string aProperty, double& aCurrentValue)
{
  if (aProperty=="alpha") {
    aCurrentValue = getAlpha();
    return boost::bind(&P44View::setAlpha, this, _1);
  }
  else if (aProperty=="rotation") {
    aCurrentValue = contentRotation;
    return boost::bind(&P44View::setContentRotation, this, _1);
  }
  else if (aProperty=="x") {
    return getGeometryPropertySetter(frame.x, aCurrentValue);
  }
  else if (aProperty=="y") {
    return getGeometryPropertySetter(frame.y, aCurrentValue);
  }
  else if (aProperty=="dx") {
    return getGeometryPropertySetter(frame.dx, aCurrentValue);
  }
  else if (aProperty=="dy") {
    return getGeometryPropertySetter(frame.dy, aCurrentValue);
  }
  else if (aProperty=="content_x") {
    return getGeometryPropertySetter(content.x, aCurrentValue);
  }
  else if (aProperty=="content_y") {
    return getGeometryPropertySetter(content.y, aCurrentValue);
  }
  else if (aProperty=="rel_content_x") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginX, this, _1, false);
  }
  else if (aProperty=="rel_content_y") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginY, this, _1, false);
  }
  else if (aProperty=="rel_center_x") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginX, this, _1, true);
  }
  else if (aProperty=="rel_center_y") {
    aCurrentValue = 0; // dummy
    return boost::bind(&P44View::setRelativeContentOriginY, this, _1, true);
  }
  else if (aProperty=="content_dx") {
    return getGeometryPropertySetter(content.dx, aCurrentValue);
  }
  else if (aProperty=="content_dy") {
    return getGeometryPropertySetter(content.dy, aCurrentValue);
  }
  else if (aProperty.substr(0,6)=="color.") {
    return getColorComponentSetter(aProperty.substr(6), foregroundColor, aCurrentValue);
  }
  else if (aProperty.substr(0,8)=="bgcolor.") {
    return getColorComponentSetter(aProperty.substr(8), backgroundColor, aCurrentValue);
  }
  // unknown
  return NULL;
}


ValueAnimatorPtr P44View::animatorFor(const string aProperty)
{
  double startValue;
  ValueSetterCB valueSetter = getPropertySetter(aProperty, startValue);
  ValueAnimatorPtr animator = ValueAnimatorPtr(new ValueAnimator(valueSetter, false)); // not self-timed
  if (animator->valid()) {
    animations.push_back(animator);
    makeDirtyAndUpdate(); // to make sure animation sequence starts
  }
  return animator->from(startValue);
}




void P44View::stopAnimations()
{
  for (AnimationsList::iterator pos = animations.begin(); pos!=animations.end(); ++pos) {
    (*pos)->stop(false); // stop with no callback
  }
  animations.clear();
}




#endif // ENABLE_ANIMATION
