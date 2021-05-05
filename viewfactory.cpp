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

#include "extutils.hpp"

using namespace p44;

// MARK: ===== View factory functions

#if ENABLE_JSON_APPLICATION

ErrorPtr p44::createViewFromResourceOrObj(JsonObjectPtr aResourceOrObj, const string aResourcePrefix, P44ViewPtr &aNewView, P44ViewPtr aParentView)
{
  ErrorPtr err;
  JsonObjectPtr cfg = Application::jsonObjOrResource(aResourceOrObj, &err, aResourcePrefix);
  if (Error::isOK(err)) {
    err = createViewFromConfig(cfg, aNewView, aParentView);
  }
  return err;
}

#endif // ENABLE_JSON_APPLICATION

ErrorPtr p44::createViewFromConfig(JsonObjectPtr aViewConfig, P44ViewPtr &aNewView, P44ViewPtr aParentView)
{
  LOG(LOG_DEBUG, "createViewFromConfig: %s", aViewConfig->c_strValue());
  JsonObjectPtr o;
  if (aViewConfig->get("type", o)) {
    string vt = o->stringValue();
    if (vt==TextView::staticTypeName()) {
      aNewView = P44ViewPtr(new TextView);
    }
    #if ENABLE_IMAGE_SUPPORT
    else if (vt==ImageView::staticTypeName()) {
      aNewView = P44ViewPtr(new ImageView);
    }
    #endif
    #if ENABLE_EPX_SUPPORT
    else if (vt==EpxView::staticTypeName()) {
      aNewView = P44ViewPtr(new EpxView);
    }
    #endif
    else if (vt==CanvasView::staticTypeName()) {
      aNewView = CanvasViewPtr(new CanvasView);
    }
    else if (vt==ViewSequencer::staticTypeName()) {
      aNewView = P44ViewPtr(new ViewSequencer);
    }
    else if (vt==ViewStack::staticTypeName()) {
      aNewView = P44ViewPtr(new ViewStack);
    }
    else if (vt==ViewScroller::staticTypeName()) {
      aNewView = P44ViewPtr(new ViewScroller);
    }
    else if (vt==LifeView::staticTypeName()) {
      aNewView = P44ViewPtr(new LifeView);
    }
    else if (vt==TorchView::staticTypeName()) {
      aNewView = P44ViewPtr(new TorchView);
    }
    else if (vt==LightSpotView::staticTypeName()) {
      aNewView = P44ViewPtr(new LightSpotView);
    }
    else if (vt==P44View::staticTypeName()) {
      aNewView = P44ViewPtr(new P44View);
    }
    else {
      return TextError::err("unknown view type '%s'", vt.c_str());
    }
  }
  else if (!aNewView) {
    // Note: is an error only if no view was passed in
    return TextError::err("missing 'type'");
  }
  aNewView->setParent(aParentView);
  // now let view configure itself
  return aNewView->configureView(aViewConfig);
}

#endif // ENABLE_VIEWCONFIG

