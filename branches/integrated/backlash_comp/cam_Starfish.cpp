/*
 *  cam_Starfish.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2007-2010 Craig Stark.
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
#if defined (STARFISH)
#include "camera.h"
#include "time.h"
#include "image_math.h"

#include "Cam_Starfish.h"

#if defined (__WINDOWS__)
#define kIOReturnSuccess 0
typedef int IOReturn;
using namespace FcCamSpace;
void    fcUsb_init(void) { FcCamFuncs::fcUsb_init(); }
void    fcUsb_close(void) { FcCamFuncs::fcUsb_close(); }
void    fcUsb_CloseCameraDriver(void) { FcCamFuncs::fcUsb_close(); }
int fcUsb_FindCameras(void) { return FcCamFuncs::fcUsb_FindCameras(); }
int fcUsb_cmd_setRegister(int camNum, UInt16 regAddress, UInt16 dataValue) { return FcCamFuncs::fcUsb_cmd_setRegister(camNum, regAddress, dataValue); }
UInt16 fcUsb_cmd_getRegister(int camNum, UInt16 regAddress) { return FcCamFuncs::fcUsb_cmd_getRegister(camNum, regAddress); }
int fcUsb_cmd_setIntegrationTime(int camNum, UInt32 theTime) { return FcCamFuncs::fcUsb_cmd_setIntegrationTime(camNum, theTime); }
int fcUsb_cmd_startExposure(int camNum) { return FcCamFuncs::fcUsb_cmd_startExposure(camNum); }
int fcUsb_cmd_abortExposure(int camNum) { return FcCamFuncs::fcUsb_cmd_abortExposure(camNum); }
UInt16 fcUsb_cmd_getState(int camNum) { return FcCamFuncs::fcUsb_cmd_getState(camNum); }
int fcUsb_cmd_setRoi(int camNum, UInt16 left, UInt16 top, UInt16 right, UInt16 bottom) { return FcCamFuncs::fcUsb_cmd_setRoi(camNum,left,top,right,bottom); }
int fcUsb_cmd_setRelay(int camNum, int whichRelay) { return FcCamFuncs::fcUsb_cmd_setRelay(camNum,whichRelay); }
int fcUsb_cmd_clearRelay(int camNum, int whichRelay) { return FcCamFuncs::fcUsb_cmd_clearRelay(camNum,whichRelay); }
int fcUsb_cmd_pulseRelay(int camNum, int whichRelay, int onMs, int offMs, bool repeats) { return FcCamFuncs::fcUsb_cmd_pulseRelay(camNum,whichRelay, onMs, offMs, repeats); }
int fcUsb_cmd_setTemperature(int camNum, SInt16 theTemp) { return FcCamFuncs::fcUsb_cmd_setTemperature(camNum,theTemp); }
bool fcUsb_cmd_getTECInPowerOK(int camNum) { return FcCamFuncs::fcUsb_cmd_getTECInPowerOK(camNum); }
int fcUsb_cmd_getRawFrame(int camNum, UInt16 numRows, UInt16 numCols, UInt16 *frameBuffer) { return FcCamFuncs::fcUsb_cmd_getRawFrame(camNum,numRows,numCols, frameBuffer); }
int fcUsb_cmd_setReadMode(int camNum, int DataXfrReadMode, int DataFormat) { return FcCamFuncs::fcUsb_cmd_setReadMode(camNum,DataXfrReadMode,DataFormat); }
bool fcUsb_haveCamera(void) { return FcCamFuncs::fcUsb_haveCamera(); }
#endif

Camera_StarfishClass::Camera_StarfishClass()
{
    Connected = false;
    Name = _T("Fishcamp Starfish");
    FullSize = wxSize(1280,1024);
    HasSubframes = true;
    HasGainControl = true;
    m_hasGuideOutput = true;
    DriverLoaded = false;
}



bool Camera_StarfishClass::Connect() {
// returns true on error

    IOReturn rval;

    wxBeginBusyCursor();
    if (!DriverLoaded) {
        fcUsb_init();               // Init the driver
        DriverLoaded = true;
    }
    NCams = fcUsb_FindCameras();
    int i = NCams;
    wxEndBusyCursor();
    if (NCams == 0)
        return true;
    else {
        CamNum = 1;  // Assume just the one cam for now
        // set to polling mode and turn off black adjustment but turn on auto balancing of the offsets in the 2x2 matrix
        rval = fcUsb_cmd_setReadMode(CamNum,  fc_classicDataXfr, fc_16b_data);
        if (rval != kIOReturnSuccess) return true;
        if (fcUsb_cmd_getTECInPowerOK(CamNum))
            fcUsb_cmd_setTemperature(CamNum,10);
    }
    lastSubFrame = wxRect(wxSize(1, 1));        // To differentiate between full-frame "null" subframe
    Connected = true;
    return false;
}

bool Camera_StarfishClass::Disconnect() {
    if (fcUsb_haveCamera())
        fcUsb_CloseCameraDriver();
    Connected = false;
    return false;
}
void Camera_StarfishClass::InitCapture() {
    // Set gain
    unsigned short Gain = (unsigned short) GuideCameraGain;
    if (Gain < 25 ) {  // Low noise 1x-4x in .125x mode maps on 0-24
        Gain = 8 + Gain;        //103
    }
    else if (Gain < 57) { // 4.25x-8x in .25x steps maps onto 25-56
        Gain = 0x51 + (Gain - 25)/2;  // 81-96 aka 0x51-0x60
    }
    else { // 9x-15x in 1x steps maps onto 57-95
        Gain = 0x61 + (Gain - 57)/6;
    }
    if (Gain > 0x67) Gain = 0x67;
    fcUsb_cmd_setRegister(CamNum,0x35,Gain);

}

static bool StopExposure(int camNum)
{
    Debug.AddLine("Starfish: StopExposure");
    int ret = fcUsb_cmd_abortExposure(camNum);
    return ret == kIOReturnSuccess;
}

bool Camera_StarfishClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool debug = true;
    int xsize, ysize, xpos, ypos;
    bool usingSubFrames = UseSubframes && subframe.GetWidth() > 0 && subframe.GetHeight() > 0;
    IOReturn rval;

    // init memory
    if (img.Init(FullSize)) {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }


    if (usingSubFrames)
    {
        xsize = subframe.GetWidth();
        ysize = subframe.GetHeight();
        xpos = subframe.GetLeft();
        ypos = subframe.GetTop();
        subImage.Init(xsize, ysize);
        subImage.Clear();
    }
    else
    {
        xsize = FullSize.GetWidth();
        ysize = FullSize.GetHeight();
        xpos = 0;
        ypos = 0;
    }
    img.Clear();

    // set ROI if something has changed
    if (lastSubFrame != subframe)
    {
        rval = fcUsb_cmd_setRoi(CamNum, (unsigned short)xpos, (unsigned short)ypos, (unsigned short)(xpos + xsize - 1), (unsigned short)(ypos + ysize - 1));
        //Debug.AddLine(wxString::Format("Starfish: using %s", usingSubFrames ? " sub-frames" : " full frames"));
        //Debug.AddLine(wxString::Format("Starfish: ROI set from {%d,%d} to {%d,%d)", xpos, ypos, xpos + xsize - 1, ypos + ysize - 1));
        if (rval != kIOReturnSuccess) { if (debug) pFrame->Alert(_T("Starfish Err 1")); return true; }
        lastSubFrame = subframe;
    }
    // set duration
    fcUsb_cmd_setIntegrationTime(CamNum, (unsigned int) duration);

    rval = fcUsb_cmd_startExposure(CamNum);
    if (rval != kIOReturnSuccess) { if (debug) pFrame->Alert(_T("Starfish Err 2")); return true; }

    CameraWatchdog watchdog(duration, GetTimeoutMs());

    if (duration > 100)
    {
        // wait until near end of exposure
        if (WorkerThread::MilliSleep(duration - 100, WorkerThread::INT_ANY) &&
            (WorkerThread::TerminateRequested() || StopExposure(CamNum)))
        {
            return true;
        }
    }

    // wait for image to finish and d/l
    while (fcUsb_cmd_getState(CamNum) != 0)
    {
        wxMilliSleep(50);
        if (WorkerThread::InterruptRequested() &&
            (WorkerThread::TerminateRequested() || StopExposure(CamNum)))
        {
            return true;
        }
        if (watchdog.Expired())
        {
            DisconnectWithAlert(CAPT_FAIL_TIMEOUT);
            return true;
        }
    }

    if (usingSubFrames)
    {
        rval = fcUsb_cmd_getRawFrame(CamNum, (unsigned short)ysize, (unsigned short)xsize, subImage.ImageData);
        // Transfer the subframe to the corresponding location in the full-size frame
        for (int y = 0; y < ysize; y++)
        {
            const unsigned short *pSrc = subImage.ImageData + y * xsize;
            unsigned short *pDest = img.ImageData + (ypos + y) * FullSize.GetWidth() + xpos;
            for (int x = 0; x < xsize; x++)
                *pDest++ = *pSrc++;
        }
        img.Subframe = subframe;
    }
    else
    {
        rval = fcUsb_cmd_getRawFrame(CamNum, (unsigned short)ysize, (unsigned short)xsize, img.ImageData);

    }

    /*  if (rval != kIOReturnSuccess) {
        if (debug) pFrame->Alert(wxString::Format("Starfish Err 3 %d",rval));
        //return true;
    }*/

    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    return false;
}

bool Camera_StarfishClass::ST4PulseGuideScope(int direction, int duration) {

    if (direction == WEST) direction = EAST;  // my ENUM and theirs are flipped
    else if (direction == EAST) direction = WEST;

    if (fcUsb_cmd_pulseRelay(CamNum, direction, duration, 0, false) != kIOReturnSuccess)
        return true;

    return false;
}

#endif
