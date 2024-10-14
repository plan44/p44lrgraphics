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

#include "fonts.hpp"
#include "tlv.hpp"
#include <cstring> // for memset

using namespace p44;

// MARK: UTF-8 codepoint to Glyph mapping

const int placeholderGlyphNo = 0; // placeholder must be the first glyph

/// @return glyph index or -1 if EOT
int p44::glyphIndexFromText(const font_t& aFont, size_t &aIdx, const char* aText) {
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


// MARK: - LrgFont

LrgFont::LrgFont(const font_t& aFontData) :
  mFontData(aFontData)
{
}


LrgFont::~LrgFont()
{
}


// MARK: - UserLrgFont

UserLrgFont::UserLrgFont() :
  inherited(mLoadedFontData)
{
  mLoadedFontData.glyphRanges = nullptr;
  mLoadedFontData.glyphs = nullptr;
  reset();
}


UserLrgFont::~UserLrgFont()
{
  reset();
}


void UserLrgFont::reset()
{
  if (mLoadedFontData.glyphRanges) {
    for (const GlyphRange* grP = mLoadedFontData.glyphRanges; grP->prefix; grP++) {
      if (grP->prefix) delete grP->prefix;
    }
    delete mLoadedFontData.glyphRanges;
    mLoadedFontData.glyphRanges = nullptr;
  }
  if (mLoadedFontData.glyphs) {
    for (int i = 0; i<mLoadedFontData.numGlyphs; i++) {
      if (mLoadedFontData.glyphs[i].coldata) delete mLoadedFontData.glyphs[i].coldata;
    }
    delete mLoadedFontData.glyphs;
    mLoadedFontData.glyphs = nullptr;
  }
  mLoadedFontData.fontName = "<empty>";
  mLoadedFontData.numGlyphs = 0;
  mLoadedFontData.glyphHeight = 0;
}


ErrorPtr UserLrgFont::loadFromFile(const string& aFontFileName)
{
  string tlv;
  ErrorPtr err = string_fromfile(aFontFileName, tlv);
  if (Error::isOK(err)) {
    uint8_t vers;
    TLVReader r(tlv);
    if (!r.nextIs(tlv_unsigned, "p44lrg_font")) return TextError::err("not a p44lrg font");
    if (!r.read_unsigned(vers) || vers!=1) return TextError::err("unknown font format version %d, expected 1", vers);
    if (!r.nextIs(tlv_string, "name")) goto err;
    if (!r.read_string(mName)) goto err;
    mLoadedFontData.fontName = mName.c_str();
    if (!r.nextIs(tlv_unsigned, "height")) goto err;
    if (!r.read_unsigned(mLoadedFontData.glyphHeight)) goto err;
    // Ranges
    if (!r.nextIs(tlv_counted_container, "ranges")) goto err;
    size_t n;
    if (!r.open_counted_container(n)) goto err;
    GlyphRange* ranges = new GlyphRange[n+1];
    mLoadedFontData.glyphRanges = ranges;
    // - init all as terminators
    for (int i = 0; i<n+1; i++) { ::memset((void*)&ranges[i], 0, sizeof(GlyphRange)); }
    // - now read ranges
    for (int i = 0; i<n; i++) {
      if (!r.open_container()) goto err;
      string s;
      if (!r.read_string(s)) goto err;
      ranges[i].prefix = new char[s.size()+1];
      strncpy((char*)ranges[i].prefix, s.c_str(), s.size()+1);
      if (!r.read_unsigned(ranges[i].first)) goto err;
      if (!r.read_unsigned(ranges[i].last)) goto err;
      if (!r.read_unsigned(ranges[i].glyphOffset)) goto err;
      r.close_container();
    }
    r.close_container();
    // Glyphs
    if (!r.nextIs(tlv_counted_container, "glyphs")) goto err;
    if (!r.open_counted_container(n)) goto err;
    glyph_t* glyphs = new glyph_t[n];
    mLoadedFontData.glyphs = glyphs;
    // - reset all
    for (int i = 0; i<n; i++) { ::memset((void*)&glyphs[i], 0, sizeof(glyph_t)); }
    // - now read glyphs
    for (int i = 0; i<n; i++) {
      if (!r.open_container()) goto err;
      if (!r.read_unsigned(glyphs[i].width)) goto err;
      string b;
      if (!r.read_blob(b)) goto err;
      glyphs[i].coldata = new char[b.size()+1];
      memcpy((char*)glyphs[i].coldata, b.c_str(), b.size()+1); // can contain NULs
      r.close_container();
    }
    r.close_container();
    // successfully loaded
  }
  return err;
err:
  reset();
  return TextError::err("corrupt font file");
}


#ifdef GENERATE_FONT_SOURCE

// MARK: developer-only utilities for font generation/verification

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
  fprintf(aOutputFile, "\nstatic const char * font_%s_glyphstrings[] = {\n", aFont.fontName);
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
  fprintf(aOutputFile, "\nstatic const glyph_t font_%s_glyphs[] = {\n", aFontName);
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
  fprintf(aOutputFile, "static const GlyphRange font_%s_ranges[] = {\n", aFontName);
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
  fprintf(aOutputFile, "  { NULL, 0, 0, 0 }\n");
  fprintf(aOutputFile, "};\n");
  // now the font head record
  fprintf(aOutputFile, "\nstatic const font_t font_%s = {\n", aFontName);
  fprintf(aOutputFile, "  .fontName = \"%s\",\n", aFontName);
  fprintf(aOutputFile, "  .glyphHeight = %d,\n", glyphHeight);
  fprintf(aOutputFile, "  .numGlyphs = %d,\n", gno);
  fprintf(aOutputFile, "  .glyphRanges = font_%s_ranges,\n", aFontName);
  fprintf(aOutputFile, "  .glyphs = font_%s_glyphs\n", aFontName);
  fprintf(aOutputFile, "   #ifdef GENERATE_FONT_SOURCE\n");
  fprintf(aOutputFile, "   , .glyphstrings = font_%s_glyphstrings\n", aFontName);
  fprintf(aOutputFile, "   #endif\n");
  fprintf(aOutputFile, "};\n");
  fprintf(aOutputFile, "\nstatic BuiltinFontRegistrar r(font_%s);\n", aFontName);

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
  fprintf(aOutputFile, "//  Include this file into a p44lrgraphics build to define a built-in font\n");
  fprintf(aOutputFile, "\n#include \"fonts.hpp\"\n");
  fprintf(aOutputFile, "\nusing namespace p44;\n");
  fprintf(aOutputFile, "\n#ifdef GENERATE_FONT_SOURCE\n");
  fontAsGlyphStrings(aFont, aOutputFile);
  fprintf(aOutputFile, "\n#endif // GENERATE_FONT_SOURCE\n");
  glyphStringsToFont(aGlyphStrings, aFont.fontName, aOutputFile);
}


class FontWriter : public TLVWriter
{
public:
  void put_font(const font_t& aFont)
  {
    put_id_string("p44lrg_font");
    put_unsigned(1); // format version
    put_id_string("name");
    put_string(aFont.fontName);
    put_id_string("height");
    put_unsigned(aFont.glyphHeight);
    put_id_string("ranges");
    start_counted_container();
    const GlyphRange* r = aFont.glyphRanges;
    while (r->prefix) {
      start_container();
      put_string(r->prefix);
      put_unsigned(r->first);
      put_unsigned(r->last);
      put_unsigned(r->glyphOffset);
      end_container();
      r++;
    }
    end_container();
    put_id_string("glyphs");
    start_counted_container();
    for (int i=0; i<aFont.numGlyphs; i++) {
      const glyph_t& glyph = aFont.glyphs[i];
      start_container();
      put_unsigned(glyph.width);
      int colbytes = glyph.width*((aFont.glyphHeight+7)>>3);
      put_blob(glyph.coldata, colbytes);
      end_container();
    }
    end_container();
  }
};


#endif // GENERATE_FONT_SOURCE


// MARK: - LrgFonts

static LrgFonts* gSharedFontsP = nullptr;

LrgFonts::LrgFonts()
{
  #if EXPLICIT_FONT_INCLUDES
  // build list of built-in fonts
  size_t fi = 0;
  while (builtinFonts[fi]) {
    const font_t& font = *builtinFonts[fi];
    mAvailableFonts[font.fontName] = new LrgFont(font);
    fi++;
  }
  #endif
}


LrgFonts& LrgFonts::sharedFonts()
{
  if (!gSharedFontsP) {
    gSharedFontsP = new LrgFonts();
  }
  return *gSharedFontsP;
}


LrgFontPtr LrgFonts::fontByName(const string aFontName)
{
  FontsMap::iterator f = mAvailableFonts.find(aFontName);
  if (f!=mAvailableFonts.end()) return f->second;
  return nullptr;
}


ErrorPtr LrgFonts::addFontFromFile(const string& aFontFileName)
{
  ErrorPtr err;
  UserLrgFontPtr userfont = new UserLrgFont();
  err = userfont->loadFromFile(aFontFileName);
  if (Error::isOK(err)) {
    addFont(userfont);
  }
  return err;
}


void LrgFonts::addFont(LrgFontPtr aFont)
{
  if (aFont) {
    mAvailableFonts[aFont->name()] = aFont;
  }
}


#ifdef GENERATE_FONT_SOURCE

void LrgFonts::generate_all_builtin_fonts_to_tmp()
{
  for (FontsMap::iterator pos = mAvailableFonts.begin(); pos!=mAvailableFonts.end(); ++pos) {
    // Source
    string fn = string_format("/tmp/font_%s.cpp", pos->first.c_str());
    printf("Generating .cpp and .lrgf to /tmp/%s ...\n", pos->first.c_str());
    FILE* outfile = fopen(fn.c_str(), "w");
    assert(pos->second->fontData().glyphstrings);
    generateFontSource(pos->second->fontData(), pos->second->fontData().glyphstrings , outfile);
    fclose(outfile);
    // loadable font
    FontWriter fw;
    fw.put_font(pos->second->fontData());
    string tlv = fw.finalize();
    string_tofile(string_format("/tmp/font_%s.lrgf", pos->first.c_str()), tlv);
    // load back in to verify
    //TLVReader fr(tlv);
    //fputs(fr.dump().c_str(), stdout);
  }
}

#endif GENERATE_FONT_SOURCE


#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr LrgFonts::fontsArray()
{
  ArrayValuePtr farr = new ArrayValue();
  for (FontsMap::iterator pos = mAvailableFonts.begin(); pos!=mAvailableFonts.end(); ++pos) {
    farr->appendMember(new StringValue(pos->second->name()));
  }
  return farr;
}

#endif // ENABLE_P44SCRIPT

// MARK: - Font Registrar

BuiltinFontRegistrar::BuiltinFontRegistrar(const font_t& aFontData)
{
  LrgFonts::sharedFonts().addFont(new LrgFont(aFontData));
}

