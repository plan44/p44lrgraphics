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


void LightSpotView::setRelativeContentOrigin(double aRelX, double aRelY)
{
  // special version, content origin is a center of the relevant area
  setContentOrigin({
    (int)(aRelX*max(content.dx,frame.dx)+frame.dx/2),
    (int)(aRelY*max(content.dy,frame.dy)+frame.dy/2)
  });
}



void LightSpotView::recalculateColoring()
{
  gradientPixels.clear();
  if (briGradient==0 && hueGradient==0 && satGradient==0) return; // optimized
  // initial HSV
  Row3 rgb = { (double)foregroundColor.r/255, (double)foregroundColor.g/255, (double)foregroundColor.b/255 };
  Row3 hsb;
  RGBtoHSV(rgb, hsb);
  hsb[2] = (double)foregroundColor.a/255;
  Row3 resHsb;
  // now create gradient pixels covering larger extent dimension
  int numGradientPixels = radial ? max(frame.dx, frame.dy) : frame.dx;
  int extentPixels = radial ? max(extent.x, extent.y) : extent.x;
  for (int i=0; i<numGradientPixels; i++) {
    // progress within the extent (0..1)
    double pr = (double)i/extentPixels;
    // - brightness
    resHsb[2] = gradiated(hsb[2], pr, briGradient, briMode, 1, false);
    // - hue
    resHsb[0] = gradiated(hsb[0], pr, hueGradient, hueMode, 360, true);
    // - saturation
    resHsb[1] = gradiated(hsb[1], pr, satGradient, satMode, 1, false);
    // store the pixel
    PixelColor gpix = hsbToPixel(resHsb[0], resHsb[1], resHsb[2], true);
    gradientPixels.push_back(gpix);
  }
  inherited::recalculateColoring();
}



MLMicroSeconds LightSpotView::step(MLMicroSeconds aPriorityUntil)
{
  MLMicroSeconds now = MainLoop::now();
  MLMicroSeconds nextCall = inherited::step(aPriorityUntil);
  // do stuff
  MLMicroSeconds nextstepin = 100*MilliSecond;

  updateNextCall(nextCall, now+nextstepin);
  return nextCall;
}


PixelColor LightSpotView::contentColorAt(PixelCoord aPt)
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
  if (progress<1 || (contentWrapMode&clipXY)==0) {
    if (numGPixels>0) {
      int i = progress*extentPixels;
      if (i<0) i = 0; else if (i>=numGPixels) i = numGPixels-1;
      pix = gradientPixels[i];
    }
    else {
      pix = foregroundColor;
    }
  }
  return pix;
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr LightSpotView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    if (aViewConfig->get("extent_x", o)) {
      extent.x = o->doubleValue();
      makeDirty();
    }
    if (aViewConfig->get("extent_y", o)) {
      extent.y = o->doubleValue();
      makeDirty();
    }
  }
  return err;
}


#endif // ENABLE_VIEWCONFIG
