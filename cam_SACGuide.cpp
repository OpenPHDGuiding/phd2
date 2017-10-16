 /*
 * cam_SACGuide.cpp
 *
 *  Created by Craig Stark .
 *  Copyright (c) 2006-2009 Craig Stark.
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

#if defined (SAC_FCLAB_GUIDE)
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "cam_SACGuide.h"

// FC Labs version -- draws from SAC4-2 for all
CameraSACGuider::CameraSACGuider()
{
    Connected = false;
    Name = _T("SAC Guider");
    FullSize = wxSize(1280,1024);
    hDriver = NULL;
    ColorArray = true;  // Not color, but seem to still be uneven -- this fixes it with the quickL
    CapInfo.Gain[0]=(unsigned char) 60;  // 30 for even
    CapInfo.Gain[1]=(unsigned char) 60;  // 30 for even
    CapInfo.Gain[2]=(unsigned char) 60;  // 60 for even
    MaxExposure = 2000;
}

#elif defined (SAC_CMOS_GUIDE)
// QHY CMOS guide camera version

CameraSACGuider::CameraSACGuider()
{
    Connected = false;
    Name = _T("SAC Guider");
    FullSize = wxSize(1280,1024);
    m_hasGuideOutput = true;
    HasGainControl = true;
}

wxByte CameraSACGuider::BitsPerPixel()
{
    return 8;
}

bool CameraSACGuider::Connect(const wxString& camId)
{
// returns true on error
    CameraDLL = LoadLibrary("cmosDLL");
    bool retval;
    sprintf(DevName,"EZUSB-0");
    if (CameraDLL != NULL) {
        OpenUSB = (B_Cp_DLLFUNC) GetProcAddress(CameraDLL,"openUSB");
        if (!OpenUSB)   {
            FreeLibrary(CameraDLL);
            return CamConnectFailed(_("Didn't find openUSB in DLL"));
        }
        else {
            retval = OpenUSB(DevName);
            if (retval) {  // Good to go, now get other functions
            //  CloseUSB = (B_V_DLLFUNC)GetProcAddress(CameraDLL,"closeUSB");
            //  if (!CloseUSB)
            //      (void) wxMessageBox(wxT("Didn't find closeUSB in DLL"),_("Error"),wxOK | wxICON_ERROR);
                CmosReset = (V_Cp_DLLFUNC)GetProcAddress(CameraDLL,"cmosReset");
                if (!CmosReset) {
                    FreeLibrary(CameraDLL);
                    return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"),"cmosReset"));
                }
                GetFrame = (GUIDEREG_DLLFUNC)GetProcAddress(CameraDLL,"readUSB2_OnePackage");
                if (!GetFrame) {
                    FreeLibrary(CameraDLL);
                    return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"),"readUSB2_OnePackage"));
                }
                SendI2C = (Uc_CpUCp_DLLFUNC)GetProcAddress(CameraDLL,"sendI2C");
                if (!SendI2C) {
                    FreeLibrary(CameraDLL);
                    return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"),"sendI2C"));
                }
                SendGuideCommand = (Uc_CpUCUC_DLLFUNC)GetProcAddress(CameraDLL,"sendGuideCommand");
                if (!SendGuideCommand) {
                    FreeLibrary(CameraDLL);
                    return CamConnectFailed(wxString::Format(_("Didn't find %s in DLL"),"sendGuideCommand"));
                }
            }
            else {
                FreeLibrary(CameraDLL);
                return true;
            }
        }
    }
    else {
        return CamConnectFailed(_("Can't find cmosDLL.dll"));
    }
    CmosReset(DevName);

//    wxMessageBox(_T("RA+")); wxGetApp().Yield(); ST4PulseGuideScope(WEST,2000); wxGetApp().Yield();
//    wxMessageBox(_T("Dec+"));  wxGetApp().Yield(); ST4PulseGuideScope(NORTH,2000);wxGetApp().Yield();
//    wxMessageBox(_T("Dec-"));  wxGetApp().Yield(); ST4PulseGuideScope(EAST,2000);wxGetApp().Yield();
//    wxMessageBox(_T("RA-"));  wxGetApp().Yield(); ST4PulseGuideScope(SOUTH,2000);wxGetApp().Yield();
//    wxMessageBox(_T("Done"));
    }
    ClearGuidePort();
    Connected = true;
    return false;
}

bool CameraSACGuider::SetGlobalGain(unsigned char gain) {
    // Set global gain
    // User's call of 0-95% gets mapped onto the 1-15x
    // If > 95%, enter undocumented extra boost mode
    unsigned char REG[19];
    if (gain > 100) gain = 100;

    REG[0]=0;
    REG[1]=0x35;    // Register x35 is global gain
    REG[2]=0;       // 0 = normal mode
    if (gain < 25 ) {  // Low noise 1x-4x in .125x mode maps on 0-24
        REG[3]=8 + gain;        //103
    }
    else if (gain < 57) { // 4.25x-8x in .25x steps maps onto 25-56
        REG[3] = 0x51 + (gain - 25)/2;  // 81-96 aka 0x51-0x60
    }
    else if (gain < 96) { // 9x-15x in 1x steps maps onto 57-95
        REG[3] = 0x61 + (gain - 57)/6;
    }
    else { // Turbo boost...
        REG[3] = 255;
        REG[2] = 6 - (100-gain);
    }
    SendI2C(DevName,REG);
    return false;
}

bool CameraSACGuider::ST4PulseGuideScope(int direction, int duration) {
    unsigned char dur;
    unsigned char reg = 0;

    if (duration > 2550) duration = 2550;

    // Output pins are NC, Com, RA+(W), Dec+(N), Dec-(S), RA-(E) ??  http://www.starlight-xpress.co.uk/faq.htm
    dur = (unsigned char) ROUND((float) duration / 10.0);       // Actual guides are 10x what's specified
    switch (direction) {
        case WEST: reg = 0x80; break;   // 1000 0000
        case NORTH: reg = 0x40; break;  // 0100 0000
        case SOUTH: reg = 0x20; break;  // 0010 0000
        case EAST: reg = 0x10;  break;  // 0001 0000
        default: return true; // bad direction passed in
    }
    pFrame->StatusMsg(wxString::Format("%s %x %x",DevName,reg,dur));
    SendGuideCommand(DevName,reg,dur);
    return false;
}

void CameraSACGuider::ClearGuidePort() {
    SendGuideCommand(DevName,0,0);
}

void CameraSACGuider::InitCapture()
{
    // Reset chip, just to be safe
    CmosReset(DevName);
    SetGlobalGain((unsigned char) GuideCameraGain);
}

bool CameraSACGuider::Disconnect() {
    //if (CloseUSB) CloseUSB();
    FreeLibrary(CameraDLL);
    Connected = false;
    return false;
}

bool CameraSACGuider::GenericCapture(int duration, usImage& img, int xsize, int ysize, int xpos, int ypos) {
// Only does full frames still

    unsigned char *bptr;
    unsigned short *dptr;
    unsigned char *buffer;
    int  x,y;
    static int offset = 12440;  // initial pixel
    xsize = FullSize.GetWidth();
    ysize = FullSize.GetHeight();
    bool firstimg = true;
    static unsigned long raw_imgsize = 1600200; //1524 * 1050

    buffer = new unsigned char[1600200+2000];
    //buffer = new unsigned char[9000000];
    GetFrame(DevName,raw_imgsize,(unsigned long) duration,buffer);
    if (img.Init(FullSize)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        delete[] buffer;
        return true;
    }

    //bptr = buffer;

    // Get the data into a 1280x1024 buffer
    dptr = img.ImageData;
    for (y=0; y<ysize; y++) {
        bptr = buffer + offset + y*1524;
        for (x=0; x<xsize; x++, dptr++, bptr++) {
            *dptr = (unsigned short) (*bptr);
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);
    // Do quick L recon to remove bayer array
    if (options & CAPTURE_RECON) QuickLRecon(img);

    delete[] buffer;

    return false;
}

bool CameraSACGuider::CaptureCrop(int duration, usImage& img)
{
    GenericCapture(duration, img, width,height,startX,startY);
    return false;
}

bool CameraSACGuider::CaptureFull(int duration, usImage& img)
{
    GenericCapture(duration, img, FullSize.GetWidth(),FullSize.GetHeight(),0,0);
    return false;
}

#endif
