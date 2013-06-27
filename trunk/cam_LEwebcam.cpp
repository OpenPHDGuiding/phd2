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

Camera_LEWebcamClass::Camera_LEWebcamClass(int devNumber)
    : Camera_OpenCVClass(devNumber)
{
    Name=_T("Generic LE Webcam");
    readDelay = 0;
}

Camera_LEWebcamClass::~Camera_LEWebcamClass(void)
{
}

bool Camera_LEWebcamClass::Connect()
{
    bool bError = false;

    try
    {
        if (Camera_OpenCVClass::Connect())
        {
            throw ERROR_INFO("Unable to open base class camera");
        }
        LEControl(LECAMERA_LED_OFF|LECAMERA_SHUTTER_CLOSED|LECAMERA_TRANSFER_FIELD_NONE|LECAMERA_AMP_OFF);
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

    bError = Camera_OpenCVClass::Disconnect();

    return bError;
}

bool Camera_LEWebcamClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    bool bError = false;

    try
    {
        if (!pCapDev)
        {
            throw ERROR_INFO("!pCapDev");
        }

        if (!pCapDev->isOpened())
        {
            throw ERROR_INFO("!pCapDev->isOpened()");
        }

        int ampOnTime = 250;
        int ampOffTime = duration - ampOnTime;

        if (ampOffTime <= 0)
        {
            ampOnTime = duration;
        }
        else
        {
            // do the "amp off" part of the exposure.
            LEControl(LECAMERA_LED_RED|LECAMERA_SHUTTER_OPEN|LECAMERA_TRANSFER_FIELD_NONE|LECAMERA_AMP_OFF);
            Sleep(ampOffTime);
        }

        // do the "amp on" part of the exposure
        LEControl(LECAMERA_LED_GREEN|LECAMERA_SHUTTER_OPEN|LECAMERA_TRANSFER_FIELD_NONE|LECAMERA_AMP_ON);
        Sleep(ampOnTime);

        // exposure complete - release the frame
        LEControl(LECAMERA_SHUTTER_CLOSED|LECAMERA_AMP_ON|LECAMERA_TRANSFER_FIELD_A|LECAMERA_TRANSFER_FIELD_B);

        // wait the final delay before reading (if there is one)
        if (readDelay)
        {
            Sleep(readDelay);
        }

        // now record the frame.
        // Start by grabbing three frames
        Mat frame1;
        pCapDev->read(frame1);

        Mat frame2;
        pCapDev->read(frame2);

        Mat frame3;
        pCapDev->read(frame3);

        cvtColor(frame1, frame1, CV_RGB2GRAY);
        cvtColor(frame2, frame2, CV_RGB2GRAY);
        cvtColor(frame3, frame3, CV_RGB2GRAY);

        unsigned char *dptr1 = frame1.data;
        unsigned char *dptr2 = frame2.data;
        unsigned char *dptr3 = frame3.data;

        UINT64 sum1=0;
        UINT64 sum2=0;
        UINT64 sum3=0;

        cv::Size sz = frame1.size();

        // we only use the data from the frame with the largest sum.
        // This is because we are not exactly sure when we will capture the "Long Exposure"
        // frame

        for(int i=0;i < sz.width * sz.height; i++)
        {
            sum1 += *dptr1++;
            sum2 += *dptr2++;
            sum3 += *dptr3++;
        }

        unsigned char *srcPtr = frame1.data;

        if (sum2 > sum1 && sum2 > sum3)
        {
            srcPtr = frame2.data;
        }
        else if (sum3 > sum1 && sum3 > sum2)
        {
            srcPtr = frame3.data;
        }

        if (img.Init(sz.width,sz.height))
        {
            wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
            throw ERROR_INFO("img.Init failed");
        }

        // copy the image data from the frame with the largest sum
        for (int i=0; i<img.NPixels; i++)
        {
            img.ImageData[i] = srcPtr[i];
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    // turn off the LE camera
    LEControl(LECAMERA_LED_OFF|LECAMERA_SHUTTER_CLOSED|LECAMERA_TRANSFER_FIELD_NONE|LECAMERA_AMP_OFF);

    return bError;
}

#endif // defined(OPENCV_CAMERA) && defined(LE_SERIAL_CAMERA)
