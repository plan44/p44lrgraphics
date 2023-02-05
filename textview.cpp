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

#include "textview.hpp"

using namespace p44;


// MARK: fonts

// font generator
//#define GENERATE_FONT_SOURCE font_5x7
#define GLYPHSTRINGS_VAR font_5x7_glyphstrings

#ifdef GENERATE_FONT_SOURCE
#if !defined(DEBUG) || !defined(__APPLE__)
  #error GENERATE_FONT_SOURCE is enabled
#else
  #warning GENERATE_FONT_SOURCE is enabled
#endif
#endif

// default font
#define DEFAULT_FONT font_5x7

// Font data includes
#include "fonts/font_5x7.cpp"
#include "fonts/font_5x8.cpp"

// Font table
const font_t* fonts[] {
  &font_5x7,
  &font_5x8,
  nullptr
};


// MARK: UTF-8 codepoint to Glyph mapping

const int placeholderGlyphNo = 0; // placeholder must be the first glyph

/// @return glyph number or -1 if EOT
static int glyphNoFromText(const font_t& aFont, size_t &aIdx, const char* aText) {
  string prefix;
  uint8_t textbyte = aText[aIdx++];
  if (!textbyte) return -1; // end of text
  if (textbyte>=0xC0) {
    // start of UTF8 sequence, get prefix
    while ((aText[aIdx]&0xC0)==0x80) {
      // next is still a continuation
      prefix += textbyte;
      textbyte = aText[aIdx++];
      if (!textbyte) return -1; // invalid sequence ends text
    }
  }
  else if (textbyte>=0x80) {
    return placeholderGlyphNo; // UTF-8 continuation without start -> invalid
  }
  // we have the prefix and the last char
  const GlyphRange* grP = aFont.glyphRanges;
  while (grP->prefix) {
    if (prefix == grP->prefix) {
      // prefix matches
      if (textbyte>=grP->first && textbyte<=grP->last) {
        // match
        return grP->glyphOffset+(textbyte-grP->first);
      }
    }
    grP++;
  }
  // no match
  return placeholderGlyphNo;
}


#ifdef GENERATE_FONT_SOURCE

static void renderGlyphTextPixels(int aGlyphNo, const font_t& aFont, FILE* aOutputFile)
{
  const glyph_t &glyph = aFont.glyphs[aGlyphNo];
  int colbytes = (aFont.glyphHeight+7)>>3;
  for (int i=0; i<glyph.width; i++) {
    // one column, bit 0 is topmost pixel, we want to print that last
    string colstr;
    for(int colbit = aFont.glyphHeight-1; colbit>=0; --colbit) {
      // bytes of one column are MSB first in coldata
      bool bitset = glyph.coldata[(i+1)*colbytes-1-(colbit>>3)] & (1<<(colbit&0x07));
      colstr += bitset ? "X" : ".";
    }
    string colhex;
    for(int cbidx = 0; cbidx<colbytes; ++cbidx) {
      string_format_append(colhex, " %02X", (uint8_t)(glyph.coldata[i*colbytes+cbidx]));
    }
    fprintf(aOutputFile, "  \"\\n\"   \"%s\" %c //%s\n", colstr.c_str(), i==glyph.width-1 ? ',' : ' ', colhex.c_str());
  }
  fprintf(aOutputFile, "\n");
}


static void fontAsGlyphStrings(const font_t& aFont, FILE* aOutputFile)
{
  fprintf(aOutputFile, "\n// MARK: - '%s' generated font verification data\n", aFont.fontName);
  fprintf(aOutputFile, "\nstatic const char * %s_glyphstrings[] = {\n", aFont.fontName);
  fprintf(aOutputFile, "  \"placeholder\" /* 0x00 - Glyph 0 */\n\n");
  renderGlyphTextPixels(placeholderGlyphNo, aFont, aOutputFile);
  for (const GlyphRange* grP = aFont.glyphRanges; grP->prefix; grP++) {
    int glyphno = grP->glyphOffset;
    for (uint8_t lastchar = grP->first; lastchar<=grP->last; lastchar++) {
      string chardesc = grP->prefix;
      chardesc += lastchar;
      string codedesc = "UTF-8";
      for (size_t i=0; i<chardesc.size(); i++) string_format_append(codedesc, " %02X", (uint8_t)chardesc[i]);
      if (chardesc.size()==1) {
        if (lastchar=='\\') chardesc = "\\\\";
        else if (lastchar=='"') chardesc = "\\\"";
        else if (lastchar<0x20 || lastchar>0x7E) {
          chardesc = string_format("\\x%02x", lastchar);
        }
      }
      fprintf(aOutputFile, "  \"%s\" /* %s - Glyph %d */\n\n", chardesc.c_str(), codedesc.c_str(), glyphno);
      renderGlyphTextPixels(glyphno, aFont, aOutputFile);
      glyphno++;
    }
  }
  fprintf(aOutputFile, "  nullptr // terminator\n");
  fprintf(aOutputFile, "};\n\n");
  fprintf(aOutputFile, "// MARK: - end of generated font verification data\n\n");
}


typedef std::pair<string, string> StringPair;
static bool mapcmp(const StringPair &a, const StringPair &b)
{
  return a.first < b.first;
}

static bool glyphStringsToFont(const char** aGlyphStrings, const char* aFontName, FILE* aOutputFile)
{
  fprintf(aOutputFile, "\n\n// MARK: - '%s' generated font data\n", aFontName);
  fprintf(aOutputFile, "\nstatic const glyph_t %s_glyphs[] = {\n", aFontName);
  // UTF8 mappings
  typedef std::list<StringPair> GlyphMap;
  GlyphMap gm;
  int glyphHeight = -1;
  int gh = 0;
  for (int g = 0; aGlyphStrings[g]!=nullptr; ++g) {
    const char* bs0 = aGlyphStrings[g];
    const char* bs = bs0;
    // first string up to newline is character, possibly UTF8
    while (*bs && *bs!='\n') bs++;
    string code;
    code.assign(bs0, bs-bs0);
    bs++; // skip LF
    // create character definition
    int w = 0;
    string chr = "\"";
    while (*bs) {
      // new glyph column
      w++;
      uint64_t pixmap = 0;
      gh = 0;
      while (*bs && *bs!='\n') {
        pixmap = (pixmap<<1) | (*bs!='.' && *bs!=' ' ? 0x01 : 0x00);
        gh++;
        bs++;
      }
      // first line determines height, all others must match
      if (glyphHeight<0) glyphHeight = gh;
      for (int i=(glyphHeight-1)>>3; i>=0; --i) {
        string_format_append(chr, "\\x%02x", (unsigned int)(pixmap>>(i*8))&0xFF);
      }
      if (*bs=='\n') bs++;
    }
    chr += "\"";
    string codedesc = code; // default to non-code
    if (code=="placeholder") {
      code="\0"; // make sure it comes first
    }
    else {
      codedesc = string_format("'%s' UTF-8", code.c_str());
      for (size_t i=0; i<code.size(); i++) string_format_append(codedesc, " %02X", (uint8_t)code[i]);
    }
    string chardef = string_format("  { %2d, %-42s },  // %-20s (input # %d", w, chr.c_str(), codedesc.c_str(), g);
    if (gh!=glyphHeight) {
      fprintf(aOutputFile, "#error incorrect pixel column height in character '%s', input glyph #%d", codedesc.c_str(), g);
    }


    gm.push_back(make_pair(code,chardef));
  }
  // now sort by Code
  gm.sort(mapcmp);
  int gno = 0;
  for (GlyphMap::iterator g = gm.begin(); g!=gm.end(); ++g, ++gno) {
    fprintf(aOutputFile, "%s -> glyph # %d)\n", g->second.c_str(), gno);
  }
  fprintf(aOutputFile, "};\n\n");
  // generate glyph number lookup
  fprintf(aOutputFile, "static const GlyphRange %s_ranges[] = {\n", aFontName);
  string prefix="NONE";
  string rangedesc;
  gno = 0;
  int glyphOffset = 0;
  string dispchars;
  for (GlyphMap::iterator g = gm.begin(); g!=gm.end(); ++gno) {
    string code = g->first;
    ++g;
    if (code[0]==0) continue; // this is the 0 codepoint which is the placeholder
    dispchars += code; // accumulate for display
    // split in prefix and lastbyte
    uint8_t lastbyte = code[code.size()-1];
    code.erase(code.size()-1);
    if (code!=prefix) {
      // start a new prefix
      prefix = code;
      glyphOffset = gno;
      rangedesc = "  { \"";
      for (size_t i=0; i<code.size(); i++) string_format_append(rangedesc, "\\x%02x", (uint8_t)code[i]);
      string_format_append(rangedesc, "\", 0x%02X, ", lastbyte);
    }
    if (g==gm.end() || (uint8_t)(g->first[g->first.size()-1])!=lastbyte+1) {
      // end of range (but maybe same prefix)
      prefix = "NONE";
      string_format_append(rangedesc, "0x%02X, %d }, // %s", lastbyte, glyphOffset, dispchars.c_str());
      fprintf(aOutputFile, "%s\n", rangedesc.c_str());
      dispchars.clear();
      rangedesc.clear();
    }
  }
  fprintf(aOutputFile, "  { NULL, 0, 0}\n");
  fprintf(aOutputFile, "};\n");
  // now the font head record
  fprintf(aOutputFile, "\nstatic const font_t %s = {\n", aFontName);
  fprintf(aOutputFile, "  .fontName = \"%s\",\n", aFontName);
  fprintf(aOutputFile, "  .glyphHeight = %d,\n", glyphHeight);
  fprintf(aOutputFile, "  .numGlyphs = %d,\n", gno);
  fprintf(aOutputFile, "  .glyphRanges = %s_ranges,\n", aFontName);
  fprintf(aOutputFile, "  .glyphs = %s_glyphs\n", aFontName);
  fprintf(aOutputFile, "};\n");
  // end
  fprintf(aOutputFile, "\n// MARK: - end of generated font data\n\n");
  return true;
}


static void generateFontSource(const font_t& aFont, const char** aGlyphStrings, FILE* aOutputFile)
{
  fprintf(aOutputFile, "//  SPDX-License-Identifier: GPL-3.0-or-later\n");
  fprintf(aOutputFile, "//\n");
  fprintf(aOutputFile, "//  Autogenerated by p44lrgraphics font generator\n");
  fprintf(aOutputFile, "//\n");
  fprintf(aOutputFile, "//  You should have received a copy of the GNU General Public License\n");
  fprintf(aOutputFile, "//  along with p44lrgraphics. If not, see <http://www.gnu.org/licenses/>.\n");
  fprintf(aOutputFile, "//\n\n");
  fprintf(aOutputFile, "// This is a file to be included from textview.cpp to define a font\n");
  fprintf(aOutputFile, "\n#ifdef GENERATE_FONT_SOURCE\n");
  fontAsGlyphStrings(aFont, aOutputFile);
  fprintf(aOutputFile, "\n#endif // GENERATE_FONT_SOURCE\n");
  glyphStringsToFont(aGlyphStrings, aFont.fontName, aOutputFile);
}

#endif // GENERATE_FONT_SOURCE


// MARK: ===== TextView


TextView::TextView()
{
  mTextSpacing = 2;
  mVisible = true;
  mFont = &DEFAULT_FONT;
  #ifdef GENERATE_FONT_SOURCE
  generateFontSource(GENERATE_FONT_SOURCE, GLYPHSTRINGS_VAR, stdout);
  exit(1);
  #endif
}


void TextView::clear()
{
  stopAnimations();
  setText("");
  mVisible = true;
}


TextView::~TextView()
{
}


void TextView::setText(const string aText)
{
  mText = aText;
  renderText();
}


void TextView::setVisible(bool aVisible)
{
  mVisible = aVisible;
  renderText();
}


/// set font
void TextView::setFont(const char* aFontName)
{
  const font_t* fP = fonts[0];
  while (fP) {
    if (strucmp(aFontName, fP->fontName)==0) {
      // found, set it
      mFont = fP;
      // re-render text
      renderText();
      return;
    }
    fP++;
  }
}




void TextView::renderText()
{
  mTextPixelData.clear();
  int cols = 0;
  if (mFont) {
    if (mVisible) {
      // render
      int colbytes = (mFont->glyphHeight+7)>>3;
      size_t textIdx = 0;
      while (textIdx<mText.size()) {
        int glyphNo = glyphNoFromText(*mFont, textIdx, mText.c_str());
        const glyph_t &g = mFont->glyphs[glyphNo];
        // append glyph data
        mTextPixelData.append(g.coldata, g.width*colbytes);
        cols += g.width;
        // add spacing
        mTextPixelData.append(mTextSpacing*colbytes, 0);
        cols += mTextSpacing;
      }
    }
    // set content size
    setContentSize({cols, mFont->glyphHeight});
    makeDirty();
  }
}


void TextView::recalculateColoring()
{
  calculateGradient(mContent.dx, mExtent.x);
  inherited::recalculateColoring();
}


void TextView::geometryChanged(PixelRect aOldFrame, PixelRect aOldContent)
{
  // coloring is dependent on geometry
  recalculateColoring();
  inherited::geometryChanged(aOldFrame, aOldContent);
}


PixelColor TextView::contentColorAt(PixelPoint aPt)
{
  if (mFont) {
    if (isInContentSize(aPt)) {
      if (aPt.y<mFont->glyphHeight && aPt.y>=0) {
        int colbytes = (mFont->glyphHeight+7)>>3;
        int colbit = mFont->glyphHeight-1-aPt.y; // bit 0 = topmost bit in text rendering
        size_t dataIdx = (aPt.x+1)*colbytes -1 - (colbit>>3); // bytes in pixeldata are MSB first
        if (dataIdx<mTextPixelData.size() && ((uint8_t)mTextPixelData[dataIdx] & (1<<(colbit&0x07)))) {
          // text pixel is set
          if (!mGradientPixels.empty()) {
            // horizontally gradiated text
            return gradientPixel(aPt.x);
          }
          return mForegroundColor;
        }
      }
    }
  }
  return mBackgroundColor;
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr TextView::configureView(JsonObjectPtr aViewConfig)
{
  ErrorPtr err = inherited::configureView(aViewConfig);
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    if (aViewConfig->get("font", o)) {
      setFont(o->stringValue().c_str());
    }
    if (aViewConfig->get("text", o)) {
      setText(o->stringValue());
    }
    if (aViewConfig->get("visible", o)) {
      setVisible(o->boolValue());
    }
    if (aViewConfig->get("spacing", o)) {
      setTextSpacing(o->int32Value());
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG


#if ENABLE_VIEWSTATUS

JsonObjectPtr TextView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("text", JsonObject::newString(mText));
  status->add("font", mFont ? JsonObject::newString(mFont->fontName) : JsonObject::newNull());
  status->add("visible", JsonObject::newBool(mVisible));
  status->add("spacing", JsonObject::newInt32(mTextSpacing));
  return status;
}

#endif // ENABLE_VIEWSTATUS
