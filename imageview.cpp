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

// File scope debugging options
// - Set ALWAYS_DEBUG to 1 to enable DBGLOG output even in non-DEBUG builds of this file
#define ALWAYS_DEBUG 0
// - set FOCUSLOGLEVEL to non-zero log level (usually, 5,6, or 7==LOG_DEBUG) to get focus (extensive logging) for this file
//   Note: must be before including "logger.hpp" (or anything that includes "logger.hpp")
#define FOCUSLOGLEVEL 0


#include "imageview.hpp"

#if ENABLE_IMAGE_SUPPORT

#if ENABLE_VIEWCONFIG
  #include "application.hpp"
#endif

using namespace p44;

// MARK: ===== ImageView


ImageView::ImageView() :
  mPngBuffer(NULL)
{
}


ImageView::~ImageView()
{
  clear();
}


void ImageView::clear()
{
  inherited::clear(); // zero content size
  // init libpng image structure
  memset(&mPngImage, 0, (sizeof mPngImage));
  mPngImage.version = PNG_IMAGE_VERSION;
  // free the buffer if allocated
  if (mPngBuffer) {
    free(mPngBuffer);
    mPngBuffer = NULL;
  }
}


ErrorPtr ImageView::loadPNG(const string aPNGFileName)
{
  // clear any previous pattern (and make dirty)
  clear();
  // read image
  if (png_image_begin_read_from_file(&mPngImage, aPNGFileName.c_str()) == 0) {
    // error
    return TextError::err("could not open PNG file %s", aPNGFileName.c_str());
  }
  else {
    // We want full RGBA
    mPngImage.format = PNG_FORMAT_RGBA;
    // Now allocate enough memory to hold the image in this format; the
    // PNG_IMAGE_SIZE macro uses the information about the image (width,
    // height and format) stored in 'image'.
    mPngBuffer = (png_bytep)malloc(PNG_IMAGE_SIZE(mPngImage));
    FOCUSLOG("Image size in bytes = %d", PNG_IMAGE_SIZE(mPngImage));
    FOCUSLOG("Image width = %d", mPngImage.width);
    FOCUSLOG("Image height = %d", mPngImage.height);
    FOCUSLOG("Image width*height = %d", mPngImage.height*mPngImage.width);
    setContentSize({ (int)mPngImage.width, (int)mPngImage.height});
    if (mPngBuffer==NULL) {
      return TextError::err("Could not allocate buffer for reading PNG file %s", aPNGFileName.c_str());
    }
    // now actually read the image
    if (png_image_finish_read(
      &mPngImage,
      NULL, // background
      mPngBuffer,
      0, // row_stride
      NULL //colormap
    ) == 0) {
      // error
      ErrorPtr err = TextError::err("Error reading PNG file %s: error: %s", aPNGFileName.c_str(), mPngImage.message);
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
    return mBackgroundColor;
  }
  else {
    // if content is set bigger than image, just duplicate border pixels
    if (aPt.x>=mPngImage.width) aPt.x = mPngImage.width-1;
    if (aPt.y>=mPngImage.height) aPt.y = mPngImage.height-1;
    PixelColor pc;
    // get pixel information from image buffer
    uint8_t *pix = mPngBuffer+((mPngImage.height-1-aPt.y)*mPngImage.width+aPt.x)*4;
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
      if (mFrame.dx==0 && mFrame.dy==0) mSizeToContent = true;
      err = loadPNG(Application::sharedApplication()->resourcePath(o->stringValue()));
    }
  }
  return err;
}

#endif // ENABLE_VIEWCONFIG

#if ENABLE_P44SCRIPT

using namespace P44Script;

ScriptObjPtr ImageView::newViewObj()
{
  return new ImageViewObj(this);
}

#if P44SCRIPT_FULL_SUPPORT

// loadimage(filepath)
FUNC_ARG_DEFS(loadimage, { text|undefres } );
static void loadimage_func(BuiltinFunctionContextPtr f)
{
  ImageViewObj* v = dynamic_cast<ImageViewObj*>(f->thisObj().get());
  assert(v);
  // if no dx or dy is set when loading a new image, auto-enable sizeToContent
  if (v->image()->getDx()==0 && v->image()->getDy()==0) v->image()->setSizeToContent(true);
  ErrorPtr err = v->image()->loadPNG(Application::sharedApplication()->resourcePath(f->arg(0)->stringValue()));
  if (Error::notOK(err)) {
    f->finish(new ErrorValue(err));
    return;
  }
  f->finish();
}

#endif // P44SCRIPT_FULL_SUPPORT

#define ACCESSOR_CLASS ImageView

static ScriptObjPtr property_accessor(BuiltInMemberLookup& aMemberLookup, ScriptObjPtr aParentObj, ScriptObjPtr aObjToWrite, const struct BuiltinMemberDescriptor* aMemberDescriptor)
{
  ACCFN_DEF
  ImageViewPtr view = reinterpret_cast<ACCESSOR_CLASS*>(reinterpret_cast<ImageViewObj*>(aParentObj.get())->image().get());
  ACCFN acc = reinterpret_cast<ACCFN>(aMemberDescriptor->memberAccessInfo);
  view->announceChanges(true);
  ScriptObjPtr res = acc(*view, aObjToWrite);
  view->announceChanges(false);
  return res;
}

ACC_IMPL_RO_INT(ImageDx);
ACC_IMPL_RO_INT(ImageDy);
ACC_IMPL_RO_INT(ImageBytes);

static const BuiltinMemberDescriptor imageViewMembers[] = {
  #if P44SCRIPT_FULL_SUPPORT
  FUNC_DEF_W_ARG(loadimage, executable|null|error),
  #endif
  // property accessors
  ACC_DECL("image_dx", numeric, ImageDx),
  ACC_DECL("image_dy", numeric, ImageDy),
  ACC_DECL("bytes", numeric, ImageBytes),
  { NULL } // terminator
};

static BuiltInMemberLookup* sharedImageMemberLookupP = NULL;

ImageViewObj::ImageViewObj(P44ViewPtr aView) :
  inherited(aView)
{
  registerSharedLookup(sharedImageMemberLookupP, imageViewMembers);
}

#endif // ENABLE_P44SCRIPT

#endif // ENABLE_IMAGE_SUPPORT


