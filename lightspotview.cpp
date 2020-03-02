//
//  Copyright (c) 2017-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
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
  calculateGradient(radial ? max(frame.dx, frame.dy) : frame.dx,  radial ? max(extent.x, extent.y) : extent.x);
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
  int numGPixels = (int)gradientPixels.size();
  // - factor relative to the size (0..1)
  double xf = (double)aPt.x/extent.x;
  int extentPixels;
  double progress;
  if (radial) {
    // radial
    double yf = (double)aPt.y/extent.y;
    extentPixels = max(extent.x, extent.y);
    progress = sqrt(xf*xf+yf*yf);
  }
  else {
    // linear
    extentPixels = extent.x;
    progress = fabs(xf);
  }
  if (progress<1 || (progress==1 && xf<0)  || (contentWrapMode&clipXY)==0) {
    if (numGPixels>0) {
      int i = progress*extentPixels;
      pix = gradientPixel(i);
    }
    else {
      pix = foregroundColor;
    }
  }
  return pix;
}

