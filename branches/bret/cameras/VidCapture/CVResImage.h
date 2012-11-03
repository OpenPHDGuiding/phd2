/*
// CVResImage.h Result codes for image classes.
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
/// \file CVResImage.h 
/// \brief Result codes for imaging subsystem.
///
/// Prefer to include into .cpp files that need these codes, and include
/// CVRes.h into headers to speed compile times and reduce recompilation
/// when changing result codes.
/// \sa CVRes.h
///
/// $RCSfile: CVResImage.h,v $
/// $Date: 2004/02/08 23:47:38 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $
*/

#ifndef _CVRESIMAGE_H_
#define _CVRESIMAGE_H_

#include "CVRes.h"

enum CVRES_IMAGE_ENUM
{
   /*------------------------------------------------------------------------
   // Image class status codes
   */

   /*------------------------------------------------------------------------
   // Image class errors   
   */

   /*! Invalid size requested. */
   CVRES_IMAGE_INVALID_SIZE = CVRES_IMAGE_ERROR + 1,

   /*! Image must be initialized first. */
   CVRES_IMAGE_MUST_INITIALIZE_ERR,       
   /*! Image is currently empty. */
   CVRES_IMAGE_EMPTY_ERR,                 
   /*! Image file already exists. */
   CVRES_IMAGE_ALREADY_EXISTS,            
   /*! Image type is unknown. */
   CVRES_IMAGE_UNKNOWN_TYPE,
   /*! Image type is unsupported. */
   CVRES_IMAGE_UNSUPPORTED_TYPE,          
   /*! Image format is unsupported. */
   CVRES_IMAGE_UNSUPPORTED_FORMAT,
   /*! Invalid data passed to image. */
   CVRES_IMAGE_INVALID_DATA,              
   /*! Conversion between specified image types is unsupported. */
   CVRES_IMAGE_CONVERSION_NOT_SUPPORTED,  
                                          
   /*! Sub image operations are invalid on the root image. */
   CVRES_IMAGE_OPERATION_INVALID_ON_ROOT, 
                                         
   /*! Invalid sub image position - position must be within parent.	 */
   CVRES_IMAGE_INVALID_SUB_POSITION,

   /*! Image file is corrupt */
   CVRES_IMAGE_FILE_CORRUPTED,
   /*! Coordinates are out of range in image (e.g. x > width) */
   CVRES_IMAGE_OUT_OF_RANGE,
   /*! Null callback passed when callback needed. */
   CVRES_IMAGE_INVALID_CALLBACK
                                          
};

#endif /* _CVRESIMAGE_H_ */

