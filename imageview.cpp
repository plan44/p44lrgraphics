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

// File scope debugging options
// - Set ALWAYS_DEBUG to 1 to enable DBGLOG output even in non-DEBUG builds of this file
#define ALWAYS_DEBUG 0
// - set FOCUSLOGLEVEL to non-zero log level (usually, 5,6, or 7==LOG_DEBUG) to get focus (extensive logging) for this file
//   Note: must be before including "logger.hpp" (or anything that includes "logger.hpp")
#define FOCUSLOGLEVEL 7


#include "imageview.hpp"

#if ENABLE_IMAGE_SUPPORT

#if ENABLE_VIEWCONFIG
  #include "application.hpp"
#endif

using namespace p44;

// MARK: ===== ImageView


ImageView::ImageView() :
  pngBuffer(NULL)
{
}


ImageView::~ImageView()
{
  clear();
}


void ImageView::clear()
{
  inherited::clear();
  // init libpng image structure
  memset(&pngImage, 0, (sizeof pngImage));
  pngImage.version = PNG_IMAGE_VERSION;
  // free the buffer if allocated
  if (pngBuffer) {
    free(pngBuffer);
    pngBuffer = NULL;
  }
}


ErrorPtr ImageView::loadPNG(const string aPNGFileName)
{
  // clear any previous pattern (and make dirty)
  clear();
  // read image
  if (png_image_begin_read_from_file(&pngImage, aPNGFileName.c_str()) == 0) {
    // error
    return TextError::err("could not open PNG file %s", aPNGFileName.c_str());
  }
  else {
    // We want full RGBA
    pngImage.format = PNG_FORMAT_RGBA;
    // Now allocate enough memory to hold the image in this format; the
    // PNG_IMAGE_SIZE macro uses the information about the image (width,
    // height and format) stored in 'image'.
    pngBuffer = (png_bytep)malloc(PNG_IMAGE_SIZE(pngImage));
    FOCUSLOG("Image size in bytes = %d", PNG_IMAGE_SIZE(pngImage));
    FOCUSLOG("Image width = %d", pngImage.width);
    FOCUSLOG("Image height = %d", pngImage.height);
    FOCUSLOG("Image width*height = %d", pngImage.height*pngImage.width);
    setContentSize({ (int)pngImage.width, (int)pngImage.height});
    if (pngBuffer==NULL) {
      return TextError::err("Could not allocate buffer for reading PNG file %s", aPNGFileName.c_str());
    }
    // now actually read the image
    if (png_image_finish_read(
      &pngImage,
      NULL, // background
      pngBuffer,
      0, // row_stride
      NULL //colormap
    ) == 0) {
      // error
      ErrorPtr err = TextError::err("Error reading PNG file %s: error: %s", aPNGFileName.c_str(), pngImage.message);
      clear(); // clear only after pngImage.message has been used
      return err;
    }
  }
  // image read ok
  makeDirty();
  return ErrorPtr();
}


PixelColor ImageView::contentColorAt(PixelPoint aPt)
{
  if (!isInContentSize(aPt)) {
    return backgroundColor;
  }
  else {
    PixelColor pc;
    // get pixel infomration from image buffer
    uint8_t *pix = pngBuffer+((pngImage.height-1-aPt.y)*pngImage.width+aPt.x)*4;
    pc.r = *pix++;
    pc.g = *pix++;
    pc.b = *pix++;
    pc.a = *pix++;
    return pc;
  }
}


#if ENABLE_VIEWCONFIG

// MARK: ===== view configuration

ErrorPtr ImageView::configureView(JsonObjectPtr aViewConfig)
{
  // will possibly set dx, dy and/or sizetocontent first
  ErrorPtr err = inherited::configureView(aViewConfig);
  // now check for loading actual image
  if (Error::isOK(err)) {
    JsonObjectPtr o;
    if (aViewConfig->get("file", o)) {
      // if no dx or dy is set when loading a new image, auto-enable sizeToContent
      if (frame.dx==0 && frame.dy==0) sizeToContent = true;
      err = loadPNG(Application::sharedApplication()->resourcePath(o->stringValue()));
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG

#endif // ENABLE_IMAGE_SUPPORT


