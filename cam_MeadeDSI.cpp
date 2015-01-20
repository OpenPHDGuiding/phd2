/*
 *  cam_MeadeDSI.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
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

//#pragma unmanaged


#include "phd.h"

#ifdef MEADE_DSI

#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "cam_MeadeDSI.h"

#include "DsiDevice.h"

Camera_DSIClass::Camera_DSIClass()
{
    Name = _T("Meade DSI");
    FullSize = wxSize(768,505); // CURRENTLY ULTRA-RAW
    HasGainControl = true;
}

bool Camera_DSIClass::Connect()
{
    bool retval = false;
//  MeadeCam = gcnew DSI_Class;
//  retval = MeadeCam->DSI_Connect();
    //if (!retval)

    MeadeCam = new DsiDevice();
    unsigned int NDevices = MeadeCam->EnumDsiDevices();
    unsigned int DevNum = 1;
    if (!NDevices) {
        wxMessageBox(_T("No DSIs found"), _("Error"));
        return true;
    }
    else if (NDevices > 1) {
        // Put up a dialog to choose which one
        wxArrayString CamNames;
        unsigned int i;
        DsiDevice *TmpCam;
        for (i=1; i<= NDevices; i++) {
            TmpCam = new DsiDevice;
            if (TmpCam->Open(i))
                CamNames.Add(wxString::Format("%u: %s",i,TmpCam->ModelName));
            else
                CamNames.Add(_T("Unavailable"));
            TmpCam->Close();
            delete TmpCam;
        }
        int choice = wxGetSingleChoiceIndex(wxString::Format("If using Envisage, disable live\npreview for this camera"),_("Which DSI camera?"),CamNames);
        if (choice == -1) return true;
        else DevNum = (unsigned int) choice + 1;

    }
    retval = !(MeadeCam->Open(DevNum));
//  wxMessageBox(wxString::Format("Color: %d\n%u x %u",
//      MeadeCam->IsColor,MeadeCam->GetWidth(),MeadeCam->GetHeight()));
    if (!retval) {
        FullSize = wxSize(MeadeCam->GetWidth(),MeadeCam->GetHeight());
//      wxMessageBox(wxString::Format("%s\n%s (%d)\nColor: %d\n-II: %d\n%u x %u",MeadeCam->CcdName,MeadeCam->ModelName, MeadeCam->ModelNumber,
//          MeadeCam->IsColor,MeadeCam->IsDsiII, FullSize.GetWidth(), FullSize.GetHeight()) + "\n" + MeadeCam->ErrorMessage);
//      wxMessageBox(wxString::Format("%s\n%s (%d)\nColor: %d\n-USB2: %d\n%u x %u",MeadeCam->CcdName,MeadeCam->ModelName, MeadeCam->ModelNumber,
//                                    MeadeCam->IsColor,MeadeCam->IsUSB2, FullSize.GetWidth(), FullSize.GetHeight()) + "\n" + MeadeCam->ErrorMessage);
        MeadeCam->Initialize();
        MeadeCam->SetHighGain(true);
        if (!MeadeCam->IsDsiIII) MeadeCam->SetDualExposureThreshold(501);
        else MeadeCam->SetBinMode(1);

        MeadeCam->SetOffset(255);
        MeadeCam->SetFastReadoutSpeed(true);
        Connected = true;
        // Set the PixelSize property for cients.  If the pixels aren't square, use the smaller dimension because the image
        // is "squared up" by scaling to the smaller dimension
        if (MeadeCam->IsDsiIII)
            PixelSize = 6.6;
        else
        if (MeadeCam->IsDsiII)
            PixelSize = 8.3;
        else
            PixelSize = 7.5;

    }

    return retval;
}
bool Camera_DSIClass::Disconnect()
{
    MeadeCam->Close();
    Connected = false;
    delete MeadeCam;

    return false;
}

bool Camera_DSIClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    MeadeCam->SetGain((unsigned int) (GuideCameraGain * 63 / 100));
    MeadeCam->SetExposureTime(duration);

    if (img.Init(MeadeCam->GetWidth(), MeadeCam->GetHeight()))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    bool retval = MeadeCam->GetImage(img.ImageData, true);
    if (!retval)
        return true;

// The AbortImage method does not appear to work with the DSI camera.  If abort is called and the thread is terminated, the
// pending image is still downloaded and PHD2 will crash
#if AbortActuallyWorks
    CameraWatchdog watchdog(duration, GetTimeoutMs());

    // wait for image to finish and d/l
    while (!MeadeCam->ImageReady)
    {
        wxMilliSleep(20);
        if (WorkerThread::InterruptRequested())
        {
            MeadeCam->AbortImage();
            return true;
        }
        if (watchdog.Expired())
        {
            MeadeCam->AbortImage();
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }
#else // handle the pending image download, regardless

    // We also need to prevent the thread from being killed when phd2 is closed
    WorkerThreadKillGuard _guard;

    if (duration > 100) {
        wxMilliSleep(duration - 100); // wait until near end of exposure, nicely
    }
    bool still_going = true;
    while (still_going) {  // wait for image to finish and d/l
        wxMilliSleep(20);
        still_going = !(MeadeCam->ImageReady);
    }

#endif // end of waiting for the image

    if (recon) SubtractDark(img);

    if (recon)
    {
        if (MeadeCam->IsColor)
            QuickLRecon(img);
        if (MeadeCam->IsDsiII)
            SquarePixels(img, 8.6, 8.3);
        else if (!MeadeCam->IsDsiIII)           // Original DSI
            SquarePixels(img, 9.6, 7.5);
    }

    return false;
}

bool Camera_DSIClass::HasNonGuiCapture(void)
{
    return true;
}

#endif // MEADE_DSI
