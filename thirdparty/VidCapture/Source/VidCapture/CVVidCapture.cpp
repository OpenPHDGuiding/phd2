// CVVidCapture - Video capture interface class
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
/// \file  CVVidCapture.cpp
/// \brief Implements common functions between all video capture devices.
/// 
/// CVVidCapture provides the pure interface for derived video capture
/// classses, and also provides some basic functionality for all video
/// capture classes.
///
/// You *must* derive a class from it, and cannot simply instantiate it.
/// 
/// To use any CVVidCapture object, instantiated the desired derived type.
///
/// Call Init() to to initialize the capture library as needed.
///
/// Successful initialization, you can call EnumDevices() to enumerate
/// the available video capture devices. 
///
/// Once you've decided which capture device to use, call
/// Connect() with the desired device name to connect to it.  Now
/// you can read and modify any of the camera properties and 
/// video modes.
///
/// When you're ready to receive images, either call Grab() for a single-
/// shot grab, or setup a callback and call StartImageCap(). 
///
/// If you are using Grab(), be sure to call CVImage::ReleaseImage() 
/// on the grabbed image when done.
///
/// If you are doing a continuous capture, then the images are automatically
/// released after the callback returns. However, if you want to keep the
/// image around (for example, to place it on a queue for later processing
/// outside of the callback), you may call CVImage->AddRef() on the image
/// and it will not be deleted. Just make sure to call CVImage::ReleaseImage()
/// later when done with that image.
/// 
/// Call Stop() to end a StartImageCap() continuous capture.
///
/// When you're done, call Disconnect(), then Uninit() to clean up.
///
///
/// $RCSfile: CVVidCapture.cpp,v $
/// $Date: 2004/03/01 16:28:15 $
/// $Revision: 1.3 $
/// $Author: mikeellison $

#include <string.h> // strncpy
#include <memory.h> // memset
#include "CVVidCapture.h"
#include "CVUtil.h"

// Version format:                xxMMxxmm
// xx - currently unused / reserved.
// MM - Major version digits (current 00)
// mm - minor version digits (current 20)
// So, the current version is 00.20, or 0.2
const int kVIDCAPTURE_VERSION = 0x00000030;

/// Property name table string length
const int  kCVVidCapture_MaxPropNameLen      = 32;

/// Max length of a format name
const int kCVVidCapture_MaxFormatNameLen     = 32;

const char *kVIDCAPTURE_STRING = 
   "CodeVis VidCapture Version 0.30\n" \
   "Copyright (c) 2003-2004 by Michael Ellison\n" \
   "Documentation and code at http://www.codevis.com\n";

/// Property name table for translating
/// CAMERA_PROPERTY values into user-readable
/// strings.
///
/// \sa GetPropertyName(), CAMERA_PROPERTY
const char kCVVidCapture_Prop_Names
               [CVVidCapture::CAMERAPROP_NUMPROPS]
               [kCVVidCapture_MaxPropNameLen] =
{
   "Brightness",              // CAMERAPROP_BRIGHT
   "Contrast",                // CAMERAPROP_CONTRAST
   "Hue",                     // CAMERAPROP_HUE
   "Saturation",              // CAMERAPROP_SAT
   "Sharpness",               // CAMERAPROP_SHARP
   "Gamma",                   // CAMERAPROP_GAMMA
   "Color Enabled",           // CAMERAPROP_COLOR
   "White Balance",           // CAMERAPROP_WHITEBALANCE
   "Backlight Compensation",  // CAMERAPROP_BACKLIGHT
   "Gain",                    // CAMERAPROP_GAIN   
};
/// Video format name table - should be kept up to date with 
/// VIDCAP_FORMAT enumeration
const char kVIDCAP_FORMAT_NAMES[VIDCAP_NUM_FORMATS][kCVVidCapture_MaxFormatNameLen] =
{
   "Unknown",        // VIDCAP_FORMAT_UNKNOWN
   "YVU9",           // VIDCAP_FORMAT_YVU9
   "Y411",           // VIDCAP_FORMAT_Y411
   "Y41P",           // VIDCAP_FORMAT_Y41P
   "YUY2",           // VIDCAP_FORMAT_YUY2
   "YVYU",           // VIDCAP_FORMAT_YVYU
   "UYVY",           // VIDCAP_FORMAT_UYVY
   "Y211",           // VIDCAP_FORMAT_Y211
   "CLJR",           // VIDCAP_FORMAT_CLJR
   "IF09",           // VIDCAP_FORMAT_IF09
   "CPLA",           // VIDCAP_FORMAT_CPLA
   "MJPG",           // VIDCAP_FORMAT_MJPG
   "TVMJ",           // VIDCAP_FORMAT_TVMJ
   "WAKE",           // VIDCAP_FORMAT_WAKE
   "CFCC",           // VIDCAP_FORMAT_CFCC
   "IJPG",           // VIDCAP_FORMAT_IJPG
   "Plum",           // VIDCAP_FORMAT_Plum
   "RGB1",           // VIDCAP_FORMAT_RGB1
   "RGB4",           // VIDCAP_FORMAT_RGB4
   "RGB8",           // VIDCAP_FORMAT_RGB8
   "RGB565",         // VIDCAP_FORMAT_RGB565
   "RGB555",         // VIDCAP_FORMAT_RGB555
   "RGB24",          // VIDCAP_FORMAT_RGB24
   "RGB32",          // VIDCAP_FORMAT_RGB32
   "ARGB32",         // VIDCAP_FORMAT_ARGB32
   "Overlay",        // VIDCAP_FORMAT_Overlay
   "QTMovie",        // VIDCAP_FORMAT_QTMovie
   "QTRpza",         // VIDCAP_FORMAT_QTRpza
   "QTSmc",          // VIDCAP_FORMAT_QTSmc
   "QTRle",          // VIDCAP_FORMAT_QTRle
   "QTJpeg",         // VIDCAP_FORMAT_QTJpeg
   "dvsd",           // VIDCAP_FORMAT_dvsd
   "dvhd",           // VIDCAP_FORMAT_dvhd
   "dvsl",           // VIDCAP_FORMAT_dvsl
   "MPEG1Packet",    // VIDCAP_FORMAT_MPEG1Packet
   "MPEG1Payload",   // VIDCAP_FORMAT_MPEG1Payload
   "VPVideo",        // VIDCAP_FORMAT_VPVideo
   "MPEG1 Video",    // VIDCAP_FORMAT_MPEG1Video

   "I420",            // VIDCAP_FORMAT_I420
   "IYUV",            // VIDCAP_FORMAT_IYUV

   "Y444",           // VIDCAP_FORMAT_Y444
   "Y800",           // VIDCAP_FORMAT_Y800
   "Y422",           // VIDCAP_FORMAT_Y422
};

//---------------------------------------------------------------------------
CVVidCapture::CVVidCapture()
{
   fInitialized = false;
   fConnected   = false;
   fStarted     = false;
   fDeviceName  = 0;
   fModeList    = 0; 

   fNumDevices  = 0;   
   fDeviceList  = 0;
   memset(&fCurMode,0,sizeof(fCurMode));
   
   fLastState   = VIDCAP_UNCONNECTED;
}

//---------------------------------------------------------------------------
CVVidCapture::~CVVidCapture()
{
   // Sanity check - caller should stop, disconnect, and 
   // uninitialize in that order prior to deleting
   // the object.

   if (fStarted)
   {
      CVTrace("Capture device deleted while running!");
      Stop();
   }

   if (fConnected)
   {
      CVTrace("Capture device deleted while connected!");
      Disconnect();
   }

   if (fInitialized)
   {
      CVTrace("Capture device deleted while initialized!");
      Uninit();
   }  

   ClearDeviceList();
   
   fModeList = 0; 
}

//---------------------------------------------------------------------------
// ClearDeviceList
//    Clears the list of available devices, if any.
//---------------------------------------------------------------------------
void CVVidCapture::ClearDeviceList()
{
   if (fDeviceList != 0)
   {
      VIDCAP_DEVICE* curDevice  = fDeviceList;
      VIDCAP_DEVICE* prevDevice = 0;
   
      while (curDevice != 0) 
      {
         prevDevice = curDevice;  
         curDevice = curDevice->NextDevice;
         delete [] prevDevice->DeviceString;
         delete prevDevice;     
      }  
      fDeviceList = 0;
   }
   fNumDevices = 0;
}

//---------------------------------------------------------------------------
// GetNumDevices() returns the number of devices available.
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetNumDevices(int& numDevices)
{
   if (fInitialized != true)
   {
      return CVRES_VIDCAP_NOT_INITIALIZED;
   }
   numDevices = fNumDevices;
   
   return CVRES_SUCCESS;   
}

//---------------------------------------------------------------------------
// GetDeviceInfo()
//    index is the index into the list (max from GetNumDevices)
//    fills devInfo if index is valid.
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetDeviceInfo(  int                 index,
                                    VIDCAP_DEVICE&        devInfo)
{
   if (fInitialized != true)
   {
      return CVRES_VIDCAP_NOT_INITIALIZED;
   }
   if (fDeviceList == 0)
   {
      return CVRES_VIDCAP_NO_DEVICES;
   }
      
   if ((index < 0) || (index >= this->fNumDevices))
   {
      return CVRES_VIDCAP_INVALID_DEVICE_INDEX;
   }

   
   int devNum = 0;
   VIDCAP_DEVICE* curDevice = fDeviceList;
   while ((curDevice != 0) && (devNum != index))
   {
      devNum++;
      curDevice = curDevice->NextDevice;
   }

   if (curDevice != 0)
   {
      devInfo = *curDevice;
      return CVRES_SUCCESS;
   }

   return CVRES_VIDCAP_INVALID_DEVICE_INDEX;
}

//---------------------------------------------------------------------------
// Stop
//    Child classes should override. This should stop what 
//    StartImageCap() and any other continuous grabs start.
//---------------------------------------------------------------------------
CVRES CVVidCapture::Stop()
{
   if (fStarted != true)
   {
      return CVRES_VIDCAP_ALREADY_STOPPED;
   }
   fStarted = false;
   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// Disconnect
//    Child classes should override. 
//    This should release anything allocated by Connect().
//---------------------------------------------------------------------------
CVRES CVVidCapture::Disconnect()
{
   if (fConnected != true)
   {
      return CVRES_VIDCAP_NOT_CONNECTED;
   }

   fLastState = VIDCAP_UNCONNECTED;
   fConnected = false;

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// Uninit
//    Child classes should override.
//    This should release anything allocated by Init()
//---------------------------------------------------------------------------
CVRES CVVidCapture::Uninit()
{
   if (fInitialized != true)
   {
      return CVRES_VIDCAP_NOT_INITIALIZED;
   }
   fInitialized = false;
   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// GetPropertyInfo()
//    Retrieves the information for the specified camera property.
// If you receive CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED, then either your camera
// or the derived video capture class does not support the property.
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetPropertyInfo (  CAMERA_PROPERTY         property,
                                       long*                   curVal,
                                       long*                   defVal,
                                       long*                   minVal,
                                       long*                   maxVal,
                                       long*                   step)
{
   return CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED;
}

//---------------------------------------------------------------------------
// SetProperty
//    Sets a property if it is available and the specified value is within
//    range.  Use GetPropertyInfo to get the property min/max and step.
//---------------------------------------------------------------------------
CVRES CVVidCapture::SetProperty  (  CAMERA_PROPERTY            property,
                                    long                       value)
{
   return CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED;
}

//---------------------------------------------------------------------------
// GetPropertyName()
//    Retrieve the property name for a specified property
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetPropertyName (  CAMERA_PROPERTY   property,
                                       char*             nameBuffer,
                                       int               maxLength)
{
   if ((property < 0) || (property >= CAMERAPROP_NUMPROPS))
   {
      return CVRES_INVALID_PARAMETER;
   }
   if ((nameBuffer == 0) || (maxLength < 2))
   {
      return CVRES_INVALID_PARAMETER;
   }  

   memset(nameBuffer,0,maxLength);
   strncpy(nameBuffer,kCVVidCapture_Prop_Names[property],maxLength-1);

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// GetNumSupportedModes
//    Retrieves the number of supported modes by
//    walking the mode list.  Mode list should be
//    created inside Connect().
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetNumSupportedModes( int&  numModes )
{
   if (fConnected != true)
   {
      return CVRES_VIDCAP_NOT_CONNECTED;
   }
   numModes = 0;

   VIDCAP_MODE* curMode = fModeList;

   // Count modes
   while (curMode != 0)
   {
      numModes++;
      curMode = curMode->NextMode;
   }

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// GetModeInfo()
//    index is the index into the list (max from GetNumSupportedModes)
//    fills modeInfo if index is valid.
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetModeInfo(  int                 index,
                                  VIDCAP_MODE&        modeInfo)
{
   if (fConnected != true)
   {
      return CVRES_VIDCAP_NOT_CONNECTED;
   }
   
   int modeNum = 0;
   VIDCAP_MODE* curMode = fModeList;
   while ((curMode != 0) && (modeNum != index))
   {
      modeNum++;
      curMode = curMode->NextMode;
   }

   if (curMode != 0)
   {
      modeInfo = *curMode;
      return CVRES_SUCCESS;
   }

   return CVRES_VIDCAP_MODE_NOT_SUPPORTED;
}

//---------------------------------------------------------------------------
// SetMode()
//    index is the index into the list (max from GetNumSupportedModes)
//    sets the capture mode.
//---------------------------------------------------------------------------
CVRES CVVidCapture::SetMode(  int   index,
                              bool  rawYUY2 )
{
   if (fConnected != true)
   {
      return CVRES_VIDCAP_NOT_CONNECTED;
   }

   VIDCAP_MODE* curMode = fModeList;
   int modeNum = 0;
   while ((curMode != 0) && (modeNum != index))
   {
      modeNum++;
      curMode = curMode->NextMode;
   }

   if (curMode != 0)
   {
      return SetMode(*curMode, rawYUY2);
   }

   return CVRES_VIDCAP_MODE_NOT_SUPPORTED;
}


//---------------------------------------------------------------------------
// GetCurrentMode()
//    Retrieve the current mode
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetCurrentMode( VIDCAP_MODE& curMode )
{
   if (fConnected != true)
   {
      return CVRES_VIDCAP_NOT_CONNECTED;
   }
   
   curMode = fCurMode;  
   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// SetMode()
//    Set the mode to a specific mode.
//    Override in child classes!
//---------------------------------------------------------------------------
CVRES CVVidCapture::SetMode( VIDCAP_MODE& curMode,
                             bool rawYUY2 )
{
   return CVRES_VIDCAP_MODE_NOT_SUPPORTED;
}


//---------------------------------------------------------------------------
// ClearModes()
//    Clear out the mode list
//    Probably need to override this in child classes if internal
//    void* is used.
//---------------------------------------------------------------------------
void CVVidCapture::ClearModes()
{
   VIDCAP_MODE* curMode  = fModeList;
   VIDCAP_MODE* prevMode = 0;
   
   while (curMode != 0) 
   {
      prevMode = curMode;  
      curMode = curMode->NextMode;
      delete prevMode;     
   }  
   
   fModeList = 0; 
}

//---------------------------------------------------------------------------
// AddMode()
//    Adds a mode to the mode list.
//---------------------------------------------------------------------------
CVRES CVVidCapture::AddMode(  VIDCAP_MODE& addMode )
{
   VIDCAP_MODE* newMode = new VIDCAP_MODE;
   if (newMode == 0)
   {
      return CVRES_OUT_OF_MEMORY;
   }

   // Copy mode information
   *newMode = addMode;

   // Prepend to list
   newMode->NextMode = fModeList;
   fModeList = newMode;
   
   return CVRES_SUCCESS;
}


//---------------------------------------------------------------------------
// IsInitialized()
//     Returns true if the capture object is initialized
//---------------------------------------------------------------------------
bool CVVidCapture::IsInitialized()
{
   return this->fInitialized;
}

//---------------------------------------------------------------------------
// IsStarted()
//     Returns true if a capture has been started
//---------------------------------------------------------------------------
bool CVVidCapture::IsStarted()
{
   return this->fStarted;
}

//---------------------------------------------------------------------------
// IsConnected()
//     Returns true if we're connected to a camera
//---------------------------------------------------------------------------
bool CVVidCapture::IsConnected()
{
   return this->fConnected;
}

//---------------------------------------------------------------------------
// GetFormatModeName() retrieves the video format mode name
// from the enumeration value.
//
//    vidcapFormat - video format of camera
//    const char* - ptr to const string describing video format
//---------------------------------------------------------------------------
const char* CVVidCapture::GetFormatModeName( VIDCAP_FORMAT format)
{
   if ((format >= VIDCAP_NUM_FORMATS) || (format < 0))
   {
      return kVIDCAP_FORMAT_NAMES[0];
   }

   return kVIDCAP_FORMAT_NAMES[format];
}

//---------------------------------------------------------------------------
// GetDeviceName() retrieve the device name into a buffer/
// The buffer must already be created.
// You may call it with a null buffer to receive the length of the string
// in maxLength.
//---------------------------------------------------------------------------
CVRES CVVidCapture::GetDeviceName  (  char*                   nameBuffer, 
                                      int&                     maxLength )
{
   int copyLen = maxLength;
   maxLength = strlen(this->fDeviceName);
   
   if ((nameBuffer == 0) || (copyLen == 0))
   {
      return CVRES_VIDCAP_NAME_BUFFER_TOO_SMALL;
   }
   
   strncpy(nameBuffer,fDeviceName,copyLen);
   if (copyLen < maxLength)
   {
      // ensure null termination
      nameBuffer[copyLen-1] = 0;
      return CVRES_VIDCAP_NAME_BUFFER_TOO_SMALL;
   }
   return CVRES_SUCCESS;
}
