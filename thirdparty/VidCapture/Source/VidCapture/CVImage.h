/// \file CVImage.h 
/// \brief Parent image class for image capture and processing.
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
/// \class CVImage CVImage.h 
/// CVImage is the root image class for image capture and processing.
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
/// <i>A note on threading....<BR>
/// CVImage objects should be accessed in a serialized manner.
/// Additionally, all child (sub)images and parent images of a CVImage
/// object should be serialized with each other.  If you want to access
/// a single image (or an image and its sub-images) simultaneously from 
/// multiple threads for anything but the const functions, I'd highly 
/// recommend making a copy of it first with CVImage::CreateCopy().
/// </i>
///
/// \sa CVImageGrey, CVImageRGB24, CVImageRGBFloat
///
/// $RCSfile: CVImage.h,v $
/// $Date: 2004/02/08 23:47:39 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $

#ifndef _CVImage_H_
#define _CVImage_H_


#include "CVResImage.h"
#include "CVImageStructs.h"
#ifdef WIN32
   #include <windows.h> // For SetFromWin32Bmp
#endif

class CVImageRGB24;
class CVImageRGBFloat;
class CVImageGrey;
class CVFile;

class CVImage
{
   public:
      //---------------------------------------------------------------------
      // Image types
      //---------------------------------------------------------------------
      
      // I've pulled out the original numeric definitions to CVImageStructs.h
      // so that they can be used by the seperate 'C'-style interface, but
      // these are kept for compatibility with existing code.
      enum CVIMAGE_TYPE
      {
         /// Default type (sometimes used for auto-detect)
         CVIMAGE_DEFAULT   = CVIMAGETYPE_DEFAULT,
         
         /// 8-bit red, green, blue triplets
         CVIMAGE_RGB24     = CVIMAGETYPE_RGB24,
         
         /// 32-bit float red, green, blue triplets
         CVIMAGE_RGBFLOAT  = CVIMAGETYPE_RGBFLOAT,
         
         /// 8-bit intensity values
         CVIMAGE_GREY      = CVIMAGETYPE_GREY,
         
         //--- currently unimplemented below this line 
         //CVIMAGE_RGBINT,      // 32-bit integer red, green, blue triplets
         //CVIMAGE_GREYINT,     // 32-bit integer intensity values
         //CVIMAGE_GREYFLOAT    // 32-bit floating point intensity values
      };

      //---------------------------------------------------------------------
      // static image file functions
      // These  
   public:                             

      /// CreateImage creates an image of the appropriate type.
      ///
      /// Use this instead of new to create image objects.
      /// If width and height are non-zero, creates the appropriate memory for
      /// the image and adds a reference. Otherwise, does not create memory 
      /// buffer or add reference.
      ///
      /// Call CVImage::ReleaseImage() on the returned image when done.
      ///
      /// \param type - specifies the type of image to create.
      /// \param image - uninitialized image ptr. Set on return.
      /// \param width - desired width of image in pixels.
      /// \param height - desired height of image in pixels.
      /// \param init - if true, image data is set to 0.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      static CVRES CreateImage      (  CVIMAGE_TYPE         type,
                                       CVImage*&            image,
                                       int                  width,
                                       int                  height,
                                       bool                 init = true);


      /// ReleaseImage decrements the reference count of an image and
      /// will free the image if it hits zero.
      /// It may also free parent images if the specified
      /// image holds the last reference to a parent.
      ///
      /// \param image - pointer to image to release. Is set to NULL
      ///                if last reference was deleted.
      /// 
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::CreateImage()
      static CVRES ReleaseImage     (  CVImage*&         image );

      /// CreateCompatible creates an image of the same type as the
      /// specified srcImg.  
      /// This version also uses the source image's width and height 
      /// for the new image.
      ///
      /// Call CVImage::ReleaseImage() on returned image when done.
      ///
      /// \param srcImg - source image to get information from.
      /// \param dstImg - uninitialized image ptr. Set on return.
      /// \param init - if true, image data is set to 0.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      static CVRES CreateCompatible (  const CVImage*    srcImg,
                                       CVImage*&         dstImg,
                                       bool              init = true);

      /// CreateCompatible creates an image of the same type as the
      /// specified srcImg.  
      /// This version uses user-specified dimensions for the new image.
      /// Call CVImage::ReleaseImage() on returned image when done.
      ///
      /// \param srcImg - source image to get information from.
      /// \param dstImg - uninitialized image ptr. Set on return.
      /// \param width - width of desired image in pixels.
      /// \param height - height of desired image in pixels.
      /// \param init - if true, image data is set to 0.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      static CVRES CreateCompatible (  const CVImage*    srcImg,
                                       CVImage*&         dstImg,
                                       int               width,
                                       int               height,
                                       bool              init = true);

      /// CreateSub creates a sub-image of the specified parent.
      ///
      /// dstImage should not be instantiated prior to calling CreateSub.
      /// Sub image is returned in dstImage.
      ///
      /// The sub image references the parent's fData
      /// member, and the parent image must not be deleted before you
      /// are done using the sub image.
      ///
      /// You may create a sub image of a sub image, ad infinum. It'll
      /// handle the offsets.  Call CVImage::ReleaseImage() when done.
      ///
      /// \param orgImg - parent image to derive subimage from.
      /// \param dstImg - uninitialized image ptr. Set on return.
      /// \param xOffset - relative x offset for top-left of sub image.
      /// \param yOffset - relative y offset for top-left of sub image.
      /// \param width - width of sub image.
      /// \param height - height of sub image.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      /// \sa CVImage::SetSubPosition()
      static CVRES CreateSub        (  const CVImage*    orgImg,
                                       CVImage*&         dstImg,
                                       int               xOffset,
                                       int               yOffset,
                                       int               width,
                                       int               height);

      /// CopyImage() creates a new image of the same type as srcImg
      /// and stores it in dstImg.  The data from srcImg is
      /// copied into a buffer owned by dstImg. Caller must call
      /// CVImage::ReleaseImage() on the returned image when done.
      ///
      /// \param srcImg - source image to copy from
      /// \param dstImg - uninitizlied image ptr. Contains copy
      ///                 of srcImg on return.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      static CVRES CopyImage        (  const CVImage*    srcImg,
                                       CVImage*&         dstImg);
      
      /// CopyImage() creates a new image of the same type as srcImg
      /// and stores it in dstImg.  The data from srcImg is
      /// copied into a buffer owned by dstImg. Caller must call
      /// CVImage::ReleaseImage() on the returned image when done.
      ///
      /// This version allows you to specify offsets, width, and
      /// height of the data to copy into a new image.
      /// 
      /// \param srcImg - source image to copy from
      /// \param dstImg - uninitialized image ptr. Contains copy
      ///                 of srcImg on return.
      /// \param xOffset - relative x offset for top-left of copy.
      /// \param yOffset - relative y offset for top-left of copy.
      /// \param width - width of destination image.
      /// \param height - height of destination image.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      static CVRES CopyImage        (  const CVImage*    srcImg,
                                       CVImage*&         dstImg,
                                       int               xOffset,
                                       int               yOffset,
                                       int               width,
                                       int               height);
      
#ifdef WIN32
      /// CreateFromWin32Bmp creates an image from a bitmap buffer.
      /// WARNING: Currently only supports 24-bit uncompressed RGB bitmaps
      ///
      /// Bitmap header and data may be freed after call - we do a deep copy
      /// of the data we care about.  You must call CVImage::ReleaseImage()
      /// on the image when you are done.
      ///      
      /// \param imageType - type of image to create
      /// \param dstImage - uninitialized image ptr. Set on return.
      /// \param bmih - BITMAPINFOHEADER with format information
      /// \param data - raw bitmap data matching bmih format.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      static CVRES CreateFromWin32Bmp(  CVIMAGE_TYPE             imageType,
                                        CVImage*&                dstImage,
                                        const BITMAPINFOHEADER*  bmih, 
                                        const unsigned char*     data);
#endif      
      /// IsBigEndianMachine returns true if it is on a big-endian machine,
      /// such as a Macintosh, and returns false on little-endian machines
      /// like Intel or AMD PC's.
      ///
      /// \return bool - true if big-endian, false if little-endian.
      static bool IsBigEndianMachine();

      
      /// Load() creates the appropriate image type based on type of file.
      ///
      /// newImage should *not* be instantiated prior to passing it in.
      ///
      /// Currently only supports binary .pgm, .ppm, .pxm, and .pdm formats.
      ///
      /// .pgm and .ppm were created by Jef Poskanzer with his 
      /// Portable Bitmap Utilities, and are not only very simple but also
      /// widely supported and well documented.
      ///
      /// .pxm is based on the .ppm format, but using 32-bit floating point
      /// values for R, G, and B.  The magic value used for .pxm files 
      /// differs depending on endian-ness.  I'm using P7 for little endian, 
      /// and P8 for big-endian.
      ///
      /// .pdm is a similar idea, but with 32-bit integer values.  The magic
      /// values for .pdm files are P9 for little-endian, and PA for 
      /// big-endian.
      ///
      /// In both .pxm and .pdm files, the max value is ignored on load,
      /// although written in Save().  The max pixel value is written as
      /// 255 for .ppm's and .pgm's and is also ignored on load.
      ///
      /// \param filename - ASCIIZ filename of the image to load
      /// \param newImage - uninitialized pointer to image. Set on return.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, ::CVRES_FILE_ENUM,
      /// \sa CVImage::Save()      
      static CVRES Load(   const char*    filename, 
                           CVImage*&      newImage);
      
      /// Save() saves an image file to disk. 
      ///
      /// Format is automatically chosen depending on image type.  
      /// See Load() for comments.
      /// 
      /// File extension is appended if none is set (preferred)
      ///
      /// \param filename - ASCIIZ filename of image to save. You do not need
      ///          to include the extension - it is automatically determined.
      /// \param outputImage - Image to write to the file.
      /// \param overwrite - if true, overwrites the file if 
      ///                    one already exists.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, ::CVRES_FILE_ENUM,
      /// \sa CVImage::Load()
      static CVRES Save(   const char*    filename, 
                           const CVImage* outputImage,
                           bool           overwrite = true);


   public:
      //---------------------------------------------------------------------
      // Public non-static member functions that need to be overridden by
      // child classes.
      //---------------------------------------------------------------------      

      /// GetNumChannels retrieves the number of channels per pixel.
      /// This is one in greyscale, 3 in RGB, and 4 in RGBA
      ///
      /// \return int - number of channels per pixel.
      /// \sa GetBytesPerPixel()
      virtual int GetNumChannels() const = 0;

      /// GetBytesPerPixel retrieves the number of bytes per pixel.  
      /// Note that pixel can be in floating point or integer format, depending
      /// on the image type.      
      ///
      /// \return int - bytes per pixel
      /// \sa GetNumChannels()
      virtual int GetBytesPerPixel() const = 0;

      /// GetImageType() retrieves the image type. 
      /// See CVIMAGE_TYPE enum.
      ///
      /// \return CVIMAGE_TYPE specifying the image's type.
      virtual CVIMAGE_TYPE   GetImageType   () const = 0;

      /// GetPNMExtension() retrieves the default file extension for PNM
      /// file saving. (e.g. ".pgm" for greyscale)
      ///
      /// \return const char* - ASCIIZ default file extension, 
      ///                       including preceeding '.'
      /// \sa Load(), Save()
      virtual const char *GetPNMExtension() const = 0;

      /// GetPNMMagicVal() retrieves the magic value for a pnm file
      /// matching the current image format.
      ///
      /// \return char - Magic value for PNM files (e.g. '5' for 'P5' .pgm's)
      /// \sa Load(), Save()
      virtual char GetPNMMagicVal() const = 0;

      /// GetMaxPixelValue() retrieves the maximum value of any pixel in
      /// the image.  
      ///
      /// In multichannel images (e.g. RGB triplets), it will
      /// return the maximum value on any of the channels.
      ///
      /// All child classes should implement this.
      /// \param maxValue - reference to max pixel value, set on return.
      /// \return CVRES result code
      /// \sa GetPixel(), SetPixel(), GetMaxPixel()
      virtual CVRES GetMaxPixelValue(float& maxValue) const = 0;

      /// GetPixel() retrieves the red, green, and blue values for a specified
      /// pixel as floating points.
      ///
      /// This is for convenience and prototyping - for high-speed image
      /// processing you'll need to work more directly with the image
      /// buffer.
      ///
      /// All child classes should implement this.
      ///
      /// \param x - x position within the image of the pixel
      /// \param y - y position within the image of the pixel
      /// \param r - receives the red value of the pixel
      /// \param g - receives the green value of the pixel
      /// \param b - receives the blue value of the pixel
      ///
      /// \return CVRES result code.  CVRES_SUCCESS on success.
      /// \sa SetPixel()
      virtual CVRES GetPixel( int      x,
                              int      y,
                              float&   r,
                              float&   g,
                              float&   b) const = 0;

      /// SetPixel() sets the red, green, and blue pixel values
      /// for a pixel
      ///
      /// This is for convenience and prototyping - for high-speed image
      /// processing you'll need to work more directly with the image
      /// buffer.
      ///
      /// All child classes should implement this.
      ///
      /// \param x - x position within the image of the pixel
      /// \param y - y position within the image of the pixel
      /// \param r - receives the red value of the pixel
      /// \param g - receives the green value of the pixel
      /// \param b - receives the blue value of the pixel
      ///
      /// \return CVRES result code.  CVRES_SUCCESS on success.
      /// \sa GetPixel()
      virtual CVRES SetPixel       (   int      x,
                                       int      y,
                                       float    r,
                                       float    g,
                                       float    b) = 0;
                                 

      //---------------------------------------------------------------------
      // Protected non-static functions that should be overridden
      //---------------------------------------------------------------------       
   protected:
#ifdef WIN32
      /// Win32-only function for setting image data from a bitmap.      
      /// WARNING: Currently only supports 24-bit uncompressed RGB bitmaps
      ///
      /// Bitmap header and data may be freed after call - we do a deep copy
      /// of the data we care about.
      ///
      /// \param bmih - BITMAPINFOHEADER with format information
      /// \param data - raw bitmap data matching bmih format.
      ///
      /// \return CVRES result code 
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM
      virtual CVRES SetFromWin32Bmp(  const BITMAPINFOHEADER* bmih, 
                                      const unsigned char*    data);
#endif // WIN32

  
   public:
      //---------------------------------------------------------------------
      // Public implemented functions
      //
      // The rest are probably okay as is. Make 'em virtual and move them up
      // if overriding becomes necessary for some reason.
      //
      //---------------------------------------------------------------------
               
      /// Clear() sets all the pixels in the image to 0.
      ///
      /// \return CVRES result code.
      CVRES Clear();

      /// SetSubPosition() moves the ROI of the image within its parent.
      /// Will return CVRES_IMAGE_OPERATION_INVALID_ON_ROOT if you try
      /// to use this on a root image instead of a sub image.
      ///
      /// \param newXOffset - relative x offset of new pos within parent.
      /// \param newYOffset - relative y offset of new pos within parent.
      /// \param newWidth - new width of sub image.
      /// \param newHeight - new height of sub image.
      ///
      /// \return CVRES result code.      
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::CreateSub()
      CVRES SetSubPosition (  int      newXOffset,
                              int      newYOffset,
                              int      newWidth,
                              int      newHeight );

      /// GetRawDataPtr() retrieves the image data ptr.
      ///
      /// Remember that this may be a sub image, in which case the data ptr 
      /// returned is to the raw buffer of a parent image. It can also be
      /// of any format / byte size - check GetImageType() to determine
      /// how to access the buffer.
      ///
      /// So, any time you access the raw data you should use the
      /// XOffsetAbs, YOffsetAbs, AbsWidth, and AbsHeight functions
      /// when calculating your pointers into the data.
      ///
      /// Also remember that if you modify a sub image, you'll be modifying
      /// the parent image and any other overlapping sub images.  If you
      /// want to muck with an image without messing up any others, use
      /// CVImage::CopyImage() first to get a base image that owns its own
      /// data.
      ///
      /// Note that the xOffset and yOffset are relative to parent. They will
      /// be calculated at runtime if you call this function.      
      ///
      /// \return unsigned char* to image data
      unsigned char* GetRawDataPtr() const;
      
      /// Returns relative offset within parent image.
      /// \return int - X Offset relative to parent.
      int   XOffsetRel  () const;

      /// Returns relative offset within parent image.
      /// \return int - Y Offset relative to parent.
      int   YOffsetRel  () const;
   
      /// Returns absolute X offset within fData.
      /// \return int - Absolute x offset of current image within root
      int   XOffsetAbs  () const;

      /// Returns absolute Y offset within fData.
      /// \return int - Absolute y offset of current image within root
      int   YOffsetAbs  () const;

      /// Returns the absolute width of the fData image buffer.
      /// \return int - Absolute width of root image
      int   AbsWidth    () const;

      /// Returns the absolute height of the fData image buffer.
      /// \return int - Absolute height of root image
      int   AbsHeight   () const;

      /// Returns width of image in pixels.
      /// \return int - Width of image
      int   Width       () const;

      /// Returns height of image in pixels.
      /// \return int - Height of image
      int   Height      () const;

      /// Returns size of image buffer in bytes.
      /// \return int - Size of image in bytes
      int   Size        () const;

      /// Returns absolute size of parent image.
      /// \return int - Absolute size of root image in bytes.
      int   AbsSize     () const;

      /// IsImageRoot() returns true if the image is a root (parent)
      /// image that owns its own data.  If the image is a sub image,
      /// it returns false.
      ///
      /// \return bool - true if the image is a root (without a parent),
      ///                or fase otherwise.
      bool  IsImageRoot () const;

      /// AddRef increments our reference count and calls
      /// AddRef on our parent if one exists.
      /// \return long - current reference count
      unsigned long  AddRef();

      /// DecRef decrements our reference count and calls
      /// ReleaseImage() on our parent if one exists. 
      /// \return long - current reference count
      unsigned long  DecRef();      
      

      //---------------------------------------------------------------------
      // Protected functions that shouldn't need overriding
   protected:
      /// Constructor for CVImage - use CVImage::CreateImage() or similar 
      /// static functions instead!
      ///
      CVImage  ();
      
      /// Destructor for CVImage - use CVImage::ReleaseImage()
      virtual ~CVImage();

      /// operator= is overridden to generate an error if someone
      /// attempts to use it - Don't use it!
      /// Use CreateCopy() or CreateSub() instead depending on whether you
      /// want a deep or shallow copy of the object.            
      CVImage& operator=(const CVImage& img);

      /// Create - internal image creation function.
      /// Creates the buffer and sets it up, then calls AddRef().
      ///
      /// \param width - width of image in pixels.
      /// \param height - height of image in pixels.
      /// \param init - if true, initializes image pixels to 0.
      ///
      /// \return CVRES result code.
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM
      CVRES    Create   (  int            width, 
                           int            height, 
                           bool           init);


      /// ReadNonCommentLine() reads a non-comment line from a .p?m.
      ///
      /// Eventually this function should be moved to an image-file
      /// handling class or image factory.
      ///
      /// \param file - an open file to read from. 
      /// \param buffer - buffer to load line into
      /// \param maxBufLen - max amount to read into buffer
      /// \return CVRES result code
      /// \sa CVFile::ReadLine(), Load()
      static CVRES ReadNonCommentLine( CVFile*        file, 
                                       char*          buffer, 
                                       int            maxBufLen);

      /// GetMaxPixel() provides an easy way to retrieve the maximum pixel
      /// value. 
      ///
      /// Right now, we call it from GetMaxPixelValue() in the
      /// child classes, so the caller doesn't have to deal with undue
      /// templating and we don't have to deal with rewriting the code each
      /// time.
      ///
      /// Note that the PixelTypeT type is specific to the type of image.
      /// CVImageGrey uses an unsigned character, CVImageRGBFloat uses
      /// a float, etc. Using the wrong type is a good way to go haywire
      /// in the memory, which is one reason why this is protected and 
      /// encapsulated via GetMaxPixelValue().
      ///
      /// \param maxVal - reference to value, set on return. Must be of the same
      ///                 pixel type that the image contains.
      ///
      /// \return CVRES result code.      
      /// \sa GetMaxPixelValue()
      template <class PixelTypeT>
      CVRES GetMaxPixel(PixelTypeT& maxVal) const
      {
         int x,y,i;

         CVAssert(fData != 0, "Image must be created first!");
         if (fData == 0)
         {
            return CVRES_IMAGE_MUST_INITIALIZE_ERR;
         }
   
         int numChannels =  this->GetNumChannels();
         // Offset into line for source buffer in bytes
         int lineOffset = this->XOffsetAbs() * numChannels;

         // Absolute length of line in fData in bytes ( >= fWidth * 3)
         int lineLength = this->AbsWidth()   * numChannels;
   
         // Distance from end of one line to beginning of the next in bytes.
         int lineStep   = lineLength - (fWidth * numChannels);
   
         // current position of start of buffer in source
         PixelTypeT* curPtr = ((PixelTypeT *)fData) + lineOffset + 
                              (YOffsetAbs() * lineLength);

         // Initialize max value to first pixel
         // (initializing it to 0 doesn't work if we have an all-negative
         //  image)
         maxVal = *curPtr;

         // Loop through our image finding the maximum pixel value.
         for (y = 0; y < fHeight; y++)
         {      
            for (x = 0; x < fWidth; x++)
            {
               // loop for each color channel (r,g,b)
               for (i = 0; i < numChannels; i++)
               {
                  maxVal = CVMax(maxVal, *curPtr);
                  curPtr++;
               }
            }
            curPtr += lineStep;
         }
         
         return CVRES_SUCCESS;
      }
   //---------------------------------------------------------------------
   protected:
      
      // Basic image information
      int            fWidth;           // Width in pixels.      
      int            fHeight;          // Height in pixels.      
      unsigned char* fData;            // Image data.
                                       // This may be a pointer to the parent
                                       // image's buffer. Remember to use
                                       // offsets. 
      // SubImage/ROI information
      int            fXOffset;         // X Offset within parent image.
                                       // To find offset within fData, 
                                       // call XOffsetAbs().      
      int            fYOffset;         // Y Offset within parent image.
                                       // To find offset within fData,
                                       // call YOffsetAbs().      
      bool           fOwnData;         // Set to true if we own image data.
                                       // (i.e. are we the parent?)      
   
      unsigned long  fRefCount;        // Ref count - inc for each sub image.      
      CVImage*       fParentImage;     // Parent image - so we can decrement
                                       //       ref count on destruction.
};

#endif // _CVImage_H_

