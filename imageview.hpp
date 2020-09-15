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

#ifndef _p44lrgraphics_imageview_hpp__
#define _p44lrgraphics_imageview_hpp__

#include "p44lrg_common.hpp"

#if ENABLE_IMAGE_SUPPORT

#include <png.h>

namespace p44 {

  class ImageView : public P44View
  {
    typedef P44View inherited;

    png_image pngImage; /// The control structure used by libpng
    png_bytep pngBuffer; /// byte buffer

  public :

    ImageView();
    virtual ~ImageView();

    static const char* staticTypeName() { return "image"; };
    virtual const char* viewTypeName() P44_OVERRIDE { return staticTypeName(); }

    /// clear image
    virtual void clear() P44_OVERRIDE;

    /// load PNG image
    ErrorPtr loadPNG(const string aPNGFileName);

    #if ENABLE_VIEWCONFIG
    /// configure view from JSON
    virtual ErrorPtr configureView(JsonObjectPtr aViewConfig) P44_OVERRIDE;
    #endif

  protected:

    /// get content color at aPt
    virtual PixelColor contentColorAt(PixelPoint aPt) P44_OVERRIDE;

  };
  typedef boost::intrusive_ptr<ImageView> ImageViewPtr;


} // namespace p44

#endif // ENABLE_IMAGE_SUPPORT

#endif /* _p44lrgraphics_imageview_hpp__ */
