#ifndef WIN32
   #pragma message ("WIN32 not defined. Not compiling CVVidCaptureDSWin32")
#else

// CVVidCaptureDSWin32 - DirectShow video capture class
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
/// \file CVVidCaptureDSWin32.cpp
/// \brief CVVidCaptureDSWin32 provides the core interface to a 
/// DirectShow-based video capture device under Win32. 
///
/// Implements CVVidCapture and ISampleGrabberCB.
///
/// See CVVidCaptureDSWin32.h for usage.
///
/// $RCSfile: CVVidCaptureDSWin32.cpp,v $
/// $Date: 2004/03/01 18:31:06 $
/// $Revision: 1.10 $
/// $Author: mikeellison $

// Setup for Windows 98 features. We support 98/ME/2K/XP.
#define _WIN32_WINDOWS 0x0410
#define _WIN32_DCOM

#include "CVUtil.h"              // Utility and debugging
#include "CVVidCaptureDSWin32.h" // DirectShow video capture
#include "CVDShowUtil.h"         // DirectShow utilities
#include "CVImage.h"             // image class
#include <process.h>             // _beginthreadex()


// Table for converting between format enumerations...
struct VIDCAP_FORMAT_CONV
{
   VIDCAP_FORMAT   VidCapFormat;
   const GUID*     DirectShowFormat;
};

// I420 isn't defined in any of the headers, but everyone seems to use it.... 
extern "C" const __declspec(selectany) GUID CVMEDIASUBTYPE_I420 =
{0x30323449,0x0000,0x0010, {0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}};

// IYUV
extern "C" const __declspec(selectany) GUID CVMEDIASUBTYPE_IYUV =
   {0x56555949,0x0000,0x0010, {0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}};

// Y444
extern "C" const __declspec(selectany) GUID CVMEDIASUBTYPE_Y444 = 
   {0x34343459, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

// Y800
extern "C" const __declspec(selectany) GUID CVMEDIASUBTYPE_Y800 = 
   {0x30303859, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

// Y422
extern "C" const __declspec(selectany) GUID CVMEDIASUBTYPE_Y422 = 
   {0x32323459, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};


// Keep this table up-to-date with the 
// enumeration definition in CVVidCapture.h...
const VIDCAP_FORMAT_CONV kDSWin32VideoFormats[VIDCAP_NUM_FORMATS] =
{
   VIDCAP_FORMAT_UNKNOWN,        0,
   VIDCAP_FORMAT_YVU9,           &MEDIASUBTYPE_YVU9,
   VIDCAP_FORMAT_Y411,           &MEDIASUBTYPE_Y411,
   VIDCAP_FORMAT_Y41P,           &MEDIASUBTYPE_Y41P,
   VIDCAP_FORMAT_YUY2,           &MEDIASUBTYPE_YUY2,
   VIDCAP_FORMAT_YVYU,           &MEDIASUBTYPE_YVYU,
   VIDCAP_FORMAT_UYVY,           &MEDIASUBTYPE_UYVY,
   VIDCAP_FORMAT_Y211,           &MEDIASUBTYPE_Y211,
   VIDCAP_FORMAT_CLJR,           &MEDIASUBTYPE_CLJR,
   VIDCAP_FORMAT_IF09,           &MEDIASUBTYPE_IF09,
   VIDCAP_FORMAT_CPLA,           &MEDIASUBTYPE_CPLA,
   VIDCAP_FORMAT_MJPG,           &MEDIASUBTYPE_MJPG,
   VIDCAP_FORMAT_TVMJ,           &MEDIASUBTYPE_TVMJ,
   VIDCAP_FORMAT_WAKE,           &MEDIASUBTYPE_WAKE,
   VIDCAP_FORMAT_CFCC,           &MEDIASUBTYPE_CFCC,
   VIDCAP_FORMAT_IJPG,           &MEDIASUBTYPE_IJPG,
   VIDCAP_FORMAT_Plum,           &MEDIASUBTYPE_Plum,
   VIDCAP_FORMAT_RGB1,           &MEDIASUBTYPE_RGB1,
   VIDCAP_FORMAT_RGB4,           &MEDIASUBTYPE_RGB4,
   VIDCAP_FORMAT_RGB8,           &MEDIASUBTYPE_RGB8,
   VIDCAP_FORMAT_RGB565,         &MEDIASUBTYPE_RGB565,
   VIDCAP_FORMAT_RGB555,         &MEDIASUBTYPE_RGB555,
   VIDCAP_FORMAT_RGB24,          &MEDIASUBTYPE_RGB24, 
   VIDCAP_FORMAT_RGB32,          &MEDIASUBTYPE_RGB32, 
   VIDCAP_FORMAT_ARGB32,         &MEDIASUBTYPE_ARGB32,
   VIDCAP_FORMAT_Overlay,        &MEDIASUBTYPE_Overlay,
   VIDCAP_FORMAT_QTMovie,        &MEDIASUBTYPE_QTMovie,
   VIDCAP_FORMAT_QTRpza,         &MEDIASUBTYPE_QTRpza,
   VIDCAP_FORMAT_QTSmc,          &MEDIASUBTYPE_QTSmc,
   VIDCAP_FORMAT_QTRle,          &MEDIASUBTYPE_QTRle,
   VIDCAP_FORMAT_QTJpeg,         &MEDIASUBTYPE_QTJpeg,
   VIDCAP_FORMAT_dvsd,           &MEDIASUBTYPE_dvsd,
   VIDCAP_FORMAT_dvhd,           &MEDIASUBTYPE_dvhd,
   VIDCAP_FORMAT_dvsl,           &MEDIASUBTYPE_dvsl,
   VIDCAP_FORMAT_MPEG1Packet,    &MEDIASUBTYPE_MPEG1Packet,
   VIDCAP_FORMAT_MPEG1Payload,   &MEDIASUBTYPE_MPEG1Payload,
   VIDCAP_FORMAT_VPVideo,        &MEDIASUBTYPE_VPVideo,
   VIDCAP_FORMAT_MPEG1Video,     &MEDIASUBTYPE_MPEG1Video,
   
   // Undeclared intel modes
   VIDCAP_FORMAT_I420,           &CVMEDIASUBTYPE_I420,
   VIDCAP_FORMAT_IYUV,           &CVMEDIASUBTYPE_IYUV,

   // More undeclared modes....
   VIDCAP_FORMAT_Y444,           &CVMEDIASUBTYPE_Y444,
   VIDCAP_FORMAT_Y800,           &CVMEDIASUBTYPE_Y800,
   VIDCAP_FORMAT_Y422,           &CVMEDIASUBTYPE_Y422
};



//---------------------------------------------------------------------------
// Constructor
CVVidCaptureDSWin32::CVVidCaptureDSWin32() 
: ISampleGrabberCB(), CVVidCapture()
{
   fFiltersConnected    = false;
   fCaptureCallback     = 0;
   fRawCallback         = 0;
   fCaptureUserParam    = 0;
   fCaptureFilter       = 0;
   fCapturePin          = 0;
   fSampleGrabberFilter = 0;
   fRenderer            = 0;
   fVideoHeader         = 0;
   fClock               = 0;
   fCaptureEvent        = 0;
   fCaptureControl      = 0;
   fCapMediaFilter      = 0;
   fVideoProcAmp        = 0;
   fStreamConfig        = 0;  
   fSampleGrabber       = 0;
   fGraph               = 0;  
   fCaptureAbortThread  = 0;
   fCaptureAbortThreadReady = 0;
   fAbortEvent      = 0;
   fCallbackStatus      = CVRES_SUCCESS;
   fAborted             = false;
      
   // Default to capturing RGB24 images.
   fImageType           = CVImage::CVIMAGE_RGB24;

   // For now, start with a 1 reference count.
   // We aren't really paying attention to this for destruction,
   // but we can make sure we're released on deletion with it.
   fRefCount            = 1;

   memset(&fMediaType,0,sizeof(fMediaType));
   memset(&fProcAmpProps,0,sizeof(fProcAmpProps));
}
//---------------------------------------------------------------------------
// Destructor
//    Make sure to both disconnect and uninitialize the video capture
//    devices!
CVVidCaptureDSWin32::~CVVidCaptureDSWin32()
{  
   fRefCount--;
   CVAssert(fRefCount == 0, "The reference count did not reach 0!");
}
//---------------------------------------------------------------------------
// Init
//    Initializes COM and the DirectShow capture graph
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::Init()
{
   CVRES    result = CVRES_SUCCESS;
   HRESULT  hres   = 0;
   
   if (fInitialized)
   {
      return CVRES_VIDCAP_ALREADY_INITIALIZED;
   }

   // Create an event for detecting aborts
   // Starts out unsignalled.
   if (INVALID_HANDLE_VALUE == (fAbortEvent = CreateEvent(0,TRUE,FALSE,0)))
   {
      return CVRES_OUT_OF_HANDLES;
   }

   // Reset aborted flag.
   fAborted        = false;

   // Create a lock for accessing the callback status and capture stop
   // calls so we can synch between our thread, the callers thread, and 
   // the DirectShow thread. Initialize to unlocked.
   fStatusLock = CreateMutex(0, FALSE,0);

   if (fStatusLock == INVALID_HANDLE_VALUE)
   {
      // Puked on mutex creation - bail.
      CloseHandle(fAbortEvent);
      fAbortEvent = 0;
      fStatusLock = 0;
      return CVRES_OUT_OF_HANDLES;
   }

   // Create a lock for accessing the capture stop.
   // This must be seperate from our status lock.
   fStopLock = CreateMutex(0, FALSE,0);

   if (fStopLock == INVALID_HANDLE_VALUE)
   {
      // Puked on mutex creation - bail.
      CloseHandle(fAbortEvent);
      CloseHandle(fStatusLock);
      fAbortEvent = 0;
      fStatusLock = 0;
      fStopLock = 0;
      return CVRES_OUT_OF_HANDLES;
   }
   
   // Initialize COM. Use CoInitialize(0) for older systems,
   // and set the COINIT_MULTITHREADED flag to COINIT_APARTMENTTHREADED
   // if you need to use apartment threading. See MSDN's 
   // CoInitializeEx() documentation for more information.
   hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if (hres == S_FALSE)
   {
      // Was already initialized. We should still call CoUninitialize for
      // this though for ref count.
   }
   else if ((hres == S_OK) || (hres == RPC_E_CHANGED_MODE))
   {
      // Initialization of COM was successful. 
      // Threading mode may have changed.
   }
   else
   {
      // Error initializing COM. Bail out.
      return CVRES_VIDCAP_COM_ERR;
   }
   
   // Create our graph
   if (FAILED(CoCreateInstance(  CLSID_FilterGraph,
                                 0, 
                                 CLSCTX_INPROC_SERVER, 
                                 IID_IGraphBuilder, 
                                 (void**)&fGraph)))
   {
      CoUninitialize();    
      return CVRES_VIDCAP_NO_FILTER_GRAPH;
   }

   // Get graph control and event
   if (FAILED(fGraph->QueryInterface(  IID_IMediaControl, 
                                       (void **)&fCaptureControl)))
   {     
      UninitObjects();
      return CVRES_VIDCAP_NO_CAPTURE_CONTROL;
   }

   if (FAILED(fGraph->QueryInterface(  IID_IMediaEvent, 
                                       (void **)&fCaptureEvent)))
   {
      UninitObjects();
      return CVRES_VIDCAP_NO_CAPTURE_EVENT;
   }
         
   // Create Sample Grabber   
   hres = CoCreateInstance(CLSID_SampleGrabber, 
                           0, 
                           CLSCTX_INPROC_SERVER,
                           IID_IBaseFilter,
                           (void**)&fSampleGrabberFilter);
   
   // Bail on failure.
   if (FAILED(hres))
   {
      UninitObjects();
      return CVRES_VIDCAP_NO_SAMPLE_GRABBER;
   }

   // Add the grabber into the graph   
   if (FAILED(fGraph->AddFilter(fSampleGrabberFilter, L"Sample Grabber")))
   {
      UninitObjects();
      return CVRES_VIDCAP_ADD_GRABBER_ERR;
   }     

   // Get the sample grabber interface from the filter
   fSampleGrabberFilter->QueryInterface(  IID_ISampleGrabber, 
                                          (void**)&fSampleGrabber);
   if (fSampleGrabber == 0)
   {
      CVTrace("Couldn't get sample grabber interface.");
      UninitObjects();
      return CVRES_VIDCAP_NO_ISAMPLEGRABBER;
   }

   // Setup media format for grabber to RGB 24-bit
   memset(&fMediaType,0,sizeof(fMediaType));
   
   fMediaType.majortype    = MEDIATYPE_Video;
   fMediaType.subtype      = MEDIASUBTYPE_RGB24;
      
   if (FAILED(hres = fSampleGrabber->SetMediaType(&fMediaType)))
   {     
      CVTrace("Couldn't set media type.");
      UninitObjects();
      return CVRES_VIDCAP_MEDIATYPE_SET_ERR;
   }

   fInitialized = true;

   return RefreshDeviceList();
}

//---------------------------------------------------------------------------
// RefreshDeviceList() refreshes the list of devices
// available to VidCapture.
//
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::RefreshDeviceList()
{
   // Clear out any previous enumeration.
   ClearDeviceList();
   
   CVRES    result   = CVRES_SUCCESS;
   HRESULT  hres     = 0;

   // Bail if not initialized ( we need COM! )
   CVAssert(   this->fInitialized, 
               "You must initialize the CVVidCapture object!");

   if (false == this->fInitialized)
   {     
      return CVRES_VIDCAP_MUST_INITIALIZE_ERR;
   }

   // Enumerate capture devices
   ICreateDevEnum*   devEnum     = 0;
   IEnumMoniker*     capEnum     = 0;
   IMoniker*         capDev      = 0;
   
   hres = CoCreateInstance(   CLSID_SystemDeviceEnum, 
                              0,
                              CLSCTX_INPROC_SERVER, 
                              IID_ICreateDevEnum, 
                              (void**)&devEnum);
   if (FAILED(hres))
   {
      return CVRES_VIDCAP_ENUM_ERR;
   }

   // Create an enumerator for the video capture category.
   hres = devEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory,
                                          &capEnum, 
                                          0);
   
   // Bail if we couldn't get it.
   if ( FAILED(hres) )
   {
      devEnum->Release();
      return CVRES_VIDCAP_NO_ENUMERATOR;
   }

   if (capEnum == 0)
   {
      // No devices found.
      devEnum->Release();
      return CVRES_VIDCAP_NO_DEVICES;
   }

   CVTrace("Enumerating Video Capture Devices:");

   // Step through capture devices
   while (S_OK == capEnum->Next(1, &capDev, NULL))
   {
       IPropertyBag* propBag;
       VARIANT       devName;

      // Get device properties
      if (FAILED(capDev->BindToStorage(   0, 
                                          0, 
                                          IID_IPropertyBag, 
                                          (void**)(&propBag))))
      {
         capDev->Release();
         continue;
      } 

      // Get the device name from the property bag
      VariantInit(&devName);           
      if (FAILED(hres = propBag->Read(L"Description", &devName, 0)))
      {
         hres = propBag->Read(L"FriendlyName", &devName, 0);
      }

      if (SUCCEEDED(hres))
      {
         // Convert string to ASCII
         int strLength = WideCharToMultiByte(   CP_ACP, 
                                                0, 
                                                devName.bstrVal, 
                                                -1, 
                                                0, 
                                                0, 
                                                0, 
                                                0);
                  
         char* deviceString = new char[strLength+1];
         
         if ( deviceString != 0)
         {
            WideCharToMultiByte( CP_ACP, 
                                 0, 
                                 devName.bstrVal, 
                                 -1, 
                                 deviceString, 
                                 strLength+1, 
                                 0, 
                                 0);         
   
            // create a new device struct and add it to the list.
            VIDCAP_DEVICE* curDevice   = new VIDCAP_DEVICE;
            curDevice->NextDevice      = fDeviceList;
            curDevice->DeviceString    = deviceString;
            curDevice->DeviceExtra     = (void*)capDev;   // This *must* be released later!
            fDeviceList = curDevice;
            fNumDevices++;

            CVTrace(curDevice->DeviceString);
         }

         VariantClear(&devName); 
      }
      
      // Clean up
      propBag->Release();      
   }

   capEnum->Release();
   devEnum->Release();

   return result;
}


//---------------------------------------------------------------------------
// UninitObjects
//    Assume we've been initialized, and clean up objects
//    in order (protected)
//---------------------------------------------------------------------------
void CVVidCaptureDSWin32::UninitObjects()
{  
   if (fSampleGrabber)
   {
      fSampleGrabber->Release();
      fSampleGrabber = 0;
   }

   if (fSampleGrabberFilter)
   {
      fSampleGrabberFilter->Release();
      fSampleGrabberFilter = 0;
   }

   if (fCaptureControl)
   {
      fCaptureControl->Release();
      fCaptureControl = 0;
   }

   if (fCaptureEvent)
   {
      fCaptureEvent->Release();
      fCaptureEvent = 0;
   }

   if (fGraph)
   {
      fGraph->Release();
      fGraph = 0;
   }   

   if (fAbortEvent)
   {
      CloseHandle(fAbortEvent);
      fAbortEvent = 0;
   }

   // Delete our status mutex
   if (fStatusLock)
   {
      CloseHandle(fStatusLock);
      fStatusLock = 0;
   }   

   // Delete our stop mutex
   if (fStopLock)
   {
      CloseHandle(fStopLock);
      fStopLock = 0;
   }

   CoUninitialize();
}
//---------------------------------------------------------------------------
// Uninit
//    Uninitializes our device and COM
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::Uninit()
{
   CVRES result = CVRES_SUCCESS;

   if (this->fConnected)
   {
      CVTrace("Uninitializing a connected Video Capture device!");

      if (CVFAILED(result = this->Disconnect()))
      {
         return result;
      }
   }

   // Bail with successful status if not initialized.
   if (fInitialized == false)
   {
      return CVRES_VIDCAP_NOT_INITIALIZED;
   }

   UninitObjects();


   fInitialized = false;

   return result;
}

//---------------------------------------------------------------------------
// ClearDeviceList
//    Clears the list of available devices, if any.
//---------------------------------------------------------------------------
void CVVidCaptureDSWin32::ClearDeviceList()
{
   if (fDeviceList != 0)
   {
      VIDCAP_DEVICE* curDevice  = fDeviceList;
      VIDCAP_DEVICE* prevDevice = 0;
   
      while (curDevice != 0) 
      {
         prevDevice = curDevice;  
         curDevice = curDevice->NextDevice;
         IMoniker* moniker = (IMoniker*)prevDevice->DeviceExtra;
         moniker->Release();
         delete [] prevDevice->DeviceString;
         delete prevDevice;     
      }  
      fDeviceList = 0;
   }
   fNumDevices = 0;
}

//---------------------------------------------------------------------------
// Connect
//    Connects to the specified device.  You can find the
// proper device name by stepping through the devices after
// refreshing the device list with RefreshDeviceList(), GetNumDevices(),
// and GetDeviceName().
//
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::Connect( int devIndex)
{
   CVRES result = CVRES_SUCCESS;
   
   CVAssert(   this->fInitialized, 
               "You must initialize the CVVidCapture object!");

   if (false == this->fInitialized)
   {     
      CVTrace("CVVidCaptureDSWin32 object not initialized.");
      return CVRES_VIDCAP_MUST_INITIALIZE_ERR;
   }

   // If we're already connected, disconnect first.
   if (fConnected)
   {
      if (CVFAILED(result = this->Disconnect()))
      {
         // Bail if we got an error disconnecting.         
         return result;
      }
   }

   if (fDeviceList == 0)
   {
      CVTrace("No video capture devices found... The list is empty.");
      return CVRES_VIDCAP_NO_DEVICES;
   }
      
   if ((devIndex < 0) || (devIndex >= this->fNumDevices))
   {
      CVTrace("Invalid capture device index.");
      return CVRES_VIDCAP_INVALID_DEVICE_INDEX;
   }

   // Find the appropriate device
   int curDevNum = 0;
   VIDCAP_DEVICE* curDev = fDeviceList;
   while (devIndex != curDevNum)
   {
      curDev = curDev->NextDevice;
      curDevNum++;
      if (curDev == 0)
      {
         CVTrace("Invalid capture device index.");
         return CVRES_VIDCAP_INVALID_DEVICE_INDEX;
      }
   }
   
   // Now have the device in curDev... connect to it.
   CVTrace("Connecting to device:");
   CVTrace(curDev->DeviceString);

   // Store device name
   size_t cbDeviceName = 1 + strlen(curDev->DeviceString);
   fDeviceName = new char[cbDeviceName];
   strcpy_s(fDeviceName, cbDeviceName, curDev->DeviceString);

   // Pull moniker from device
   IMoniker* moniker = (IMoniker*)curDev->DeviceExtra;

   // Connect to the camera...
   // Create our capture filter  
   HRESULT hres = 0;
   if (FAILED(moniker->BindToObject(   0, 
                                       0, 
                                       IID_IBaseFilter, 
                                       (void**)&this->fCaptureFilter)))
   {
      delete [] fDeviceName;
      fDeviceName = 0;
      CVTrace("BindToObject() failed for capture filter.");
      return CVRES_VIDCAP_CAPTURE_BIND_FAILED;
   }
   
   // Add it to the graph
   hres = this->fGraph->AddFilter(   this->fCaptureFilter, 
                                       L"Capture Filter");
   if (FAILED(hres))
   {
      this->fCaptureFilter->Release();
      this->fCaptureFilter = 0;
      delete [] fDeviceName;
      fDeviceName = 0;
      CVTrace("Failed to add capture filter to graph...");
      return CVRES_VIDCAP_CAPTURE_ADD_FAILED;
   }

   CVRES tmpResult = CVRES_SUCCESS;
   // Create a null renderer
   //   
   //   
   if (FAILED(CoCreateInstance(  CLSID_NullRenderer, 
                                 NULL, 
                                 CLSCTX_INPROC_SERVER, 
                                 IID_IBaseFilter, 
                                 (void**)&fRenderer)))
   {
      CVTrace("Couldn't create the Null Renderer.");
      tmpResult = CVRES_VIDCAP_NO_NULL_RENDERER;
   }
   // Add renderer into the graph
   else if (FAILED(hres = fGraph->AddFilter(fRenderer, L"Null Renderer")))
   {
      CVTrace("Couldn't add null renderer");
      tmpResult = CVRES_VIDCAP_ADD_RENDER_ERR;
   }     

   // If we failed to create a video renderer, then bail.
   if (CVFAILED(tmpResult))
   {
      CVTrace("Failed to create renderer. Disconnecting.");
      this->fGraph->RemoveFilter(fCaptureFilter);
      this->fCaptureFilter->Release();
      this->fCaptureFilter = 0;
      delete [] fDeviceName;
      fDeviceName = 0;
      return CVRES_VIDCAP_CAPTURE_ADD_FAILED;
   }      
   // Attempt to get unconnected pin to connect to from capture device
   if (FAILED(hres = GetUnconnectedPin(   this->fCaptureFilter, 
                                          PINDIR_OUTPUT, 
                                          &this->fCapturePin)))
   {
      CVTrace("Could not find an available capture pin.");
      this->fGraph->RemoveFilter(this->fCaptureFilter);
      this->fCaptureFilter->Release();
      this->fCaptureFilter = 0;
      delete [] fDeviceName;
      fDeviceName = 0;
      return CVRES_VIDCAP_CAPTURE_NO_AVAILABLE_PIN;
   }

   // Old connect.... MAEDEBUG
     
   // Set the graph clock to null so we get as fast a response 
   // as the capture can produce 
   this->fGraph->QueryInterface(  IID_IMediaFilter, 
                                    (void**)&this->fCapMediaFilter);
   if (this->fCapMediaFilter)
   {
      // Right now, fClock is always null. 
      // If we want a specific clock frequency though,
      // create an IReferenceClock COM object as fClock and pass it here.
      this->fCapMediaFilter->SetSyncSource(this->fClock);
   }

   // We're all connected and set up now!
   this->fConnected = true;

   // Reset property info
   memset(&this->fProcAmpProps,0,sizeof(this->fProcAmpProps));

   // Check to see if we can control the VideoProcAmp interface for the camera
   this->fCaptureFilter->QueryInterface(   IID_IAMVideoProcAmp, 
                                             (void**)&this->fVideoProcAmp);
   if (this->fVideoProcAmp != 0)
   {
      // Got video proc amp interface, so we can control 
      // brightness, contrast, etc.

      // Get property info
      int curProp;
      for (curProp = 0; curProp < CAMERAPROP_NUMPROPS; curProp++)
      {
         this->fProcAmpProps[curProp].Property = curProp;
         if (SUCCEEDED(this->fVideoProcAmp->GetRange(
                           curProp,
                           &this->fProcAmpProps[curProp].Min,
                           &this->fProcAmpProps[curProp].Max,
                           &this->fProcAmpProps[curProp].SteppingDelta,
                           &this->fProcAmpProps[curProp].Default,
                           &this->fProcAmpProps[curProp].CapsFlags)))
         {
            this->fProcAmpProps[curProp].Supported = true;
         }        
      }
   }

   // Figure out what format modes we have available.
   this->fCapturePin->QueryInterface(   IID_IAMStreamConfig, 
                                          (void**)&this->fStreamConfig);

   if (this->fStreamConfig != 0)
   {
      // Got stream configuration interface. We can config the format.
      int numCaps    = 0;
      int sizeCaps   = 0;

      // If we get an error retrieving the number of caps, 
      // just release and ignore  the fact that we got a 
      // stream config object in the first place.     
      if (FAILED(this->fStreamConfig->GetNumberOfCapabilities(
                                                &numCaps, &sizeCaps)))
      {
         CVTrace("Could not retrieve stream capabilities..");
         this->fStreamConfig->Release();
         this->fStreamConfig = 0;
         return CVRES_VIDCAP_CAPABILITY_CHECK_FAILED;
      }

      // Now see what formats we want to support
      int curFmt;
      BYTE* videoFormatBuf = new BYTE[sizeCaps];

      // Bail if we can't allocate space for the capabilities info
      if (videoFormatBuf == 0)
      {
         this->fStreamConfig->Release();
         this->fStreamConfig = 0;
         CVTrace("Could not retrieve stream capabilities..");
         return CVRES_VIDCAP_CAPABILITY_CHECK_FAILED;
      }

      // Cycle through the supported modes.
      // Only add 24-bit RGB modes to our list.
      for (curFmt = 0; curFmt < numCaps; curFmt++)
      {        
         AM_MEDIA_TYPE* mt = 0;
         
         // Get the video format information
         if (FAILED(this->fStreamConfig->GetStreamCaps(   curFmt, 
                                                            &mt, 
                                                            videoFormatBuf)))
            continue;

         // Get a pointer to video config capabilities
         VIDEO_STREAM_CONFIG_CAPS *vCaps = 
                   (VIDEO_STREAM_CONFIG_CAPS*)videoFormatBuf;
         
         //
		 // We now filter on VideoInfo / Anything else further down
		 // Dave Partridge 16 October 2016
		 //
         // if (vCaps->guid != FORMAT_VideoInfo)
         // {
         //   LocalDeleteMediaType(mt);
         //   continue;
         // }

         VIDCAP_MODE newMode;
         newMode.XRes         = vCaps->InputSize.cx;
         newMode.YRes         = vCaps->InputSize.cy;
         newMode.EstFrameRate = 0;
         newMode.InputFormat  = GetVidCapFormat(&mt->subtype);

         // Calculate the expected frames per second.                  
         if (mt->formattype == FORMAT_VideoInfo) 
         {
            VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)mt->pbFormat;
            if (pvi->AvgTimePerFrame != 0) {
                newMode.EstFrameRate = (int)(10000000 / pvi->AvgTimePerFrame);
            }
         }
		 else 
		 {
			 //
			 // We only support the use of VideoInfoHeader
			 // any others e.g.: VideoInfoHeader2 are discarded
			 // Dave Partridge 16 October 2016
			 //
			 LocalDeleteMediaType(mt);
			 continue;
		 }
		  
         newMode.InternalRef  = mt;

         if (CVFAILED(this->AddMode(newMode)))
         {
            // Delete media type on failure, otherwise
            // we'll delete it on removal from list         
            LocalDeleteMediaType(mt);
         }
      }

      delete [] videoFormatBuf;
   }

   if (fConnected)
   {
      CVTrace("Connected successfully to capture device..");
      return CVRES_SUCCESS;
   }
   
   CVTrace("An error occurred connecting to the video capture device.");
   return CVRES_VIDCAP_CONNECT_ERR;
}



//---------------------------------------------------------------------------
// Disconnect
//    Disconnects from a previously connected video capture device.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::Disconnect()
{
   CVRES result = CVRES_SUCCESS;
   
   CVTrace("Disconnecting...");
   
   CVAssert(this->fInitialized, "You must initialize CVVidCapture first!");      
   if (false == this->fInitialized)
   {           
      return CVRES_VIDCAP_MUST_INITIALIZE_ERR;
   }

   if (this->fStarted)
   {
      CVTrace("Video capture was started when disconnected!");
      if (CVFAILED(result = this->Stop()))
      {
         return result;
      }
   }
   
  
   // Assert on it in debug, but pass successful in release, since it does 
   // actually end up disconnected....
   CVAssert(   this->fConnected,
               "Disconnecting from CVVidCapture without being connected.");
   if (fConnected == false)
   {           
      return CVRES_VIDCAP_NOT_CONNECTED;
   }

   if (fVideoProcAmp != 0)
   {
      fVideoProcAmp->Release();
      fVideoProcAmp = 0;
   }

   if (fStreamConfig != 0)
   {
      fStreamConfig->Release();
      fStreamConfig = 0;
   }

   if (fCaptureFilter)
   {
      // Remove the filter before releasing it
      if (fCapturePin)
      {
         fCapturePin->Release();
         fCapturePin    = 0;
      }

      fGraph->RemoveFilter(fCaptureFilter);
      fCaptureFilter->Release();
      fCaptureFilter = 0;
   }

   if (fDeviceName)
   {
      delete [] fDeviceName;
      fDeviceName = 0;
   }  
   
   if (fCapMediaFilter)
   {
      fCapMediaFilter->Release();
      fCapMediaFilter = 0;
   }


   if (fRenderer)
   {
      fGraph->RemoveFilter(fRenderer);
      fRenderer->Release();
      fRenderer = 0;
   }
   // Free video header buffer from media type
   LocalFreeMediaType(this->fMediaType);

   // Free mode list
   ClearModes();
   
   // Just reset current mode, since it will get freed elsewhere
   memset(&fCurMode,0,sizeof(fCurMode));

   fLastState = VIDCAP_UNCONNECTED;

   fVideoHeader = 0;
   fConnected = false;
   
   CVTrace("Capture device Disconnected.");
   return result;
}

//---------------------------------------------------------------------------
// StartImageCap()
//    Start a continuous capture. Calls the callback when captures
//    occur.
//
//    Callback will be on seperate thread.
//
//    This version uses image classes - it's slower than the raw capture,
//    but more convenient.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::StartImageCap( CVImage::CVIMAGE_TYPE   imgType,
                                          CVVIDCAP_CALLBACK       callback, 
                                          void*                   userParam)
{
   CVRES result = CVRES_SUCCESS;

   // Sanity checks
   CVAssert(this->fInitialized && this->fConnected, 
            "You must call Initialize and Connect before calling Start!");
   
   if (!this->fInitialized)
   {     
      return CVRES_VIDCAP_MUST_INITIALIZE_ERR;
   }

   if (!this->fConnected)
   {     
      return CVRES_VIDCAP_MUST_CONNECT_ERR;
   }

   // Stop if already started
   if (this->fStarted)
   {
      CVTrace("Starting an already started CVVidCapture...");
      if (CVFAILED(result = this->Stop()))
      {
         return result;
      }
   }

   // Try to connect up the graph...
   if (CVFAILED(result = this->ConnectGraph()))
   {
      this->DisconnectGraph();
      return result;
   }

   
   // Start the abort thread to watch for aborts
   if (CVFAILED(result = this->StartAbortThread()))
   {
      return result;
   }

   // Store callback / user param
   this->fCaptureCallback  = callback;
   this->fCaptureUserParam = userParam;
   this->fRawCallback      = 0;

   // If default image type is specified, use last type.
   // Initializes as RGB24
   // Otherwise, save image type and use it.
   if (imgType != CVImage::CVIMAGE_DEFAULT)
   {
      fImageType = imgType;
   }

   // Now start it.  
   if (this->fLastState != VIDCAP_CONTINUOUS_MODE)
   {
      // Set it in continuous mode and kick it off
      this->fSampleGrabber->SetOneShot(FALSE);

      // We don't use buffering for continuous mode      
      this->fSampleGrabber->SetBufferSamples(FALSE);

      // Set the callback to call our SampleCB function
      this->fSampleGrabber->SetCallback(this,0);

      // Save last state
      this->fLastState = VIDCAP_CONTINUOUS_MODE;
   }
   
   if (FAILED(this->fCaptureControl->Run()))
   {
      // Halt the abort thread - we're not running.
      HaltAbortThread();      

      return CVRES_VIDCAP_START_ERR;
   }

   // It started!   
   fStarted = true;
   return result;
}

//---------------------------------------------------------------------------
// StartRawCap()
//    Start a continuous capture. Calls the callback when captures
//    occur.
//
//    Callback will be on seperate thread.
//
//    This version uses image classes - it's slower than the raw capture,
//    but more convenient.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::StartRawCap(CVVIDCAP_RAWCB callback, 
                                       void*          userParam)
{
   CVRES result = CVRES_SUCCESS;

   // Sanity checks
   CVAssert(this->fInitialized && this->fConnected, 
            "You must call Initialize and Connect before calling Start!");
   
   if (!this->fInitialized)
   {     
      return CVRES_VIDCAP_MUST_INITIALIZE_ERR;
   }

   if (!this->fConnected)
   {     
      return CVRES_VIDCAP_MUST_CONNECT_ERR;
   }

   // Stop if already started
   if (this->fStarted)
   {
      CVTrace("Starting an already started CVVidCapture...");
      if (CVFAILED(result = this->Stop()))
      {
         return result;
      }
   }

   // Try to connect up the graph...
   if (CVFAILED(result = this->ConnectGraph()))
   {
      this->DisconnectGraph();
      return result;
   }

   // Start the abort thread to watch for aborts
   if (CVFAILED(result = this->StartAbortThread()))
   {
      return result;
   }
   
   // Store callback / user param
   this->fCaptureCallback  = 0;
   this->fRawCallback      = callback;
   this->fCaptureUserParam = userParam;

   // Now start it.  
   if (this->fLastState != VIDCAP_CONTINUOUS_MODE)
   {
      // Set it in continuous mode and kick it off
      this->fSampleGrabber->SetOneShot(FALSE);

      // We don't use buffering for continuous mode      
      this->fSampleGrabber->SetBufferSamples(FALSE);

      // Set the callback to call our SampleCB function
      this->fSampleGrabber->SetCallback(this,0);

      // Save last state
      this->fLastState = VIDCAP_CONTINUOUS_MODE;
   }
   
   if (FAILED(this->fCaptureControl->Run()))
   {
      // Halt abort thread - we're not running
      HaltAbortThread();

      return CVRES_VIDCAP_START_ERR;
   }

   fStarted = true;
   return result;
}
//---------------------------------------------------------------------------
// Stop()
//    Stop a continuous capture.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::Stop()
{
   CVTrace("Stopping video capture...");

   CVRES result = CVRES_SUCCESS;

   // Sanity checks
   CVAssert(this->fInitialized && this->fConnected, 
            "You must call Initialize and Connect before calling Stop!");
   
   if (!this->fInitialized)
   {     
      return CVRES_VIDCAP_MUST_INITIALIZE_ERR;
   }

   if (!this->fConnected)
   {     
      return CVRES_VIDCAP_MUST_CONNECT_ERR;
   }

   // End the capture abort thread if its running
   this->HaltAbortThread();

   // Are we already stopped?
   if (false == this->fStarted)
   {     
      CVTrace("Video capture already stopped.");
      return CVRES_VIDCAP_ALREADY_STOPPED;
   }


   // Stop the capture graph. Here, we don't care about
   // the return code - the thread's already stopped anyway.
   (void)this->StopCaptureGraph();

   this->DisconnectGraph();
   
   fStarted = false;
   
   CVTrace("Video capture stopped.");
   return result;
}
//---------------------------------------------------------------------------
// Grab()
//    Grab a single frame.
//    Caller should pass an uninstantiated image ptr.
//    Image must be deleted by caller when done by 
//    calling CVImage::ReleaseImage().
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::Grab( CVImage::CVIMAGE_TYPE   imageType,
                                 CVImage*&               imagePtr)
{
   // Set to 0 in case of error.
   imagePtr = 0;

   CVRES result = CVRES_SUCCESS;

   // Sanity checks
   CVAssert(this->fInitialized && this->fConnected, 
            "You must call Initialize and Connect before calling Stop!");
   
   if (!this->fInitialized)
   {     
      return CVRES_VIDCAP_MUST_INITIALIZE_ERR;
   }

   if (!this->fConnected)
   {     
      return CVRES_VIDCAP_MUST_CONNECT_ERR;
   }

   // Technically, we could stop it and do a grab. 
   // But - probably better to enforce here so people don't 
   // use Start() thinking they need to before calling Grab() 
   // and then leaving it...
   CVAssert(false == this->fStarted, "CVVidCapture is currently streaming.");
   if (this->fStarted)
   {
      return CVRES_VIDCAP_STOP_BEFORE_GRABS_ERR;
   }

   if (CVFAILED(result = this->ConnectGraph()))
      return result;

   if (this->fLastState != VIDCAP_SINGLE_SHOT_MODE)
   {
      // Set it in single-shot buffered mode and kick it off
      this->fSampleGrabber->SetOneShot(TRUE);
      
      // Use buffering here, since they're waiting on the results anyway. 
      // That way we don't need convoluted callback constructs.
      this->fSampleGrabber->SetBufferSamples(TRUE);

      // Make sure callback is cleared
      this->fSampleGrabber->SetCallback(0,0);

      // Save last state
      this->fLastState = VIDCAP_SINGLE_SHOT_MODE;
   }
   
   // Start it for the single shot
   if (FAILED(this->fCaptureControl->Run()))
   {
      DisconnectGraph();
      return CVRES_VIDCAP_START_ERR;
   }

   // Wait for the run to complete a grab
   long evCode = 0;
   if (S_OK != fCaptureEvent->WaitForCompletion( kCVVidCapture_Timeout, 
                                                &evCode ))
   {
      // Stop the graph
      this->fCaptureControl->Stop();
      DisconnectGraph();
      return CVRES_VIDCAP_TIMEOUT;
   }

   // Get the image out....
   // ...
   long bufLen = 0;
   fSampleGrabber->GetCurrentBuffer(&bufLen, NULL);
   unsigned char* buffer = new unsigned char[bufLen];
   if (buffer == 0)
   {     
      this->fCaptureControl->Stop();
      DisconnectGraph();
      return CVRES_OUT_OF_MEMORY;
   }

   if (FAILED(fSampleGrabber->GetCurrentBuffer(&bufLen, (long*)buffer)))
   {
      this->fCaptureControl->Stop();
      DisconnectGraph();
      delete [] buffer;
      return CVRES_VIDCAP_GET_BUFFER_ERR;
   }

   // Stop the graph
   this->fCaptureControl->Stop();
   
   // Disconnect filters in the graph...
   DisconnectGraph();

   // If default image type is specified, use last type.
   // Initializes as RGB24
   // Otherwise, save image type and use it.
   if (imageType != CVImage::CVIMAGE_DEFAULT)
   {
      fImageType = imageType;
   }
      
   // Convert image data to image object
   if (CVFAILED( result = CVImage::CreateFromWin32Bmp(
                                    fImageType, 
                                    imagePtr, 
                                    &fVideoHeader->bmiHeader, 
                                    buffer)))
   {
      delete [] buffer;    
      return result;
   }
   
   // Free raw data pointer
   delete [] buffer;
   
   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// QueryInterface
//    Implement QueryInterface so we can be treated as a COM object.
//---------------------------------------------------------------------------
HRESULT WINAPI CVVidCaptureDSWin32::QueryInterface(   REFIID iid, 
                                                      void** ppvObject )
{
   // Return requested interface
   if (IID_IUnknown == iid)
   {
      *ppvObject = dynamic_cast<IUnknown*>( this );
   }
   else if (IID_ISampleGrabberCB == iid)
   {
      // Sample grabber callback object
      *ppvObject = dynamic_cast<ISampleGrabberCB*>( this );
   }
   else
   {     
      // No interface for requested iid - return error.
      *ppvObject = NULL;
      return E_NOINTERFACE;
   }

   // inc reference count
   this->AddRef();
   
   return S_OK;
}
//---------------------------------------------------------------------------
// AddRef()
//    COM reference increment
//---------------------------------------------------------------------------
ULONG WINAPI CVVidCaptureDSWin32::AddRef()
{
   return(fRefCount++);
}

//---------------------------------------------------------------------------
// Release()
//    COM reference decrement
//---------------------------------------------------------------------------
ULONG WINAPI CVVidCaptureDSWin32::Release()
{
   CVAssert(fRefCount > 0,"COM reference count invalid!");
   
   if (fRefCount > 0)
      fRefCount--;

   return fRefCount;
}
//---------------------------------------------------------------------------
// SampleCB
//    Sample callback - called for each frame of video
//---------------------------------------------------------------------------
HRESULT WINAPI CVVidCaptureDSWin32::SampleCB (  double         sampleTimeSec,
                                                IMediaSample*  mediaSample)
{  
   // Pull in the data from the media sample
   BYTE* rawData = 0;
 
   // if keepGoing gets set to false, then we'll set an event for the
   // capture thread to halt callbacks.
   bool  keepGoing = true;
   
   // Get the callback status
   CVRES cbStatus = this->GetCallbackStatus();

   // If so far our callback status is happy, pull the data out of
   // our media sample.
   if (CVSUCCESS(cbStatus))
   {
      if (FAILED(mediaSample->GetPointer(&rawData)))
      {
         // If we can't get a raw pointer, then set an error.
         this->SetCallbackStatus(CVRES_VIDCAP_CAPTURE_ERROR);
         keepGoing = false;
      }
   }
   else
   {
      // Perform an abort if we've got an error for the status.
      // However, we still do want to call the callback with the error code!
      keepGoing = false;
   }

   if (fCaptureCallback != 0)
   {  
      if (CVSUCCESS(cbStatus))
      {
         CVImage* imagePtr = 0;         

         if (CVFAILED(cbStatus = CVImage::CreateFromWin32Bmp(
                                 fImageType,
                                 imagePtr,
                                 &fVideoHeader->bmiHeader,
                                 rawData)))
         {
            // If we can't get a raw pointer, then set an error.
            this->SetCallbackStatus(cbStatus);
            keepGoing = fCaptureCallback(cbStatus, 0,fCaptureUserParam);            
            keepGoing = false;
         }
         else
         {         
            // Call the callback
            // Remember - don't do heavy processing in the callback.                  
            keepGoing = fCaptureCallback(cbStatus, imagePtr,fCaptureUserParam);

            // Release image (it'll stick around if the user added a ref)
            CVImage::ReleaseImage(imagePtr);
         }
      }
      else
      {
         // Call the callback with an error status. We still support aborts.         
         keepGoing = fCaptureCallback(cbStatus, 0,fCaptureUserParam);
      }         
   }
   else if (fRawCallback != 0)
   {
      // Call callback with raw video header and data if available.
      // fCallbackStatus has been set to an error if rawData is invalid.
      keepGoing = fRawCallback(cbStatus, fVideoHeader, rawData, fCaptureUserParam);
   }

   // If the user returned false from the callback, then set event so the
   // abort thread can halt capturing and bail out.
   if (!keepGoing)
   {
      // Set stopping flag to indicate we're stoppping. The 
      // post a message to our monitoring thread to let it
      // know we need to be stopped...
      this->fAborted = true;
      SetEvent(this->fAbortEvent);
   }

   return S_OK;
}

//---------------------------------------------------------------------------
// BufferCB - unimplemented. Use SampleCB for callbacks
//---------------------------------------------------------------------------
HRESULT WINAPI CVVidCaptureDSWin32::BufferCB (  double   sampleTimeSec,
                                                BYTE*    bufferPtr,
                                                long     bufferLength   )
{
   /// Implement it if you want...
   CVAssert(false, "Buffer callback is not implemented. Use SampleCB");
   return S_OK;
}

//
// Re-created by David C. Partridge, October 2016 
//
// Shows the property dialog for the Video Capture Filter if available
//
void CVVidCaptureDSWin32::ShowPropertyDialog(HWND parent)
{
	ISpecifyPropertyPages *pSpec;
	CAUUID cauuid;
	HRESULT hr;

	//
	// Locate the capture filter's Property Pages
	//
	hr = this->fCaptureFilter->QueryInterface(IID_ISpecifyPropertyPages,
		(void **)&pSpec);
	if (hr == S_OK)
	{
		hr = pSpec->GetPages(&cauuid);

		hr = OleCreatePropertyFrame(parent, 30, 30, NULL, 1,
			(IUnknown **)&this->fCaptureFilter, cauuid.cElems,
			(GUID *)cauuid.pElems, 0, 0, NULL);

		CoTaskMemFree(cauuid.pElems);
		pSpec->Release();
	}
}


//---------------------------------------------------------------------
// Property settings for cameras.
// These may or may not be supported on any given camera.
// If you receive CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED, then your camera
// doesn't support the specified property.

//---------------------------------------------------------------------------
// GetPropertyInfo()
//    Retrieve information about the specified property.
//    Any NULL pointer will be ignored.
//    The others will be filled in on success.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::GetPropertyInfo(  
                                       CAMERA_PROPERTY   property,
                                       long*             curVal,
                                       long*             defVal,
                                       long*             minVal,
                                       long*             maxVal,
                                       long*             step)
{
   if ((property < 0) || (property >= CAMERAPROP_NUMPROPS))
   {
      return CVRES_INVALID_PARAMETER;
   }

   if ((fVideoProcAmp == 0) || (fProcAmpProps[property].Supported == false))
   {
      return CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED;
   }
   
   if (defVal) *defVal  =  fProcAmpProps[property].Default;
   if (minVal) *minVal  =  fProcAmpProps[property].Min;
   if (maxVal) *maxVal  =  fProcAmpProps[property].Max;
   if (step)   *step    =  fProcAmpProps[property].SteppingDelta;

   if (curVal)
   {  
      long manualFlag;
      fVideoProcAmp->Get(property, curVal, &manualFlag);
   }

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// SetProperty()
//    Sets a property if it is available and the specified value is within
//    range.  Use GetPropertyInfo to get the property min/max and step.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::SetProperty(   CAMERA_PROPERTY   property,
                                          long              value)
{
   if ((property < 0) || (property >= CAMERAPROP_NUMPROPS))
   {
      return CVRES_INVALID_PARAMETER;
   }

   if ((fVideoProcAmp == 0) || (fProcAmpProps[property].Supported == false))
   {
      return CVRES_VIDCAP_PROPERTY_NOT_SUPPORTED;
   }

   fVideoProcAmp->Set(property, value, VideoProcAmp_Flags_Manual);

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// SetMode()
//    Set mode for stream config
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::SetMode(  VIDCAP_MODE& newMode,
                                     bool         rawYUY2 )
{
   AM_MEDIA_TYPE* mt = (AM_MEDIA_TYPE*)newMode.InternalRef;

   if (SUCCEEDED(fStreamConfig->SetFormat(mt)))
   {
      // Make a copy for current mode 
      // (it shouldn't be deleted - just overwritten)
      fCurMode = newMode;

      // Store pointer to new video header
      fVideoHeader = (VIDEOINFOHEADER*)mt->pbFormat;
      
      // Setup media format for grabber to RGB 24-bit
      VIDEOINFOHEADER infoHdr;
      memcpy(&infoHdr,fVideoHeader,sizeof(VIDEOINFOHEADER));
      fMediaType.majortype    = MEDIATYPE_Video;
      fMediaType.subtype      = rawYUY2 ?
                                    MEDIASUBTYPE_YUY2 :
                                    MEDIASUBTYPE_RGB24;
      fMediaType.formattype   = FORMAT_VideoInfo;
      fMediaType.cbFormat     = sizeof(VIDEOINFOHEADER);
      fMediaType.pbFormat     = (BYTE*)&infoHdr;
      
      if (rawYUY2)
      {
         infoHdr.bmiHeader.biCompression  = MAKEFOURCC('Y','U','Y','2');
         infoHdr.bmiHeader.biBitCount     = 16;
         infoHdr.bmiHeader.biSizeImage    = infoHdr.bmiHeader.biWidth * 
                                            infoHdr.bmiHeader.biHeight * 2;
      }
      else
      {
         infoHdr.bmiHeader.biCompression  = 0;
         infoHdr.bmiHeader.biBitCount     = 24;
         infoHdr.bmiHeader.biSizeImage    = infoHdr.bmiHeader.biWidth * 
                                            infoHdr.bmiHeader.biHeight * 3;
      }
      infoHdr.bmiHeader.biPlanes       = 1;
      infoHdr.bmiHeader.biClrImportant = 0;
      infoHdr.bmiHeader.biClrUsed      = 0;
      infoHdr.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);

      HRESULT hres = S_OK;
      if (FAILED(hres = fSampleGrabber->SetMediaType(&fMediaType)))
      {     
         CVTrace("Couldn't set media type.");
         return CVRES_VIDCAP_MEDIATYPE_SET_ERR;
      }
      
      
      return CVRES_SUCCESS;
   }

   return CVRES_VIDCAP_MODE_NOT_SUPPORTED;
}
//---------------------------------------------------------------------------
// ClearModes()
//    Clear mode list
//---------------------------------------------------------------------------
void  CVVidCaptureDSWin32::ClearModes()
{
   VIDCAP_MODE* curMode  = fModeList;
   VIDCAP_MODE* prevMode = 0;
   
   while (curMode != 0) 
   {
      prevMode = curMode;  
      curMode = curMode->NextMode;
      LocalDeleteMediaType( (AM_MEDIA_TYPE *)prevMode->InternalRef);
      delete prevMode;
   }  

   fModeList = 0; 
}

//---------------------------------------------------------------------------
// StartAbortThread
//     Starts the abort thread for continuous captures
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::StartAbortThread()
{
   CVAssert( ((fAbortEvent != 0) && 
             (fAbortEvent != INVALID_HANDLE_VALUE)),
             "Abort event must be valid before starting it.");
   
   // Sanity check
   if ((fAbortEvent == 0) || (fAbortEvent == INVALID_HANDLE_VALUE))
   {
      return CVRES_VIDCAP_MUST_CONNECT_ERR;
   }

   // Reset abort event and flag
   ::ResetEvent(fAbortEvent);
   fAborted = false;

   // Create an event to let us wait for the thread to be
   // ready before we start the capture.
   fCaptureAbortThreadReady = CreateEvent(0,TRUE,FALSE,0);
   if (fCaptureAbortThreadReady == INVALID_HANDLE_VALUE)
   {
      fCaptureAbortThreadReady = 0;
      return CVRES_OUT_OF_HANDLES;
   }

   // Set the callback status to running
   fCallbackStatus = CVRES_VIDCAP_RUNNING;

   // Start the abort thread
   fCaptureAbortThread = 
      (HANDLE)_beginthreadex(0,0,CaptureAbortThreadFunc,this,0,0);
   if (fCaptureAbortThread == INVALID_HANDLE_VALUE)
   {
      CloseHandle(fCaptureAbortThreadReady);
      fCaptureAbortThreadReady = 0;
      fCaptureAbortThread = 0;
      return CVRES_OUT_OF_HANDLES;
   }
   
   // Wait for the abort thread ready event to be signalled, 
   // letting us know the capture abort thread is up, running, and ready
   WaitForSingleObject(fCaptureAbortThreadReady,INFINITE);
   
   // Delete the ready event - we no longer need it.
   CloseHandle(fCaptureAbortThreadReady);
   fCaptureAbortThreadReady = 0;      

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// HaltAbortThread
//     Halts the abort thread used for continuous captures
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::HaltAbortThread()
{
   CVAssert( ((fAbortEvent != 0) && 
             (fAbortEvent != INVALID_HANDLE_VALUE)),
             "Abort event must be valid before starting it.");
   
   // Sanity check
   if ((fAbortEvent == 0) || (fAbortEvent == INVALID_HANDLE_VALUE))
   {
      return CVRES_VIDCAP_MUST_CONNECT_ERR;
   }

   // Clean up the threading mess we've started before telling
   // the caller it puked.
   ::SetEvent(fAbortEvent);

   if (WAIT_TIMEOUT == ::WaitForSingleObject(fCaptureAbortThread,1000))
   {
      ::TerminateThread(fCaptureAbortThread, -1);
   }
   
   ::CloseHandle(fCaptureAbortThread);
   
   // Reset abort event
   fCaptureAbortThread = 0;
   fAborted = false;
   ::ResetEvent(fAbortEvent);

   return CVRES_SUCCESS;
}

//---------------------------------------------------------------------------
// CaptureAbortThreadFunc()
//     Thread to detect when a continuous capture has been aborted.
//     It'll then stop the capture.
//---------------------------------------------------------------------------
unsigned int WINAPI CVVidCaptureDSWin32::CaptureAbortThreadFunc(
                                                   void *userParam)
{
   CVVidCaptureDSWin32* vidCap = (CVVidCaptureDSWin32*)userParam;

   // Let the caller know the thread is started and ready.
   SetEvent(vidCap->fCaptureAbortThreadReady);
   
   // Wait for either the user to abort the capture or the thread to end.
   // While we're at it, watch for unhappy events we may need to know about
   bool sendAbort = false;
   CVRES cbStatus = CVRES_SUCCESS;

   DWORD waitResult;
   do
   {
      // Check DirectShow event queue for anything we might care about...
      long eventCode, lParam1, lParam2;

      // Ping for events - pull out all that are available, but don't
      // sit around waiting for them.
      while (E_ABORT != (vidCap->fCaptureEvent->GetEvent(&eventCode,
                                                         &lParam1, 
                                                         &lParam2,
                                                         0)))
      {
         // Check for errors we're concerned with
         switch (eventCode)
         {
            // Right now we're just watching for abort indicators...

            // A filter has requested that the graph be restarted.
            // We abort the current capture in this case
            case EC_NEED_RESTART: 

            // An abort can occur when when a device errors out. 
            // It can come prior to plug and play notification
            case EC_ERRORABORT:

            // Stream has stopped as a result of an error. 
            // Haven't seen sent yet, but it seems like a good time to abort.
            case EC_STREAM_ERROR_STOPPED: 
               
               // Set a generic capture error and the abort flag
               cbStatus = CVRES_VIDCAP_CAPTURE_ERROR;
               sendAbort = true;
               break;

            // Device lost occurs when the device we're capturing with 
            // gets removed. we need to abort in this case.
            case EC_DEVICE_LOST:
               // Set a flag that we need to abort the capture.
               cbStatus = CVRES_VIDCAP_CAPTURE_DEVICE_DISCONNECTED;
               sendAbort = true;   
               break;

            // Unhandled event types - these just go to default right
            // now and we ignore them. However, here they are if
            // we need to handle another one or debug a problem.
/*
            case EC_ACTIVATE:                   case EC_BUFFERING_DATA:
            case EC_BUILT:                      case EC_CLOCK_CHANGED:
            case EC_CLOCK_UNSET:                case EC_CODECAPI_EVENT:
            case EC_COMPLETE:                   case EC_DISPLAY_CHANGED:
            case EC_END_OF_SEGMENT:             case EC_ERROR_STILLPLAYING:
            case EC_EXTDEVICE_MODE_CHANGE:      case EC_FULLSCREEN_LOST:
            case EC_GRAPH_CHANGED:              case EC_LENGTH_CHANGED:
            case EC_NOTIFY_WINDOW:              case EC_OLE_EVENT:
            case EC_OPENING_FILE:               case EC_PALETTE_CHANGED:
            case EC_REPAINT:                    case EC_SEGMENT_STARTED:
            case EC_SHUTTING_DOWN:              case EC_SNDDEV_IN_ERROR:
            case EC_SNDDEV_OUT_ERROR:           case EC_STARVATION:
            case EC_STATE_CHANGE:               case EC_STEP_COMPLETE:
            case EC_STREAM_CONTROL_STARTED:     case EC_STREAM_CONTROL_STOPPED:
            case EC_STREAM_ERROR_STILLPLAYING:  case EC_TIMECODE_AVAILABLE:
            case EC_UNBUILT:                    case EC_USERABORT:
            case EC_VIDEO_SIZE_CHANGED:         case EC_VMR_RENDERDEVICE_SET:
            case EC_VMR_SURFACE_FLIPPED:        case EC_VMR_RECONNECTION_FAILED:
            case EC_WINDOW_DESTROYED:           case EC_WMT_EVENT:
            case EC_WMT_INDEX_EVENT:            case EC_QUALITY_CHANGE: 
            case EC_PAUSED:

*/
            default:
               break;
         }

         // Free any event parameters
         vidCap->fCaptureEvent->FreeEventParams(eventCode,lParam1,lParam2);
      }          
      
      // If we need to abort, go ahead and stop the capture control now.
      // Then call the callback with notification of the error. 
      if (sendAbort)
      {
         // We don't really care if it succeeds here - it may be halted or
         // otherwised hosed.  But we don't want it to keep going at any rate.
         vidCap->SetCallbackStatus(cbStatus);

         // Try to stop the capture graph. If this fails, the user
         // is in the process of doing so and we don't need to call
         // the callback to notify them of the error.  If, however,
         // it succeeds, then we still need to call the callback to
         // let the user know that the capture is hosed.
         if (CVSUCCESS(vidCap->StopCaptureGraph()))
         {
            // Check for standard callback first.
            if (vidCap->fCaptureCallback)
            {
               // We're aborting now either way, so who care's if the callback
               // agrees?
               (void)vidCap->fCaptureCallback(  cbStatus, 
                                                0,
                                                vidCap->fCaptureUserParam);
            }
            // If the standard callback wasn't there, try
            // the raw one.
            else if (vidCap->fRawCallback)
            {
               (void)vidCap->fRawCallback(   cbStatus,
                                             0,
                                             0,
                                             vidCap->fCaptureUserParam);
            }            
         }

         // Exit thread. We've killed the capture or it's being killed by
         // the user.
         return -1;
      }

      // Wait on the abort event for 1/10th of a second. This gives us
      // a bit of a delay in the worst case if an error occurs from 
      // DirectShow until we abort, but not really a very noticeable 
      // one to a human.
      waitResult = WaitForSingleObject(vidCap->fAbortEvent, 100);
             
   } while (waitResult == WAIT_TIMEOUT);
   
   if (WAIT_OBJECT_0 != waitResult)
   {
      CVAssert(false,"Error waiting on user abort event in capture thread!");
   }

   // If the user aborted from a callback, stop the capture now.
   if (vidCap->fAborted)
   {
      // Stop the capture graph directly. If this fails, it means someone
      // else is already stopping it.  If we reached here, then the user
      // returned false from a callback and set the abort event, so 
      // we don't need to be too concerned with the result code.
      (void)vidCap->StopCaptureGraph();
   }
   
   return 0;
}

//---------------------------------------------------------------------------
// SetCallbackStatus
//     Sets fCallbackStatus inside of a mutex.
//     We use a mutex here to avoid collision of threads, especially later
//     if we change CVRES to be more complex.
//---------------------------------------------------------------------------
void CVVidCaptureDSWin32::SetCallbackStatus(CVRES newStatus)
{
   // Snag the lock.
   if (WAIT_OBJECT_0 != WaitForSingleObject(fStatusLock,1000))
   {
      CVAssert(false,"Timeout retrieving status lock!");
      
      // Ugh, set it anyway.
      fCallbackStatus = newStatus;
      return;
   }
   
   // Set the status
   fCallbackStatus = newStatus;
   
   // Release it
   ReleaseMutex(fStatusLock);
}
   
//---------------------------------------------------------------------------
// GetCallbackStatus
//     Gets fCallbackStatus inside of a mutex
//     We use a mutex here to avoid collision of threads, especially later
//     if we change CVRES to be more complex.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::GetCallbackStatus()
{
   CVRES cbStatus;

   // Enter the lock - both uses of this lock should be
   // pretty fast - 1 second is actually a long time to wait
   // before asserting on it and bailing.
   if (WAIT_OBJECT_0 != WaitForSingleObject(fStatusLock,1000))
   {
      CVAssert(false,"Timeout retrieving status lock!");
      return CVRES_VIDCAP_SYNC_TIMEOUT;
   }
   
   // Snag the status
   cbStatus = fCallbackStatus;
   
   // Release the lock
   ReleaseMutex(fStatusLock);

   // return it
   return cbStatus;
}

//---------------------------------------------------------------------------
// StopCaptureGraph
//    Stops the capture graph within a mutex - 
//    do this so that we can safely call it from our abort thread.
//---------------------------------------------------------------------------
CVRES CVVidCaptureDSWin32::StopCaptureGraph()
{
   // Try to enter the lock. If we can't, then someone
   // is already attempting to stop the capture graph
   // and we don't need to (we return an error for simple checking)

   if (WAIT_OBJECT_0 == WaitForSingleObject(fStopLock,0))
   {
      // Stop the capture graph directly.
      if (FAILED(this->fCaptureControl->Stop()))
      {
         CVAssert(false,
            "IMediaControl::Stop failed in CVVidCaptureDSWin32::Stop.\n" \
            "It may already be stopped?");
      }
   
      // Release the lock and return success.
      ReleaseMutex(fStopLock);
      return CVRES_SUCCESS;
   }

   // Someone's already trying to stop the graph, just bail and
   // let the caller know.
   return CVRES_VIDCAP_CAPTURE_STOP_IN_USE;
}

// GetVidCapFormat() converts the video format type from a DirectX
// value to one of the values in the VIDCAP_FORMAT enumeration.
VIDCAP_FORMAT CVVidCaptureDSWin32::GetVidCapFormat( GUID* directXFormat)
{
   if (directXFormat == 0)
      return VIDCAP_FORMAT_UNKNOWN;

   // Skip first one, since it's 0 anyway and can't be dereferenced...
   for (int i = 1; i < VIDCAP_NUM_FORMATS; i++)
   {   
      // Some may be null?   
      if (kDSWin32VideoFormats[i].DirectShowFormat != 0)
      {
         if (*kDSWin32VideoFormats[i].DirectShowFormat == *directXFormat)
         {
            return kDSWin32VideoFormats[i].VidCapFormat;
         }
      }
   }

   return VIDCAP_FORMAT_UNKNOWN;
}

// GetDirectXFormat() converts the video format type from a
// VIDCAP_FORMAT value to the appropriate MEDIASUBTYPE_*
// value for DirectX.
const GUID* CVVidCaptureDSWin32::GetDirectXFormat( VIDCAP_FORMAT vidCapFormat)
{
   if ((vidCapFormat < 0) || (vidCapFormat > VIDCAP_NUM_FORMATS))
   {
      CVAssert(false, "Invalid format parameter passed to CVVidCAptureDSWin32::GetDirectXFormat!");
      return 0;
   }

   return kDSWin32VideoFormats[vidCapFormat].DirectShowFormat;
}


// ConnectGraph() connects up the capture graph
CVRES CVVidCaptureDSWin32::ConnectGraph()
{
   HRESULT hres;
   CVTrace("Connecting filters...");
   // If the filters are already connected, disconnect and reconnect.
   if (fFiltersConnected)
   {
      this->DisconnectGraph();
   }

   // Sanity check inputs
   CVAssert(fGraph != 0, "Graph must be valid.");
   CVAssert(fCapturePin != 0, "Capture pin must already be queried.");
   CVAssert(fSampleGrabberFilter != 0,"SampleGrabber must already be allocated.");
   CVAssert(fRenderer != 0, "Renderer must already be allocated.");
   if ((!fConnected) || (fGraph == 0) || (fCapturePin == 0) || (fSampleGrabberFilter == 0) || (fRenderer == 0))
   {
      return CVRES_VIDCAP_NOT_CONNECTED;
   }

   // Connect the capture filter to our sample grabber....
   if (FAILED(hres = ConnectFilters(   this->fGraph, 
                                       this->fCapturePin, 
                                       this->fSampleGrabberFilter)))
   {
      CVTrace("Couldn't connect capture to sample grabber.");
      return CVRES_VIDCAP_CAPTURE_GRABBER_CONNECT_FAILED;
   }
   
   // Connect the sample grabber to the null renderer....
   if (FAILED(hres = ConnectFilters(   this->fGraph, 
                                       this->fSampleGrabberFilter, 
                                       this->fRenderer)))
   {
      CVTrace("Couldn't connect null renderer to sample grabber.");
      DisconnectPins(fCaptureFilter);
      DisconnectPins(fSampleGrabberFilter);
      return CVRES_VIDCAP_GRABBER_CONNECT_FAILED;
   }

   // Get the current media format - remember to free media type's buffers when done
   this->fSampleGrabber->GetConnectedMediaType(&this->fMediaType);
   
   if ((this->fMediaType.formattype == FORMAT_VideoInfo) && 
       (this->fMediaType.cbFormat >= sizeof(VIDEOINFOHEADER)) &&
       (this->fMediaType.pbFormat != NULL) ) 
   {
      // If it really is a video type, the right size, and not null, then
      // we're all good.
       this->fVideoHeader = (VIDEOINFOHEADER*)this->fMediaType.pbFormat;
   }  
   else
   {
      CVTrace("Invalid media format!");      
      LocalFreeMediaType(this->fMediaType);
      memset(&this->fMediaType,0,sizeof(this->fMediaType));      
      this->fVideoHeader    = 0;
      DisconnectPins(fCaptureFilter);
      DisconnectPins(fSampleGrabberFilter);
      DisconnectPins(fRenderer);
      return CVRES_VIDCAP_VIDEO_FORMAT_NOT_SUPPORTED;
   }
   
   // Save current mode
   this->fCurMode.XRes = this->fVideoHeader->bmiHeader.biWidth;
   this->fCurMode.YRes = (int)abs(this->fVideoHeader->bmiHeader.biHeight);
   this->fCurMode.InternalRef = 0;

   fFiltersConnected = true;
   CVTrace("Filters connected.");
   return CVRES_SUCCESS;
}

// DisconnectGraph() disconnects the items in the capture graph
CVRES CVVidCaptureDSWin32::DisconnectGraph()
{  
   CVTrace("Disconnecting Filters...");
   if (!fConnected)
   {
      CVAssert(false,"Must call DisconnectGraph() prior to Disconnect().");
      return CVRES_VIDCAP_NOT_CONNECTED;
   }

   if (fFiltersConnected)
   { 
      if (fCaptureFilter)
         DisconnectPins(fCaptureFilter);

      if (fSampleGrabberFilter)
         DisconnectPins(fSampleGrabberFilter);
  
      if (fRenderer)
         DisconnectPins(fRenderer);

      fFiltersConnected = false;
   }

   CVTrace("Filters disconnected.");
   return CVRES_SUCCESS;
}


#endif // WIN32

