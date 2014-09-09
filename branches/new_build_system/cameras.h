/*
 *  cameras.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CAMERAS_INCLUDED

/* Current issues:
- Need to fix the LE webcams to either not need wxVidCapLib or need a good way
  to detect or package this
  */

#ifndef OPENPHD
/* Open PHD defines the available drivers in CMakeLists.txt rather than
   statically here
 */

// Defines to define specific camera availability

#if defined (ORION)
# define ORION_DSCI
# define SSAG
# define SSPIAG

#elif defined (__WINDOWS__)  // Windows cameras
# define QGUIDE
# define ORION_DSCI
# define WDM_CAMERA
# define SAC42
# define ATIK16
# define SSAG
# define SSPIAG
# define MEADE_DSI
# define STARFISH
# define SIMULATOR
# define SXV
# define ATIK_GEN3
# define INOVA_PLC
# define ASCOM_LATECAMERA
# define SBIG
# define SBIGROTATOR_CAMERA // must follow SBIG
# define QHY5II
# define QHY5LII
# define OPENCV_CAMERA
# define LE_CAMERA
# define LE_SERIAL_CAMERA
# define LE_PARALLEL_CAMERA
# define LE_LXUSB_CAMERA
# define ZWO_ASI

#ifdef CLOSED_SOURCE
# define OS_PL130  // Opticstar's library is closed
# define FIREWIRE // This uses the The Imaging Source library, which is closed
#endif

#ifdef HAVE_WXVIDCAP   // These need wxVidCapLib, which needs to be built-up separately.  The LE-webcams could go to WDM
# define VFW_CAMERA
#endif

#elif defined (__APPLE__)  // Mac cameras
# define FIREWIRE
# define SBIG
# define MEADE_DSI
# define STARFISH
# define SIMULATOR
# define SXV
# define OPENSSAG
# define KWIQGUIDER

//TODO DISABLING ZWO cameras on OSX: binaries are lacking
//# define ZWO_ASI

#elif defined (__LINUX__)
# define CAM_QHY5
# define INDI_CAMERA
# define ZWO_ASI
# define SIMULATOR
#endif

// Currently unused
// #define NEB_SBIG   // This is for an on-hold project that would get the guide chip data from an SBIG connected in Neb

extern bool DLLExists(const wxString& DLLName);

#endif /* OPENPHD */

#endif // CAMERAS_INCLUDED
