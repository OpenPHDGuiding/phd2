/*
 *  cam_LESerialWebcam.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2013 Craig Stark.
 *  Ported to opencv by Bret McKee.
 *  Copyright (c) 2013 Bret McKee.
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

#if defined(OPENCV_CAMERA) && defined(LE_CAMERA)

#include "camera.h"
#include "cam_LEWebcam.h"

using namespace cv;

Camera_LEWebcamClass::Camera_LEWebcamClass(void)
     : Camera_WDMClass()
{
    Name = _T("Generic LE Webcam");
    PropertyDialogType = PROPDLG_WHEN_CONNECTED;
    HasDelayParam = true;
}

Camera_LEWebcamClass::~Camera_LEWebcamClass(void)
{
}

bool Camera_LEWebcamClass::Connect()
{
    bool bError = false;

    try
    {
        if (Camera_WDMClass::Connect())
        {
            throw ERROR_INFO("Unable to open base class camera");
        }

        LEControl(LECAMERA_LED_OFF | LECAMERA_SHUTTER_CLOSED | LECAMERA_EXPOSURE_FIELD_NONE | LECAMERA_AMP_OFF);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool Camera_LEWebcamClass::Disconnect()
{
    bool bError = false;

    bError = Camera_WDMClass::Disconnect();

    return bError;
}

bool Camera_LEWebcamClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool bError = false;

    try
    {
        int ampOffTime = 400;
        int ampOnTime = duration - ampOffTime;

        if (ampOnTime <= 0)
        {
            ampOffTime = duration;
        }
        else
        {
            // do the "amp on" part of the exposure
            LEControl(LECAMERA_LED_GREEN | LECAMERA_SHUTTER_OPEN | LECAMERA_EXPOSURE_FIELD_A | LECAMERA_EXPOSURE_FIELD_B | LECAMERA_AMP_ON);
            wxMilliSleep(ampOnTime);
        }

        // do the "amp off" part of the exposure.
        LEControl(LECAMERA_LED_RED | LECAMERA_SHUTTER_OPEN | LECAMERA_EXPOSURE_FIELD_A | LECAMERA_EXPOSURE_FIELD_B | LECAMERA_AMP_OFF);
        wxMilliSleep(ampOffTime);

        // exposure complete - release the frame
        LEControl(LECAMERA_SHUTTER_CLOSED | LECAMERA_AMP_ON | LECAMERA_EXPOSURE_FIELD_NONE | LECAMERA_AMP_OFF);

        // wait the final delay before reading (if there is one)
        if (ReadDelay)
        {
            wxMilliSleep(ReadDelay);
        }

        // now record the frame.
        // Start by grabbing three frames
        usImage frame1;
        if (CaptureOneFrame(frame1, options, subframe))
        {
            throw ERROR_INFO("CaptureOneFrame(frame1) failed");
        }

        usImage frame2;
        if (CaptureOneFrame(frame2, options, subframe))
        {
            throw ERROR_INFO("CaptureOneFrame(frame2) failed");
        }

        usImage frame3;
        if (CaptureOneFrame(frame3, options, subframe))
        {
            throw ERROR_INFO("CaptureOneFrame(frame3) failed");
        }

        unsigned short *dptr1 = frame1.ImageData;
        unsigned short *dptr2 = frame2.ImageData;
        unsigned short *dptr3 = frame3.ImageData;

        UINT64 sum1=0;
        UINT64 sum2=0;
        UINT64 sum3=0;

        // we only use the data from the frame with the largest sum.
        // This is because we are not exactly sure when we will capture the "Long Exposure"
        // frame

        for(int i=0;i < frame1.NPixels; i++)
        {
            sum1 += *dptr1++;
            sum2 += *dptr2++;
            sum3 += *dptr3++;
        }

        Debug.AddLine("sum1=%lld sum2=%lld sum3=%lld", sum1, sum2, sum3);

        usImage *srcPtr = &frame1;

        if (sum2 > sum1 && sum2 > sum3)
        {
            srcPtr = &frame2;
        }
        else if (sum3 > sum1 && sum3 > sum2)
        {
            srcPtr = &frame3;
        }

        if (img.Init(srcPtr->Size))
        {
            throw ERROR_INFO("img.Init() failed");
        }

        img.SwapImageData(*srcPtr);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    // turn off the LE camera
    LEControl(LECAMERA_LED_OFF | LECAMERA_SHUTTER_CLOSED | LECAMERA_EXPOSURE_FIELD_NONE | LECAMERA_AMP_OFF);

    return bError;
}

#endif // defined(OPENCV_CAMERA) && defined(LE_SERIAL_CAMERA)
