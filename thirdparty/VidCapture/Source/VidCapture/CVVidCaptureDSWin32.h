#ifndef WIN32
   #pragma message ("WIN32 not defined. Not compiling CVVidCaptureDSWin32")
#else
/// \file CVVidCaptureDSWin32.h
/// \brief DirectShow video capture class definition.
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
/// \class CVVidCaptureDSWin32 CVVidCaptureDSWin32.h
/// CVVidCaptureDSWin32 provides the core interface to a 
/// DirectShow-based video capture device under Win32. It implements
/// CVVidCapture and ISampleGrabberCB.
///
/// To use, first call Init() to initialize COM and set up the underlying
/// capture graph.
///
/// After successful initialization, you can call GetNumDevices() and 
/// GetDeviceInfo() to enumerate the available video capture devices.  
///
/// Once you've decided which capture device to use, call
/// Connect() with the desired device index to connect to it.  Now
/// you can read and modify any of the camera properties and 
/// video modes.
///
/// When you're ready to receive images, either call Grab() for a single-
/// shot grab, or setup a callback and call StartImageCap().  If you
/// absolutely need speed over convenience and portability, you can use
/// StartRawCap() and receive the video headers and bitmap information
/// directly.
/// 
/// <i>Always check the incoming status code in the callbacks prior to 
///    attempting to access the data.  If the status code is a failed result,
///    the data is not present!</i>
///
/// Call Stop() to end a StartImageCap() continuous capture. Do NOT call 
/// Stop() from within a callback or it will deadlock.  You still need to
/// call Stop() when you're done to clean up even if you've aborted by 
/// returning false from the callback.
///
/// When you're done, call Disconnect(), then Uninit() to clean up.
///
/// COM Usage:
///       CVVidCaptureDSWin32 currently calls CoInitializeEx in 
///       multithreaded mode. If you are already using COM, but 
///       need apartment model, change the Init() function as 
///       necessary.
///
/// Threading: 
///       Right now, it is recommended to use a single thread to
///       access a single instantiation of the CVVidCaptureDSWin32 object.
///       Otherwise, you should synchronize access via a mutex or 
///       critical section.
///
///       Callbacks will come from a DirectShow thread, so you'll need
///       to synchronize on any resources you use from the callback. However,
///       you shouldn't take too long inside a callback - I'd recommend just
///       copying the data out and throwing it on a queue for later processing
///       outside of the callback itself.
///
/// $RCSfile: CVVidCaptureDSWin32.h,v $
/// $Date: 2004/03/01 18:31:06 $
/// $Revision: 1.6 $
/// $Author: mikeellison $

#ifndef _CVVidCaptureDSWin32_H_
#define _CVVidCaptureDSWin32_H_

#include <Dshow.h>         // DirectShow (using DirectX 9.0 for dev)
#include <dvdmedia.h>	   // VideoInfoHeader2
#include <qedit.h>         // Sample grabber defines
#include "CVVidCapture.h"  // Parent class

// Deriving from ISampleGrabberCB to receive callbacks during capture runs.
// CVVidCapture supplies our basic interface.
class CVVidCaptureDSWin32 : public CVVidCapture, private ISampleGrabberCB
{
   //-------------------------------------------------------------------------
   // Public definitions and callbacks
   // 
   public:
      /// CVVIDCAP_ENUMDSWIN32CB is a specialized callback definition for 
      /// enumeration of video capture devices under DirectShow in win32. 
      /// It allows us to pass the IMoniker* in addition to information
      /// passed by CVVIDCAP_ENUMCB
      ///
      /// \param devName - ASCIIZ device name. Use this for calls to Connect()
      /// \param moniker - DirectX device moniker.
      /// \param userParam - user defined paramter, passed into EnumDevices()
      ///
      /// \return bool - true continues enumeration, false halts enumeration.
      /// \sa EnumDevices(), CVVidCap::CVVIDCAP_ENUMCB
      typedef bool (*CVVIDCAP_ENUMDSWIN32CB) (  const char* devName, 
                                                IMoniker*   moniker, 
                                                void*       userParam   );
      
      /// CVVIDCAP_RAWCB is the specialized callback definition for 
      /// continuous raw captures under DirectShow in win32.
      ///
      /// It's faster than the standard image callback, but less convenient 
      /// than the standard image capture callback since it doesn't set up a 
      /// CVImage object for us.  The header and buffer passed are only
      /// valid during the callback.
      ///
      /// First, check the status code - if it's a successful status code,
      /// (e.g. if CVSUCCESS(status) returns true), then the imagePtr is 
      /// valid. Otherwise, some sort of error has occurred -
      /// most likely, the camera has been disconnected.      
      ///
      /// Returning true continues the capture, returning false aborts it.
      /// You'll still have to call Stop() from another thread 
      /// (e.g. outside the callback), but no more callbacks will 
      /// be received after an abort and the processing will halt.
      ///
      /// Do NOT call Stop() from within a callback or it will cause
      /// a deadlock. Instead, return false from the callback to
      /// cause an abort, then call Stop() from your main thread.
      ///
      /// \param status - Status of the capture 
      ///                (CVRES_VIDCAP_OK, CVRES_VIDCAP_CAPTURE_ERROR)
      /// \param vHeader - raw VIDEOINFOHEADER. See DirectShow docs.
      /// \param buffer - raw bitmap buffer, matches format in vHeader
      /// \param userParam - user defined parameter, passed into StartRawCap
      ///
      /// \return bool - true continues capture, false halts it.
      /// \sa StartRawCap(), StartImageCap(), CVVIDCAP_CALLBACK      
      typedef bool (*CVVIDCAP_RAWCB)   (  CVRES                  status,
                                          const VIDEOINFOHEADER* vHeader,
                                          unsigned char*         buffer,
                                          void*                  userParam );

   //-------------------------------------------------------------------------
   // Public interface
   //
   public:      
      /// Constructor for DirectShow video capture
      /// \sa CVVidCapture::CVVidCapture
      CVVidCaptureDSWin32        (     );

      /// Destructor for DirectShow video capture
      /// \sa CVVidCapture::~CVVidCapture
      virtual ~CVVidCaptureDSWin32  (     );

      /// Init must be called first before any other operations.
      /// Init() must set the fInitialized member to true on success.
      ///
      /// \return CVRES result code.
      /// \sa Uninit(), CVVidCapture::Init(), CVRes.h, CVResVidCap.h
      virtual CVRES Init         (     );
      
      /// Uninit object before deletion (matches with Init()).
      /// Must set fInitialized to false on success.
      ///
      /// \return CVRES result code.
      /// \sa Init(), CVVidCapture::Uninit(), CVRes.h, CVResVidCap.h
      virtual CVRES Uninit       (     );

      virtual void ClearDeviceList();
      
      /// RefreshDeviceList() refreshes the list of devices
      /// available to VidCapture.
      ///
      /// \return CVRES result code
      virtual CVRES RefreshDeviceList();

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
      virtual CVRES Connect      (  int devIndex );

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
                                    void*                   userParam);

      /// StartRawCap() starts a continuous grab sending raw data to callback.
      /// Call Stop() to stop the capture.
      ///
      /// Buffers will currently always be 24-bit RGB bitmaps due to the
      /// ISampleGrabber configuration.
      ///
      /// Both Init() and Connect() must have been successfully called prior
      /// to calling StartImageCap().
      ///
      /// Must set fStarted to true on success.
      /// Must also set fLastState to VIDCAP_CONTINUOUS_MODE
      ///
      /// \param callback - function to receive callbacks for each frame
      /// \param userParam - user defined param to send to callback
      /// \return CVRES result code.
      /// \sa StartImageCap(), Grab(), Stop(), CVVIDCAP_RAWCB
      /// \sa CVRes.h, CVResVidCap.h
      virtual CVRES StartRawCap  (  CVVIDCAP_RAWCB          callback, 
                                    void*                   userParam);
      
      /// Stop() stops an active image capture started with StartImageCap()
      /// or StartRawCap().
      ///
      /// Do NOT call Stop() from within a callback or it will cause
      /// a deadlock. Instead, return false from the callback to
      /// cause an abort, then call Stop() from your main thread.
      ///
      /// Must set fStarted to false on success.
      ///
      /// \return CVRES result code.
      /// \sa StartImageCapture(), StartRawCap(), CVRes.h, CVResVidCap.h
      /// \sa CVVIDCAP_CALLBACK, CVVIDCAL_RAWCB
      virtual CVRES Stop         (     );

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
                                    CVImage*&               imagePtr);


      //---------------------------------------------------------------------
      // Property settings for cameras.
      // These may or may not be supported on any given camera.
      // If you receive CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED, then your camera
      // doesn't support the specified property.
      
	  // Dave Partridge October 2016 - reinstate Show Property Dialog method.
	  void ShowPropertyDialog(HWND parent);

      
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
      virtual CVRES  GetPropertyInfo(  CAMERA_PROPERTY      property,
                                       long*                curVal = 0,
                                       long*                defVal = 0,
                                       long*                minVal = 0,
                                       long*                maxVal = 0,
                                       long*                step   = 0);

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
      virtual CVRES  SetProperty    (  CAMERA_PROPERTY      property,
                                       long                 value);
                                                                        

      /// SetMode - sets the video mode for a connected camera.
      ///           This version must be overridden in child classes.
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      /// 
      /// \param newMode - new mode to use for connected device.
      /// \param rawYUY2 - PHD2 hack - do not convert to RGB
      /// \return CVRES result code
      /// \sa GetNumSupportedModes(), GetModeInfo(), GetCurrentMode()
      /// \sa VIDCAP_MODE, CVRes.h, CVResVidCap.h
      virtual CVRES  SetMode              (  VIDCAP_MODE&         newMode,
                                             bool                 rawYUY2 );

   protected:
      /// ClearModes() - clears the mode list.
      /// You must have called Connect() already to enumerate the available
      /// modes for the connected device before calling this function.
      /// \sa GetNumSupportedModes(), GetModeInfo(), AddMode()
      virtual void   ClearModes           ();


   private:
      // GetVidCapFormat() converts the video format type from a DirectX
      // value to one of the values in the VIDCAP_FORMAT enumeration.
      static VIDCAP_FORMAT GetVidCapFormat( GUID* directXFormat);

      // GetDirectXFormat() converts the video format type from a
      // VIDCAP_FORMAT value to the appropriate MEDIASUBTYPE_*
      // value for DirectX.
      static const GUID* GetDirectXFormat( VIDCAP_FORMAT vidCapFormat);

      // Local Enumeration callback to find the device we want to connect to.
      static bool ConnectEnum (  const char*             devName, 
                                 IMoniker*               moniker, 
                                 void*                   userParam);

      // Thread to watch for the user aborting the capture and stop it.
      static unsigned int WINAPI CaptureAbortThreadFunc(void *userParam);

      // Internal uninitialization function
      void        UninitObjects  (     );

      // Starts the abort thread for StartImageCap() and StartRawCap()
      CVRES StartAbortThread();

      // Halt the abort thread
      CVRES HaltAbortThread();

      // Sets the fCallbackStatus inside a critical section
      // (it may be accessed by DirectShow's thread, our abort thread,
      //  or the host caller's thread)
      void SetCallbackStatus(CVRES newStatus);

      // Retrieves the fCallbackStatus inside a critical section
      CVRES GetCallbackStatus();

      // Stops the capture graph within a critical section - 
      // do this so that we can safely call it from our abort thread.
      CVRES StopCaptureGraph();

      // ConnectGraph() connects up the capture graph
      CVRES ConnectGraph();

      // DisconnectGraph() disconnects the items in the capture graph
      CVRES DisconnectGraph();
      //-----------------------------------------------------------------
      // COM ISampleGrabberCB interface.
      // We keep this private.

      /// COM QueryInterface - we support IUnknown and ISampleGrabberCB
      /// Retrieves the requested interface.
      /// Call Release() on the returned ppvObject when done.
      ///
      /// \param iid - REFIID for the desired interface
      /// \param ppvObject - ptr to object ptr to be set to interface ptr.
      /// 
      /// \return HRESULT - S_OK on success
      /// \sa Release()
      HRESULT WINAPI QueryInterface( REFIID iid, void** ppvObject );

      /// AddRef() increments the COM reference count for our ISampleGrabberCB
      /// interface.
      /// \returns ULONG - new reference count.
      /// \sa QueryInterface(), Release()
      ULONG WINAPI AddRef();

      /// Release() decrements the COM reference count for our 
      /// ISampleGrabberCB interface.
      /// We don't actually free our object here.
      /// However, the value should never become 0 here either, since 
      /// instantiating our class starts the reference count at 1.
      /// \returns ULONG - new reference count.
      /// \sa QueryInterface(), AddRef()
      ULONG WINAPI Release();

      /// SampleCB() COM Sample callback for video capture.
      /// This is called by DirectShow when we do capture runs.
      /// It is derived from the ISampleGrabberCB interface.
      /// \param sampleTimeSec - time stamp of when image was taken
      /// \param mediaSample* - IMediaSample interface containing sample
      /// \return HRESULT - S_OK on success.
      /// \sa StartImageCap(), StartRawCap(), CVVIDCAP_CALLBACK, CVVIDCAP_RAWCB
      HRESULT WINAPI SampleCB (  double         sampleTimeSec,
                                 IMediaSample*  mediaSample    );

      /// BufferCB() COM buffer callback for video capture.
      /// It is derived from the ISampleGrabberCB interface.
      /// We do not currently implement this function - use
      /// SampleCB() instead.
      ///
      /// \sa SampleCB()
      HRESULT WINAPI BufferCB (  double         sampleTimeSec,
                                 BYTE*          bufferPtr,
                                 long           bufferLength   );

      // Reference counter for our COM interface (ISampleGrabberCB)
      unsigned long     fRefCount;

      //-----------------------------------------------------------------
      // Internal variables - all private
      //

      // Main filter graph for building up our capture system.
      // We'll build it as CaptureDevice->SampleGrabber->NullRenderer.
      IGraphBuilder*          fGraph;

      // SampleGrabber filter.
      // This allows us to intercept frames from within a capture graph
      IBaseFilter*            fSampleGrabberFilter;   
      
      // Capture device filter - acts as our source.
      // Could be from any video capture device selected.
      IBaseFilter*            fCaptureFilter;

      // Capture output pin.
      // We need this to get stream configuration interface
      IPin*                   fCapturePin;

      // fRenderer - rendering filter used to terminate the 
      // filter graph for DirectX without requiring a window to display in
      //
      // May be a NullRenderer, or a video renderer for an HWND.
      IBaseFilter*            fRenderer;

      // Interface to the SampleGrabber for direct control
      ISampleGrabber*         fSampleGrabber;

      // Video preprocessing interface. Allows us to set brightness,
      // contrast, and other options for the capture device.
      IAMVideoProcAmp*        fVideoProcAmp;
      
      // Stream configuration - if available, used to set resolution 
      // and format.
      IAMStreamConfig*        fStreamConfig;

      // Media type - we force to 24-bit RGB.
      AM_MEDIA_TYPE           fMediaType;

      // Video header - contains information about our video stream.
      // check fVideoHeader->bmiHeader for a bitmap header for raw stream.
      VIDEOINFOHEADER*        fVideoHeader;

      // Capture Graph controller. Used to start/stop the capture system.
      IMediaControl*          fCaptureControl;

      // Capture event - used for sync'ing to stops in single shot mode
      //                 and watching for the termination of capture runs.
      IMediaEvent*            fCaptureEvent;
      
      // Thread to watch for user aborted captures. Stops the
      // capture if fAbortEvent is signalled and fAborted is
      // true.
      HANDLE                  fCaptureAbortThread;

      // Event handle used to indicate that the abort thread is ready.
      HANDLE                  fCaptureAbortThreadReady;

      // Event handle used to watch for aborted capture runs.
      HANDLE                  fAbortEvent;

      // Status to pass to callback. May be set by the
      // abort thread if it detects an error like the
      // camera being disconnected...
      //
      // Don't access this directly while running a capture sequence.
      // Instead, use [Get|Set]CallbackStatus() to access it from
      // within a critical section.
      //
      CVRES                   fCallbackStatus;

      // Mutex used when checking callback status. 
      HANDLE                  fStatusLock;

      // Mutex used when stopping the capture
      HANDLE                  fStopLock;

      // Flag to indicate if the user aborted from a callback, or if
      // the fAbortEvent was set by Stop().
      bool                    fAborted;

      // Are the filters connected?
      bool                    fFiltersConnected;

      // Media filter - allows control of clock for the graph
      IMediaFilter*           fCapMediaFilter;

      // Unused right now, but this can be made as the reference clock
      // for clocking image capture if desired.
      IReferenceClock*        fClock;

      // Only one of the two following callbacks should be used at a time.
      // The other one must be set to null.
      // StartImageCap() uses fCaptureCallback, 
      // StartRawCap() uses fRawCallback.

      // Capture callback that receives a CVImage*.
      CVVIDCAP_CALLBACK       fCaptureCallback;

      // Capture callback that receives a raw video header and data ptr.
      CVVIDCAP_RAWCB          fRawCallback;

      // User parameter used for both of the above callbacks
      void*                   fCaptureUserParam;

      // Image type to use for capture
      CVImage::CVIMAGE_TYPE   fImageType;

      // Camera properties - this array tells us what properties, if any, are
      // supported through fVideoProcAmp and what the value ranges are.
      VIDCAP_PROCAMP_PROPS    fProcAmpProps[CAMERAPROP_NUMPROPS];

};

#endif // _CVVidCaptureDSWin32_H_

#endif // WIN32
