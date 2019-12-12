#ifndef WIN32
   #pragma message ("WIN32 not defined. Not compiling CVPlatformWin32")
#else

/// \file CVPlatformWin32.cpp
/// \brief Win32 Platform-specific object factory
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
// CVPlatformWin32
// Win32 Platform-specific object factory
//
// CVPlatform is a singleton class factory for platform-specific classes.
// This allows us to instantiate totally different objects on different
// platforms without making the calling code platform-specific.
//
// When setting up a project for a specific platform, make sure to add in
// the appropriate CVPlatform*.cpp for that platform. It implements
// the CVPlatform class for us.
//
// $RCSfile: CVPlatformWin32.cpp,v $
// $Date: 2004/02/08 23:47:39 $
// $Revision: 1.1.1.1 $
// $Author: mikeellison $

#include "CVPlatform.h"
#include "CVVidCaptureDSWin32.h"

/// Our static platform pointer
CVPlatform* CVPlatform::sPlatform = 0;

//--------------------------------------------------------------------------
// GetPlatform() retrieves a pointer to the singleton platform object.
//
// Only one instance will be created on the first call. 
// It is automatically freed on exit from the process. 
//
// \return CVPlatform* - pointer to CVPlatform object
//--------------------------------------------------------------------------
CVPlatform* CVPlatform::GetPlatform()
{
   // If we've already created the platform object, just return it.
   if (sPlatform != 0)
   {
      return sPlatform;
   }

   sPlatform = new CVPlatform();

   // Register for deletion at process exit.
   // This isn't strictly necessary, but it makes BoundsChecker and similar
   // tools whine a lot less (otherwise we leak a few bytes on exit).   
   atexit(FreePlatform);
   

   return sPlatform;
}

//--------------------------------------------------------------------------
// AcquireVideoCapture() acquires a CVVidCapture object appropriate
// for the current system.
//--------------------------------------------------------------------------
CVVidCapture* CVPlatform::AcquireVideoCapture()
{
   return new CVVidCaptureDSWin32();
}

//--------------------------------------------------------------------------
// Release() releases an object and sets the pointer to NULL.
//
// This version is for video capture objects (CVVidCapture and children).
//
// \param vidCap - a CVVidCapture* previously allocated by 
//                 AcquireVideoCapture().
// \sa CVVidCapture, CVVidCaptureDSWin32, AcquireVideoCapture()
//--------------------------------------------------------------------------
void CVPlatform::Release(CVVidCapture*& vidCap)
{
   if (vidCap != 0)
   {
      delete vidCap;
      vidCap = 0;
   }
}

//--------------------------------------------------------------------------
// CVPlatform constructor - use GetPlatform() instead.
//
// GetPlatform() creates the platform object if needed.
//--------------------------------------------------------------------------
CVPlatform::CVPlatform()
{
}

//--------------------------------------------------------------------------
// CVPlatform destructor - don't delete CVPlatform directly.
// 
// GetPlatform() registers with the runtime library for 
// deletion when the process exits.
//--------------------------------------------------------------------------
CVPlatform::~CVPlatform()
{
}

//--------------------------------------------------------------------------
// FreePlatform() frees the static platform object as the
// process exits.
//
// Do not call it directly. It is made to be called
// from the atexit() handler.
//
//--------------------------------------------------------------------------
void CVPlatform::FreePlatform()
{
   if (sPlatform != 0)
   {
      delete sPlatform;
      sPlatform = 0;
   }
}

#endif // WIN32
