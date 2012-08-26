/*
// CVResFile.h - Result codes for file classes.
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
/// \file CVResFile.h 
/// \brief Result codes for file subsystem.
///
/// Prefer to include into .cpp files that need these codes, and include
/// CVRes.h into headers to speed compile times and reduce recompilation
/// when changing result codes.
/// \sa CVRes.h
///
/// $RCSfile: CVResFile.h,v $
/// $Date: 2004/02/08 23:47:37 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $
*/
#ifndef _CVRESFILE_H_
#define _CVRESFILE_H_

#include "CVRes.h"

enum CVRES_FILE_ENUM
{
   /*------------------------------------------------------------------------
   // File class status codes
   */

   /*! File has already been closed. */
   CVRES_FILE_ALREADY_CLOSED = CVRES_FILE_STATUS + 1,

   /*! End of file reached: Non-error status! */
   CVRES_FILE_EOF,                  
   
   /*------------------------------------------------------------------------
   // File class errors
   */

   /*! File does not exist. */
   CVRES_FILE_DOES_NOT_EXIST = CVRES_FILE_ERROR + 1, 
   
   /*! Error creating file. */
   CVRES_FILE_CREATE_ERROR,
   
   /*! Error opening file. */
   CVRES_FILE_OPEN_ERROR,
   /*! Error reading from file. */
   CVRES_FILE_READ_ERROR,
   /*! Error writing to file. */
   CVRES_FILE_WRITE_ERROR,          
   /*! File must be open before operation. */
   CVRES_FILE_MUST_BE_OPEN,         
   /*! File is already open. */
   CVRES_FILE_ALREADY_OPEN,         
   /*! Error seeking in file. */
   CVRES_FILE_SEEK_ERROR,           
   /*! Invalid buffer passed for read/write */
   CVRES_FILE_INVALID_BUFFER        

};

#endif /* _CVRESFILE_H_ */

