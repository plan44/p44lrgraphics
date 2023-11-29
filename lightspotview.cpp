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
  calculateGradient(mRadial ? max(mFrame.dx, mFrame.dy) : mFrame.dx,  mRadial ? max(mExtent.x, mExtent.y) : mExtent.x);
  inherited::recalculateColoring();
}


void LightSpotView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  // coloring is dependent on geometry
  recalculateColoring();
  inherited::geometryChanged(aOldFrame, aOldContent);
}


PixelColor LightSpotView::contentColorAt(PixelPoint aPt)
{
  PixelColor pix = transparent;

  // aPt are coordinates from center (already offset by content frame origin)
  int numGPixels = (int)mGradientPixels.size();
  // - factor relative to the size (0..1)
  double xf = (double)aPt.x/mExtent.x;
  int extentPixels;
  double progress;
  if (mRadial) {
    // radial
    double yf = (double)aPt.y/mExtent.y;
    extentPixels = max(mExtent.x, mExtent.y);
    progress = sqrt(xf*xf+yf*yf);
  }
  else {
    // linear
    extentPixels = mExtent.x;
    progress = fabs(xf);
  }
  if (progress<1 || (progress==1 && xf<0)  || (mFramingMode&clipXY)==0) {
    if (numGPixels>0) {
      int i = progress*extentPixels;
      pix = gradientPixel(i);
    }
    else {
      pix = mForegroundColor;
    }
  }
  return pix;
}

