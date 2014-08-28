/*
 *  serialport_loopback.h
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

#ifdef USE_LOOPBACK_SERIAL

wxArrayString SerialPortLoopback::GetSerialPortList(void)
{
    wxArrayString ret;

    try
    {
        DWORD res = 0;

        ret.Add("Loopback 1");
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    return ret;
}

SerialPortLoopback::SerialPortLoopback(void)
{
    m_data = 0;
}

SerialPortLoopback::~SerialPortLoopback(void)
{
}

bool SerialPortLoopback::Connect(const wxString& portName, int baud, int dataBits, int stopBits, PARITY Parity, bool useRTS, bool useDTR)
{
    bool bError = false;

    try
    {
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortLoopback::Disconnect(void)
{
    bool bError = false;

    try
    {
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortLoopback::SetReceiveTimeout(int timeoutMs)
{
    bool bError = false;

    try
    {
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortLoopback::Send(const unsigned char * const pData, const unsigned count)
{
    bool bError = false;

    try
    {
        m_data = *pData;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortLoopback::Receive(unsigned char *pData, const unsigned count)
{
    bool bError = false;

    try
    {
        if (count == 3)
        {
            char *pVersion = "999";

            memcpy(pData, pVersion, strlen(pVersion));
        }
        else
        {
            if (m_data == 0)
            {
                throw ERROR_INFO("not enough characters");
            }

            if (m_data == 'R')
            {
                m_data = 'K';
            }

            *pData = m_data;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

#endif // USE_LOOPBACK_SERIAL
