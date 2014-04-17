/*
 *  serialport_win32.h
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

#ifdef __WINDOWS__

wxArrayString SerialPortWin32::GetSerialPortList(void)
{
    wxArrayString ret;
    char *pBuffer = NULL;

    try
    {
        int bufferSize = 4096;
        DWORD res = 0;

        do
        {
            bufferSize *= 2;
            delete [] pBuffer;
            pBuffer = new char[bufferSize];
            if (pBuffer == NULL)
            {
                throw ERROR_INFO("new failed in GetSerialPortList");
            }
            res = QueryDosDeviceA(NULL, pBuffer, bufferSize);
        } while (res == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        if (res == 0)
        {
            throw ERROR_INFO("QueryDosDevice failed");
        }

        for (char *pEntry=pBuffer;*pEntry; pEntry += strlen(pEntry) + 1)
        {
            if (strncmp(pEntry, "COM", 3) == 0)
            {
                ret.Add(pEntry);
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    delete [] pBuffer;

    return ret;
}

SerialPortWin32::SerialPortWin32(void)
{
    m_handle = INVALID_HANDLE_VALUE;
}

SerialPortWin32::~SerialPortWin32(void)
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_handle);
    }
}

bool SerialPortWin32::Connect(const wxString& portName, int baud, int dataBits, int stopBits, PARITY Parity, bool useRTS, bool useDTR)
{
    bool bError = false;

    try
    {
        wxString portPath = "\\\\.\\" + portName;
        m_handle = CreateFile(portPath,
                               GENERIC_READ | GENERIC_WRITE,
                               0,    // must be opened with exclusive-access
                               NULL, // no security attributes
                               OPEN_EXISTING, // must use OPEN_EXISTING
                               0,    // not overlapped I/O
                               NULL  // hTemplate must be NULL for comm devices
                               );
        if (m_handle == INVALID_HANDLE_VALUE)
        {
            throw ERROR_INFO("SerialPortWin32: CreateFile("+portName+") failed");
        }

        DCB dcb;

        if (!GetCommState(m_handle, &dcb))
        {
            throw ERROR_INFO("SerialPortWin32: GetCommState failed");
        }

        dcb.BaudRate = baud;
        dcb.ByteSize = dataBits;

        switch (stopBits)
        {
            case 1:
                dcb.StopBits = ONESTOPBIT;
                break;
            case 2:
                dcb.StopBits = TWOSTOPBITS;
                break;
            default:
                throw ERROR_INFO("SerialPortWin32: invalid stopBits");
                break;
        }

        // no need to map the parity enum --- ours matches theirs
        dcb.Parity = Parity;

        dcb.fDtrControl = useDTR ? DTR_CONTROL_HANDSHAKE : DTR_CONTROL_ENABLE;
        dcb.fRtsControl = useRTS ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;

        if (!SetCommState(m_handle, &dcb))
        {
            throw ERROR_INFO("SerialPortWin32: GetCommState failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortWin32::Disconnect(void)
{
    bool bError = false;

    try
    {
        if (!CloseHandle(m_handle))
        {
            throw ERROR_INFO("SerialPortWin32: CloseHandle failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    m_handle = INVALID_HANDLE_VALUE;

    return bError;
}

bool SerialPortWin32::SetReceiveTimeout(int timeoutMs)
{
    bool bError = false;

    try
    {
        COMMTIMEOUTS timeouts;

        timeouts.ReadIntervalTimeout = 0;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = timeoutMs;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts(m_handle, &timeouts))
        {
            throw ERROR_INFO("SerialPortWin32: unable to set serial port timeouts");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortWin32::Send(const unsigned char * const pData, const unsigned count)
{
    bool bError = false;

    try
    {
        DWORD nBytesWritten = 0;

        Debug.AddBytes("Sending", pData, count);

        if (!WriteFile(m_handle, pData, count, &nBytesWritten, NULL))
        {
            throw ERROR_INFO("SerialPortWin32: WriteFile failed");
        }

        if (nBytesWritten != count)
        {
            throw ERROR_INFO("SerialPortWin32: nBytesWritten != count");
        }

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortWin32::Receive(unsigned char *pData, const unsigned count)
{
    bool bError = false;

    try
    {
        DWORD receiveCount;

        if (!ReadFile(m_handle, pData, count, &receiveCount, NULL))
        {
            throw ERROR_INFO("SerialPortWin32: Readfile Failed");
        }

        if (receiveCount != count)
        {
            throw ERROR_INFO("SerialPortWin32: recieveCount != count");
        }

        Debug.AddBytes("Received", pData, receiveCount);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortWin32::EscapeFunction(DWORD command)
{
    bool bError = false;

    try
    {
        Debug.AddLine("EscapeFunction(0x%x)", command);

        if (!EscapeCommFunction(m_handle, command))
        {
            throw ERROR_INFO("SerialPortWin32: EscapeCommFunction failed");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortWin32::SetRTS(bool asserted)
{
    return SerialPortWin32::EscapeFunction(asserted?SETRTS:CLRRTS);
}

bool SerialPortWin32::SetDTR(bool asserted)
{
    return SerialPortWin32::EscapeFunction(asserted?SETDTR:CLRDTR);
}

#endif // _WINDOWS_
