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

#ifndef _p44lrgraphics_textview_hpp__
#define _p44lrgraphics_textview_hpp__

#include "p44lrg_common.hpp"
#include "coloreffectview.hpp"

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
  } font_t;

  class TextView : public ColorEffectView
  {
    typedef ColorEffectView inherited;

    // text rendering
    string mText; ///< internal representation of text
    bool mCollapsed; ///< if not set, text view is reduced to zero width
    int mTextSpacing; ///< pixels between characters
    int mStretch; ///< pixel row repetitions
    int mBolden; ///< how many shifted overlays
    string mTextPixelData; ///< string of text column data (might be multiple bytes per columnt for fonts with dy>8)
    const font_t* mFont; ///< the font to use
    bool mTextChanges;

  public :

    TextView();
    virtual ~TextView();

    static const char* staticTypeName() { return "text"; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    virtual void beginChanges() P44_OVERRIDE { inherited::beginChanges(); mTextChanges = false; }
    virtual void finalizeChanges() P44_OVERRIDE;

    void flagTextChange() { flagChange(mTextChanges); }

    /// get font
    const char* getFont() const { return mFont ? mFont->fontName : nullptr; };

    /// set font
    void setFont(const char* aFontName);

    /// @name trivial property getters/setters
    /// @{
    // - text
    string getText() const { return mText; }
    void setText(const string aVal) { mText = aVal; flagTextChange(); } ;
    // - spacing
    int getTextSpacing() const { return mTextSpacing; }
    void setTextSpacing(int aVal) { mTextSpacing = aVal; flagTextChange(); }
    // - effects
    int getBolden() const { return mBolden; }
    void setBolden(int aVal) { mBolden = aVal; flagTextChange(); }
    int getStretch() const { return mStretch; }
    void setStretch(int aVal) { mStretch = aVal; flagTextChange(); }
    // - flags
    bool getCollapsed() const { return mCollapsed; }
    void setCollapsed(bool aVal) { mCollapsed = aVal; flagTextChange(); }
    /// @}

    /// clear contents of this view
    virtual void clear() P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG && !ENABLE_P44SCRIPT
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

    #if ENABLE_VIEWSTATUS && !ENABLE_P44SCRIPT
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif // ENABLE_VIEWSTATUS

    #if ENABLE_P44SCRIPT
    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;
    #endif

  protected:

    /// color effect params have changed
    virtual void recalculateColoring() P44_OVERRIDE;

    /// geometry has changed
    virtual void geometryChanged(PixelRect aOldFrame, PixelRect aOldContent) P44_OVERRIDE;

    /// get content color at aPt
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

  private:

    void renderText();

  };
  typedef boost::intrusive_ptr<TextView> TextViewPtr;

  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a TextView
    class TextViewObj : public ColorEffectViewObj
    {
      typedef ColorEffectViewObj inherited;
    public:
      TextViewObj(P44ViewPtr aView);
      TextViewPtr text() { return boost::static_pointer_cast<TextView>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT

} // namespace p44



#endif /* _p44lrgraphics_textview_hpp__ */
