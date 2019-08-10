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

using namespace p44;


// MARK: ===== Light Spot View


LightSpotView::LightSpotView()
{
}

LightSpotView::~LightSpotView()
{
}


void LightSpotView::clear()
{
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


  pix.a = 255; // opaque
  return pix;
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr LightSpotView::configureView(JsonObjectPtr aViewConfig)
{
  JsonObjectPtr o;
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
//    if (aViewConfig->get("apropertry", o)) {
//      apropertry  = o->doubleValue()*MilliSecond;
//    }
  }
  return err;
}


#endif // ENABLE_VIEWCONFIG
