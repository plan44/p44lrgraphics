//  SPDX-License-Identifier: GPL-3.0-or-later
//
//  Copyright (c) 2016-2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#ifndef _p44lrgraphics_fonts_hpp__
#define _p44lrgraphics_fonts_hpp__

#include "p44lrg_common.hpp"

// font generator
//#define GENERATE_FONT_SOURCE 1

#ifdef GENERATE_FONT_SOURCE
#if !defined(DEBUG) || !defined(__APPLE__)
  #error GENERATE_FONT_SOURCE is enabled
#else
  #warning GENERATE_FONT_SOURCE is enabled
#endif
#endif

namespace p44 {

typedef struct {
  uint8_t width;
  const char *coldata; // for multi-byte columns: MSByte first. Bit order: Bit0 = top pixel
} glyph_t;

typedef struct {
  const char* prefix;
  uint8_t first;
  uint8_t last;
  uint8_t glyphOffset;
} GlyphRange;

typedef struct {
  const char* fontName; ///< name of the font
  uint8_t glyphHeight; ///< height of the glyphs in pixels (max 32)
  size_t numGlyphs; ///< total number of glyphs
  const GlyphRange* glyphRanges; ///< mapping to codepoints
  const glyph_t* glyphs; ///< actual glyphs
  const char *copyright; ///< optional copyright string
  #ifdef GENERATE_FONT_SOURCE
  const char** glyphstrings; ///< ASCII-text glyph strings
  #endif
} font_t;


/// Get glyph index from UTF8 text
/// @param aFontData the font data to consult for UTF-8 to glyph mapping
/// @param aIdx index into aText where to look for a UTF-8 char, updated past UTF-8 sequence at return
/// @param aText the UTF-8 string
int glyphIndexFromText(const font_t& aFontData, size_t &aIdx, const char* aText);


class LrgFont : public P44Obj
{
  const font_t& mFontData;

public:
  LrgFont(const font_t& aFontData);
  virtual ~LrgFont();

  virtual const font_t& fontData() { return mFontData; }
  const char* name() { return mFontData.fontName; }

};
typedef boost::intrusive_ptr<LrgFont> LrgFontPtr;


class UserLrgFont : public LrgFont
{
  typedef LrgFont inherited;

  string mName;
  font_t mLoadedFontData;

public:
  UserLrgFont();
  virtual ~UserLrgFont();

  ErrorPtr loadFromFile(const string& aFontFileName);

private:
  void reset();

};
typedef boost::intrusive_ptr<UserLrgFont> UserLrgFontPtr;


class LrgFonts
{
  typedef std::map<string, LrgFontPtr, lessStrucmp> FontsMap;
  FontsMap mAvailableFonts;

  LrgFonts(); // private constructor

public:
  static LrgFonts& sharedFonts();

  LrgFontPtr fontByName(const string aFontName);

  ErrorPtr addFontFromFile(const string& aFontFileName);
  void addFont(LrgFontPtr aFont);

  #if ENABLE_P44SCRIPT
  P44Script::ScriptObjPtr fontsArray();
  #endif

  #ifdef GENERATE_FONT_SOURCE
  void generate_all_builtin_fonts_to_tmp();
  #endif

};


class BuiltinFontRegistrar
{
public:
  BuiltinFontRegistrar(const font_t& aFontData);
};




} // namespace p44

#endif /* _p44lrgraphics_fonts_hpp__ */
