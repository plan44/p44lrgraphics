//
//  Copyright (c) 2016-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

  class TextView : public ColorEffectView
  {
    typedef ColorEffectView inherited;

    // text rendering
    string text; ///< internal representation of text
    bool visible; ///< if not set, text view is reduced to zero width
    int textSpacing; ///< pixels between characters
    string textPixelCols; ///< string of text column bytes

  public :

    TextView();
    virtual ~TextView();

    static const char* staticTypeName() { return "text"; };
    virtual const char* viewTypeName() P44_OVERRIDE { return staticTypeName(); }

    /// set new text
    /// @note: sets the content size of the view according to the text
    void setText(const string aText);

    /// set visibility
    void setVisible(bool aVisible);

    /// get current text
    string getText() const { return text; }

    /// set character spacing
    void setTextSpacing(int aTextSpacing) { textSpacing = aTextSpacing; renderText(); }

    /// get text color
    int getTextSpacing() const { return textSpacing; }

    /// clear contents of this view
    virtual void clear() P44_OVERRIDE;

    #if ENABLE_VIEWCONFIG
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

    #if ENABLE_VIEWSTATUS
    /// @return the current status of the view, in the same format as accepted by configure()
    virtual JsonObjectPtr viewStatus() P44_OVERRIDE;
    #endif // ENABLE_VIEWSTATUS

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


} // namespace p44



#endif /* _p44lrgraphics_textview_hpp__ */
