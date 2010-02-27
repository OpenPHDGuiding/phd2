/*
 *  camera.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Defines to define specific camera availability

#if defined (ORION)
 #define ORION_DSCI
 #define SSAG
 #define SSPIAG
 //#define WDM_CAMERA
#elif defined (__WINDOWS__)  // Windows cameras
 #define QGUIDE
 #define VFW_CAMERA
 #define LE_PARALLEL_CAMERA
 #define LE_LXUSB_CAMERA
 #define ORION_DSCI
 #define WDM_CAMERA
 #define SAC42
 #define ATIK16
 #define SSAG
 #define SSPIAG
 #define FIREWIRE
 #define SBIG
 #define MEADE_DSI
 #define STARFISH
 #define OS_PL130
 #define SIMULATOR
 #define SXV
 #define ASCOM_CAMERA
 #define ATIK_GEN3
// #define NEB_SBIG
#elif defined (__APPLE__)  // Mac cameras
 #define FIREWIRE
 #define SBIG
 #define MEADE_DSI
 #define STARFISH
 #define SIMULATOR
 #define SXV
// #define NEB_SBIG
#else // Linux
 #define SIMULATOR
 #define INDI 
#endif

extern void InitCameraParams();
extern bool DLLExists (wxString DLLName);
