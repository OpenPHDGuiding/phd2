/*
 *  cam_LELxUsbWebcam.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2013 Craig Stark.
 *  Ported to OpenCV by Bret McKee.
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

#if defined(OPENCV_CAMERA) && defined(LE_LXUSB_CAMERA)

#include "camera.h"
#include "cam_LELXUSBWebcam.h"
#include "cameras/ShoestringLXUSB_DLL.h"

using namespace cv;

Camera_LELxUsbWebcamClass::Camera_LELxUsbWebcamClass(void)
    : Camera_LEWebcamClass()
{
    m_isOpen = false;
    Name = _T("Usb USB Webcam");
}

Camera_LELxUsbWebcamClass::~Camera_LELxUsbWebcamClass(void)
{
    Disconnect();
}

bool Camera_LELxUsbWebcamClass::Connect()
{
    bool bError = false;

    try
    {
        if (!LXUSB_Open())
        {
            wxMessageBox(_("Unable to open LXUSB device"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("LXUSB_Open failed");
        }
        m_isOpen = true;

        LXUSB_Reset();

        if (Camera_LEWebcamClass::Connect())
        {
            throw ERROR_INFO("base class Connect() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;

        Disconnect();
    }

    return bError;
}

bool Camera_LELxUsbWebcamClass::Disconnect()
{
    bool bError = false;

    try
    {
        LXUSB_Reset();

        if (m_isOpen)
        {
            LXUSB_Close();
            m_isOpen = false;
        }

        if (Camera_LEWebcamClass::Disconnect())
        {
            throw ERROR_INFO("Base class Disconnect() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool Camera_LELxUsbWebcamClass::LEControl(int actions)
{
    bool bError = false;

    try
    {
        int frame1State;
        int frame2State;
        int shutterState;
        int ampState;
        int ledState;

        LXUSB_Status(&frame1State, &frame2State, &shutterState, &ampState, &ledState);

        if (actions & LECAMERA_EXPOSURE_FIELD_NONE)
        {
            frame1State = LXUSB_FRAME1_DEASSERTED;
            frame2State = LXUSB_FRAME2_DEASSERTED;
        }
        else
        {
            if (actions & LECAMERA_EXPOSURE_FIELD_A)
            {
                frame1State = LXUSB_FRAME1_ASSERTED;
            }

            if (actions & LECAMERA_EXPOSURE_FIELD_B)
            {
                frame2State = LXUSB_FRAME2_ASSERTED;
            }
        }

        if (actions & LECAMERA_SHUTTER_CLOSED)
        {
            shutterState = LXUSB_SHUTTER_DEASSERTED;
        }
        else if (actions & LECAMERA_SHUTTER_OPEN)
        {
            shutterState = LXUSB_SHUTTER_ASSERTED;
        }

        if (actions & LECAMERA_AMP_OFF)
        {
            ampState = LXUSB_CCDAMP_DEASSERTED;
        }
        else if (actions & LECAMERA_AMP_ON)
        {
            ampState = LXUSB_CCDAMP_ASSERTED;
        }

        if (actions & LECAMERA_LED_OFF)
        {
            ledState  = LXUSB_LED_OFF_RED;
        }
        else if (actions & LECAMERA_LED_RED)
        {
            ledState  = LXUSB_LED_ON_RED;
        }
        else if (actions & LECAMERA_LED_GREEN)
        {
            ledState  = LXUSB_LED_ON_GREEN;
        }

        LXUSB_SetAll(frame1State, frame2State, shutterState, ampState, ledState);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

#endif // defined(OPENCV_CAMERA) && defined(LE_SERIAL_CAMERA)
