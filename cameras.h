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
# define ALTAIR
# define ASCOM_CAMERA
# define ATIK16
# define ATIK_GEN3
# define INDI_CAMERA
# define INOVA_PLC
# define LE_CAMERA
# define LE_LXUSB_CAMERA
# define LE_PARALLEL_CAMERA
# define LE_SERIAL_CAMERA
# define MEADE_DSI_CAMERA
# define OPENCV_CAMERA
# define ORION_DSCI
# define QGUIDE
# define QHY_CAMERA
# define SAC42
# define SBIG
# define SBIGROTATOR_CAMERA
# define SIMULATOR
# define SSAG
# define SSPIAG
# define STARFISH_CAMERA
# define SXV
# define TOUPTEK_CAMERA
# define WDM_CAMERA
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
# define INDI_CAMERA
# define KWIQGUIDER
# define OPENSSAG
# define QHY_CAMERA
# define SBIG
# define SIMULATOR
# ifdef HAVE_MEADE_DSI_CAMERA
#  define MEADE_DSI_CAMERA
# endif
# ifdef HAVE_SKYRAIDER_CAMERA
#  define SKYRAIDER_CAMERA
# endif
# ifdef HAVE_STARFISH_CAMERA
#  define STARFISH_CAMERA
# endif
# define SXV
# ifdef HAVE_TOUPTEK_CAMERA
#  define TOUPTEK_CAMERA
# endif
# define ZWO_ASI

#elif defined (__linux__)

# define SIMULATOR
# define CAM_QHY5
# ifdef HAVE_QHY_CAMERA
#  define QHY_CAMERA
# endif
# define INDI_CAMERA
# ifdef HAVE_ZWO_CAMERA
#  define ZWO_ASI
# endif
# ifdef HAVE_TOUPTEK_CAMERA
#  define TOUPTEK_CAMERA
# endif
# define SXV
# ifdef HAVE_SBIG_CAMERA
#   define SBIG
# endif
// this should work ... needs testing
//# define OPENSSAG

#endif

// Currently unused
// #define NEB_SBIG   // This is for an on-hold project that would get the guide chip data from an SBIG connected in Neb

#endif /* OPENPHD */

#endif // CAMERAS_INCLUDED
