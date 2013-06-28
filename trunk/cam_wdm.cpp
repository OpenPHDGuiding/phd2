/*
 *  cam_wdm.cpp
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
#include "phd.h"
#ifdef WDM_CAMERA
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include <Dshow.h>         // DirectShow (using DirectX 9.0 for dev)
#include "cam_WDM.h"

//int gNumFrames;
bool WDM_Stack_Mode = false;

Camera_WDMClass::Camera_WDMClass() {
    Connected = FALSE;
//  HaveBPMap = FALSE;
//  NBadPixels=-1;
    Name=_T("Windows Camera");      // should get overwritten on connect
    FullSize = wxSize(640,480); // should get overwritten on connect
    DeviceNum = 0;                          // Which WDM device connected
    HasPropertyDialog = true;
    HasDelayParam = false;
    HasPortNum = false;
}


bool Camera_WDMClass::CaptureCallback( CVRES status, CVImage* imagePtr, void* userParam) {
   if (!WDM_Stack_Mode) {  return true; }// streaming, but not saving / doing anything

    Camera_WDMClass* cam = (Camera_WDMClass*)userParam;

    if (CVSUCCESS(status)) {
        cam->NAttempts++;
        int i, npixels;
        npixels = cam->FullSize.GetWidth() * cam->FullSize.GetHeight();
        unsigned short *dptr;
        dptr = cam->stackptr;
        unsigned char *imgdata;
        imgdata = imagePtr->GetRawDataPtr();
        unsigned int sum = 0;
        for (i=0; i<npixels; i++, imgdata++, dptr++) {
            *dptr = *dptr + (unsigned short) (*imgdata);
            sum = sum + (unsigned int) (*imgdata);
        }
        if (sum > 100) // non-black
            cam->NFrames++;

        return true;
    }
   return false;
}

bool Camera_WDMClass::Connect() {
// returns true on error
    int NDevices;
//  char DevName[256];

    // Setup VidCap library
    VidCap = CVPlatform::GetPlatform()->AcquireVideoCapture();

    // Init the library
    if (CVFAILED(VidCap->Init())) {
        wxMessageBox(_T("Error initializing WDM services"),_("Error"),wxOK | wxICON_ERROR);
        CVPlatform::GetPlatform()->Release(VidCap);
      return true;
   }
    // Enumerate devices
   if (CVFAILED(VidCap->GetNumDevices(NDevices))) {
        wxMessageBox(_T("Error detecting WDM devices"),_("Error"),wxOK | wxICON_ERROR);
      VidCap->Uninit();
      CVPlatform::GetPlatform()->Release(VidCap);
      return true;
   }

    DeviceNum = 0;
    // Put up choice box to let user choose which camera
    if (NDevices > 1) {
        wxArrayString Devices;
        int i;
        CVVidCapture::VIDCAP_DEVICE devInfo;
        for (i=0; i<NDevices; i++) {
            if (CVSUCCESS(VidCap->GetDeviceInfo(i,devInfo)))
                Devices.Add(wxString::Format("%d: %s",i,devInfo.DeviceString));
            else
                Devices.Add(wxString::Format("%d: Not available"),i);
        }
        DeviceNum = wxGetSingleChoiceIndex(_("Select WDM camera"),_("Camera choice"),Devices);
        if (DeviceNum == -1) return true;
 /* int curDevIndex = 0;
        for (curDevIndex = 0; curDevIndex < numDevices; curDevIndex++)
        {
         if (CVSUCCESS(vidCap->GetDeviceInfo(curDevIndex,devInfo)))
         {
            printf("Device %d: %s\n",curDevIndex,devInfo.DeviceString);
         }
        }*/
    }

    // Connect to camera
    if (CVSUCCESS(VidCap->Connect(DeviceNum))) {
        int devNameLen = 0;
        VidCap->GetDeviceName(0,devNameLen);
        devNameLen++;
        char *devName = new char[devNameLen];
        VidCap->GetDeviceName(devName,devNameLen);
        Name = wxString::Format("%s",devName);  //
//      wxMessageBox(Name + _T(" Connected"),_T("Info"),wxOK);
        delete [] devName;
    }
    else {
        wxMessageBox(wxString::Format("Error connecting to WDM device #%d",DeviceNum),_("Error"),wxOK | wxICON_ERROR);
        VidCap->Uninit();
        CVPlatform::GetPlatform()->Release(VidCap);
        return true;
    }
    // Get modes
    CVVidCapture::VIDCAP_MODE modeInfo;
    int numModes = 0;
    int curmode;
    VidCap->GetNumSupportedModes(numModes);
    wxArrayString ModeNames;
    for (curmode = 0; curmode < numModes; curmode++) {
        if (CVSUCCESS(VidCap->GetModeInfo(curmode, modeInfo))) {
            ModeNames.Add(wxString::Format("%dx%d (%s)",modeInfo.XRes,modeInfo.YRes,
                VidCap->GetFormatModeName(modeInfo.InputFormat)));
        }
    }
    // Let user choose mode
    curmode = wxGetSingleChoiceIndex(_("Select camera mode"),_("Camera mode"),ModeNames);
    if (curmode == -1) { //canceled
        VidCap->Uninit();
        CVPlatform::GetPlatform()->Release(VidCap);
        return true;
    }
    // Activate this mode
    if (CVFAILED(VidCap->SetMode(curmode))) {
        wxMessageBox(wxString::Format("Error activating video mode %d",curmode),_("Error"),wxOK | wxICON_ERROR);
        VidCap->Uninit();
        CVPlatform::GetPlatform()->Release(VidCap);
        return true;
    }

    // Setup x and y size
    if (CVFAILED(VidCap->GetCurrentMode(modeInfo))) {
        wxMessageBox(wxString::Format("Error probing video mode %d",curmode),_("Error"),wxOK | wxICON_ERROR);
        VidCap->Uninit();
        CVPlatform::GetPlatform()->Release(VidCap);
        return true;
    }
    // Start the stream
    WDM_Stack_Mode = false; // Make sure we don't start saving
    if (CVFAILED(VidCap->StartImageCap(CVImage::CVIMAGE_GREY,  CaptureCallback, this))) {
        wxMessageBox(_T("Failed to start image capture!"),_("Error"),wxOK | wxICON_ERROR);
        VidCap->Uninit();
        CVPlatform::GetPlatform()->Release(VidCap);
      return true;
    }
    FullSize=wxSize(modeInfo.XRes, modeInfo.YRes);
    pFrame->SetStatusText(wxString::Format("%d x %d mode activated",modeInfo.XRes, modeInfo.YRes),1);

    Connected = true;
    return false;
}

bool Camera_WDMClass::Disconnect() {
   CVRES result = 0;

    VidCap->Stop();     // Stop it

    result = VidCap->Disconnect();

   result = VidCap->Uninit();
    CVPlatform::GetPlatform()->Release(VidCap);
//  delete VidCap;
    Connected = false;

    return false;
}


bool Camera_WDMClass::Capture(int duration, usImage& img, wxRect subframe, bool recon) {
    int xsize,ysize, i;
    NFrames = NAttempts = 0;
    xsize = FullSize.GetWidth();
    ysize = FullSize.GetHeight();
    unsigned short *dptr;

    //gNumFrames = 0;
    if (img.NPixels != (xsize*ysize)) {
        if (img.Init(xsize,ysize)) {
            wxMessageBox(_T("Memory allocation error during capture"),_("Error"),wxOK | wxICON_ERROR);
            Disconnect();
            return true;
        }
    }
    dptr = img.ImageData;
    for (i=0; i<img.NPixels; i++, dptr++)
        *dptr = (unsigned short) 0;

//  if (CVFAILED(VidCap->StartImageCap(CVImage::CVIMAGE_GREY,  CaptureCallback, this))) {
//  if (CVFAILED(VidCap->StartImageCap(CVImage::CVIMAGE_GREY,  CapStackCallback, 0))) {
//  if (CVFAILED(VidCap->StartRawCap(RawCallback, 0))) {
//      wxMessageBox(_T("Failed to start image capture!"),_("Error"),wxOK | wxICON_ERROR);
  //    Disconnect();
  //    return true;
//  }
    stackptr = img.ImageData;
    WDM_Stack_Mode = true;
    Sleep(duration);  // let capture go on
    while ((NFrames < 1) && (NAttempts < 3))
        Sleep(50);
     WDM_Stack_Mode = false;
  //  VidCap->Stop();       // Stop it
    pFrame->SetStatusText(wxString::Format("%d frames",NFrames),1);
    if (recon) SubtractDark(img);
    return false;
}


void Camera_WDMClass::ShowPropertyDialog() {
    VidCap->ShowPropertyDialog((HWND) pFrame->GetHandle());
}
#endif // WDM_CAMERA defined
