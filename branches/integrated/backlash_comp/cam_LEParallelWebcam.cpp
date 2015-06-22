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

Camera_LEParallelWebcamClass::Camera_LEParallelWebcamClass(void)
    : Camera_LEWebcamClass()
{
    Name=_T("Parallel LE Webcam");
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

        wxString lastParallelPort = pConfig->Profile.GetString("/camera/parallelLEWebcam/parallelport", "");

        wxString choice = m_pParallelPort->ChooseParallelPort(lastParallelPort);

        Debug.AddLine("Camera_LEParallelWebcamClass::Connect: parallel port choice is: %s", choice);

        if (choice.IsEmpty())
        {
            throw ERROR_INFO("no parallel port selected");
        }

        if (m_pParallelPort->Connect(choice))
        {
            throw ERROR_INFO("LEParallelWebcamClass::Connect: parallel port connect failed");
        }

        pConfig->Profile.SetString("/camera/parallelLEWebcam/parallelport", choice);

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

bool Camera_LEParallelWebcamClass::Disconnect()
{
    bool bError = false;

    try
    {
        delete m_pParallelPort;
        m_pParallelPort = NULL;

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

        if (actions & LECAMERA_EXPOSURE_FIELD_NONE)
        {
            bitsToClear |= PARALLEL_BIT_TRANSFER;
        }
        else if ((actions & LECAMERA_EXPOSURE_FIELD_A) || (actions & LECAMERA_EXPOSURE_FIELD_B))
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
