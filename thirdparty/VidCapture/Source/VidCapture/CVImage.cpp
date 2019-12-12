// CVImage - base image class interface
// Written by Michael Ellison
//-------------------------------------------------------------------------
//                      CodeVis's Free License
//                         www.codevis.com
//
// Copyright (c) 2003 by Michael Ellison (mike@codevis.com)
// All rights reserved.
//
// You may use this software in source and/or binary form, with or without
// modification, for commercial or non-commercial purposes, provided that 
// you comply with the following conditions:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
//
// * Redistributions of modified source must be clearly marked as modified,
//   and due notice must be placed in the modified source indicating the
//   type of modification(s) and the name(s) of the person(s) performing
//   said modification(s).
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//---------------------------------------------------------------------------
// Modifications:
//
//---------------------------------------------------------------------------
//
/// \file CVImage.cpp
/// \brief CVImage.cpp implements common functions for CVImage, the root image class
/// for image capture and processing.
///
/// Stores the image as an unpadded array of pixels.  Multiple formats
/// are supported as derived classes. If you add a new type, you'll need to
/// add it into the CVIMAGE_TYPE enum and add support where the type
/// is referenced in CVImage in addition to creating the new child 
/// class itself.
///
/// Externally, use CVImage::CreateImage(), ReleaseImage(), and the other
///  static functions to construct and destruct images. Do *not* use new and 
///  delete!  CVImages are reference counted objects.
///
/// CVImage objects can now be sub images of other CVImage objects.  What that
/// means is that if you want to access the fData buffer directly via
/// CVImage::GetRawDataPtr(), you need to take account of the x/y offsets 
/// and the fact that the size of the data buffer won't necessarily be the 
/// same size as the image.  
///
/// You can get the absolute width and height of the image buffer from 
/// CVImage::AbsWidth() and CVImage::AbsHeight().
///
/// For the absolute X and Y offsets, use CVImage::XOffsetAbs() and 
/// CVImage::YOffsetAbs().
///
/// Please use these functions rather than just checking for parents and
/// grabbing the parent's data!  The parent might be a sub image as well.
///
/// Loading and saving currently support PNM (Portable Anywhere Maps) in
/// binary formats only.  Floating point and 32-bit integer formats have
/// their own derivation of these formats. See documentation for 
/// the CVImage::Load() and CVImage::Save() functions in the code.
///
/// When adding new functions, if it's easy and portable please add them
/// to only the CVImage base class.  However, for image-type specific stuff,
/// make them pure virtuals in the base class (if possible - if not return
/// CVRES_NOT_IMPLEMENTED from the base), then implement them on all image 
/// types individually.  If it's going to kill the speed to make the function
/// generic in the base class, use the latter method and implement it 
/// for each type.
///
/// $RCSfile: CVImage.cpp,v $
/// $Date: 2004/02/08 23:47:39 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $

#include <stdio.h>
#include <memory.h>
#include "CVUtil.h"
#include "CVFile.h"
#include "CVImage.h"

// Include each type here for creation from static factory-style 
// functions. Eventually, we'll probably want to move the static
// functions into a seperate factory class or classes, especially
// as file format support improves.
#include "CVImageRGB24.h"
#include "CVImageRGBFloat.h"
#include "CVImageGrey.h"

//------------------------------------------------------------------------
// CreateImage creates an image of the appropriate type.
//
// Use this instead of new to create image objects.
// If width and height are non-zero, creates the appropriate memory for
// the image and adds a reference. Otherwise, does not create memory 
// buffer or add reference.
//
// Call CVImage::ReleaseImage() on the returned image when done.
//
//    type - specifies the type of image to create.
//    image - uninitialized image ptr. Set on return.
//    width - desired width of image in pixels.
//    height - desired height of image in pixels.
//    init - if true, image data is set to 0.
//------------------------------------------------------------------------
CVRES CVImage::CreateImage(   CVIMAGE_TYPE   imageType,
                              CVImage*&      image,
                              int            width,
                              int            height,
                              bool           init)
{
   image = 0;

   switch (imageType)
   {
      case  CVIMAGE_DEFAULT:     // On a normal create, default = RGB24
      case  CVIMAGE_RGB24:       image = new CVImageRGB24();      break;

      case  CVIMAGE_RGBFLOAT:    image = new CVImageRGBFloat();   break;

      case  CVIMAGE_GREY:        image = new CVImageGrey();       break;
               
      //case  CVIMAGE_RGBINT:
      //case  CVIMAGE_GREYINT:
      //case  CVIMAGE_GREYFLOAT:

      default:
         CVAssert(false,"Invalid CVIMAGE_TYPE!");
         return CVRES_IMAGE_UNKNOWN_TYPE;
   }
   
   if (image == 0)
   {
      return CVRES_OUT_OF_MEMORY;
   }

   if ((width != 0) && (height != 0))
   {
      return image->Create(width, height, init);
   }
   
   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// ReleaseImage decrements the reference count of an image and
// will free the image if it hits zero.
// It may also free parent images if the specified
// image holds the last reference to a parent.
//
// image - pointer to image to release. Is set to NULL
//         if last reference was deleted.
//---------------------------------------------------------------------------
CVRES CVImage::ReleaseImage( CVImage*& image)
{
   CVAssert(image != 0, "Invalid image released!");

   if (image != 0)
   {     
      // DecRef also decrements parent(s)
      if (image->fRefCount <= 1)
      {
         // Delete if reference count hit zero.
         delete image;

         // Null caller's pointer since it's invalid now.
         image = 0;
      }
      else
      {
         // If we have other references, just decrement our count.
         image->DecRef();
      }
      
      return CVRES_SUCCESS;
   }
   return CVRES_INVALID_PARAMETER;
   
}


//---------------------------------------------------------------------------
// CreateCompatible creates an image of the same type as the
// specified srcImg.  
// This version also uses the source image's width and height 
// for the new image.
//
// Call CVImage::ReleaseImage() on returned image when done.
//
//       srcImg - source image to get information from.
//       dstImg - uninitialized image ptr. Set on return.
//       init - if true, image data is set to 0.
//---------------------------------------------------------------------------
CVRES CVImage::CreateCompatible  (  const CVImage*    orgImg,
                                    CVImage*&         dstImg,
                                    bool              init)
{
   return CreateImage(  orgImg->GetImageType(), 
                        dstImg, 
                        orgImg->Width(), 
                        orgImg->Height(), 
                        init);
}

//---------------------------------------------------------------------------
// CreateCompatible creates an image of the same type as the
// specified srcImg.  
// This version uses user-specified dimensions for the new image.
// Call CVImage::ReleaseImage() on returned image when done.
//
//        srcImg - source image to get information from.
//        dstImg - uninitialized image ptr. Set on return.
//        width - width of desired image in pixels.
//        height - height of desired image in pixels.
//        init - if true, image data is set to 0.
//---------------------------------------------------------------------------
CVRES CVImage::CreateCompatible  (  const CVImage*    orgImg,
                                    CVImage*&         dstImg,
                                    int               width,
                                    int               height,
                                    bool              init)
{
   return CreateImage(  orgImg->GetImageType(), 
                        dstImg, 
                        width, 
                        height, 
                        init);
}

//---------------------------------------------------------------------------
// CreateSub creates a sub-image of the specified parent.
//
// dstImage should not be instantiated prior to calling CreateSub.
// Sub image is returned in dstImage.
//
// The sub image references the parent's fData
// member, and the parent image must not be deleted before you
// are done using the sub image.
//
// You may create a sub image of a sub image, ad infinum. It'll
// handle the offsets.  Call CVImage::ReleaseImage() when done.
//
//        orgImg - parent image to derive subimage from.
//        dstImg - uninitialized image ptr. Set on return.
//        xOffset - relative x offset for top-left of sub image.
//        yOffset - relative y offset for top-left of sub image.
//        width - width of sub image.
//        height - height of sub image.
//---------------------------------------------------------------------------
CVRES CVImage::CreateSub      (  const CVImage*    orgImg,
                                 CVImage*&         dstImg,
                                 int               xOffset,
                                 int               yOffset,
                                 int               width,
                                 int               height)
{
   dstImg = 0;
   
   CVAssert(xOffset >= 0, "XOffset must be >= 0");
   CVAssert(yOffset >= 0, "YOffset must be >= 0");
   
   CVAssert(xOffset + width  <= orgImg->fWidth,  
            "Invalid sub image width");
   
   CVAssert(yOffset + height <= orgImg->fHeight, 
            "Invalid sub image height");

   CVAssert(orgImg->fData != 0, "Parent image is invalid.");

   // Bail on invalid parameters (same checks as asserts above)
   if ((xOffset < 0) || (yOffset < 0)        ||
       (xOffset + width  > orgImg->fWidth)      ||
       (yOffset + height > orgImg->fHeight)     ||
       (orgImg->fData == 0))
   {
      return CVRES_IMAGE_INVALID_SUB_POSITION;
   }

   // Create an uninitialized image object
   CVRES result;
   if (CVFAILED(result = 
         CreateImage(orgImg->GetImageType(), dstImg, 0, 0, false)))
   {
      return result;
   }

   
   // Point it at parent and store options
   dstImg->fOwnData     = false;
   dstImg->fParentImage = (CVImage*)orgImg;
   dstImg->fData        = orgImg->fData;

   // Remember these are just width and height of the sub image. To retrieve
   // the width and height of the fData data buffer, use AbsWidth() and
   // AbsHeight().
   dstImg->fWidth       = width;
   dstImg->fHeight      = height;

   // Remember on the x and y offsets that these are
   // relative to the parent only - to get the absolute x and y offsets
   // within the fData data buffer, use XOffsetAbs() and YOffsetAbs().
   dstImg->fXOffset     = xOffset;
   dstImg->fYOffset     = yOffset;
   
   // Add reference to ourself and our parents.  CreateImage() did not do this
   // previously, since width and height were set to 0.
   dstImg->AddRef();

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// CopyImage() creates a new image of the same type as srcImg
// and stores it in dstImg.  The data from srcImg is
// copied into a buffer owned by dstImg. Caller must call
// CVImage::ReleaseImage() on the returned image when done.
//
//       srcImg - source image to copy from
//       dstImg - uninitizlied image ptr. Contains copy
//                 of srcImg on return.
//---------------------------------------------------------------------------
CVRES CVImage::CopyImage      (  const CVImage*    srcImg,
                                 CVImage*&         dstImg)
{
   dstImg = 0;
   CVAssert(srcImg != 0, "Can't copy a null image.");
   if (srcImg == 0)
   {
      return CVRES_INVALID_PARAMETER;
   }

   // Simply copy - make full copy of source 
   return CopyImage( srcImg, 
                     dstImg, 
                     srcImg->XOffsetRel(), 
                     srcImg->YOffsetRel(),
                     srcImg->Width(),
                     srcImg->Height());
}
//---------------------------------------------------------------------------
// CopyImage() creates a new image of the same type as srcImg
// and stores it in dstImg.  The data from srcImg is
// copied into a buffer owned by dstImg. Caller must call
// CVImage::ReleaseImage() on the returned image when done.
//
// This version allows you to specify offsets, width, and
// height of the data to copy into a new image.
// 
//        srcImg - source image to copy from
//        dstImg - uninitialized image ptr. Contains copy
//                 of srcImg on return.
//        xOffset - relative x offset for top-left of copy.
//        yOffset - relative y offset for top-left of copy.
//        width - width of destination image.
//        height - height of destination image.
//---------------------------------------------------------------------------
CVRES CVImage::CopyImage      (  const CVImage*    srcImg,
                                 CVImage*&         dstImg,
                                 int               xOffset,
                                 int               yOffset,
                                 int               width,
                                 int               height)
{
   dstImg = 0;
   CVAssert(srcImg != 0, "Can't copy a null image.");
   if (srcImg == 0)
   {
      return CVRES_INVALID_PARAMETER;
   }
   
   CVAssert(xOffset >= 0, "XOffset must be >= 0");
   CVAssert(yOffset >= 0, "YOffset must be >= 0");
   CVAssert(xOffset + width  <= srcImg->fWidth,  "Invalid offset/width");
   CVAssert(yOffset + height <= srcImg->fHeight, "Invalid offset/height");
   CVAssert(srcImg->fData != 0, "Source image is invalid.");

   // Bail on invalid parameters (same checks as asserts above)
   if ((xOffset < 0) || (yOffset < 0)        ||
       (xOffset + width  > srcImg->fWidth)      ||
       (yOffset + height > srcImg->fHeight)     ||
       (srcImg->fData == 0))
   {
      return CVRES_IMAGE_INVALID_SUB_POSITION;
   }

   // Create an image object and allocate space for memory.
   CVRES result;
   if (CVFAILED(result = CreateImage(  srcImg->GetImageType(), 
                                       dstImg, 
                                       width, 
                                       height, 
                                       false)))
   {
      return result;
   }

   // Copy image buffer...
   int y;
   
   // Offset into line for source buffer in bytes
   int srcLineOffset = 
         (srcImg->XOffsetAbs() + xOffset)*srcImg->GetBytesPerPixel();
   
   // current position of start of buffer in source
   unsigned char* srcPtr = 
            srcImg->fData + srcLineOffset + 
            ((srcImg->YOffsetAbs() + yOffset) * srcImg->AbsWidth() * srcImg->GetBytesPerPixel());

   unsigned char* dstPtr = dstImg->fData;

   for (y = 0; y < height; y++)
   {
      // Copy one row at a time into our new image
      memcpy(  dstPtr,
               srcPtr,
               width * dstImg->GetBytesPerPixel() );

      // step to next line in source
      srcPtr += srcImg->AbsWidth() * srcImg->GetBytesPerPixel();
      
      // and in destination....
      dstPtr += width * dstImg->GetBytesPerPixel();
   }

   return CVRES_SUCCESS;
}

// win32 only
#ifdef WIN32
//---------------------------------------------------------------------------
// CreateFromWin32Bmp creates an image from a bitmap buffer.
// WARNING: Currently only supports 24-bit uncompressed RGB bitmaps
//
// Bitmap header and data may be freed after call - we do a deep copy
// of the data we care about.  You must call CVImage::ReleaseImage()
// on the image when you are done.
//      
//        imageType - type of image to create
//        dstImage - uninitialized image ptr. Set on return.
//        bmih - BITMAPINFOHEADER with format information
//        data - raw bitmap data matching bmih format.
//---------------------------------------------------------------------------
CVRES CVImage::CreateFromWin32Bmp(  CVIMAGE_TYPE              imageType,
                                    CVImage*&                 dstImg,
                                    const BITMAPINFOHEADER*   bmih, 
                                    const unsigned char*      data)
{
   dstImg = 0;

   // Sanity checks on input
   CVAssert(bmih != 0, "Invalid bitmap info header.");
   CVAssert(data != 0, "Invalid data ptr");
   if ((bmih == 0) || (data == 0))
   {
      return CVRES_IMAGE_INVALID_DATA;
   }

   CVAssert(bmih->biCompression == BI_RGB,
            "Only uncompressed bmps are supported.");
   if (bmih->biCompression != BI_RGB)
   {
      return CVRES_IMAGE_UNSUPPORTED_FORMAT;
   }

   CVAssert(bmih->biBitCount == 24, "Only 24-bit images are supported here.");
   if (bmih->biBitCount != 24)
   {
      return CVRES_IMAGE_UNSUPPORTED_FORMAT;
   }

   // Pick image type
   switch (imageType)
   {
      case  CVIMAGE_DEFAULT:     // On a normal create, default = RGB24
      case  CVIMAGE_RGB24:       dstImg = new CVImageRGB24();     break;

      case  CVIMAGE_RGBFLOAT:    dstImg = new CVImageRGBFloat();  break;
      
      case  CVIMAGE_GREY:        dstImg = new CVImageGrey();      break;

      //case  CVIMAGE_RGBINT:      
      //case  CVIMAGE_GREYINT:
      //case  CVIMAGE_GREYFLOAT:

      default:
         CVAssert(false,"Invalid CVIMAGE_TYPE!");
         return CVRES_IMAGE_UNKNOWN_TYPE;
   }
   
   // Bail if we couldn't create it 
   if (dstImg == 0)
   {
      return CVRES_OUT_OF_MEMORY;
   }

   // Set it using imagetype-specific set function
   // SetFromWin32Bmp also adds our reference.
   return dstImg->SetFromWin32Bmp(bmih, data);
}
#endif // win32

//---------------------------------------------------------------------------
// Increment reference count
//---------------------------------------------------------------------------
unsigned long  CVImage::AddRef()
{
   // Increment parent reference count
   if (fParentImage != 0)
   {
      fParentImage->AddRef();
   }

   fRefCount++;
   return fRefCount;
}

//---------------------------------------------------------------------------
// Decrement reference count
//---------------------------------------------------------------------------
unsigned long  CVImage::DecRef()
{
   if (fParentImage != 0)
   {
      // If parent is non-null, release our ref count for it.
      // This may delete it.
      ReleaseImage(fParentImage);
      
      // If parent was deleted, then our ref count should be 1    
      if (fParentImage == 0)
      {
         // Make sure that if we deleted parent, we're going to delete here
         CVAssert(fRefCount == 1, "Deleted parent, but we're not done here!");
      }
   }

   CVAssert(fRefCount != 0, "Decrementing reference count too far!");
   if (fRefCount == 0)
      return fRefCount;

   fRefCount--;
   return fRefCount;
}

//---------------------------------------------------------------------------
// Protected constructor
//---------------------------------------------------------------------------
CVImage::CVImage()
{
   fOwnData       = false;
   fData          = 0;
   fWidth         = 0;
   fHeight        = 0;   
   fXOffset       = 0;
   fYOffset       = 0;
   fParentImage   = 0;
   fRefCount      = 0;
}

//---------------------------------------------------------------------------
// Protected destructor
//---------------------------------------------------------------------------
CVImage::~CVImage()
{
   // Dec and check our reference count - make sure we're not in here when
   // we shouldn't be.
   if (fData)
      DecRef();

   CVAssert(fRefCount == 0, "Destructor called while active!");

   if (fOwnData)
   {
      if (fData != 0)
      {
         delete [] fData;
         fData = 0;
      }
   }
}


//---------------------------------------------------------------------------
// Create
//    Creates the buffer for the image. If init is true,
//    the data is initialized to 0. Otherwise, it's whatever
//    happened to be there.
//---------------------------------------------------------------------------
CVRES CVImage::Create(int width, int height, bool init)
{
   CVRES result = CVRES_SUCCESS;

   if ( (width == 0) || (height == 0) )
   {
      return CVRES_IMAGE_INVALID_SIZE;
   }

   
   fData = new unsigned char[width*height*GetBytesPerPixel()];
   if (fData == 0)
   {
      return CVRES_OUT_OF_MEMORY;
   }
   fOwnData = true;

   fWidth   = width;
   fHeight  = height;

   if (init)
      this->Clear();

   this->AddRef();
   
   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// Clear
//    Clears the image data to 0
//---------------------------------------------------------------------------
CVRES CVImage::Clear()
{
   CVAssert(fData != 0, "Can't clear an image unless it's been created.");
   if (fData == 0)
   {
      return CVRES_IMAGE_MUST_INITIALIZE_ERR;
   }

   memset(fData,0,this->Size());
   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// XOffsetRel
//    Returns relative x offset within parent image.
//---------------------------------------------------------------------------
int CVImage::XOffsetRel () const
{
   return fXOffset;
}

//---------------------------------------------------------------------------
// YOffsetRel
//    Returns relative y offset within parent image.
//---------------------------------------------------------------------------
int CVImage::YOffsetRel () const
{
   return fYOffset;
}

//---------------------------------------------------------------------------
// XOffsetAbs
//    Returns absolute X offset within fData
//---------------------------------------------------------------------------
int CVImage::XOffsetAbs () const
{
   int absOffset = fXOffset;
   
   // Recursively add all parent offsets into ours to find real offset
   // within fData.
   if (fParentImage != 0)
   {
      absOffset += fParentImage->XOffsetAbs();
   }
   
   return absOffset;
}

//---------------------------------------------------------------------------
// YOffsetAbs
//    Returns absolute Y offset within fData
//---------------------------------------------------------------------------
int CVImage::YOffsetAbs () const
{
   int absOffset = fYOffset;

   // Recursively add all parent offsets into ours to find real offset
   // within fData.
   if (fParentImage != 0)
   {
      absOffset += fParentImage->YOffsetAbs();
   }
   
   return absOffset;
}
//---------------------------------------------------------------------------
// AbsWidth
//    Returns the absolute width of the fData image buffer
//---------------------------------------------------------------------------
int CVImage::AbsWidth   () const
{
   int absWidth = fWidth;
   if (fParentImage != 0)
   {
      absWidth = fParentImage->AbsWidth();
   }

   return absWidth;
}

//---------------------------------------------------------------------------
// AbsHeight
//    Returns the absolute height of the fData image buffer
//---------------------------------------------------------------------------
int CVImage::AbsHeight  () const
{
   int absHeight = fHeight;
   if (fParentImage != 0)
   {
      absHeight = fParentImage->AbsHeight();
   }

   return absHeight;
}

//---------------------------------------------------------------------------
// Width()
//    Retrieve the width of the image in pixels
//---------------------------------------------------------------------------
int CVImage::Width() const
{
   return fWidth;
}

//---------------------------------------------------------------------------
// Height()
//    Retrieve the height of the image in pixels
//---------------------------------------------------------------------------
int CVImage::Height() const
{
   return fHeight;
}

//---------------------------------------------------------------------------
// Size()
//    Retrieve the size of the image in bytes
//---------------------------------------------------------------------------
int CVImage::Size() const
{
   return fWidth * fHeight * GetBytesPerPixel();
}

//---------------------------------------------------------------------------
// AbsSize()
//    Retrieve the absolute size of the root image in bytes
//---------------------------------------------------------------------------
int CVImage::AbsSize() const
{
   return AbsWidth() * AbsHeight() * GetBytesPerPixel();
}

//---------------------------------------------------------------------------
// IsImageRoot()
//    Checks if an image is the root image. Returns true if so.
//    Otherwise, it is a sub image of another image.
//---------------------------------------------------------------------------
bool CVImage::IsImageRoot() const
{
   bool isRoot = fOwnData;
   
   CVAssert(isRoot == (fParentImage == 0), 
      "Root image should not have a parent image, sub images must.");

   return isRoot;
}

//---------------------------------------------------------------------------
// GetRawDataPtr()
//    Retrieve the image data ptr
//
//    Remember that this may be a sub image, in which case the data ptr 
//    returned is to the raw buffer of a parent image.
//
//    So, any time you access the raw data you should use the
//    XOffsetAbs, YOffsetAbs, AbsWidth, and AbsHeight functions
//    when calculating your pointers into the data.
//
//    Also remember that if you modify a sub image, you'll be modifying
//    the parent image and any other overlapping sub images.  If you
//    want to muck with an image without messing up any others, use
//    CVImage::CopyImage() first to get a base image that owns its own
//    data.
//
//    Note that the xOffset and yOffset are relative to parent. They will
//    be calculated at runtime if you call this function.      
//---------------------------------------------------------------------------
unsigned char* CVImage::GetRawDataPtr() const
{
   return fData;
}

//---------------------------------------------------------------------------
// IsBigEndianMachine()
//    Returns true if we are on a big endian machine
//    False if we are on a little endian machine.
//---------------------------------------------------------------------------
bool CVImage::IsBigEndianMachine()
{
   unsigned char endianTest[4] = {1,2,3,4};
   int endianInt = *(int *)endianTest;
   if (endianInt == 0x1234)
   {
      return true;
   }
   return false;
}


//---------------------------------------------------------------------------
// Load()
//    Creates the appropriate image type based on type of file.
// newImage should *not* be instantiated prior to passing it in.
//
// Currently only supports binary .pgm, .ppm, .pxm, and .pdm formats.
//
// .pgm and .ppm were created by Jef Poskanzer with his 
// Portable Bitmap Utilities, and are not only very simple but also
// widely supported and well documented.
//
// .pxm is based on the .ppm format, but using 32-bit floating point
// values for R, G, and B.  The magic value used for .pxm files differs
// depending on endian-ness.  I'm using P7 for little endian, and P8 for
// big-endian.
//
// .pdm is a similar idea, but with 32-bit integer values.  The magic
// values for .pdm files are P9 for little-endian, and PA for big-endian.
//
// In both .pxm and .pdm files, the max value is ignored, although written.
//
//---------------------------------------------------------------------------
CVRES CVImage::Load(const char* filename, CVImage*& newImage)
{
   CVRES result = CVRES_SUCCESS;

   // Init new image to nadda.
   newImage = 0;

   CVFile file;
   int    i;
   // Supported formats - .pgm, .ppm, .pxm, .pdm.
   // First check to see if filename exists as requested,
   // then scan for files matching the filename with
   // those extensions in that order.
   const int kNumFormats = 4;
   const char* kFormatExtensions[kNumFormats] = 
   {
      ".pgm",
      ".ppm",
      ".pxm",
      ".pdm"
   };

   // Allocate enough memory for filename + added extension and null if required.
   char *fullFilename = new char[strlen(filename) + 5];
   if (fullFilename == 0)
   {
      return CVRES_OUT_OF_MEMORY;
   }

   strcpy(fullFilename, filename);

   bool foundFile = false;

   if (!file.FileExists(fullFilename))
   {
      // File doesn't exist as specified. Check for file with extensions.
      char *fileExtensionPtr = fullFilename + strlen(fullFilename);
      
      i = 0;
      while ((!foundFile) && (i < kNumFormats))
      {
         strcpy(fileExtensionPtr,kFormatExtensions[i]);
         if (file.FileExists(fullFilename))
         {
            // Found it!
            foundFile = true;
         }
         i++;
      }

      // Bail if we didn't find any matching files.
      if (!foundFile)
      {         
         CVTrace("Requested image file does not exist.");
         delete [] fullFilename;
         return CVRES_FILE_DOES_NOT_EXIST;
      }
   }

   if (CVFAILED(result = file.OpenForRead(fullFilename)))
   {
      CVTrace("Error opening image file.\n");
      delete [] fullFilename;
      return result;
   }
   
   // clean up filename now - we don't need it.
   delete [] fullFilename;

   // Create a large buffer for reading in header.
   // We really shouldn't need it, but maybe someone makes huge comments?
   char* loadBuffer = new char[1024];
   if (loadBuffer == 0)
   {
      file.Close();
      return CVRES_OUT_OF_MEMORY;
   }

   memset(loadBuffer,0,1024);

   unsigned long amountRead = 0;

   // Load in first line - it should contain the magic values 'Px', where
   // x specifies the format.
   if (CVFAILED(result = file.ReadLine(loadBuffer, 1024, amountRead)))
   {
      delete [] loadBuffer;
      file.Close();
      return result;
   }

   if (loadBuffer[0] != 'P')
   {
      // It isn't a P?M file. Bail now.
      delete [] loadBuffer;
      file.Close();
      return CVRES_IMAGE_UNKNOWN_TYPE;
   }

   // Type of image we're loading
   CVIMAGE_TYPE imageType;

   // Do we need to flip the endian-ness of the data?
   bool endianFun = false;

   switch (loadBuffer[1])
   {
      case '5':
         // It's a grey image
         imageType = CVIMAGE_GREY;
         break;
      case '6':
         // It's a 24-bit RGB image
         imageType = CVIMAGE_RGB24;
         break;
      case '7':
         // Custom little-endian RGB floating point
         imageType = CVIMAGE_RGBFLOAT;
         if (CVImage::IsBigEndianMachine())
         {
            endianFun = true;
         }
         break;
      case '8':
         // Custom big-endian RGB floating point
         imageType = CVIMAGE_RGBFLOAT;
         if (!CVImage::IsBigEndianMachine())
         {
            endianFun = false;
         }
         break;      

      case '1':   // These are all ascii types that we don't support.
      case '2':   // ""
      case '3':   // ""
      case '4':   // ""
      case '9':   // Custom little-endian RGB 32-bit integer format
      case 'A':   // Custom big-endian RGB 32-bit integer format;
      default:
         delete [] loadBuffer;
         file.Close();
         return CVRES_IMAGE_UNSUPPORTED_TYPE;
         break;
   }

   // OK, we now know the image type and endian-ness, get the
   // image size and ignore the number of colors and we're ready to go.
	// Scan for width/height string, skip comment lines
   if (CVFAILED(result = ReadNonCommentLine(&file, loadBuffer, 1024)))
   {
      delete [] loadBuffer;
      file.Close();
      return result;
   }
   
   // Got a non-comment line. It should be "Width Height\n". Parse it.
   int imgWidth, imgHeight;

   if (2 != sscanf(loadBuffer,"%d %d",&imgWidth,&imgHeight))
   {
      // It should have returned 2 - one for width and one for height.
      // Dunno what it is, but it's not a proper .p?m.
      delete [] loadBuffer;
      file.Close();
      return CVRES_IMAGE_UNKNOWN_TYPE;
   }

   // Get the number of colors line, and we're done!
   if (CVFAILED(result = ReadNonCommentLine(&file,loadBuffer,1024)))
   {
      // Hmm... couldn't find another good line. Bail.
      delete [] loadBuffer;
      file.Close();
      return CVRES_IMAGE_UNKNOWN_TYPE;
   }

   // Done with loadBuffer now - we've got the info, so now create
   // the image.
   delete [] loadBuffer;

   // create it, but don't bother initializing the memory - we're
   // going to load the buffer directly in over it.
   if (CVFAILED(result = CVImage::CreateImage(  imageType, 
                                                newImage, 
                                                imgWidth, 
                                                imgHeight, 
                                                false)))
   {
      // Bizarre - couldn't create image. Return the error code.
      file.Close();
      return result;
   }

   // Got image, got file, load it in...
   if (CVFAILED(result = file.Read(  newImage->GetRawDataPtr(), 
                                     newImage->Size(),
                                     amountRead)))
   {
      // Error on read. Bail.
      file.Close();
      CVImage::ReleaseImage(newImage);
      return result;
   }

   // Double check that we got what we were expecting...
   if (amountRead != (unsigned long)newImage->Size())
   {
      // Invalid amount read!
      file.Close();
      CVImage::ReleaseImage(newImage);
      return CVRES_IMAGE_FILE_CORRUPTED;
   }

   // Ah, now for endian fun.   
   if (endianFun)
   {
      // We need to flip all of the bytes loaded into the data.
      // Width and height loaded previously will be fine, since
      // they were byte-wise ASCII, but floats or integers will
      // be flipped.

      // right now, we're assuming that any image
      // with 4-byte, 12-byte, or 16-byte pixels uses
      // 4-byte values to store the pixel channels.  If this
      // changes, the assert will need to be changed.
      //
      // The only one we'll actually see right now is
      // the 12-byte one for RGBFloat.  RGBInt type
      // will also be 12 bytes.
      //
      // 16 bytes would be used for RGBA32 or RGBAFloat.
      // 4 bytes would be used for GreyInt or GreyFloat.
      //
      CVAssert( (newImage->GetBytesPerPixel() == 12) ||
                (newImage->GetBytesPerPixel() == 16) ||
                (newImage->GetBytesPerPixel() == 4),
               "Need to add support for flipping endian-ness for " \
               "this format!");

      // Number of flips = total size / 4.
      int numFlips = newImage->Size() / 4;
      
      // Not really planning on using it on a non 32-bit platform yet,
      // but if we do we need to abstract the types appropriately. We're
      // expecting 32-bit unsigned int here.
      CVAssert(sizeof(unsigned int) == 4, 
         "Need to abstract basic types for platform. unsigned int != 32-bit");
      
      // Get a 32-bit integer pointer to the buffer
      unsigned int* flippedPtr = (unsigned int*)newImage->GetRawDataPtr();
      
      // Run through entire buffer flipping dwords as we go.
      for (i = 0; i < numFlips; i++)
      {
         unsigned int curDword = *flippedPtr;
         
         // Flip it and store.
         *flippedPtr = ( ((curDword & 0xff000000) >> 24) |
                         ((curDword & 0x00ff0000) >> 8)  |
                         ((curDword & 0x0000ff00) << 8)  |
                         ((curDword & 0x000000ff) << 24) );
         
         // Go to next dword.
         flippedPtr++;
      }               
   }
   
   // Close file and exit
   return file.Close();
}

//---------------------------------------------------------------------------
// ReadNonCommentLine()
//     Reads lines until we hit a non-comment one in a .p?m
//---------------------------------------------------------------------------
CVRES CVImage::ReadNonCommentLine(CVFile* file, char* buffer, int maxBufLen)
{
   unsigned long lastAmountRead = 0;
   unsigned long amountRead = 0;
   CVRES         result;
   
   CVAssert(file != 0, "Need a valid CVFile* to read from.");
   if (file == 0)
   {
      return CVRES_INVALID_PARAMETER;
   }


   do
	{
      lastAmountRead = amountRead;
		if (CVFAILED(result = file->ReadLine(buffer,1024,amountRead)))
      {
         return result;
      }

     // Continue reading until we hit a non-comment line.
     // If some crazy person creates a >1k comment line, then we treat 
     // the subsequent reads as a comments until we hit an EOL.
	} while ((buffer[0] == '#') || (lastAmountRead == 1024));

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// Saves an image file to disk. Format is automatically chosen depending
// on image type.  See Load() for comments.
// 
// File extension is appended if none is set (preferred)
//
//---------------------------------------------------------------------------
CVRES CVImage::Save( const char*    filename, 
                     const CVImage* outputImage, 
                     bool           overwrite)
{
   CVRES result = CVRES_SUCCESS;

   CVAssert(outputImage->fData != 0, "Empty images cannot be saved.");
   if (outputImage->fData == 0)
   {
      return CVRES_IMAGE_EMPTY_ERR;
   }

   // Copy filename to path and add extension if none is present.
   char filepath[_MAX_PATH + 1];
   memset(filepath, 0, _MAX_PATH+1);
   if (strchr(filename, '.') == 0)
   {     
      strncpy(filepath, filename, _MAX_PATH - 4);
      strcat(filepath, outputImage->GetPNMExtension());
   }
   else
   {
      strncpy(filepath, filename, _MAX_PATH);
   }
   
   // Create file    
   CVFile file;

   if (file.FileExists(filepath))
   {
      if (!overwrite)
      {
         CVTrace("Not in overwrite mode - aborting image save.");
         return CVRES_IMAGE_ALREADY_EXISTS;
      }
   }

   if (CVFAILED(result = file.Create(filepath)))
   {
      CVTrace("Error creating image file.\n");
      return result;
   }
   
   // Create a temporary buffer for strings in header
   
   // 12+32+32 max length = 78 bytes with max fields. 
   char *tmpBuf = new char[80]; 
   if (tmpBuf == 0)
   {
      file.Close();
      return CVRES_OUT_OF_MEMORY;
   }   

   // Setup .p?m header 
   float maxPix;
   outputImage->GetMaxPixelValue(maxPix);

   sprintf( tmpBuf,
            "P%c\n%d %d\n%.0f\n",
            outputImage->GetPNMMagicVal(), 
            outputImage->Width(),
            outputImage->Height(),
            maxPix);

   // Write out header and delete temp buffer
   result = file.WriteString(tmpBuf);

   delete [] tmpBuf;

   // Bail on header write failure
   if (CVFAILED(result))
   {
      file.Close();
      return result;
   }

   // Write out the raw image data a line at a time
   int y;
   for (y = 0; y < outputImage->Height(); y++)
   {
      // Calc position
      unsigned char* linePtr =   
         outputImage->GetRawDataPtr() +
         (  (y + outputImage->YOffsetAbs()) * 
            outputImage->AbsWidth() * 
            outputImage->GetBytesPerPixel() )  +
         (outputImage->XOffsetAbs() * outputImage->GetBytesPerPixel()) ;
      
      // Write line
      result = file.Write( linePtr, 
                           outputImage->Width() * 
                              outputImage->GetBytesPerPixel());
      if (CVFAILED(result))
      {  
         file.Close();
         return result;
      }
   }
   
   file.Close();

   
   return CVRES_SUCCESS;
}

// Win32 only 
#ifdef WIN32
//---------------------------------------------------------------------------
// SetFromWin32Bmp
//    This must be overridden by child classes.
//---------------------------------------------------------------------------
CVRES CVImage::SetFromWin32Bmp(  const BITMAPINFOHEADER* bmih, 
                                 const unsigned char*    data)
{
   CVAssert(false, 
      "You must override SetFromWin32Bmp in CVImage child classes.");
   
   return CVRES_NOT_IMPLEMENTED;
}
#endif // WIN32 only

//---------------------------------------------------------------------------
// Because of the factory style setup, we aren't going to support
// copy operators. So it's protected, and just asserts.
// Use CreateCopy() or CreateSub() instead depending on whether you
// want a deep or shallow copy of the object.
//---------------------------------------------------------------------------
CVImage& CVImage::operator=(const CVImage& img)
{
   CVAssert(false,"operator= not supported for CVImage.");
   return *this;
}

//---------------------------------------------------------------------------
// SetSubPosition
//    Move the ROI of the image within its parent.
//    Will return CVRES_IMAGE_OPERATION_INVALID_ON_ROOT if you try
//    to use this on a root image instead of a sub image.
//---------------------------------------------------------------------------
CVRES CVImage::SetSubPosition (  int      newXOffset,
                                 int      newYOffset,
                                 int      newWidth,
                                 int      newHeight )
{
   
   // Cannot set offsets or modify width/height on the root image.
   CVAssert(IsImageRoot() == false, 
      "Cannot SetSubPosition on a root image.");   
   
   if (IsImageRoot())
   {     
      return CVRES_IMAGE_OPERATION_INVALID_ON_ROOT;
   }

   // Perform sanity checks on values.
   CVAssert(newXOffset >= 0, "XOffset must be >= 0");
   CVAssert(newYOffset >= 0, "YOffset must be >= 0");
   
   CVAssert(newXOffset + newWidth  <= fParentImage->fWidth,  
            "Invalid sub image width");
   
   CVAssert(newYOffset + newHeight <= fParentImage->fHeight, 
            "Invalid sub image height");

   CVAssert(fParentImage->fData != 0, "Parent image is invalid.");

   // Bail on invalid parameters (same checks as asserts above)
   if ((newXOffset < 0) || (newYOffset < 0)        ||
       (newXOffset + newWidth  > fParentImage->fWidth)      ||
       (newYOffset + newHeight > fParentImage->fHeight)     ||
       (fParentImage->fData == 0))
   {
      return CVRES_IMAGE_INVALID_SUB_POSITION;
   }

   // Okay, position is valid. Make the change.
   fXOffset = newXOffset;
   fYOffset = newYOffset;
   fWidth   = newWidth;
   fHeight  = newHeight;

   return   CVRES_SUCCESS;
}


