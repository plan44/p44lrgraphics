//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2020 plan44.ch / Lukas Zeller, Zurich, Switzerland
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
static const BuiltInArgDesc dot_args[] = { { numeric|undefres }, { numeric|undefres } };
static const size_t dot_numargs = sizeof(dot_args)/sizeof(BuiltInArgDesc);
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
  { "dot", executable|null, dot_numargs, dot_args, &dot_func },
//  { "line", executable|null, line_numargs, line_args, &line_func },
//  { "rect", executable|null, rect_numargs, rect_args, &rect_func },
//  { "oval", executable|null, oval_numargs, oval_args, &oval_func },
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
  if (sharedCanvasMemberLookupP==NULL) {
    sharedCanvasMemberLookupP = new BuiltInMemberLookup(canvasViewMembers);
    sharedCanvasMemberLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedCanvasMemberLookupP);
}

#endif // ENABLE_P44SCRIPT

