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

#if defined(OPENCV_CAMERA) && defined(LE_PARALLEL_CAMERA)

#include "camera.h"
#include "cam_LEParallelWebcam.h"

/* ----Prototypes of Inp and Out32--- */
short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);

using namespace cv;

Camera_LEParallelWebcamClass::Camera_LEParallelWebcamClass(int devNumber)
    : Camera_LEWebcamClass(devNumber)
{
    Name=_T("Parallel LE Webcam");
    readDelay = 5;
    m_pParallelPort = NULL;
}

Camera_LEParallelWebcamClass::~Camera_LEParallelWebcamClass(void)
{
    Disconnect();
    delete m_pParallelPort;
}

bool Camera_LEParallelWebcamClass::Connect()
{
    bool bError = false;

    try
    {
        m_pParallelPort = ParallelPort::ParallelPortFactory();

        if (!m_pParallelPort)
        {
            throw ERROR_INFO("LEParallelWebcamClass::Connect: parallel port is NULL");
        }

        wxArrayString parallelPorts = m_pParallelPort->GetParallelPortList();

        wxString lastParallelPort = pConfig->GetString("/camera/parallelLEWebcam/parallelport", "");
        int resp = parallelPorts.Index(lastParallelPort);

        resp = wxGetSingleChoiceIndex(_("Select Parallel port"),_("Parallel Port"), parallelPorts,
                NULL, wxDefaultCoord, wxDefaultCoord, true, wxCHOICE_WIDTH, wxCHOICE_HEIGHT,
                resp);

        if (m_pParallelPort->Connect(parallelPorts[resp]))
        {
            throw ERROR_INFO("LEParallelWebcamClass::Connect: parallel port connect failed");
        }

        pConfig->SetString("/camera/parallelLEWebcam/parallelport", parallelPorts[resp]);

        if (LEControl(LECAMERA_LED_OFF|LECAMERA_SHUTTER_CLOSED|LECAMERA_TRANSFER_FIELD_NONE|LECAMERA_AMP_OFF))
        {
            throw ERROR_INFO("LEParallelWebcamClass::Connect: LEControl failed");
        }

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

bool Camera_LEParallelWebcamClass::Disconnect()
{
    bool bError = false;

    delete m_pParallelPort;
    m_pParallelPort = NULL;

    bError = Camera_OpenCVClass::Disconnect();

    return bError;
}

bool Camera_LEParallelWebcamClass::LEControl(int actions)
{
    bool bError = false;

    try
    {
        // Note: Data lines on the camera when using the parallel port are:
        // D0: Frame1
        // D1: unused
        // D2: Amp
        // D3: Shutter

        enum
        {
            PARALLEL_BIT_TRANSFER  = 1,
            PARALLEL_BIT_UNUSED    = 2,
            PARALLEL_BIT_AMPLIFIER = 4,
            PARALLEL_BIT_SHUTTER   = 8
        };

        BYTE bitsToClear = 0;
        BYTE bitsToSet = 0;

        if (actions & LECAMERA_TRANSFER_FIELD_NONE)
        {
            bitsToClear |= PARALLEL_BIT_TRANSFER;
        }
        else if ((actions & LECAMERA_TRANSFER_FIELD_A) || (actions & LECAMERA_TRANSFER_FIELD_B))
        {
            bitsToSet |= PARALLEL_BIT_TRANSFER;
        }

        if (actions & LECAMERA_AMP_OFF)
        {
            bitsToClear |= PARALLEL_BIT_AMPLIFIER;
        }
        else if (actions & LECAMERA_AMP_ON)
        {
            bitsToSet |= PARALLEL_BIT_AMPLIFIER;
        }

        if (actions & LECAMERA_SHUTTER_CLOSED)
        {
            bitsToClear |= PARALLEL_BIT_SHUTTER;
        }
        else if (actions & LECAMERA_SHUTTER_OPEN)
        {
            bitsToSet |= PARALLEL_BIT_SHUTTER;
        }

        if (m_pParallelPort->ManipulateByte(bitsToClear, bitsToSet))
        {
            throw ERROR_INFO("LEParallelWebcamClass::LEControl: ReadByte failed");
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
