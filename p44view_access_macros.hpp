//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2016-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#if ENABLE_P44SCRIPT
  #define CONCAT_HELPER(x, y) x ## y
  #define CONCAT(x, y) CONCAT_HELPER(x, y)

  // macros for synthesizing property accessors
  #define ACCESSOR_NAME CONCAT(ACCESSOR_CLASS, _accessor)
  // definition of the class-specific access function type
  #define ACCFN ACCESSOR_CLASS##_AccFn
  #define ACCFN_DEF typedef P44Script::ScriptObjPtr (*ACCFN)(ACCESSOR_CLASS& aView, P44Script::ScriptObjPtr aToWrite);
  // member declaration
  #define ACC_DECL(field, types, prop) \
    { field, builtinmember|types, 0, .memberAccessInfo=(void*)&access_##prop, .accessor=&ACCESSOR_NAME }
  // member access implementer
  #define ACC_IMPL(prop, getter, constructor) \
    static ScriptObjPtr access_##prop(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite) \
    { \
      if (!aToWrite) return new constructor(aView.get##prop()); \
      aView.set##prop(aToWrite->getter()); \
      return aToWrite; /* reflect back to indicate writable */ \
    }
  #define ACC_IMPL_RO(prop, constructor) \
    static ScriptObjPtr access_##prop(ACCESSOR_CLASS& aView, ScriptObjPtr aToWrite) \
    { \
      if (!aToWrite) return new constructor(aView.get##prop()); \
      return nullptr; /* null to indicate readonly */ \
    }
  #define ACC_IMPL_STR(prop) ACC_IMPL(prop, stringValue, StringValue)
  #define ACC_IMPL_DBL(prop) ACC_IMPL(prop, doubleValue, NumericValue)
  #define ACC_IMPL_INT(prop) ACC_IMPL(prop, intValue, IntegerValue)
  #define ACC_IMPL_BOOL(prop) ACC_IMPL(prop, boolValue, BoolValue)
  #define ACC_IMPL_RO_STR(prop) ACC_IMPL_RO(prop, StringValue)
  #define ACC_IMPL_RO_DBL(prop) ACC_IMPL_RO(prop, NumericValue)
  #define ACC_IMPL_RO_INT(prop) ACC_IMPL_RO(prop, IntegerValue)
#endif // ENABLE_P44SCRIPT
