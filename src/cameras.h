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

# if defined(__WINDOWS__)
// Windows cameras
#  define ALTAIR
#  define ASCOM_CAMERA
#  define ATIK16
#  define ATIK_GEN3
#  define INDI_CAMERA
#  define INOVA_PLC
#  define IOPTRON_CAMERA
#  define LE_CAMERA
#  define LE_LXUSB_CAMERA
#  define LE_PARALLEL_CAMERA
#  define LE_SERIAL_CAMERA
#  define MORAVIAN_CAMERA
#  define OGMA_CAMERA
#  define OPENCV_CAMERA
#  define ORION_DSCI
#  define PLAYERONE_CAMERA
#  define QGUIDE
#  define QHY_CAMERA
#  define SBIG
#  define SBIGROTATOR_CAMERA
#  define SIMULATOR
#  define SSPIAG
#  define SVB_CAMERA
#  define SXV
#  define TOUPTEK_CAMERA
#  define WDM_CAMERA
#  define ZWO_ASI

// # define OS_PL130  // the Opticstar library is not yet included
// # define FIREWIRE_CAMERA // the The Imaging Source library is not yet included

#  ifdef HAVE_WXVIDCAP // These need wxVidCapLib, which needs to be built-up separately.  The LE-webcams could go to WDM
#   define VFW_CAMERA
#  endif

# elif defined(__APPLE__)
// Mac cameras
#  ifdef HAVE_FIREWIRE_CAMERA
#   define FIREWIRE_CAMERA
#  endif
#  define INDI_CAMERA
#  ifdef HAVE_KWIQGUIDER_CAMERA
#   define KWIQGUIDER_CAMERA
#  endif
#  ifdef HAVE_OGMA_CAMERA
#   define OGMA_CAMERA
#  endif
#  ifdef HAVE_OPENSSAG_CAMERA
#   define OPENSSAG_CAMERA
#  endif
#  ifdef HAVE_PLAYERONE_CAMERA
#   define PLAYERONE_CAMERA
#  endif
#  ifdef HAVE_QHY_CAMERA
#   define QHY_CAMERA
#  endif
#  ifdef HAVE_SBIG_CAMERA
#   define SBIG
#  endif
#  define SIMULATOR
#  ifdef HAVE_SXV_CAMERA
#   define SXV
#  endif
#  ifdef HAVE_TOUPTEK_CAMERA
#   define TOUPTEK_CAMERA
#  endif
#  ifdef HAVE_ZWO_CAMERA
#   define ZWO_ASI
#  endif
#  ifdef HAVE_SVB_CAMERA
#   define SVB_CAMERA
#  endif

# elif defined(__linux__) || defined(__FreeBSD__)

#  define SIMULATOR
#  define OPENCV_CAMERA
#  define CAM_QHY5
#  ifdef HAVE_OGMA_CAMERA
#   define OGMA_CAMERA
#  endif
#  ifdef HAVE_PLAYERONE_CAMERA
#   define PLAYERONE_CAMERA
#  endif
#  ifdef HAVE_QHY_CAMERA
#   define QHY_CAMERA
#  endif
#  define INDI_CAMERA
#  ifdef HAVE_ZWO_CAMERA
#   define ZWO_ASI
#  endif
#  ifdef HAVE_TOUPTEK_CAMERA
#   define TOUPTEK_CAMERA
#  endif
#  ifdef HAVE_SXV_CAMERA
#   define SXV
#  endif
#  ifdef HAVE_SBIG_CAMERA
#   define SBIG
#  endif
#  ifdef HAVE_SVB_CAMERA
#   define SVB_CAMERA
#  endif
// this should work ... needs testing
// # define OPENSSAG

# endif

// Currently unused
// #define NEB_SBIG   // This is for an on-hold project that would get the guide chip data from an SBIG connected in Neb

#endif // CAMERAS_INCLUDED
