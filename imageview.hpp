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

#ifndef _p44lrgraphics_imageview_hpp__
#define _p44lrgraphics_imageview_hpp__

#include "p44lrg_common.hpp"

#if ENABLE_IMAGE_SUPPORT

#include <png.h>

namespace p44 {

  class ImageView : public P44View
  {
    typedef P44View inherited;

    png_image mPngImage; /// The control structure used by libpng
    png_bytep mPngBuffer; /// byte buffer

  public :

    ImageView();
    virtual ~ImageView();

    static const char* staticTypeName() { return "image"; };
    static P44View* newInstance() { return new ImageView; };
    virtual const char* getTypeName() const P44_OVERRIDE { return staticTypeName(); }

    /// clear image
    virtual void clear() P44_OVERRIDE;

    /// load PNG image
    ErrorPtr loadPNG(const string aPNGFileName);

    /// @name trivial property getters/setters
    /// @{
    PixelCoord getImageDx() { return mPngImage.width; }
    PixelCoord getImageDy() { return mPngImage.height; }
    size_t getImageBytes() { return PNG_IMAGE_SIZE(mPngImage); }
    /// @}

    #if ENABLE_VIEWCONFIG
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

    #if ENABLE_P44SCRIPT
    /// @return ScriptObj representing this view
    virtual P44Script::ScriptObjPtr newViewObj() P44_OVERRIDE;
    #endif

    /// get content color at aPt
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

  };
  typedef boost::intrusive_ptr<ImageView> ImageViewPtr;


  #if ENABLE_P44SCRIPT

  namespace P44Script {

    /// represents a ImageView
    class ImageViewObj : public P44lrgViewObj
    {
      typedef P44lrgViewObj inherited;
    public:
      ImageViewObj(P44ViewPtr aView);
      ImageViewPtr image() { return boost::static_pointer_cast<ImageView>(inherited::view()); };
    };

  } // namespace P44Script

  #endif // ENABLE_P44SCRIPT



} // namespace p44

#endif // ENABLE_IMAGE_SUPPORT

#endif /* _p44lrgraphics_imageview_hpp__ */
