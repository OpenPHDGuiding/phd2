/*
 *  stepguider_SxAO.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2013 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
 *     Craig Stark, Stark Labs nor the names of its
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
#ifdef STEPGUIDER_SXAO
StepGuiderSxAO::StepGuiderSxAO(void)
{
    m_pSerialPort = NULL;
    m_Name = "SXV-AO";
}

StepGuiderSxAO::~StepGuiderSxAO(void)
{
    delete m_pSerialPort;
}


bool StepGuiderSxAO::Connect(void)
{
    bool bError = false;

    try
    {
#ifdef USE_LOOPBACK_SERIAL
        m_pSerialPort = new SerialPortLoopback();
#else
        m_pSerialPort = SerialPort::SerialPortFactory();
#endif

        if (!m_pSerialPort)
        {
            throw ERROR_INFO("StepGuiderSxAO::Connect: serial port is NULL");
        }

        wxArrayString serialPorts = m_pSerialPort->GetSerialPortList();

        if (serialPorts.IsEmpty())
        {
            wxMessageBox(_("No serial ports found"),_("Error"), wxOK | wxICON_ERROR);
            throw ERROR_INFO("No Serial port found");
        }

        wxString lastSerialPort = pConfig->Profile.GetString("/stepguider/sxao/serialport", "");
        int resp = serialPorts.Index(lastSerialPort);

        resp = wxGetSingleChoiceIndex(_("Select serial port"),_("Serial Port"), serialPorts,
                NULL, wxDefaultCoord, wxDefaultCoord, true, wxCHOICE_WIDTH, wxCHOICE_HEIGHT,
                resp);

        if (m_pSerialPort->Connect(serialPorts[resp], 9600, 8, 1, SerialPort::ParityNone, false, false))
        {
            throw ERROR_INFO("StepGuiderSxAO::Connect: serial port connect failed");
        }

        pConfig->Profile.SetString("/stepguider/sxao/serialport", serialPorts[resp]);

        if (m_pSerialPort->SetReceiveTimeout(DefaultTimeout))
        {
            throw ERROR_INFO("StepGuiderSxAO::Connect: SetReceiveTimeout failed");
        }

        unsigned version;

        if (FirmwareVersion(version))
        {
            throw ERROR_INFO("StepGuiderSxAO::Connect: unable to get firmware version");
        }

        if (Center())
        {
            if (Unjam())
            {
                throw ERROR_INFO("StepGuiderSxAO::Connect: unable to center or Unjam");
            }
        }

        StepGuider::Connect();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::Disconnect(void)
{
    if (StepGuider::Disconnect())
    {
        return true;
    }

    bool bError = false;

    try
    {
        if (m_pSerialPort && m_pSerialPort->Disconnect())
        {
            throw ERROR_INFO("StepGuiderSxAO::serial port disconnect failed");
        }

        delete m_pSerialPort;
        m_pSerialPort = NULL;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::SendThenReceive(unsigned char sendChar, unsigned char& receivedChar)
{
    bool bError = false;

    try
    {
        if (m_pSerialPort->Send(&sendChar, 1))
        {
            throw ERROR_INFO("StepGuiderSxAO::SendThenReceive serial send failed");
        }

        if (m_pSerialPort->Receive(&receivedChar, 1))
        {
            throw ERROR_INFO("StepGuiderSxAO::SendThenReceive serial receive failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::SendThenReceive(unsigned char *pBuffer, unsigned bufferSize, unsigned char& recievedChar)
{
    bool bError = false;

    try
    {
        if (m_pSerialPort->Send(pBuffer, bufferSize))
        {
            throw ERROR_INFO("StepGuiderSxAO::SendThenReceive serial send failed");
        }

        if (m_pSerialPort->Receive(&recievedChar, 1))
        {
            throw ERROR_INFO("StepGuiderSxAO::SendThenReceive serial receive failed");
        }

        if (recievedChar == 'W')
        {
            if (m_pSerialPort->Receive(&recievedChar, 1))
            {
                throw ERROR_INFO("StepGuiderSxAO::step: Error reading another character after 'W'");
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

bool StepGuiderSxAO::SendShortCommand(unsigned char command, unsigned char& response)
{
    bool bError = false;

    try
    {
        bError = SendThenReceive(command, response);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

/*
 * Long commands send 7 bytes to the AO. The first char
 * is the command, the second is the direction and the remaining
 * 5 characters are a count.
 */

bool StepGuiderSxAO::SendLongCommand(unsigned char command, unsigned char parameter, unsigned count, unsigned char& response)
{
    bool bError = false;

    try
    {
        unsigned char cmdBuf[8]; // 7 chars + NULL

        if (count > 99999)
        {
            throw ERROR_INFO("StepGuiderSxAO::SendLongCommand invalid count");
        }

        int ret = _snprintf((char *)&cmdBuf, sizeof(cmdBuf), "%c%c%5.5d", command, parameter, count);

        if (ret < 0)
        {
            throw ERROR_INFO("StepGuiderSxAO::SendLongCommand snprintf failed");
        }

        if (ret >= sizeof(cmdBuf))
        {
            throw ERROR_INFO("StepGuiderSxAO::SendLongCommand snprintf buffer to small");
        }

        if (SendThenReceive((unsigned char *)&cmdBuf, 7, response))
        {
            throw ERROR_INFO("StepGuiderSxAO::SendLongCommand SendThenReceive failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

/*
 * the firmwareVersion command is unique command.  It sends 1 byte, and receives 3 digits
 * in response.
 */

bool StepGuiderSxAO::FirmwareVersion(unsigned& version)
{
    bool bError = false;

    try
    {
        version = 0;
        unsigned char cmd = 'V';
        unsigned char response;

        if (SendThenReceive(cmd, response))
        {
            throw ERROR_INFO("StepGuiderSxAO::firmwareVersion: SendThenReceive failed");
        }

        if (response != cmd)
        {
            throw ERROR_INFO("StepGuiderSxAO::firmwareVersion: response != cmd");
        }

        unsigned char buf[3];

        if (m_pSerialPort->Receive((unsigned char*)&buf, sizeof(buf)))
        {
            throw ERROR_INFO("StepGuiderSxAO::firmwareVersion: Receive failed");
        }

        for(int i=0;i<3;i++)
        {
            unsigned char ch = buf[i];

            if (ch < '0' || ch > '9')
            {
                throw ERROR_INFO("StepGuiderSxAO::firmwareVersion: invalid character");
            }
            version = (version * 10 ) + (ch - '0');
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::Center(unsigned char cmd)
{
    bool bError = false;

    try
    {
        unsigned char response;

        if (m_pSerialPort->SetReceiveTimeout(CenterTimeout))
        {
            throw ERROR_INFO("StepGuiderSxAO::Center: SetReceiveTimeout failed");
        }

        if (SendShortCommand(cmd, response))
        {
            throw ERROR_INFO("StepGuiderSxAO::center SendShortCommand failed");
        }

        // there are two center commands and both return 'K'
        if (response != 'K')
        {
            throw ERROR_INFO("StepGuiderSxAO::center response != cmd");
        }

        if (m_pSerialPort->SetReceiveTimeout(DefaultTimeout))
        {
            throw ERROR_INFO("StepGuiderSxAO::Center: SetReceiveTimeout failed");
        }

        StepGuider::ZeroCurrentPosition();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::Center()
{
    bool bError = false;

    try
    {
        if (Center('K'))
        {
            throw ERROR_INFO("StepGuiderSxAO::Center: Center() failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::Unjam(void)
{
    bool bError = false;

    try
    {
        if (Center('R'))
        {
            throw ERROR_INFO("StepGuiderSxAO::Center: Center() failed");
        }

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::Step(GUIDE_DIRECTION direction, int steps)
{
    bool bError = false;

    try
    {
        unsigned char cmd = 'G';
        unsigned char parameter;
        unsigned char response;
        int currentPos = 0;

        switch (direction)
        {
            case NORTH:
                parameter = 'N';
                break;
            case SOUTH:
                parameter = 'S';
                break;
            case EAST:
                parameter = 'T';
                break;
            case WEST:
                parameter = 'W';
                break;
            default:
                throw ERROR_INFO("StepGuiderSxAO::step: invalid direction");
                break;
        }

        if (SendLongCommand(cmd, parameter, steps, response))
        {
            throw ERROR_INFO("StepGuiderSxAO::step: SendLongCommand failed");
        }

        if (response == 'L')
        {
            throw ERROR_INFO("StepGuiderSxAO::step: step: at limit");
        }

        if (response != cmd)
        {
            throw ERROR_INFO("StepGuiderSxAO::step: response != cmd");
        }

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

int StepGuiderSxAO::MaxPosition(GUIDE_DIRECTION direction)
{
    return MaxSteps;
}

bool StepGuiderSxAO::IsAtLimit(GUIDE_DIRECTION direction, bool& isAtLimit)
{
    bool bError = false;

    try
    {
        unsigned char cmd = 'L';
        unsigned char response;

        if (SendThenReceive(cmd, response))
        {
            throw ERROR_INFO("StepGuiderSxAO::IsAtLimit: SendThenReceive failed");
        }

        if ((response & 0xf0) != 0x30)
        {
            throw ERROR_INFO("StepGuiderSxAO::IsAtLimit: invalid response ");
        }

        switch (direction)
        {
            case NORTH:
                isAtLimit = (response & 0x1) == 0x1;
                break;
            case SOUTH:
                isAtLimit = (response & 0x2) == 0x2;
                break;
            case EAST:
                isAtLimit = (response & 0x4) == 0x4;
                break;
            case WEST:
                isAtLimit = (response & 0x8) == 0x8;
                break;
            default:
                throw ERROR_INFO("StepGuiderSxAO::step: invalid direction");
                break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool StepGuiderSxAO::ST4HasGuideOutput(void)
{
    return true;
}

bool StepGuiderSxAO::ST4HostConnected(void)
{
    return IsConnected();
}

bool StepGuiderSxAO::ST4HasNonGuiMove(void)
{
    return true;
}

bool StepGuiderSxAO::ST4PulseGuideScope(int direction, int duration)
{
    bool bError = false;

    try
    {
        char cmd = 'M';
        char parameter = 0;
        unsigned char response;

        switch (direction)
        {
            case NORTH:
                parameter = 'N';
                break;
            case SOUTH:
                parameter = 'S';
                break;
            case EAST:
                parameter = 'T';
                break;
            case WEST:
                parameter = 'W';
                break;
            default:
                throw ERROR_INFO("StepGuiderSxAO::ST4PulseGuideScope(): invalid direction");
                break;
        }

        if (SendLongCommand(cmd, parameter, duration, response))
        {
            throw ERROR_INFO("StepGuiderSxAO::ST4PulseGuideScope(): SendLongCommand failed");
        }

        if (response != cmd)
        {
            throw ERROR_INFO("StepGuiderSxAO::ST4PulseGuideScope(): response != cmd");
        }

        // The Step function is asynchronous, and there is no way to wait for it, so we just
        // wait
        wxMilliSleep(duration);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

#endif // STEPGUIDER_SXAO

