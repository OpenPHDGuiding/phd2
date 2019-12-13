/// \file CVVidCapture.h
/// \brief Video capture interface class
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
//   type of modification(s) and the name(s) of the person(s) f
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
/// \class CVVidCapture CVVidCapture.h
/// CVVidCapture provides the pure interface for derived video capture
/// classses, and also provides some basic functionality for all video
/// capture classes.
///
/// You *must* derive a class from it, and cannot simply instantiate it.
/// 
/// To use any CVVidCapture object, instantiated the desired derived type.
/// The ideal way to do this is to go through CVPlatform, like this:<BR>
/// <CODE>
///   CVVidCapture* vidCap = CVPlatform::GetPlatform()->AcquireVideoCapture();
/// </CODE>
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
/// outside of the callback), you may call CVImage::AddRef() on the image
/// and it will not be deleted. Just make sure to call CVImage::ReleaseImage()
/// later when done with that image. 
///
/// <i>Always check the incoming status code in the callbacks prior to 
///    attempting to access the data.  If the status code is a failed result,
///    the data is not present!</i>
///
/// Call Stop() to end a continuous capture. Do NOT call 
/// Stop() from within a callback or it will deadlock.  You still need to
/// call Stop() when you're done to clean up even if you've aborted by 
/// returning false from the callback.
///
/// To clean up, call Disconnect() to disconnect from the device,
/// then Uninit().
///
/// Make sure to delete the CVVidCapture object when done.  If you
/// used CVPlatform to create it, just call CVPlatform::Release() on the
/// CVVidCapture*, like this:
///
/// <CODE>
///    CVPlatform::GetPlatform()->Release(vidCap);
/// </CODE>
///
/// $RCSfile: CVVidCapture.h,v $
/// $Date: 2004/03/01 18:31:06 $
/// $Revision: 1.7 $
/// $Author: mikeellison $

#ifndef _CVVIDCAPTURE_H_
#define _CVVIDCAPTURE_H_

#include "CVResVidCap.h"      // Result codes
#include "CVImage.h"          // Image base class                             

/// Capture timeout for grabs in miliseconds (10000 = 10 sec)
const int kCVVidCapture_Timeout           = 10000;
const int kVIDCAP_MAX_DEV_NAME_LEN = 128;

class CVVidCapture
{
   //-------------------------------------------------------------------------
   // Public definitions and callbacks
   // 
   public:
      /// CVVIDCAP_CALLBACK is the callback definition for continuous captures
      /// using the image class.
      ///
      /// First, check the status code - if it's a successful status code,
      /// (e.g. if CVSUCCESS(status) returns true), then the imagePtr is 
      /// valid. Otherwise, some sort of error has occurred -
      /// most likely, the camera has been disconnected.      
      ///
      /// imagePtr will be the captured image in the format type requested
      /// when the capture was started.  It will be released by CVVidCapture
      /// when the callback returns - however, you may call AddRef() to 
      /// add a reference to the image and keep it around outside of the
      /// callback if you want - just make sure to free it when done and
      /// be aware of memory limitations vs. number of frames captured.
      ///
      /// Returning true continues the capture, returning false aborts it.
      /// You'll still have to call CVVidCapture::Stop() from another
      /// thread (e.g. outside the callback), but no more callbacks will 
      /// be received after an abort and the processing will halt.
      ///
      /// Do NOT call Stop() from within a callback or it will cause
      /// a deadlock. Instead, return false from the callback to
      /// cause an abort, then call Stop() from your main thread.
      ///
      /// \param status - Status of the capture 
      ///                (CVRES_VIDCAP_OK, CVRES_VIDCAP_CAPTURE_ERROR)
      /// \param imagePtr - ptr to image containing current frame of status
      ///                   is CVRES_VIDCAP_OK
      /// \param userParam - user defined value (suggested: this*)
      ///      
      /// \return bool - true continues capture, false halts it.
      /// \sa StartImageCap()
      typedef bool (*CVVIDCAP_CALLBACK)(  CVRES          status,
                                          CVImage*       imagePtr,
                                          void*          userParam   );

      /// CAMERA_PROPERTY contains the identifiers for camera settings 
      /// that we can control.
      /// These match up with DirectShow's VideoProcAmpProperty enum. 
      ///
      /// Used with CVVidCapture::GetPropertyInfo(), 
      /// CVVidCapture::SetProperty(), and CVVidCapture::GetPropertyName().
      ///
      /// Later we may want to use these on other platforms, 
      /// so duplicating it here for now. If you add or remove properties, 
      /// look for usage of CAMERAPROP_NUMPROPS in child classes and modify 
      /// as needed.
      ///
      /// Also make sure to update kCVVidCapture_Prop_Names at the end
      /// of the file when changing this.
      /// \sa GetPropertyInfo(), SetProperty(), GetPropertyName()
      ///
      enum CAMERA_PROPERTY
      {
         CAMERAPROP_BRIGHT       = 0,
         CAMERAPROP_CONTRAST,
         CAMERAPROP_HUE,
         CAMERAPROP_SAT,
         CAMERAPROP_SHARP,
         CAMERAPROP_GAMMA,
         CAMERAPROP_COLOR,
         CAMERAPROP_WHITEBALANCE,
         CAMERAPROP_BACKLIGHT,
         CAMERAPROP_GAIN,
         CAMERAPROP_NUMPROPS
      };

      /// VIDCAP_MODE contains the video mode information for selecting 
      /// with CVVidCapture::SetMode(), CVVidCapture::GetModeInfo(), etc.
      ///
      /// Available video modes are stored as a linked list.  The InternalRef
      /// void* is implementation specific - under DirectShow it's used to 
      /// store an AM_MEDIA_TYPE*.
      ///
      /// \sa GetNumSupportedModes(), GetModeInfo(), GetCurrentMode(), SetMode() 
      ///
      struct VIDCAP_MODE
      {
         /// X resolution in pixels.
         int      XRes;          
         /// Y resolution in pixels.
         int      YRes;          
         
         /// Estimated frame rate - may not be accurate!
         int      EstFrameRate;         

         /// Video format (unimportant to us, really, but differentiates between various types)
         VIDCAP_FORMAT  InputFormat;

         /// Internal reference information used by child classes.
         void*    InternalRef;                                    
         
         /// Pointer to next mode in list.
         struct VIDCAP_MODE*  NextMode;  
      };

      struct VIDCAP_DEVICE
      {
         char*    DeviceString;              // Device string
         void*    DeviceExtra;               // Used for IMoniker's on DSWin32
         struct   VIDCAP_DEVICE* NextDevice; // Ptr to next device, or null
      };

   //-------------------------------------------------------------------------
   // Internal definitions - protected
   //
   protected:
      /// VIDCAP_STATES enumerates the states the video capture may be in or 
      /// was previously in before being stopped.
      ///
      /// Right now this is used to know when we need to reconfigure
      /// the capture driver for buffering.
      ///
      enum VIDCAP_STATES
      {
         VIDCAP_UNCONNECTED,
         VIDCAP_SINGLE_SHOT_MODE,
         VIDCAP_CONTINUOUS_MODE
      };

      /// VIDCAP_PROCAMP_PROPS contains the camera property information
      /// for a specific property (e.g. Brightness, Contrast, etc).
      ///
      /// This mostly mirrors the info returned by DirectShow's 
      /// IAMVideoProcAmp::GetRange for controllable camera properties.
      /// We'll use the same model on other platforms later though.
      ///
      /// \sa GetPropertyInfo(), SetProperty(), GetPropertyName(),
      /// \sa CAMERA_PROPERTY
      ///
      struct VIDCAP_PROCAMP_PROPS
      {
         /// Is this property supported?
         bool Supported;      
         /// Property ID
         long Property;       
         /// Min value of property
         long Min;            
         /// Max value of property
         long Max;            
         /// Minimum Step size between values
         long SteppingDelta;  
         /// Default value of property
         long Default;        
         /// 1 = automatically controlled by driver, 2 = manually controlled
         long CapsFlags;      
      };
   
   //-------------------------------------------------------------------------
   // Public interface - overrideables
   //
   public:
      CVVidCapture               (     );
      virtual ~CVVidCapture      (     ); 

      //---------------------------------------------------------------------
      // Override functions - you *must* implement these in child classes!
      //---------------------------------------------------------------------

      /// Init must be called first before any other operations.
      /// Init() must set the fInitialized member to true on success.
      ///
      /// \return CVRES result code.
      /// \sa CVRes.h, CVResVidCap.h, CVVidCapture::Uninit()
      virtual CVRES Init         (     )  =  0;


      /// ClearDeviceList() clears the list of available devices, if any.
      /// Some platforms may need to override this for cleanup if they
      /// store extra info in the VIDCAP_DEVICE struct.
      virtual void ClearDeviceList();

      /// RefreshDeviceList() refreshes the list of devices
      /// available to VidCapture.
      ///
      /// \return CVRES result code
      virtual CVRES RefreshDeviceList() = 0;

      /// GetNumDevices() returns the number of devices available.
      ///
      /// The device list is build on Init(), and can be refreshed
      /// by calling RefreshDeviceList().  It is recommended that
      /// RefreshDeviceList() be called each time you enumerate
      /// through the devices, as devices may be added or removed.
      ///
      /// \param numDevices - set to total number of available devices
      ///
      /// \return CVRES result code
      /// \sa RefreshDeviceList()
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES  GetNumDevices( int& numDevices);

      /// GetDeviceInfo() retrieves the video capture device info for a specified
      /// index from the enumeration.
      ///
      /// You must have called Init() previously. If it has been some time since
      /// calling init, refresh the device list with RefreshDeviceList() prior
      /// to getting the number of devices and device info.
      ///
      /// \param index - index into device list to retrieve information on.
      /// \param deviceInfo - video capture device information. Set on return.
      ///
      /// \return CVRES result code
      /// \sa VIDCAP_DEVICE, GetNumDevices(), RefreshDeviceList(), Connect()
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES  GetDeviceInfo        (  int                  index,
                                             VIDCAP_DEVICE&       deviceInfo);
      
      /// Connect() connects to a specific video capture device.  
      ///
      /// Use the index given by GetNumDevices() and GetDeviceInfo().
      //
      /// Init() must be called prior to Connect().      
      /// Connect() must set fConnected to true on success.
      ///
      /// \param devIndex - Index of device in device list.
      /// 
      /// \return CVRES result code.
      /// \sa RefreshDeviceList(), GetNumDevices(), GetDeviceInfo()
      /// \sa Init(), Disconnect(), CVRes.h, CVResVidCap.h
      virtual CVRES Connect      (  int   devIndex ) = 0;

      /// StartImageCap() starts continuous image capture until Stop() 
      /// is called.
      /// Both Init() and Connect() must have been successfully called prior
      /// to calling StartImageCap().
      ///
      /// imageType specifies the image type to return.
      /// callback must be a valid CVVIDCAP_CALLBACK callback, and is called
      /// for each image.  Try not to do heavy processing within the callback.
      ///
      /// Must set fStarted to true on success.
      /// Must also set fLastState to VIDCAP_CONTINUOUS_MODE
      ///
      /// \param imageType - type of image to send to callback for each frame
      /// \param callback  - callback to be called on each frame
      /// \param userParam - user defined value passed into callback
      /// 
      /// \return CVRES result code.
      /// \sa StartRawCap(), Grab(), Stop(), CVImage
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES StartImageCap(  CVImage::CVIMAGE_TYPE   imageType,
                                    CVVIDCAP_CALLBACK       callback, 
                                    void*                   userParam)  = 0;


      /// Grab() is a single shot synchronous grab.
      ///
      /// Caller should pass an uninstantiated image ptr and the desired
      /// image type.  imagePtr will be instantiated with the captured 
      /// image on success.
      ///
      /// Image must be deleted by caller when done by 
      /// calling CVImage::ReleaseImage().
      ///
      /// \param imageType - type of image to grab
      /// \param imagePtr - uninstantiated image ptr. Set on return.
      ///
      /// \return CVRES result code.
      /// \sa StartImageCap(), StartRawCap(), Stop(), CVImage
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES Grab         (  CVImage::CVIMAGE_TYPE   imageType,
                                    CVImage*&               imagePtr)   = 0;

      //---------------------------------------------------------------------
      // Release functions - these should also be overridden by child classes
      //---------------------------------------------------------------------

      /// Uninit object before deletion (matches with CVVidCapture::Init()).
      /// Must set fInitialized to false on success.
      ///
      /// \return CVRES result code.
      /// \sa Init(), CVRes.h, CVResVidCap.h
      virtual CVRES Uninit       (     );

      /// Disconnect from a previously connected capture device.
      /// Should only be called if a previous CVVidCapture::Connect() was 
      /// successful.
      ///
      /// Must set fConnected to false on success.
      /// Must also set fLastState to VIDCAP_UNCONNECTED
      ///
      /// \return CVRES result code.
      /// \sa Connect(), CVRes.h, CVResVidCap.h
      virtual CVRES Disconnect   (     );
      
      
      /// Stop() stops an active image capture started with StartImageCap().
      /// (In derived classes, must stop other forms of continuous capture
      ///  as well such as CVVidCaptureDSWin32::StartRawCap()).
      ///
      /// Do NOT call Stop() from within a callback or it will cause
      /// a deadlock. Instead, return false from the callback to
      /// cause an abort, then call Stop() from your main thread.
      ///
      /// Must set fStarted to false on success.
      ///
      /// \return CVRES result code.
      /// \sa StartImageCapture(), CVVIDCAP_CALLBACK, CVRes.h, CVResVidCap.h
      virtual CVRES Stop         (     );

      //---------------------------------------------------------------------
      // Property settings for cameras.
      //

	  // David C. Partridge October 2016 - reinstate Show Property Dialog method.
	  virtual void ShowPropertyDialog(HWND parent) = 0;

	  //
      // These may or may not be supported on any given camera or capture
      // implementation.
      //
      // If you receive CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED, then your camera
      // doesn't support the specified property.
      
      /// GetPropertyInfo retrieves information about a specific camera 
      /// property.       
      /// It may return CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED if the
      /// property is not support on the device or in the VidCapture 
      /// framework.
      ///
      /// All pointers may be NULL.
      ///      
      /// You must have a device connected from a successful call to
      /// Connect() to use this function.
      ///
      /// \param property - ID of property (e.g. CAMERAPROP_BRIGHT)
      /// \param curVal   - ptr to receive current value of property.
      /// \param defVal   - ptr to receive default value of property.
      /// \param minVal   - ptr to receive minimum value of property.
      /// \param maxVal   - ptr to receive maximum value of property.
      /// \param step     - ptr to receive the minimum step distance
      ///                   between values.            
      /// \return CVRES result code.
      /// \sa Init(), Connect(), SetProperty(), GetPropertyName()
      /// \sa CAMERA_PROPERTY, CVRes.h, CVResVidCap.h
      virtual CVRES  GetPropertyInfo   (  CAMERA_PROPERTY         property,
                                          long*                   curVal = 0,
                                          long*                   defVal = 0,
                                          long*                   minVal = 0,
                                          long*                   maxVal = 0,
                                          long*                   step   = 0);

      /// SetProperty() sets the specified property to the given value.
      /// Use GetPropertyInfo() to get the supported range and resolution
      /// of the property in use.
      /// You must have a device connected from a successful call to
      /// Connect() to use this function.
      ///
      /// \param property - ID of property.
      /// \param value - new value to set property to. Must be within range.
      /// \return CVRES result code
      /// \sa Connect(), GetPropertyInfo(), GetPropertyName(), CAMERA_PROPERTY
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES  SetProperty    (  CAMERA_PROPERTY            property,
                                       long                       value);
         
                                 
      /// GetPropertyName() retrieves the name of the specified property.            
      /// maxLength should be the maximum buffer length including a space for
      /// a null - use kCVVidCapture_MaxPropNameLen+1 for your buffer size.
      /// Name will be truncated to fit if necessary. 
      /// You must have a device connected from a successful call to
      /// Connect() to use this function.
      ///
      /// \param property - ID of property to retrieve the name of.
      /// \param nameBuffer - preallocated buffer to hold property name
      /// \param maxLength - length of the buffer, sans the terminating null.
      ///
      /// \return CVRES result code
      /// \sa GetPropertInfo(), SetProperty(), CAMERA_PROPERTY, Connect()
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES  GetPropertyName(  CAMERA_PROPERTY            property,
                                       char*                      nameBuffer,
                                       int                        maxLength);


      //---------------------------------------------------------------------
      // Mode settings for cameras.
      //
      // These may or may not be supported on any given camera or capture
      // implementation. During a Connect(), you enumerate the supported
      // modes into fModeList.  Disconnect() should free this list.
      //
      // You'll should only need to override SetMode(VIDCAP_MODE&) and
      // possibly the ClearModes() functions when supporting this feature.
      //
      // If you receive CVRES_VIDCAP_MODE_NOT_SUPPORTED, then your camera
      // doesn't support the specified mode or modes at all.

      /// GetNumSupportedModes() retrieves the number of modes available
      /// on the currently connected image capture device.
      ///
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      ///
      /// \param numModes - set to total number of available modes on return
      ///
      /// \return CVRES result code
      /// \sa VIDCAP_MODE, GetModeInfo(), SetMode(), GetCurrentMode()
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES  GetNumSupportedModes (  int&                 numModes);
      
      /// GetModeInfo() retrieves the video capture mode for a specified
      /// index from the enumeration.
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      ///
      /// \param index - index into mode list to retrieve information on.
      /// \param modeInfo - video mode information. Set on return.
      ///
      /// \return CVRES result code
      /// \sa VIDCAP_MODE, GetNumSupportedModes(), SetMode(), GetCurrentMode()
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES  GetModeInfo          (  int                  index,
                                             VIDCAP_MODE&         modeInfo);
      
      /// SetMode() sets the current capture mode to the one in the
      /// mode list that matches the specified index.
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      ///
      /// \param index - index into mode list
      /// \param rawYUY2 - PHD2 hack, do not convert to RGB
      /// \return CVRES result code
      /// \sa GetNumSupportedModes(), GetModeInfo(), GetCurrentMode()
      /// \sa VIDCAP_MODE, CVRes.h, CVResVidCap.h
      virtual CVRES  SetMode              (  int                  index,
                                             bool                 rawYUY2 );

      /// GetCurrentMode() retrieves the current or last-used video 
      /// capture mode.
      ///
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      ///
      /// \param curMode - set to current mode on return
      /// \return CVRES result code
      /// \sa GetNumSupportedModes(), GetModeInfo(), SetMode()
      /// \sa VIDCAP_MODE, CVRes.h, CVResVidCap.h
      virtual CVRES  GetCurrentMode       (  VIDCAP_MODE&         curMode );
      

      /// GetFormatModeName() retrieves the video format mode name
      /// from the enumeration value.
      ///
      /// \param format - video format of camera
      /// \return const char* - ptr to const string describing video format
      const char* GetFormatModeName( VIDCAP_FORMAT format);

      /// AddMode adds a video mode to the list
      /// \param addMode - Video capture mode to add to internal list
      /// \return CVRES result code
      /// \sa GetNumSupportedModes(), GetModeInfo(), VIDCAP_MODE
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES  AddMode              (  VIDCAP_MODE&         addMode );

      // ----------------------------------
      // Override these in child classes!
       
      /// SetMode - sets the video mode for a connected camera.
      ///           This version must be overridden in child classes.
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      /// 
      /// \param newMode - new mode to use for connected device.
      /// \param rawYUY2 - PHD2 hack, do not convert to RGB
      /// \return CVRES result code
      /// \sa GetNumSupportedModes(), GetModeInfo(), GetCurrentMode()
      /// \sa VIDCAP_MODE, CVRes.h, CVResVidCap.h
      virtual CVRES  SetMode              (  VIDCAP_MODE&         newMode,
                                             bool                 rawYUY2 );

   //-------------------------------------------------------------------------
   // Protected interface - overrideables
   //
   protected:
      /// ClearModes() - clears the mode list.
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      /// \sa GetNumSupportedModes(), GetModeInfo(), AddMode()
      virtual void   ClearModes           (  );
      
   //---------------------------------------------------------------------
   // Public status information functions
   // These should not need to be overridden
   //
   public:

      /// GetDeviceName() retrieve the device name into a buffer/
      /// The buffer must already be created.
      ///
      /// \param nameBuffer - buffer to copy device name into
      /// \param maxLength - maximum length of the nameBuffer, set to 
      ///                    the length of the device name on return.
      /// \return CVRES result code
      /// \sa Connect(), EnumDevices(), CVRes.h, CVResVidCap.h
      CVRES          GetDeviceName  (  char*                   nameBuffer, 
                                       int&                    maxLength );

      /// IsInitialized() returns true of the vidCapture class has been initialized.
      /// \return bool - true if initialized, false if not.
      /// \sa Init(), Uninit()
      bool           IsInitialized();

      /// IsConnected() returns true if we've connected to a DirectShow
      /// compatible capture device.
      /// \return bool - true if connected, false if not.
      /// \sa Connect(), Disconnect()
      bool           IsConnected();

      /// IsStarted() returns true if we're currently doing a continuous grab.
      /// \return bool - true if continuous capture has been started
      /// \sa StartImageCap(), Stop()      
      bool           IsStarted();
   
   //---------------------------------------------------------------------      
   // Protected members
   protected:
      // Has the class been initialized?
      bool           fInitialized;

      // Has a specific device been connected?
      bool           fConnected;

      // Are we currently capturing?
      bool           fStarted;

      // Device name of connected device (if fConnected == true)
      char*          fDeviceName;

      // Current video mode
      VIDCAP_MODE    fCurMode;
      VIDCAP_MODE*   fModeList;
      VIDCAP_DEVICE* fDeviceList;
      int            fNumDevices;
      // Previous video capture state
      VIDCAP_STATES  fLastState;
};


/// Max property name table string length
extern const int kCVVidCapture_MaxPropNameLen;
/// Max length of a format name
extern const int kCVVidCapture_MaxFormatNameLen;
// Capture library version 
extern const int kVIDCAPTURE_VERSION;
// Capture library copyright string
extern const char* kVIDCAPTURE_STRING;

#endif // _CVVIDCAPTURE_H_

