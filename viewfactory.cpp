//
//  Copyright (c) 2018-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#include "viewfactory.hpp"

#if ENABLE_VIEWCONFIG

#if ENABLE_IMAGE_SUPPORT
#include "imageview.hpp"
#endif
#include "textview.hpp"
#include "viewanimator.hpp"
#include "viewstack.hpp"
#include "viewscroller.hpp"
#include "lifeview.hpp"
#include "lightspotview.hpp"

using namespace p44;

// MARK: ===== View factory function

ErrorPtr p44::createViewFromConfig(JsonObjectPtr aViewConfig, P44ViewPtr &aNewView, P44ViewPtr aParentView)
{
  JsonObjectPtr o;
  if (aViewConfig->get("type", o)) {
    string vt = o->stringValue();
    if (vt=="text") {
      aNewView = P44ViewPtr(new TextView);
    }
    #if ENABLE_IMAGE_SUPPORT
    else if (vt=="image") {
      aNewView = P44ViewPtr(new ImageView);
    }
    #endif
    else if (vt=="animator") {
      aNewView = P44ViewPtr(new ViewAnimator);
    }
    else if (vt=="stack") {
      aNewView = P44ViewPtr(new ViewStack);
    }
    else if (vt=="scroller") {
      aNewView = P44ViewPtr(new ViewScroller);
    }
    else if (vt=="life") {
      aNewView = P44ViewPtr(new LifeView);
    }
    else if (vt=="lightspot") {
      aNewView = P44ViewPtr(new LightSpotView);
    }
    else if (vt=="plain") {
      aNewView = P44ViewPtr(new P44View);
    }
    else {
      return TextError::err("unknown view type '%s'", vt.c_str());
    }
  }
  else {
    return TextError::err("missing 'type'");
  }
  aNewView->setParent(aParentView);
  // now let view configure itself
  return aNewView->configureView(aViewConfig);
}

#endif // ENABLE_VIEWCONFIG

