/*
 *  cam_opencv.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2013 Craig Stark.
 *  Ported to PHD2 by Bret McKee.
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

#ifdef OPENCV_CAMERA

#include "cam_opencv.h"

#include <opencv/cv.h>

using namespace cv;

Camera_OpenCVClass::Camera_OpenCVClass(int devNumber)
{
    Connected = FALSE;
    Name=_T("OpenCV");
    FullSize = wxSize(640,480);
    m_hasGuideOutput = false;
    pCapDev = NULL;
    DeviceNum = devNumber;
}

Camera_OpenCVClass::~Camera_OpenCVClass(void) {
    delete pCapDev;
    pCapDev = NULL;
}

bool Camera_OpenCVClass::Connect()
{
    bool bError = false;

    try
    {
        if (!pCapDev)
        {
            pCapDev = new VideoCapture(DeviceNum);
        }

        if (!pCapDev)
        {
            throw ERROR_INFO("!pCapDev");
        }

        if (!pCapDev->isOpened())
        {
            pCapDev->open(DeviceNum);
        }

        if (!pCapDev->isOpened())
        {
            throw ERROR_INFO("!pCapDev->isOpened()");
        }

        Connected = TRUE;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;;
}

bool Camera_OpenCVClass::Disconnect()
{
    Connected = FALSE;

    if (pCapDev && pCapDev->isOpened())
    {
        pCapDev->release();
    }

    return false;
}


bool Camera_OpenCVClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    bool bError = false;

    try
    {
        wxStopWatch swatch;
        Mat captured_frame;
        int i, nframes;

        if (!pCapDev)
        {
            throw ERROR_INFO("!pCapDev");
        }

        if (!pCapDev->isOpened())
        {
            throw ERROR_INFO("!pCapDev->isOpened()");
        }

        // Grab at least one frame...
        pCapDev->read(captured_frame);
        cvtColor(captured_frame,captured_frame,CV_RGB2GRAY);

        cv::Size sz = captured_frame.size();

        if (img.Init(sz.width,sz.height))
        {
            pFrame->Alert(_("Memory allocation error"));
            throw ERROR_INFO("img.Init failed");
        }

        for (i=0; i<img.NPixels; i++)
        {
            img.ImageData[i] = 0;
        }

        nframes = 0;
        unsigned char *dptr;
        dptr = captured_frame.data;
        while (swatch.Time() < duration)
        {
            nframes++;
            pCapDev->read(captured_frame);
            cvtColor(captured_frame,captured_frame,CV_RGB2GRAY);
            dptr = captured_frame.data;
            for (i=0; i<img.NPixels; i++)
            {
                img.ImageData[i] += (unsigned int) dptr[i];
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

#endif  // OPENCV_CAMERA
