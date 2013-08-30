/*
 *  cam_LEParallelWebcam.cpp
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

#if defined(OPENCV_CAMERA) && defined(LE_SERIAL_CAMERA)

#include "camera.h"
#include "cam_LESerialWebcam.h"

using namespace cv;

Camera_LESerialWebcamClass::Camera_LESerialWebcamClass(int devNumber)
    : Camera_LEWebcamClass(devNumber)
{
    Name=_T("Serial LE Webcam");
    m_pSerialPort = NULL;
}

Camera_LESerialWebcamClass::~Camera_LESerialWebcamClass(void)
{
    Disconnect();
    delete m_pSerialPort;
}

bool Camera_LESerialWebcamClass::Connect()
{
    bool bError = false;

    try
    {
        m_pSerialPort = SerialPort::SerialPortFactory();

        if (!m_pSerialPort)
        {
            throw ERROR_INFO("LESerialWebcamClass::Connect: serial port is NULL");
        }

        wxArrayString serialPorts = m_pSerialPort->GetSerialPortList();

        wxString lastSerialPort = pConfig->Profile.GetString("/camera/serialLEWebcam/serialport", "");
        int resp = serialPorts.Index(lastSerialPort);

        resp = wxGetSingleChoiceIndex(_("Select serial port"),_("Serial Port"), serialPorts,
                NULL, wxDefaultCoord, wxDefaultCoord, true, wxCHOICE_WIDTH, wxCHOICE_HEIGHT,
                resp);

        if (m_pSerialPort->Connect(serialPorts[resp], 2400, 8, 1, SerialPort::ParityNone, false, false))
        {
            throw ERROR_INFO("LESerialWebcamClass::Connect: serial port connect failed");
        }

        pConfig->Profile.SetString("/camera/serialLEWebcam/serialport", serialPorts[resp]);

        if (Camera_OpenCVClass::Connect())
        {
            throw ERROR_INFO("Unable to open base class camera");
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

bool Camera_LESerialWebcamClass::Disconnect()
{
    bool bError = false;

    try
    {
        delete m_pSerialPort;
        m_pSerialPort = NULL;

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

bool Camera_LESerialWebcamClass::LEControl(int actions)
{
    bool bError = false;

    try
    {
        if (actions & LECAMERA_AMP_OFF)
        {
            // set the DTR line
            if (m_pSerialPort->SetDTR(true))
            {
                throw ERROR_INFO("LESerialWebcamClass::Connect: SetRTS failed");
            }
        }
        else if (actions & LECAMERA_AMP_ON)
        {
            // clear the DTR line
            if (m_pSerialPort->SetDTR(false))
            {
                throw ERROR_INFO("LESerialWebcamClass::Connect: SetRTS failed");
            }
        }

        if (actions & LECAMERA_TRANSFER_FIELD_NONE)
        {
            // set the RTS line
            if (m_pSerialPort->SetRTS(true))
            {
                throw ERROR_INFO("LESerialWebcamClass::Connect: SetRTS failed");
            }
        }
        else if ((actions & LECAMERA_TRANSFER_FIELD_A) || (actions & LECAMERA_TRANSFER_FIELD_B))
        {
            // clear the RTS line
            if (m_pSerialPort->SetRTS(false))
            {
                throw ERROR_INFO("LESerialWebcamClass::Connect: SetRTS failed");
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

#endif // defined(OPENCV_CAMERA) && defined(LE_SERIAL_CAMERA)
