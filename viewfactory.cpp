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


// MARK: - script support

#if P44SCRIPT_FULL_SUPPORT

using namespace P44Script;

// findview(viewlabel)
static const BuiltInArgDesc findview_args[] = { { text } };
static const size_t findview_numargs = sizeof(findview_args)/sizeof(BuiltInArgDesc);
static void findview_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  P44ViewPtr foundView = v->view()->getView(f->arg(0)->stringValue());
  if (foundView) {
    f->finish(new P44lrgViewObj(foundView));
    return;
  }
  f->finish();
}


// addview(view)
static const BuiltInArgDesc addview_args[] = { { object } };
static const size_t addview_numargs = sizeof(addview_args)/sizeof(BuiltInArgDesc);
static void addview_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  P44lrgViewObj* subview = dynamic_cast<P44lrgViewObj*>(f->arg(0).get());
  if (!subview) {
    f->finish(new ErrorValue(ScriptError::Invalid, "argument must be a view"));
    return;
  }
  if (!v->view()->addSubView(subview->view())) {
    f->finish(new ErrorValue(ScriptError::Invalid, "cannot add subview here"));
    return;
  }
  f->finish(subview);
}



// configure(jsonconfig/filename)
static const BuiltInArgDesc configure_args[] = { { text|json|object } };
static const size_t configure_numargs = sizeof(configure_args)/sizeof(BuiltInArgDesc);
static void configure_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
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
    // JSON from string (or file if we have a JSON app)
    string viewConfig = f->arg(0)->stringValue();
    #if ENABLE_JSON_APPLICATION
    viewCfgJSON = Application::jsonObjOrResource(viewConfig, &err);
    #else
    viewCfgJSON = JsonObject::objFromText(viewConfig.c_str(), -1, &err);
    #endif
  }
  if (Error::isOK(err)) {
    err = v->view()->configureView(viewCfgJSON);
    if (Error::isOK(err)) {
      v->view()->requestUpdate(); // to make sure changes are applied
    }
  }
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  f->finish(v); // return view itself to allow chaining
}


#if ENABLE_VIEWSTATUS

// status()
static void status_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  f->finish(new JsonValue(v->view()->viewStatus()));
}

#endif

// set(propertyname, newvalue)   convenience function to set a single property
static const BuiltInArgDesc set_args[] = { { text }, { any } };
static const size_t set_numargs = sizeof(set_args)/sizeof(BuiltInArgDesc);
static void set_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  JsonObjectPtr viewCfgJSON = JsonObject::newObj();
  viewCfgJSON->add(f->arg(0)->stringValue().c_str(), f->arg(1)->jsonValue());
  if (Error::ok(v->view()->configureView(viewCfgJSON))) {
    v->view()->requestUpdate(); // to make sure changes are applied
  }
  f->finish(v); // return view itself to allow chaining
}


// stopanimations()     stop animations
static void stopanimations_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->stopAnimations();
  f->finish(v); // return view itself to allow chaining
}


// remove()     remove from subview
static void remove_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  bool removed = v->view()->removeFromParent();
  f->finish(new NumericValue(removed)); // true if actually removed
}



// render()     trigger rendering
static void render_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->requestUpdate(); // to make sure changes are applied
  f->finish(v); // return view itself to allow chaining
}


// parent()     return parent view
static void parent_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  P44ViewPtr parent = v->view()->getParent();
  if (!parent) {
    f->finish(new AnnotatedNullValue("no parent view"));
    return;
  }
  f->finish(new P44lrgViewObj(parent));
}


// clear()     clear view
static void clear_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  v->view()->clear(); // to make sure changes are applied
  f->finish(v); // return view itself to allow chaining
}



#if ENABLE_ANIMATION

// animator(propertyname)
static const BuiltInArgDesc animator_args[] = { { text } };
static const size_t animator_numargs = sizeof(animator_args)/sizeof(BuiltInArgDesc);
static void animator_func(BuiltinFunctionContextPtr f)
{
  P44lrgViewObj* v = dynamic_cast<P44lrgViewObj*>(f->thisObj().get());
  assert(v);
  f->finish(new ValueAnimatorObj(v->view()->animatorFor(f->arg(0)->stringValue())));
}

#endif


static const BuiltinMemberDescriptor viewFunctions[] = {
  { "findview", executable|object, findview_numargs, findview_args, &findview_func },
  { "addview", executable|object, addview_numargs, addview_args, &addview_func },
  { "configure", executable|object, configure_numargs, configure_args, &configure_func },
  { "set", executable|object, set_numargs, set_args, &set_func },
  { "render", executable|object, 0, NULL, &render_func },
  { "remove", executable|numeric, 0, NULL, &remove_func },
  { "parent", executable|object, 0, NULL, &parent_func },
  { "clear", executable|object, 0, NULL, &clear_func },
  #if ENABLE_ANIMATION
  { "animator", executable|object, animator_numargs, animator_args, &animator_func },
  { "stopanimations", executable|object, 0, NULL, &stopanimations_func },
  #endif
  #if ENABLE_VIEWSTATUS
  { "status", executable|json|object, 0, NULL, &status_func },
  #endif
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedViewFunctionLookupP = NULL;

P44lrgViewObj::P44lrgViewObj(P44ViewPtr aView) :
  mView(aView)
{
  if (sharedViewFunctionLookupP==NULL) {
    sharedViewFunctionLookupP = new BuiltInMemberLookup(viewFunctions);
    sharedViewFunctionLookupP->isMemberVariable(); // disable refcounting
  }
  registerMemberLookup(sharedViewFunctionLookupP);
}


// hsv(hue, sat, bri) // convert to webcolor string
static const BuiltInArgDesc hsv_args[] = { { numeric }, { numeric+optionalarg }, { numeric+optionalarg } };
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


// makeview(viewconfig)
static const BuiltInArgDesc makeview_args[] = { { json|object } };
static const size_t makeview_numargs = sizeof(makeview_args)/sizeof(BuiltInArgDesc);
static void makeview_func(BuiltinFunctionContextPtr f)
{
  P44ViewPtr newView;
  ErrorPtr err = createViewFromConfig(f->arg(0)->jsonValue(), newView, P44ViewPtr());
  f->finish(new P44lrgViewObj(newView));
}



static ScriptObjPtr lrg_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite)
{
  P44lrgLookup* l = dynamic_cast<P44lrgLookup*>(&aMemberLookup);
  P44ViewPtr rv = l->rootView();
  if (!rv) return new AnnotatedNullValue("no root view");
  return new P44lrgViewObj(rv);
}


static const BuiltinMemberDescriptor lrgGlobals[] = {
  { "makeview", executable|object, makeview_numargs, makeview_args, &makeview_func },
  { "lrg", builtinmember, 0, NULL, (BuiltinFunctionImplementation)&lrg_accessor }, // Note: correct '.accessor=&lrg_accessor' form does not work with OpenWrt g++, so need ugly cast here
  { "hsv", executable|text, hsv_numargs, hsv_args, &hsv_func },
  { NULL } // terminator
};


P44lrgLookup::P44lrgLookup(P44ViewPtr *aRootViewPtrP) :
  inherited(lrgGlobals),
  mRootViewPtrP(aRootViewPtrP)
{
}

#endif // P44SCRIPT_FULL_SUPPORT


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

