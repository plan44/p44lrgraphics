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

#ifndef _p44lrgraphics_viewfactory_hpp__
#define _p44lrgraphics_viewfactory_hpp__

#include "p44lrg_common.hpp"

#if ENABLE_VIEWCONFIG

#include "jsonobject.hpp"

#include "p44script.hpp"
#include "expressions.hpp" // TODO: legacy, remove it later

namespace p44 {

  /// factory function
  ErrorPtr createViewFromConfig(JsonObjectPtr aViewConfig, P44ViewPtr &aNewView, P44ViewPtr aParentView);

  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a view of a P44lrgraphics view hierarchy
    class P44lrgViewObj : public P44Script::StructuredLookupObject
    {
      typedef P44Script::StructuredLookupObject inherited;
      P44ViewPtr mView;
    public:
      P44lrgViewObj(P44ViewPtr aView);
      virtual string getAnnotation() const P44_OVERRIDE { return "lrgView"; };
      P44ViewPtr view() { return mView; }
    };

    /// represents the global objects related to p44lrgraphics
    class P44lrgLookup : public BuiltInMemberLookup
    {
      typedef BuiltInMemberLookup inherited;
      P44ViewPtr mRootView;
    public:
      P44lrgLookup(P44ViewPtr aRootView);
      P44ViewPtr rootView() { return mRootView; }
    };

  } // namespace P44Script
  #endif

  #if EXPRESSION_SCRIPT_SUPPORT
  // TODO: remove legacy EXPRESSION_SCRIPT_SUPPORT later
  bool evaluateViewFunctions(EvaluationContext* aEvalContext, const string &aFunc, const FunctionArguments &aArgs, ExpressionValue &aResult, P44ViewPtr aRootView, ValueLookupCB aSubstitutionValueLookupCB);
  #endif

} // namespace p44


#endif // ENABLE_VIEWCONFIG

#endif /* _p44lrgraphics_viewfactory_hpp__ */
