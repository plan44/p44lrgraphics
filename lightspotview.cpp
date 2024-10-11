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

#include "lightspotview.hpp"
#include <math.h>

using namespace p44;


// MARK: ===== Light Spot View

static ViewRegistrar r(LightSpotView::staticTypeName(), &LightSpotView::newInstance);

LightSpotView::LightSpotView()
{
  // make sure we start dark!
  setForegroundColor(black);
  setBackgroundColor(black);
}

LightSpotView::~LightSpotView()
{
}


void LightSpotView::recalculateColoring()
{
  // enough pixels so the gradient's resolution is enough to cover the maximum frame dimension
  calculateGradient(mRadial ? max(mFrame.dx, mFrame.dy) : mFrame.dx);
  inherited::recalculateColoring();
}


void LightSpotView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  // coloring is dependent on geometry
  recalculateColoring();
  inherited::geometryChanged(aOldFrame, aOldContent);
}


void LightSpotView::setContentAppearanceSize(double aRelDx, double aRelDy)
{
  // as content size for lightspots denotes the radii, these must be 1/2 of the frame at aRelDxy == 1
  setRelativeContentSize(aRelDx/2, aRelDy/2, true);
}



PixelColor LightSpotView::contentColorAt(PixelPoint aPt)
{
  PixelColor pix = transparent;

  // aPt are coordinates from center (already offset by content frame origin)
  int numGPixels = (int)mGradientPixels.size();

  #if DEBUG_GRADIENT
  #warning // %% remove this
  if (mContent.y+aPt.y==0) {
    // render the gradient at the bottom
    return gradientPixel(mContent.x+aPt.x, false);
  }
  #endif

  // - factor relative to the size (0..1) = where within the content size
  if (mContent.dx>0) {
    double xf = mContent.dx ? (double)aPt.x/mContent.dx : 0;
    double yf = mContent.dy ? (double)aPt.y/mContent.dy : 0;
    double progress;
    if (mRadial) {
      // radial
      progress = sqrt(xf*xf+yf*yf);
    }
    else if (mEffectZoom<0 || fabs(yf)<mEffectZoom) {
      // y is in range, we can use x
      progress = fabs(xf);
    }
    else {
      // y not in range, prevent rendering
      progress = mEffectZoom;
    }
    if (mEffectZoom<0 || progress<mEffectZoom ) {
      pix = gradientPixel(progress*numGPixels, mEffectWrap);
    }
  }
  return pix;
}

