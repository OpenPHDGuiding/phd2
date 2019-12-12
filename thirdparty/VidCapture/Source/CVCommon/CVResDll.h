/*
// CVResDll.h - Result codes for DLL wrapper
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
/// \file CVResDll.h 
/// \brief Result codes for DLL wrapper
///
/// Prefer to include into .cpp files that need these codes, and include
/// CVRes.h into headers to speed compile times and reduce recompilation
/// when changing result codes.
/// \sa CVRes.h
///
/// $RCSfile: CVResDll.h,v $
/// $Date: 2004/02/08 23:47:37 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $
*/

#ifndef _CVResDll_H_
#define _CVResDll_H_

#include "CVRes.h"

enum CVRES_DLL_ENUM
{
   /*------------------------------------------------------------------------
   // DLL status codes
   */

   CVRES_DLL_ATTACHED = CVRES_DLL_STATUS + 1,

   /*------------------------------------------------------------------------
   // Dll errors
   */

   /*! DLL unable to create a video capture object */
   CVRES_DLL_VIDCAP_CREATE_ERROR = CVRES_DLL_ERROR + 1,    
   /*! Invalid CVVIDCAPTURE handle */
   CVRES_DLL_VIDCAP_INVALID_HANDLE,
   /*! Requested device was not found */
   CVRES_DLL_VIDCAP_DEV_NOT_FOUND,
   /*! A passed-in CVIMAGESTRUCT has invalid members */
   CVRES_DLL_VIDCAP_INVALID_IMAGE_STRUCT,
   /*! The requested property is not supported by the camera */
   CVRES_DLL_VIDCAP_UNSUPPORTED_PROPERTY

};

#endif /* _CVResDll_H_ */

