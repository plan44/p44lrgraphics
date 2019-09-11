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
#include "lightspotview.hpp"
#include "lifeview.hpp"
#include "torchview.hpp"

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
    else if (vt=="torch") {
      aNewView = P44ViewPtr(new TorchView);
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


// MARK: - script support

#if EXPRESSION_SCRIPT_SUPPORT

#include "application.hpp"

bool p44::evaluateViewFunctions(EvaluationContext* aEvalContext, const string &aFunc, const FunctionArguments &aArgs, ExpressionValue &aResult, P44ViewPtr aRootView, ValueLookupCB aSubstitutionValueLookupCB)
{
  if (aFunc=="configureview") {
    // configureview([viewname,]jsonconfig/filename[,substitute])
    string viewName;
    bool substitute = true;
    int ai = 0;
    if (aArgs.size()>1 && aArgs[1].isString()) {
      // first param is the view name
      if (aArgs[ai].notValue()) return aEvalContext->errorInArg(aArgs[ai], aResult); // return error/null from argument
      viewName = aArgs[ai].stringValue();
      ai++;
    }
    // next param is the json string
    if (aArgs[ai].notValue()) return aEvalContext->errorInArg(aArgs[ai], aResult); // return error/null from argument
    string viewConfig = aArgs[ai++].stringValue();
    if (aArgs.size()>ai) {
      // optional subsitute flag
      if (aArgs[ai].notValue()) return aEvalContext->errorInArg(aArgs[ai], aResult); // return error/null from argument
      substitute = aArgs[ai++].boolValue();
    }
    // get the subview by name, if specified
    if (!viewName.empty()) {
      aRootView = aRootView->getView(viewName);
      if (!aRootView) {
        return aEvalContext->throwError(ExpressionError::NotFound, "View with label '%s' not found", viewName.c_str());
      }
    }
    // get the string
    if (viewConfig.c_str()[0]!='{') {
      // must be a file name, try to load it
      string fpath = Application::sharedApplication()->resourcePath(viewConfig);
      FILE* f = fopen(fpath.c_str(), "r");
      if (f==NULL) {
        ErrorPtr err = SysError::errNo();
        err->prefixMessage("cannot open JSON view config file '%s': ", fpath.c_str());
        return aEvalContext->throwError(err);
      }
      string_fgetfile(f, viewConfig);
      fclose(f);
    }
    // substitute placeholders in the JSON
    if (substitute) {
      substituteExpressionPlaceholders(viewConfig, aSubstitutionValueLookupCB, NULL);
    }
    // parse JSON
    ErrorPtr err;
    JsonObjectPtr viewCfgJSON = JsonObject::objFromText(viewConfig.c_str(), -1, &err);
    if (!viewCfgJSON) return aEvalContext->throwError(err);
    // actually configure the view now
    err = aRootView->configureView(viewCfgJSON);
    if (Error::notOK(err)) return aEvalContext->throwError(err);
    return true;
  }
  else {
    return false; // unknown function
  }
}

#endif // EXPRESSION_SCRIPT_SUPPORT

#endif // ENABLE_VIEWCONFIG

