/*
 *  cam_SSPAIG.cpp
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
#include "phd.h"
#if defined (SSPIAG)
#include "camera.h"
#include "time.h"
#include "image_math.h"

/* To-Do
- Updating gain on the fly, esp. if also changing exposure duration?
- Guide output

*/

#include "cam_SSPIAG.h"
// Orion SS PI AG camera (aka QHY5V)
//extern "C" __declspec(dllexport) __stdcall void setDevName(PCHAR i);
typedef void (CALLBACK* Q5V_PCHAR)(char*);
Q5V_PCHAR Q5V_SetDevName;

//extern "C" __declspec(dllexport) __stdcall void getFullSizeImage(unsigned char *img);
//extern "C" __declspec(dllexport) __stdcall void RowNoiseReductionMethod(unsigned char i);
//extern "C" __declspec(dllexport) __stdcall void BlackCalibration(unsigned char i);       //0= not enable   1=enable
//extern "C" __declspec(dllexport) __stdcall void RowNoiseConstant(unsigned char i);
typedef void (CALLBACK* Q5V_UPCHAR)(unsigned char*);
Q5V_UPCHAR Q5V_GetFullSizeImage;

//extern "C" __declspec(dllexport) __stdcall unsigned char openQHY5V(void);
typedef unsigned char (CALLBACK* Q5V_UC_V)(void);
Q5V_UC_V Q5V_OpenQHY5V;

/*extern "C" __declspec(dllexport) __stdcall void AGC_enable(int i);
extern "C" __declspec(dllexport) __stdcall void AEC_enable(int i);
extern "C" __declspec(dllexport) __stdcall void bitCompanding(int i);
extern "C" __declspec(dllexport) __stdcall void LongExpMode(int i);
extern "C" __declspec(dllexport) __stdcall void HighDynamic(int i);
extern "C" __declspec(dllexport) __stdcall void BlackOffset(int i);
*/
typedef void (CALLBACK* Q5V_INT)(int);
Q5V_INT Q5V_AGC_Enable;
Q5V_INT Q5V_AEC_Enable;
Q5V_INT Q5V_BitCompanding;
Q5V_INT Q5V_LongExpMode;
Q5V_INT Q5V_HighDynamic;
Q5V_INT Q5V_BlackOffset;

//extern "C" __declspec(dllexport) __stdcall void HighGainBoost(unsigned char i);
typedef void (CALLBACK* Q5V_UC)(unsigned char);
Q5V_UC Q5V_HighGainBoost;
Q5V_UC Q5V_RowNoiseReductionMethod;
Q5V_UC Q5V_BlackCalibration;
Q5V_UC Q5V_RowNoiseConstant;

//extern "C" __declspec(dllexport) __stdcall void setQHY5VGlobalGain(unsigned short i);
//extern "C" __declspec(dllexport) __stdcall void setTotalShutterWidth(unsigned short width);
typedef void (CALLBACK* Q5V_US)(unsigned short);
Q5V_US Q5V_SetQHY5VGlobalGain;
Q5V_US Q5V_SetTotalShutterWidth;

//extern "C" __declspec(dllexport) __stdcall void RowNoiseReduction(int en,int useblacklevel,unsigned char darkpixels,unsigned char RowNoiseConstant);
//typedef void (CALLBACK* Q5V_ROWNR)(int, int, unsigned char, unsigned char);
//Q5V_ROWNR Q5V_RowNoiseReduction;

//extern "C" __declspec(dllexport) __stdcall void ReadMode(int RowFlip,int ColumnFlip,int ShowDarkRows,int ShowDarkColumns);
typedef void (CALLBACK* Q5V_RMODE)(int, int, int, int);
Q5V_RMODE Q5V_ReadMode;

//extern "C" __declspec(dllexport) __stdcall void setLongExpTime(unsigned long i);
typedef void (CALLBACK* Q5V_UL)(unsigned long);
Q5V_UL Q5V_SetLongExpTime;

//extern "C" __declspec(dllexport) __stdcall void QHY5VInit(void);
typedef void (CALLBACK* Q5V_V)(void);
Q5V_V Q5V_QHY5VInit;

//sendGuideCommand(PCHAR devname,unsigned char GuideCommand,unsigned char PulseTime);//for QHY5,QHY5V
typedef void (CALLBACK* Q5V_GUIDE) (char*, unsigned char, unsigned char);
Q5V_GUIDE Q5V_SendGuideCommand;

Camera_SSPIAGClass::Camera_SSPIAGClass()
{
    Connected = false;
    Name = _T("StarShoot PIAG");
    FullSize = wxSize(752,480);
    m_hasGuideOutput = true;
    HasGainControl = true;
    RawBuffer = NULL;
}

bool Camera_SSPIAGClass::Connect()
{
    // returns true on error
    CameraDLL = LoadLibrary(TEXT("astroDLLsspiag"));
    if (CameraDLL == NULL) {
        wxMessageBox(_T("Cannot load astroDLLsspiag.dll"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_SetDevName = (Q5V_PCHAR)GetProcAddress(CameraDLL,"setDevName");
    if (!Q5V_SetDevName) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have setDevName"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_GetFullSizeImage = (Q5V_UPCHAR)GetProcAddress(CameraDLL,"getFullSizeImage");
    if (!Q5V_GetFullSizeImage) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have getFullSizeImage"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_RowNoiseReductionMethod = (Q5V_UC)GetProcAddress(CameraDLL,"RowNoiseReductionMethod");
    if (!Q5V_RowNoiseReductionMethod) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have RowNoiseReductionMethod"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_BlackCalibration = (Q5V_UC)GetProcAddress(CameraDLL,"BlackCalibration");
    if (!Q5V_BlackCalibration) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have BlackCalibration"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_RowNoiseConstant = (Q5V_UC)GetProcAddress(CameraDLL,"RowNoiseConstant");
    if (!Q5V_RowNoiseConstant) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have RowNoiseConstant"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_OpenQHY5V = (Q5V_UC_V)GetProcAddress(CameraDLL,"openQHY5V");
    if (!Q5V_OpenQHY5V) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have openQHY5V"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_AGC_Enable = (Q5V_INT)GetProcAddress(CameraDLL,"AGC_enable");
    if (!Q5V_AGC_Enable) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have AGC_enable"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_AEC_Enable = (Q5V_INT)GetProcAddress(CameraDLL,"AEC_enable");
    if (!Q5V_AEC_Enable) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have AEC_enable"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_BitCompanding = (Q5V_INT)GetProcAddress(CameraDLL,"bitCompanding");
    if (!Q5V_BitCompanding) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have bitCompanding"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_LongExpMode = (Q5V_INT)GetProcAddress(CameraDLL,"LongExpMode");
    if (!Q5V_LongExpMode) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have LongExpMode"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_HighDynamic = (Q5V_INT)GetProcAddress(CameraDLL,"HighDynamic");
    if (!Q5V_HighDynamic) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have HighDynamic"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_BlackOffset = (Q5V_INT)GetProcAddress(CameraDLL,"BlackOffset");
    if (!Q5V_BlackOffset) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have BlackOffset"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_HighGainBoost = (Q5V_UC)GetProcAddress(CameraDLL,"HighGainBoost");
    if (!Q5V_HighGainBoost) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have HighGainBoost"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_SetQHY5VGlobalGain = (Q5V_US)GetProcAddress(CameraDLL,"setQHY5VGlobalGain");
    if (!Q5V_SetQHY5VGlobalGain) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have setQHY5VGlobalGain"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_SetTotalShutterWidth = (Q5V_US)GetProcAddress(CameraDLL,"setTotalShutterWidth");
    if (!Q5V_SetTotalShutterWidth) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have setTotalShutterWidth"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
/*  Q5V_RowNoiseReduction = (Q5V_ROWNR)GetProcAddress(CameraDLL,"RowNoiseReduction");
    if (!Q5V_RowNoiseReduction) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have RowNoiseReduction"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }*/
    Q5V_ReadMode = (Q5V_RMODE)GetProcAddress(CameraDLL,"ReadMode");
    if (!Q5V_ReadMode) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have ReadMode"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_SetLongExpTime = (Q5V_UL)GetProcAddress(CameraDLL,"setLongExpTime");
    if (!Q5V_SetLongExpTime) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have setLongExpTime"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_QHY5VInit = (Q5V_V)GetProcAddress(CameraDLL,"QHY5VInit");
    if (!Q5V_QHY5VInit) {
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("astroDLLsspiag.dll does not have QHY5VInit"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    GenericDLL = LoadLibrary(TEXT("SSPIAGCAM.dll"));
    if (GenericDLL == NULL) {
        wxMessageBox(_T("Cannot load SSPIAGCAM.dll"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    Q5V_SendGuideCommand = (Q5V_GUIDE)GetProcAddress(GenericDLL,"sendGuideCommand");
    if (!Q5V_SendGuideCommand) {
        FreeLibrary(GenericDLL);
        FreeLibrary(CameraDLL);
        wxMessageBox(_T("SSPIAGCAM.dll does not have sendGuideCommand"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }


    Q5V_SetDevName("SSPIA-0");
    if (!Q5V_OpenQHY5V()) {
        wxMessageBox(_T("Failed to open the camera"), _("Error"));
        return true;
    }
    if (RawBuffer)
        delete [] RawBuffer;
    RawBuffer = new unsigned char[856*500];
    Q5V_QHY5VInit();
    Q5V_GetFullSizeImage(RawBuffer);
    wxMilliSleep(100);
    Q5V_QHY5VInit();
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_ReadMode(0,0,1,1);
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_BlackOffset(5);
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_BlackCalibration(0);
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_RowNoiseConstant(10);
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_RowNoiseReductionMethod(1);
    Q5V_GetFullSizeImage(RawBuffer);
//  Q5V_HighGainBoost(255);
//  Q5V_GetFullSizeImage(RawBuffer);
    Q5V_AEC_Enable(0);
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_AGC_Enable(0);
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_LongExpMode(1);
    Q5V_GetFullSizeImage(RawBuffer);
    Q5V_SetQHY5VGlobalGain(60);
    Q5V_GetFullSizeImage(RawBuffer);

    Connected = true;
    return false;
}

bool Camera_SSPIAGClass::ST4PulseGuideScope(int direction, int duration) {
// Vend req 0xb5  (vendTXD)
// Buffer[0] = GuideCommand, Buffer[1]=PulseTime.
    int reg = 0;
    int dur = duration / 10;

    if (dur >= 255) dur = 254; // Max guide pulse is 2.54s -- 255 keeps it on always
    // Output pins are NC, Com, RA+(W), Dec+(N), Dec-(S), RA-(E) ??  http://www.starlight-xpress.co.uk/faq.htm
    switch (direction) {
        case WEST: reg = 0x80; break;   // 0111 0000
        case NORTH: reg = 0x40; break;  // 1011 0000
        case SOUTH: reg = 0x20; break;  // 1101 0000
        case EAST: reg = 0x10;  break;  // 1110 0000
        default: return true; // bad direction passed in
    }
    Q5V_SendGuideCommand("QHY5V-0", reg,dur);
    WorkerThread::MilliSleep(duration + 10);
    return false;
}
void Camera_SSPIAGClass::ClearGuidePort() {
    Q5V_SendGuideCommand("QHY5V-0",0,0);
}
void Camera_SSPIAGClass::InitCapture() {

    //Q5V_SetQHY5VGlobalGain(GuideCameraGain * 63 / 100);

}
bool Camera_SSPIAGClass::Disconnect() {
    //closeUSB();
    if (RawBuffer)
        delete [] RawBuffer;
    RawBuffer = NULL;
    FreeLibrary(CameraDLL);
    FreeLibrary(GenericDLL);
    Connected = false;
    return false;

}

bool Camera_SSPIAGClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
// Only does full frames still
    static int last_dur = 0;
    static int last_gain = 60;
    unsigned char *bptr;
    unsigned short *dptr;
    int  x,y;
    int xsize = FullSize.GetWidth();
    int ysize = FullSize.GetHeight();
//  bool firstimg = true;

    if (img.Init(FullSize)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    if (duration != last_dur) {
        Q5V_SetLongExpTime(duration);
        last_dur = duration;
    }
    else if (GuideCameraGain != last_gain) {
        Q5V_SetQHY5VGlobalGain(GuideCameraGain * 63 / 100);
        last_gain = GuideCameraGain;
//      Q5V_GetFullSizeImage(RawBuffer);
    }

    bptr = RawBuffer;
    Q5V_GetFullSizeImage(bptr);

    // Load and crop from the 800 x 525 image that came in
    dptr = img.ImageData;
    for (y=0; y<ysize; y++) {
        bptr = RawBuffer + 800*(y+4) + 47;
        for (x=0; x<xsize; x++, bptr++, dptr++) { // CAN SPEED THIS UP
            *dptr=(unsigned short) *bptr;
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    // Do quick L recon to remove bayer array
    if (options & CAPTURE_RECON) QuickLRecon(img);

    return false;
}

#endif
