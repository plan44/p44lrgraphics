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

#ifndef _p44lrgraphics_viewfactory_hpp__
#define _p44lrgraphics_viewfactory_hpp__

#include "p44lrg_common.hpp"

#include "p44view.hpp"

#if ENABLE_P44SCRIPT
#include "p44script.hpp"
#endif

#if ENABLE_VIEWCONFIG

#include "jsonobject.hpp"

namespace p44 {

  /// factory function to create a view (but also allows reconfiguration if view already exists)
  /// @param aViewConfig configuration JSON object, must contain a 'type' field when aNewView is NULL on entry
  /// @param aNewView will receive the new view, if 'type' is specified (replacing any previous view).
  ///   Otherwise, it can already contain a view which is then just re-configured by aViewConfig.
  /// @param aParentView the parent view. This is always applied to aNewView, even if just reconfiguring an existing view
  /// @return OK or error
  ErrorPtr createViewFromConfig(JsonObjectPtr aViewConfig, P44ViewPtr &aNewView, P44ViewPtr aParentView);

  #if ENABLE_JSON_APPLICATION
  /// factory function possibly reading from resource
  /// @param aResourceOrObj if this is a single JSON string ending on ".json", it is treated as a resource file name
  ///    which is loaded and used as view configuration. All other JSON is used as view config as-is
  /// @param aResourcePrefix will be prepended to aResourceOrObj when it is a filename
  /// @param aNewView will receive the new view, if 'type' is specified (replacing any previous view).
  ///   Otherwise, it can already contain a view which is then just re-configured by aViewConfig.
  /// @param aParentView the parent view. This is always applied to aNewView, even if just reconfiguring an existing view
  /// @return OK or error
  ErrorPtr createViewFromResourceOrObj(JsonObjectPtr aResourceOrObj, const string aResourcePrefix, P44ViewPtr &aNewView, P44ViewPtr aParentView);
  #endif // ENABLE_JSON_APPLICATION

  /// register a view to the factory
  void registerView(const char* aViewType, ViewConstructor aViewConstructor);

} // namespace p44


#endif // ENABLE_VIEWCONFIG

#endif /* _p44lrgraphics_viewfactory_hpp__ */
