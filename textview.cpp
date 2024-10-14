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

#include "textview.hpp"

using namespace p44;


// MARK: ===== TextView

// default font
#define DEFAULT_FONT "5x7"

static ViewRegistrar r(TextView::staticTypeName(), &TextView::newInstance);

TextView::TextView()
{
  mTextSpacing = 2;
  mStretch = 0;
  mBolden = 0;
  mCollapsed = false;
  mFont = LrgFonts::sharedFonts().fontByName(DEFAULT_FONT);
  #ifdef GENERATE_FONT_SOURCE
  LrgFonts::sharedFonts().generate_all_builtin_fonts_to_tmp();
  exit(1);
  #endif
}


void TextView::clear()
{
  stopAnimations();
  setText("");
  mCollapsed = true;
}


TextView::~TextView()
{
}


void TextView::finalizeChanges()
{
  inherited::finalizeChanges();
  if (mTextChanges) {
    FOCUSLOG("View '%s' changed text setting", getLabel().c_str());
    renderText();
    mTextChanges = false;
  }
}


bool TextView::setFont(const char* aFontName)
{
  LrgFontPtr newFont = LrgFonts::sharedFonts().fontByName(aFontName);
  if (newFont) {
    mFont = newFont;
    // re-render text
    flagTextChange();
  }
  return newFont!=nullptr;
}


void TextView::renderText()
{
  mTextPixelData.clear();
  int cols = 0;
  if (mFont) {
    const font_t& font = mFont->fontData();
    if (!mCollapsed) {
      // render
      int colbytes = (font.glyphHeight+7)>>3;
      size_t textIdx = 0;
      while (textIdx<mText.size()) {
        int glyphNo = glyphIndexFromText(font, textIdx, mText.c_str());
        const glyph_t &g = font.glyphs[glyphNo];
        // append glyph data
        if (mStretch || mBolden) {
          // with effects, process col by col
          for(int cidx=0; cidx<g.width+mBolden; cidx++) {
            int startat = cidx>=mBolden ? cidx-mBolden : 0;
            int endbefore = cidx<g.width ? cidx+1 : g.width;
            string col;
            col.assign(colbytes, '\x00');
            for (int i=startat; i<endbefore; i++) {
              for (int j=0; j<colbytes; j++) {
                col[j] |= g.coldata[i*colbytes+j];
              }
            }
            // then added (mStretch+1 times)
            for (int j=0; j<=mStretch; j++) {
              mTextPixelData += col;
              cols += 1;
            }
          }
        }
        else {
          // simple
          mTextPixelData.append(g.coldata, g.width*colbytes);
          cols += g.width;
        }
        // add spacing
        mTextPixelData.append(mTextSpacing*colbytes, 0);
        cols += mTextSpacing;
      }
    }
    // set content size according to rendered text
    setContentSize({cols, font.glyphHeight});
    makeDirty();
  }
  else {
    // collapse to no content
    setContentSize({0, 0});
  }
}


void TextView::recalculateColoring()
{
  calculateGradient(mContent.dx);
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
    const font_t& font = mFont->fontData();
    if (isInContentSize(aPt)) {
      if (aPt.y<font.glyphHeight && aPt.y>=0) {
        int colbytes = (font.glyphHeight+7)>>3;
        int colbit = font.glyphHeight-1-aPt.y; // bit 0 = topmost bit in text rendering
        size_t dataIdx = (aPt.x+1)*colbytes -1 - (colbit>>3); // bytes in pixeldata are MSB first
        if (dataIdx<mTextPixelData.size() && ((uint8_t)mTextPixelData[dataIdx] & (1<<(colbit&0x07)))) {
          // text pixel is set
          if (!mGradientPixels.empty()) {
            // horizontally gradiated text
            return gradientPixel(aPt.x, mEffectWrap);
          }
          return mForegroundColor;
        }
      }
    }
  }
  return mBackgroundColor;
}


#if ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT

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
    if (aViewConfig->get("collapsed", o)) {
      setVisible(o->boolValue());
    }
    if (aViewConfig->get("spacing", o)) {
      setTextSpacing(o->int32Value());
    }
    if (aViewConfig->get("stretch", o)) {
      setStretch(o->int32Value());
    }
    if (aViewConfig->get("bolden", o)) {
      setBolden(o->int32Value());
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT


#if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT

JsonObjectPtr TextView::viewStatus()
{
  JsonObjectPtr status = inherited::viewStatus();
  status->add("text", JsonObject::newString(getText()));
  status->add("font", getFont() ? JsonObject::newString(getFont()) : JsonObject::newNull());
  status->add("collapsed", JsonObject::newBool(getCollapsed()));
  status->add("spacing", JsonObject::newInt32(getTextSpacing()));
  status->add("stretch", JsonObject::newInt32(getStretch()));
  status->add("bolden", JsonObject::newInt32(getBolden()));
  return status;
}

#endif // ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT


#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr TextView::newViewObj()
{
  return new TextViewObj(this);
}


#define ACCESSOR_CLASS TextView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  TextViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<TextViewObj*>(aParentObj.get())->text().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_STR(Text);
ACC_IMPL_CSTR(Font);
ACC_IMPL_INT(TextSpacing);
ACC_IMPL_INT(Bolden);
ACC_IMPL_INT(Stretch);
ACC_IMPL_BOOL(Collapsed);


static const BuiltinMemberDescriptor textViewMembers[] = {
  // property accessors
  ACC_DECL("text", text|lvalue, Text),
  ACC_DECL("font", text|lvalue, Font),
  ACC_DECL("collapsed", numeric|lvalue, Collapsed),
  ACC_DECL("spacing", numeric|lvalue, TextSpacing),
  ACC_DECL("stretch", numeric|lvalue, Stretch),
  ACC_DECL("bolden", numeric|lvalue, Bolden),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedTextMemberLookupP = NULL;

TextViewObj::TextViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  registerSharedLookup(sharedTextMemberLookupP, textViewMembers);
}

#endif // ENABLE_P44SCRIPT
