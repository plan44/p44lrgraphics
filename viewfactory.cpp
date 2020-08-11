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

#if ENABLE_IMAGE_SUPPORT
#include "imageview.hpp"
#endif
#include "textview.hpp"
#include "viewsequencer.hpp"
#include "viewstack.hpp"
#include "viewscroller.hpp"
#include "lightspotview.hpp"
#include "lifeview.hpp"
#include "torchview.hpp"

using namespace p44;

// MARK: ===== View factory function

ErrorPtr p44::createViewFromConfig(JsonObjectPtr aViewConfig, P44ViewPtr &aNewView, P44ViewPtr aParentView)
{
  LOG(LOG_DEBUG, "createViewFromConfig: %s", aViewConfig->c_strValue());
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
    else if (vt=="sequencer") {
      aNewView = P44ViewPtr(new ViewSequencer);
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

#if ENABLE_P44SCRIPT

using namespace P44Script;

// findview(viewlabel)
static const BuiltInArgDesc findview_args[] = { { text } };
static const size_t findview_numargs = sizeof(findview_args)/sizeof(BuiltInArgDesc);
static void findview_func(BuiltinFunctionContextPtr f)
{
  P44lrgView* v = dynamic_cast<P44lrgView*>(f->thisObj().get());
  assert(v);
  P44ViewPtr foundView = v->view()->getView(f->arg(0)->stringValue());
  if (foundView) {
    f->finish(new P44lrgView(foundView));
    return;
  }
  f->finish();
}

// configure(jsonconfig/filename))
static const BuiltInArgDesc configure_args[] = { { text } };
static const size_t configure_numargs = sizeof(configure_args)/sizeof(BuiltInArgDesc);
static void configure_func(BuiltinFunctionContextPtr f)
{
  P44lrgView* v = dynamic_cast<P44lrgView*>(f->thisObj().get());
  assert(v);
  JsonObjectPtr viewCfgJSON;
  ErrorPtr err;
  #if SCRIPTING_JSON_SUPPORT
  if (f->arg(0)->hasType(json)) {
    // is already a JSON value, use it as-is
    viewCfgJSON = f->arg(0)->jsonValue();
  }
  else
  #endif
  {
    // JSON from string or file, possibly with substitution
    string viewConfig = f->arg(0)->stringValue();
    // get the string
    if (viewConfig.c_str()[0]!='{') {
      // must be a file name, try to load it
      viewCfgJSON = Application::jsonResource(viewConfig, &err);
    }
  }
  if (Error::isOK(err)) {
    err = v->view()->configureView(viewCfgJSON);
  }
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
}

static const BuiltinMemberDescriptor viewFunctions[] = {
  { "findview", object, findview_numargs, findview_args, &findview_func },
  { "configure", null, configure_numargs, configure_args, &configure_func },
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedViewFunctionLookupP = NULL;

P44lrgView::P44lrgView(P44ViewPtr aView) :
  mView(aView)
{
  if (sharedViewFunctionLookupP==NULL) {
    sharedViewFunctionLookupP = new BuiltInMemberLookup(viewFunctions);
    sharedViewFunctionLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedViewFunctionLookupP);
}


// hsv(hue, sat, bri) // convert to webcolor string
static const BuiltInArgDesc hsv_args[] = { { numeric }, { numeric+optional }, { numeric+optional } };
static const size_t hsv_numargs = sizeof(hsv_args)/sizeof(BuiltInArgDesc);
static void hsv_func(BuiltinFunctionContextPtr f)
{
  // hsv(hue, sat, bri) // convert to webcolor string
  double h = f->arg(0)->doubleValue();
  double s = 1.0;
  double b = 1.0;
  if (f->numArgs()>1) {
    s = f->arg(1)->doubleValue();
    if (f->numArgs()>2) {
      b = f->arg(2)->doubleValue();
    }
  }
  PixelColor p = hsbToPixel(h, s, b);
  f->finish(new StringValue(pixelToWebColor(p)));
}


static ScriptObjPtr lrg_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite)
{
  P44lrgLookup* l = dynamic_cast<P44lrgLookup*>(&aMemberLookup);
  return new P44lrgView(l->rootView());
}


static const BuiltinMemberDescriptor lrgGlobals[] = {
  { "lrg", builtinmember, 0, NULL, (BuiltinFunctionImplementation)&lrg_accessor }, // Note: correct '.accessor=&lrg_accessor' form does not work with OpenWrt g++, so need ugly cast here
  { "hsv", text, hsv_numargs, hsv_args, &hsv_func },
  { NULL } // terminator
};


P44lrgLookup::P44lrgLookup(P44ViewPtr aRootView) :
  inherited(lrgGlobals),
  mRootView(aRootView)
{
}

#endif // ENABLE_P44SCRIPT


// TODO: remove legacy EXPRESSION_SCRIPT_SUPPORT later
#if EXPRESSION_SCRIPT_SUPPORT

#include "application.hpp"

bool p44::evaluateViewFunctions(EvaluationContext* aEvalContext, const string &aFunc, const FunctionArguments &aArgs, ExpressionValue &aResult, P44ViewPtr aRootView, ValueLookupCB aSubstitutionValueLookupCB)
{
  if (aFunc=="hsv" && aArgs.size()>=1 && aArgs.size()<=4) {
    // hsv(hue, sat, bri) // convert to webcolor string
    double h = aArgs[0].numValue();
    double s = 1.0;
    double b = 1.0;
    if (aArgs.size()>1) {
      s = aArgs[1].numValue();
      if (aArgs.size()>2) {
        b = aArgs[2].numValue();
      }
    }
    PixelColor p = hsbToPixel(h, s, b);
    aResult.setString(pixelToWebColor(p));
  }
  else if (aFunc=="configureview") {
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
    // next param is the json configuration
    if (aArgs[ai].notValue()) return aEvalContext->errorInArg(aArgs[ai], aResult); // return error/null from argument
    JsonObjectPtr viewCfgJSON;
    ErrorPtr err;
    #if EXPRESSION_JSON_SUPPORT
    if (aArgs[ai].isJson()) {
      // is already a JSON value, use it as-is
      viewCfgJSON = aArgs[ai].jsonValue();
    }
    else
    #endif
    {
      // JSON from string or file, possibly with substitution
      string viewConfig = aArgs[ai++].stringValue();
      if (aArgs.size()>ai) {
        // optional subsitute flag
        if (aArgs[ai].notValue()) return aEvalContext->errorInArg(aArgs[ai], aResult); // return error/null from argument
        substitute = aArgs[ai++].boolValue();
      }
      // get the string
      if (viewConfig.c_str()[0]!='{') {
        // must be a file name, try to load it
        string fpath = Application::sharedApplication()->resourcePath(viewConfig);
        ErrorPtr err = string_fromfile(fpath, viewConfig);
        if (Error::notOK(err)) return aEvalContext->throwError(err);
      }
      // substitute placeholders in the JSON
      if (substitute && aSubstitutionValueLookupCB) {
        substituteExpressionPlaceholders(viewConfig, aSubstitutionValueLookupCB, NULL);
      }
      // parse JSON
      viewCfgJSON = JsonObject::objFromText(viewConfig.c_str(), -1, &err);
      if (!viewCfgJSON) return aEvalContext->throwError(err);
    }
    // get the subview by name, if specified
    if (!viewName.empty()) {
      aRootView = aRootView->getView(viewName);
      if (!aRootView) {
        return aEvalContext->throwError(ExpressionError::NotFound, "View with label '%s' not found", viewName.c_str());
      }
    }
    // actually configure the view now
    err = aRootView->configureView(viewCfgJSON);
    if (Error::notOK(err)) return aEvalContext->throwError(err);
  }
  else {
    return false; // unknown function
  }
  return true;
}

#endif // EXPRESSION_SCRIPT_SUPPORT

#endif // ENABLE_VIEWCONFIG

