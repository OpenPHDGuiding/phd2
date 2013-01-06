/*
 * CVImageStructs.h
 * Common image structure definitions used by alternative (non-C++) 
 * interfaces.
 *
 * Written by Michael Ellison
 *-------------------------------------------------------------------------
 *                      CodeVis's Free License
 *                         www.codevis.com
 *
 * Copyright (c) 2004 by Michael Ellison (mike@codevis.com)
 * All rights reserved.
 *
 * You may use this software in source and/or binary form, with or without
 * modification, for commercial or non-commercial purposes, provided that 
 * you comply with the following conditions:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer. 
 *
 * * Redistributions of modified source must be clearly marked as modified,
 *   and due notice must be placed in the modified source indicating the
 *   type of modification(s) and the name(s) of the person(s) performing
 *   said modification(s).
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 * Modifications:
 *
 *---------------------------------------------------------------------------
 *! \file CVImageStructs.h
 *! \brief Common image structure definitions used by CodeVis 'C' interfaces.
 *!
 *! $RCSfile: CVImageStructs.h,v $
 *! $Date: 2004/03/01 18:30:31 $
 *! $Revision: 1.3 $
 *! $Author: mikeellison $
*/
#ifndef _CVImageStructs_H_
#define _CVImageStructs_H_

#include "CVRes.h"

#ifndef __cplusplus
   typedef int CVIMAGETYPE;
   enum
#else
   enum CVIMAGETYPE
#endif
{
   /*! Default type (sometimes used for auto-detect) */
   CVIMAGETYPE_DEFAULT   = 0,
   /*! 8-bit red, green, blue triplets */
   CVIMAGETYPE_RGB24     = 1,
   /*! 32-bit float red, green, blue triplets */
   CVIMAGETYPE_RGBFLOAT  = 2,    
   /*! 8-bit intensity values */
   CVIMAGETYPE_GREY      = 3,
   
   
   /* --- currently unimplemented below this line */
   CVIMAGETYPE_RGBINT    = 4,    /*! 32-bit integer red, green, blue triplets */
   CVIMAGETYPE_GREYINT   = 5,    /*! 32-bit integer intensity values */
   CVIMAGETYPE_GREYFLOAT = 6,    /*! 32-bit floating point intensity values */
};


const int kCVIMAGESTRUCTVER = 1;

/*
 *! CVIMAGESTRUCT holds an image - either RGB24 or 8-bit Greyscale.
 *! The floating-point RGB images are not currently supported.
*/
struct CVIMAGESTRUCT
{
   int            Version;       /*! Structure Version (1)       */
   CVIMAGETYPE    ImageType;     /*! Type of image */
   int            BytesPerPixel; /*! # of bytes per pixel (3) */
   int            NumChannels;   /*! Number of channels (e.g. 3 for RGB, 1 for greyscale) */
   int            ImageWidth;    /*! Width of image in pixels */
   int            ImageHeight;   /*! Height of image in pixels */
   int            ImageDataSize; /*! Size of image in bytes. Redundant, but quick for checks */
   unsigned char* PixelDataPtr;  /*! Pointer to raw pixel data (typically r,g,b format) */
};

/*!
 *! These are all the formats in the DirectX 8.1 documentation,
 *! plus a few I've encountered that weren't in the docs or .h's.
 *!
 *! Honestly, we don't particularly care what the format is 
 *! for the DirectX code, but we may need to know on other 
 *! platforms that don't automagically perform conversions.
 *! 
 *! The library itself currently only returns RGB24, RGBFloat, 
 *! and Grey scale images regardless of the format of the input 
 *! video - totally independant of what format the input video
 *! is in.
 *! 
 *! We also need to tell the user what format the camera is 
 *! in to differentiate between the various modes of a camera.
 *! 
 *! While the type names are ripped from DirectX - these values are not
 *! equivalent to the DirectX codes. There is a conversion table
 *! between the two in the DirectX-specific class (CVVidCaptureDSWin32).
 *! Other platforms will need to convert their own values to these
 *! as well...
 */
#ifndef __cplusplus
   typedef int VIDCAP_FORMAT;
   enum
#else
   enum VIDCAP_FORMAT
#endif
{
   VIDCAP_FORMAT_UNKNOWN = 0,
   VIDCAP_FORMAT_YVU9,        
   VIDCAP_FORMAT_Y411,        
   VIDCAP_FORMAT_Y41P,        
   VIDCAP_FORMAT_YUY2,        
   VIDCAP_FORMAT_YVYU,        
   VIDCAP_FORMAT_UYVY,        
   VIDCAP_FORMAT_Y211,        
   VIDCAP_FORMAT_CLJR,        
   VIDCAP_FORMAT_IF09,        
   VIDCAP_FORMAT_CPLA,        
   VIDCAP_FORMAT_MJPG,        
   VIDCAP_FORMAT_TVMJ,        
   VIDCAP_FORMAT_WAKE,        
   VIDCAP_FORMAT_CFCC,        
   VIDCAP_FORMAT_IJPG,        
   VIDCAP_FORMAT_Plum,        
   VIDCAP_FORMAT_RGB1,        
   VIDCAP_FORMAT_RGB4,        
   VIDCAP_FORMAT_RGB8,        
   VIDCAP_FORMAT_RGB565,      
   VIDCAP_FORMAT_RGB555,      
   VIDCAP_FORMAT_RGB24,       
   VIDCAP_FORMAT_RGB32,       
   VIDCAP_FORMAT_ARGB32,      
   VIDCAP_FORMAT_Overlay,     
   VIDCAP_FORMAT_QTMovie,     
   VIDCAP_FORMAT_QTRpza,      
   VIDCAP_FORMAT_QTSmc,       
   VIDCAP_FORMAT_QTRle,       
   VIDCAP_FORMAT_QTJpeg,      
   VIDCAP_FORMAT_dvsd,        
   VIDCAP_FORMAT_dvhd,        
   VIDCAP_FORMAT_dvsl,        
   VIDCAP_FORMAT_MPEG1Packet, 
   VIDCAP_FORMAT_MPEG1Payload,
   VIDCAP_FORMAT_VPVideo,     
   VIDCAP_FORMAT_MPEG1Video,  

   /* These weren't defined by DirectX */
   VIDCAP_FORMAT_I420,        
   VIDCAP_FORMAT_IYUV,
   VIDCAP_FORMAT_Y444,
   VIDCAP_FORMAT_Y800,
   VIDCAP_FORMAT_Y422,

   /* Number of video capture formats... */
   VIDCAP_NUM_FORMATS
};


/* _CVImageStructs_H_ */
#endif 
