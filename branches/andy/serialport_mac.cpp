/*
 *  serialport_mac.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark on 4/17/13.
 *  Copyright (c) 2013 Craig Stark.
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

#ifdef __APPLE__

#include "phd.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <IOKit/serial/IOSerialKeys.h>

static kern_return_t createSerialIterator(io_iterator_t *serialIterator)
{
    kern_return_t   kernResult;
    mach_port_t     masterPort;
    CFMutableDictionaryRef  classesToMatch;

    if ((kernResult=IOMasterPort(NULL, &masterPort)) != KERN_SUCCESS)
    {
        printf("IOMasterPort returned %d\n", kernResult);
        return kernResult;
    }

    if ((classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
    {
        printf("IOServiceMatching returned NULL\n");
        return kernResult;
    }

    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),
                         CFSTR(kIOSerialBSDRS232Type));
//                         CFSTR(kIOSerialBSDModemType));
    kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, serialIterator);
    if (kernResult != KERN_SUCCESS)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }

    return kernResult;
}

static const char *getRegistryString(io_object_t sObj, const char *propName)
{
    static char resultStr[256];
    //   CFTypeRef  nameCFstring;
    CFStringRef nameCFstring;
    resultStr[0] = 0;
    nameCFstring = (CFStringRef) IORegistryEntryCreateCFProperty(sObj,
                                                                 CFStringCreateWithCString(kCFAllocatorDefault, propName, kCFStringEncodingASCII),
                                                                 kCFAllocatorDefault, 0);
    if (nameCFstring) {
        CFStringGetCString(nameCFstring, resultStr, sizeof(resultStr),
                           kCFStringEncodingASCII);
        CFRelease(nameCFstring);
    }
    return resultStr;
}

SerialPortMac::SerialPortMac(void)
{
    m_PortFID = 0;
}

SerialPortMac::~SerialPortMac(void)
{
    if (m_PortFID > 0) {
        close(m_PortFID);
        m_PortFID = 0;
    }
}

wxArrayString SerialPortMac::GetSerialPortList(void)
{
    wxArrayString ret;
    return ret;
}

bool SerialPortMac::Connect(const wxString& portName, int baud, int dataBits, int stopBits, PARITY Parity, bool useRTS, bool useDTR)
{
    bool bError = false;
 /*   try
    {
        m_handle = CreateFile(portName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,    // must be opened with exclusive-access
                              NULL, // no security attributes
                              OPEN_EXISTING, // must use OPEN_EXISTING
                              0,    // not overlapped I/O
                              NULL  // hTemplate must be NULL for comm devices
                              );
        if (m_handle == INVALID_HANDLE_VALUE)
        {
            throw ERROR_INFO("SerialPortWin32: CreateFile failed");
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
    }*/

    return bError;
}

bool SerialPortMac::Disconnect(void) {
    bool bError = false;

    if (m_PortFID > 0) {
        if (close(m_PortFID))
            bError = true;
        m_PortFID = 0;
    }
    return bError;
}

bool SerialPortMac::SetReceiveTimeout(int timeoutMs) {
    bool bError = false;

    try {
        struct termios  options;
        if (tcgetattr(m_PortFID, &options) == -1) {
            throw ERROR_INFO("SerialPortMac: Unable to get port attributes");
        }
        options.c_cc[VMIN] = 1;
        options.c_cc[VTIME] = timeoutMs / 10;
        if (tcsetattr(m_PortFID, TCSANOW, &options) == -1)
            throw ERROR_INFO("SerialPortMac: Unable to set port attributes");
    }
    catch (wxString Msg) {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortMac::Send(const unsigned char *pData, unsigned count)
{
    bool bError = false;
    /*
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
    */
    return bError;
}

bool SerialPortMac::Receive(unsigned char *pData, unsigned count)
{
    bool bError = false;
    /*
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
    */
    return bError;
}

#endif // _APPLE_
