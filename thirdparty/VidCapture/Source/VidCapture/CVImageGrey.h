/// \file CVImageGrey.h
/// \brief CVImageGrey - 8-bit grey intensity image class
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
/// \class CVImageGrey CVImageGrey.h 
/// CVImageGrey is an 8-bit greyscale image class derived from CVImage
/// 
/// See CVImage.h for general documentation.
///
/// \sa CVImage, CVImageRGB24, CVImageRGBFloat
///
/// $RCSfile: CVImageGrey.h,v $
/// $Date: 2004/02/08 23:47:39 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $

#ifndef _CVImageGrey_H_
#define _CVImageGrey_H_

#include "CVResImage.h"
#include "CVImage.h"

#ifdef WIN32
   #include <windows.h> // For SetFromWin32Bmp
#endif

class CVImageRGB24;

class CVImageGrey : public CVImage
{
   // Allow CVImage to use our constructor
   friend CVImage;

   public:                             
      //---------------------------------------------------------------------
      // Overridden functions
      //---------------------------------------------------------------------
      /// GetNumChannels retrieves the number of channels per pixel.
      /// This is one in greyscale, 3 in RGB, and 4 in RGBA
      ///
      /// \return int - number of channels per pixel.
      /// \sa GetBytesPerPixel()
      virtual int GetNumChannels() const;

      /// GetBytesPerPixel retrieves the number of bytes per pixel.  
      /// Note that pixel can be in floating point or integer format, depending
      /// on the image type.      
      ///
      /// \return int - bytes per pixel
      virtual int GetBytesPerPixel() const;

      /// GetImageType() retrieves the image type. 
      /// See CVIMAGE_TYPE enum.
      ///
      /// \return CVIMAGE_TYPE specifying the image's type.
      virtual CVIMAGE_TYPE   GetImageType   () const;
      
      /// GetPNMExtension() retrieves the default file extension for PNM
      /// file saving. (e.g. ".pgm" for greyscale)
      ///
      /// \return const char* - ASCIIZ default file extension, 
      ///                       including preceeding '.'
      /// \sa Load(), Save()
      virtual const char *GetPNMExtension() const;

      /// GetPNMMagicVal() retrieves the magic value for a pnm file
      /// matching the current image format.
      ///
      /// \return char - Magic value for PNM files (e.g. '5' for 'P5' .pgm's)
      /// \sa Load(), Save()
      virtual char GetPNMMagicVal() const;
      
      /// GetMaxPixelValue() retrieves the maximum value of any pixel in
      /// the image.  
      ///
      /// In multichannel images (e.g. RGB triplets), it will
      /// return the maximum value on any of the channels.
      ///
      /// All child classes should implement this.
      /// \param maxValue - reference to max pixel value, set on return.
      /// \return CVRES result code
      /// \sa GetPixel(), SetPixel(), CVImage::GetMaxPixel()
      virtual CVRES GetMaxPixelValue(float& maxValue) const;

      /// GetPixel() retrieves the red, green, and blue values for a specified
      /// pixel as floating points.
      ///
      /// This is for convenience and prototyping - for high-speed image
      /// processing you'll need to work more directly with the image
      /// buffer.
      ///
      /// Within CVImageGrey, this returns the intensity value on all
      /// three channels (red, green, and blue).
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
                              float&   b) const;

      /// SetPixel() sets the red, green, and blue pixel values
      /// for a pixel
      ///
      /// This is for convenience and prototyping - for high-speed image
      /// processing you'll need to work more directly with the image
      /// buffer.
      ///
      /// Within CVImageGrey(), this sets the pixel to:
      ///    value = 0.299r + 0.587g + 0.114b
      ///
      /// Values are from the Y (Luminance) in YIQ conversion,
      ///   Computer Graphics, Principles and Practice 2nd Ed.
      ///   by Foley, van Dam, Feiner, Hughes.
      ///
      /// Intensity values above 255 will be truncated to 255. Values
      /// below 0 will be set to 0.
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
                                       float    b);


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
      /// \return CVRES result code.      
      /// \sa ::CVRES_CORE_ENUM, ::CVRES_IMAGE_ENUM, CVImage::ReleaseImage()
      virtual CVRES SetFromWin32Bmp(  const BITMAPINFOHEADER* bmih, 
                                      const unsigned char*    data);
#endif // WIN32

      //---------------------------------------------------------------------
   protected:
      /// Protected constructor - use CVImage::CreateImage() to construct
      CVImageGrey();
      /// Protected destructor - use CVImage::ReleaseImage() to destroy
      virtual ~CVImageGrey();
};

#endif // _CVImageGrey_H_

