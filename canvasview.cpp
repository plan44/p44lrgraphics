//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2020-2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
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


#include "canvasview.hpp"

using namespace p44;

// MARK: ===== CanvasView

CanvasView::CanvasView() :
  mCanvasBuffer(NULL),
  mNumPixels(0)
{
}


CanvasView::~CanvasView()
{
  clearData();
}


void CanvasView::clear()
{
  stopAnimations();
  // no inherited::clear(), we want to retain the canvas itself...
  if (mCanvasBuffer) {
    // ...but make all pixels transparent
    for (PixelCoord i=0; i<mNumPixels; ++i) mCanvasBuffer[i] = transparent;
  }
}


void CanvasView::clearData()
{
  // free the buffer if allocated
  if (mCanvasBuffer) {
    delete[] mCanvasBuffer;
    mCanvasBuffer = NULL;
    mNumPixels = 0;
  }
}


void CanvasView::resize()
{
  clearData();
  if (mContent.dx>0 && mContent.dy>0) {
    mNumPixels = mContent.dx*mContent.dy;
    mCanvasBuffer = new PixelColor[mNumPixels];
    clear();
  }
}


void CanvasView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  if (aOldContent.dx!=mContent.dx || aOldContent.dy!=mContent.dy) {
    resize(); // size changed: clear and resize canvas
  }
  inherited::geometryChanged(aOldFrame, aOldContent);
}


#define GRADIENT_SIZE 256

void CanvasView::recalculateColoring()
{
  calculateGradient(GRADIENT_SIZE);
  inherited::recalculateColoring();
}


PixelColor CanvasView::contentColorAt(PixelPoint aPt)
{
  if (!mCanvasBuffer || !isInContentSize(aPt)) {
    return mBackgroundColor;
  }
  else {
    // get pixel information from canvas buffer
    return mCanvasBuffer[aPt.y*mContent.dx+aPt.x];
  }
}


void CanvasView::setPixel(PixelColor aColor, PixelCoord aPixelIndex)
{
  if (!mCanvasBuffer || aPixelIndex<0 || aPixelIndex>=mNumPixels) return;
  mCanvasBuffer[aPixelIndex] = aColor;
  makeDirty();
}


void CanvasView::setPixel(PixelColor aColor, PixelPoint aPixelPoint)
{
  setPixel(aColor, aPixelPoint.y*mContent.dx+aPixelPoint.x);
}


void CanvasView::drawLine(PixelPoint aStart, PixelPoint aEnd)
{
  // van Bresenham balancing positive and negative error between x and y
  PixelCoord dx = abs(aEnd.x-aStart.x);
  int sign_x = aStart.x<aEnd.x ? 1 : -1;
  PixelCoord dy = -abs(aEnd.y-aStart.y);
  int sign_y = aStart.y<aEnd.y ? 1 : -1;
  int error = dx+dy;
  int m = max(dx, -dy);
  double progressfactor = m ? (double)GRADIENT_SIZE/m : 0;
  int steps = 0;
  while (true) {
    setPixel(gradientPixel(progressfactor*steps, false), aStart);
    steps++;
    if (aStart.x==aEnd.x && aStart.y==aEnd.y) break;
    int e2 = error*2;
    if (e2>=dy) {
      if (aStart.x==aEnd.x) break;
      error += dy;
      aStart.x += sign_x;
    }
    if (e2<=dx) {
      if (aStart.y==aEnd.y) break;
      error += dx;
      aStart.y += sign_y;
    }
  }
}


void CanvasView::copyPixels(P44ViewPtr aSourceView, bool aFromContent, PixelRect aSrcRect, PixelPoint aDestOrigin, bool aNonTransparentOnly)
{
  bool sameview;
  if (!aSourceView) {
    aSourceView = this; // use myself
    sameview = true;
  }
  else {
    sameview = aSourceView.get()==this;
  }
  normalizeRect(aSrcRect);
  PixelPoint dist;
  dist.x = aDestOrigin.x-aSrcRect.x;
  dist.y = aDestOrigin.y-aSrcRect.y;
  // determine copy direction
  int xdir = !sameview || dist.x<0 ? 1 : -1; // start at left when moving left
  int ydir = !sameview || dist.y<0 ? 1 : -1; // start at bottom when moving down
  PixelPoint src;
  src.y = aSrcRect.y + (ydir<0 ? aSrcRect.dy-1 : 0);
  for (PixelCoord ny = aSrcRect.dy; ny>0; ny--) {
    src.x = aSrcRect.x + (xdir<0 ? aSrcRect.dx-1 : 0);
    for (PixelCoord nx = aSrcRect.dx; nx>0; nx--) {
      PixelColor pix;
      // get pixel
      if (aFromContent) pix = aSourceView->contentColorAt(src);
      else pix = aSourceView->colorInFrameAt(src);
      if (!aNonTransparentOnly || pix.a>0) {
        // put pixel to canvas at new position
        setPixel(pix, { src.x+dist.x, src.y+dist.y });
      }
      src.x += xdir;
    }
    src.y += ydir;
  }
  makeDirty();
}


#if ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT

ErrorPtr CanvasView::configureView(JsonObjectPtr aViewConfig)
{
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    bool draw = false;
    PixelPoint p;
    p.x = 0;
    p.y = 0;
    if (aViewConfig->get("setx", o)) {
      p.x = o->int32Value();
      draw = true;
    }
    if (aViewConfig->get("sety", o)) {
      p.y = o->int32Value();
      draw = true;
    }
    if (draw) {
      setPixel(mForegroundColor, p);
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT

#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr CanvasView::newViewObj()
{
  return new CanvasViewObj(this);
}

#if P44SCRIPT_FULL_SUPPORT

// dot(x,y)
FUNC_ARG_DEFS(dot, { numeric }, { numeric } );
static void dot_func(BuiltinFunctionContextPtr f)
{
  CanvasViewObj* v = dynamic_cast<CanvasViewObj*>(f->thisObj().get());
  assert(v);
  PixelPoint pt;
  pt.x = f->arg(0)->intValue();
  pt.y = f->arg(1)->intValue();
  v->canvas()->setPixel(v->canvas()->getForegroundColor(), pt);
  f->finish(v); // allow chaining
}


// line(x0,y0,x1,x1)
FUNC_ARG_DEFS(line, { numeric }, { numeric }, { numeric }, { numeric } );
static void line_func(BuiltinFunctionContextPtr f)
{
  CanvasViewObj* v = dynamic_cast<CanvasViewObj*>(f->thisObj().get());
  assert(v);
  PixelPoint start, end;
  start.x = f->arg(0)->intValue();
  start.y = f->arg(1)->intValue();
  end.x = f->arg(2)->intValue();
  end.y = f->arg(3)->intValue();
  v->canvas()->drawLine(start, end);
  f->finish(v); // allow chaining
}


// copy([sourceview ,] x, y, dx, dy, dest_x, dest_y [, transparentonly [, fromContent])
FUNC_ARG_DEFS(copy, { anyvalid }, { numeric }, { numeric }, { numeric }, { numeric }, { numeric }, { numeric|optionalarg }, { numeric|optionalarg }, { numeric|optionalarg } );
static void copy_func(BuiltinFunctionContextPtr f)
{
  CanvasViewObj* v = dynamic_cast<CanvasViewObj*>(f->thisObj().get());
  assert(v);
  PixelRect src;
  PixelPoint dest;
  P44ViewPtr foreignview;
  int ai = 0;
  if (!f->arg(ai)->hasType(numeric)) {
    // must be a view
    P44lrgViewObj* vo = dynamic_cast<P44lrgViewObj*>(f->arg(ai).get());
    if (!vo) {
      f->finish(new ErrorValue(ScriptError::Invalid, "first argument must be number or view"));
      return;
    }
    foreignview = vo->view();
    ai++;
  }
  src.x = f->arg(ai++)->intValue();
  src.y = f->arg(ai++)->intValue();
  src.dx = f->arg(ai++)->intValue();
  src.dy = f->arg(ai++)->intValue();
  dest.x = f->arg(ai++)->intValue();
  dest.y = f->arg(ai++)->intValue();
  bool transparentOnly = f->arg(ai++)->boolValue();
  bool fromContent = f->arg(ai++)->boolValue();
  v->canvas()->copyPixels(foreignview, fromContent, src, dest, transparentOnly);
  f->finish(v); // allow chaining
}


#endif // P44SCRIPT_FULL_SUPPORT

#define ACCESSOR_CLASS CanvasView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  CanvasViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<CanvasViewObj*>(aParentObj.get())->canvas().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_RO_INT(CanvasBytes);
ACC_IMPL_RO_INT(NumPixels);

static const BuiltinMemberDescriptor canvasViewMembers[] = {
  #if P44SCRIPT_FULL_SUPPORT
  FUNC_DEF_W_ARG(dot, executable|null),
  FUNC_DEF_W_ARG(line, executable|null),
  FUNC_DEF_W_ARG(copy, executable|null),
//  FUNC_DEF_W_ARG(rect, executable|null),
//  FUNC_DEF_W_ARG(oval, executable|null),
  #endif
  // property accessors
  ACC_DECL("bytes", numeric, CanvasBytes),
  ACC_DECL("pixels", numeric, NumPixels),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedCanvasMemberLookupP = NULL;

CanvasViewObj::CanvasViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  registerSharedLookup(sharedCanvasMemberLookupP, canvasViewMembers);
}

#endif // ENABLE_P44SCRIPT

