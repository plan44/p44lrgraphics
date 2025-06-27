//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2018-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
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
#include "fonts.hpp"

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


typedef std::map<string, ViewConstructor> AvailableViewsMap;
static AvailableViewsMap *availableViewsP = nullptr; // cannot be a static object, because it might be constructed too late

void p44::registerView(const char* aViewType, ViewConstructor aViewConstructor)
{
  if (!availableViewsP) {
    availableViewsP = new AvailableViewsMap;
  }
  (*availableViewsP)[aViewType] = aViewConstructor;
}


ErrorPtr p44::createViewFromConfig(JsonObjectPtr aViewConfig, P44ViewPtr &aNewView, P44ViewPtr aParentView)
{
  LOG(LOG_DEBUG, "createViewFromConfig: %s", aViewConfig->c_strValue());
  JsonObjectPtr o;
  if (aViewConfig->get("type", o)) {
    string vt = o->stringValue();
    auto pos = availableViewsP->find(vt);
    if (pos!=availableViewsP->end()) {
      aNewView = (pos->second)();
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


#if ENABLE_P44SCRIPT

using namespace P44Script;

// makeview(jsonconfig|filename)
FUNC_ARG_DEFS(makeview, { text|objectvalue } );
static void makeview_func(BuiltinFunctionContextPtr f)
{
  P44ViewPtr newView;
  ErrorPtr err;
  JsonObjectPtr viewCfgJSON = P44View::viewConfigFromScriptObj(f->arg(0), err);
  if (Error::isOK(err)) {
    err = createViewFromConfig(viewCfgJSON, newView, P44ViewPtr());
  }
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  f->finish(newView->newViewObj());
}


static ScriptObjPtr lrg_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, BuiltinMemberDescriptor*)
{
  P44lrgLookup* l = dynamic_cast<P44lrgLookup*>(&aMemberLookup);
  P44ViewPtr rv = l->rootView();
  if (!rv) return new AnnotatedNullValue("no root view");
  return rv->newViewObj();
}


// loadfont(filename)
FUNC_ARG_DEFS(loadfont, { text } );
static void loadfont_func(BuiltinFunctionContextPtr f)
{
  string fn = f->arg(0)->stringValue();
  // user level 1 is allowed to read everywhere
  Application::PathType ty = Application::sharedApplication()->getPathType(fn, 1, true);
  if (ty==Application::empty) {
    f->finish(new ErrorValue(ScriptError::Invalid, "no filename"));
    return;
  }
  if (ty==Application::notallowed) {
    f->finish(new ErrorValue(ScriptError::NoPrivilege, "no reading privileges for this path"));
    return;
  }
  ErrorPtr err = LrgFonts::sharedFonts().addFontFromFile(Application::sharedApplication()->resourcePath(fn));
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  f->finish();
}


static void lrgfonts_func(BuiltinFunctionContextPtr f)
{
  f->finish(LrgFonts::sharedFonts().fontsArray());
}


static void lrgviews_func(BuiltinFunctionContextPtr f)
{
  ArrayValuePtr varr = new ArrayValue();
  for (auto pos = availableViewsP->begin(); pos!=availableViewsP->end(); ++pos) {
    varr->appendMember(new StringValue(pos->first));
  }
  f->finish(varr);
}


static const BuiltinMemberDescriptor lrgGlobals[] = {
  FUNC_DEF_W_ARG(makeview, executable|objectvalue),
  FUNC_DEF_NOARG(lrgviews, executable|objectvalue),
  FUNC_DEF_W_ARG(loadfont, executable|objectvalue),
  FUNC_DEF_NOARG(lrgfonts, executable|objectvalue),
  MEMBER_DEF(lrg, builtinvalue),
  BUILTINS_TERMINATOR
};


P44lrgLookup::P44lrgLookup(P44ViewPtr *aRootViewPtrP) :
  inherited(lrgGlobals),
  mRootViewPtrP(aRootViewPtrP)
{
}

#endif // ENABLE_P44SCRIPT

#endif // ENABLE_VIEWCONFIG

