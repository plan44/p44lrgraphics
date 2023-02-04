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


#ifndef __p44lrgraphics__config__
#define __p44lrgraphics__config__

// NOTE: This is a default (template) config file only
//                 ******************
//       Usually, copy this file into a location outside p44lrgraphics in your project,
//       rename it to p44lrg_config.hpp and modify it according to
//       your needs.

#ifndef ENABLE_VIEWCONFIG
  #define ENABLE_VIEWCONFIG 0 // not by default because it pulls in JsonObject and Application
#endif

#ifndef ENABLE_VIEWSTATUS
  #define ENABLE_VIEWSTATUS ENABLE_VIEWCONFIG // by default, is enabled when JSON view config is enabled
#endif

#ifndef ENABLE_EPX_SUPPORT
  #define ENABLE_EPX_SUPPORT 0 // not by default because it pulls in JsonObject
#endif

#ifndef ENABLE_ANIMATION
  #define ENABLE_ANIMATION 0 // not by default because it pulls in ValueAnimator
#endif

#ifndef ENABLE_IMAGE_SUPPORT
  #define ENABLE_IMAGE_SUPPORT 0 // not by default because it has external dependencies
#endif


#endif // __p44lrgraphics__config__
