/*
// CVRes.h Result codes definitions for CodeVis.
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
/// \file CVRes.h Result codes definitions for CodeVis.
/// \brief This header defines the basic error codes for most CodeVis functions,
/// provides macros to interpret them, and defines the group values for
/// other CVRes* files for subsystems.
/// 
/// Define CVRES_VIDCAP_OFFSET to offset all of the result codes if you wish
/// to use it alongside a similar error system without collisions.
///
/// $RCSfile: CVRes.h,v $
/// $Date: 2004/02/08 23:47:37 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $
*/
#ifndef _CVRES_H_
#define _CVRES_H_

/*! Result type for most CodeVis functions. */
typedef unsigned long CVRES;

#define CVHIGHBIT 0x80000000

/*
/// CVSUCCESS returns true if a CVRES result successful.
/// This includes status result codes.
*/
#define CVSUCCESS(x) ( ((x) & CVHIGHBIT) == 0 )

/*! CVFAILED returns true if a CVRES result failed. */
#define CVFAILED(x)  ( ((x) & CVHIGHBIT) != 0 )

/*
// Code groups
//
// These define result code ranges for each
// subsystem.  The actual defines for the result
// codes are elsewhere as specified 
//
// We split them out so that we don't have to recompile the entire
// project when error codes change - just the subsystem who's codes
// changed.
//
*/
#ifndef CVRES_VIDCAP_OFFSET 
   /*
   /// Defines an offset that may be used to simplify integration of 
   /// CVRES result codesinto a larger system using similar
   /// codes.  If defined, all codes except for CVRES_SUCCESS are
   /// offset by CVRES_VIDCAP_OFFSET.
   ///
   /// NOTE: DO NOT DEFINE AN OFFSET IF YOU ARE USING THE DLL!
   /// Or at least - not if you want to ever use the standard release
   /// .DLL..... It's compiled with an offset of 0.
   */
   #define CVRES_VIDCAP_OFFSET 0
#endif

/*! Video capture result code group - Defined in CVResVidCap.h. */
#define CVRES_VIDCAP_GROUP 0x1000 + CVRES_VIDCAP_OFFSET  

/*! Imaging result code group - Defined in CVResImage.h. */
#define CVRES_IMAGE_GROUP  0x2000 + CVRES_VIDCAP_OFFSET

/*! File result code group - Defined in CVResFile.h. */
#define CVRES_FILE_GROUP   0x3000 + CVRES_VIDCAP_OFFSET

/*! DLL result code group - Defined in CVResDll.h */
#define CVRES_DLL_GROUP    0x4000 + CVRES_VIDCAP_OFFSET

/*
/// Core CVRES result code and group definitions.
/// Success = 0
/// Status is nonzero with high bit unset
/// Error  is nonzero with high bit set
*/
enum CVRES_CORE_ENUM
{
   
   /*! Operation was successful (0) */
   CVRES_SUCCESS        = 0,  

   /*------------------------------------------------------------------------
   // Status definitions
   */

   /*! Core status code group. */
   CVRES_STATUS         = 1 + CVRES_VIDCAP_OFFSET, 

   /*! Video capture status group. */
   CVRES_VIDCAP_STATUS  = CVRES_VIDCAP_GROUP, 

   /*! Imaging status group. */
   CVRES_IMAGE_STATUS   = CVRES_IMAGE_GROUP,  
   
   /*! File status group. */
   CVRES_FILE_STATUS    = CVRES_FILE_GROUP,   

   /*! DLL status group */
   CVRES_DLL_STATUS     = CVRES_DLL_GROUP,
         
   /*------------------------------------------------------------------------
   // Error definitions
   */

   /*! Generic error - try not to use this one. */
   CVRES_ERROR          =  CVHIGHBIT + CVRES_VIDCAP_OFFSET,  

   /*! Function or feature is not yet implemented. */
   CVRES_NOT_IMPLEMENTED,     

   /*! Out of memory! */
   CVRES_OUT_OF_MEMORY,       

   /*! Out of handles! */
   CVRES_OUT_OF_HANDLES,

   /*! Out of threads! */
   CVRES_OUT_OF_THREADS,

   /*! Invalid parameter passed to function. */
   CVRES_INVALID_PARAMETER,   

   /*! Video capture error group. */
   CVRES_VIDCAP_ERROR   = CVRES_ERROR +          
                          CVRES_VIDCAP_GROUP,

   /*! Imaging error group. */
   CVRES_IMAGE_ERROR    = CVRES_ERROR +
                          CVRES_IMAGE_GROUP,
   
   /*! File error group. */
   CVRES_FILE_ERROR     = CVRES_ERROR +
                          CVRES_FILE_GROUP,

   /*! DLL error  group */
   CVRES_DLL_ERROR      = CVRES_ERROR +
                          CVRES_DLL_GROUP
};

#endif /* _CVRES_H_ */

