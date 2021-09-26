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
  canvasBuffer(NULL),
  numPixels(0)
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
  if (canvasBuffer) {
    // ...but make all pixels transparent
    for (PixelCoord i=0; i<numPixels; ++i) canvasBuffer[i] = transparent;
  }
}


void CanvasView::clearData()
{
  // free the buffer if allocated
  if (canvasBuffer) {
    delete[] canvasBuffer;
    canvasBuffer = NULL;
    numPixels = 0;
  }
}


void CanvasView::resize()
{
  clearData();
  if (content.dx>0 && content.dy>0) {
    numPixels = content.dx*content.dy;
    canvasBuffer = new PixelColor[numPixels];
    clear();
  }
}


void CanvasView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  if (aOldContent.dx!=content.dx || aOldContent.dy!=content.dy) {
    resize(); // size changed: clear and resize canvas
  }
  inherited::geometryChanged(aOldFrame, aOldContent);
}


PixelColor CanvasView::contentColorAt(PixelPoint aPt)
{
  if (!canvasBuffer || !isInContentSize(aPt)) {
    return backgroundColor;
  }
  else {
    // get pixel information from canvas buffer
    return canvasBuffer[aPt.y*content.dx+aPt.x];
  }
}


void CanvasView::setPixel(PixelColor aColor, PixelCoord aPixelIndex)
{
  if (!canvasBuffer || aPixelIndex<0 || aPixelIndex>=numPixels) return;
  canvasBuffer[aPixelIndex] = aColor;
}


void CanvasView::setPixel(PixelColor aColor, PixelPoint aPixelPoint)
{
  setPixel(aColor, aPixelPoint.y*content.dx+aPixelPoint.x);
}


#if ENABLE_VIEWCONFIG

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
      setPixel(foregroundColor, p);
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG


