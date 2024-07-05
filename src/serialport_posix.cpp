/*
 *  serialport_posix.cpp
 *  PHD Guiding
 *
 *  Created by Hans Lambermont
 *  Copyright (c) 2016 Hans Lambermont
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
 *    Neither the name Craig Stark, Stark Labs nor the names of its
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

#if defined (__linux__) || defined (__APPLE__) || defined (__FreeBSD__)

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

wxArrayString SerialPortPosix::GetSerialPortList(void)
{
    wxArrayString ret;

    // TODO generate this list
    ret.Add("/dev/ttyS0");
    ret.Add("/dev/ttyS1");
    ret.Add("/dev/ttyUSB0");
    ret.Add("/dev/ttyUSB1");
    ret.Add("/dev/sx-ao-lf");

    return ret;
}

SerialPortPosix::SerialPortPosix(void)
{
    m_fd = -1;
}

SerialPortPosix::~SerialPortPosix(void)
{
    if (m_fd > 0) {
        close(m_fd);
        m_fd = -1;
    }
}

bool SerialPortPosix::Connect(const wxString& portName, int baud, int dataBits, int stopBits, PARITY Parity, bool useRTS, bool useDTR)
{
    bool bError = false;

    try
    {
        if ((m_fd = open(portName.mb_str(), O_RDWR|O_NOCTTY)) < 0) {
            wxString exposeToUser = wxString::Format("open %s failed %s(%d)", portName, strerror((int)errno), (int)errno);
            throw ERROR_INFO("SerialPortPosix::Connect " + exposeToUser);
        }

        struct termios attr;

        if (tcgetattr(m_fd, &attr) < 0) {
            wxString errorWxs = wxString::Format("tcgetattr failed %s(%d)", strerror((int)errno), (int)errno);
            throw ERROR_INFO("SerialPortPosix::Connect " + errorWxs);
        }

#if defined (__APPLE__)
        m_originalAttrs = attr;
        cfmakeraw(&attr);
#endif

        attr.c_iflag = 0; // input modes
        attr.c_oflag = 0; // output modes
        attr.c_cflag = CLOCAL;  // CLOCAL == Ignore modem control lines.
        attr.c_cflag |= CREAD ; // CREAD == Enable receiver.
        switch (dataBits) {
        case 7:
            attr.c_cflag |= CS7;    // 7 bit Character size mask.
            break;
        case 8:
            attr.c_cflag |= CS8;    // 8 bit Character size mask.
            break;
        default:
            throw ERROR_INFO("SerialPortPosix::Connect unsupported amount of dataBits");
            break;
        }
        switch (stopBits) {
        case 1:
            break;
        case 2:
            attr.c_cflag |= CSTOPB; // 2 stop bits
            break;
        default:
            throw ERROR_INFO("SerialPortPosix::Connect invalid amount of stopBits");
            break;
        }
        switch (Parity) {
        case ParityNone:
            break;
        case ParityOdd:
            attr.c_cflag |= PARENB | PARODD;
            break;
        case ParityEven:
            attr.c_cflag |= PARENB; // Enable parity generation on output and parity checking for input.
            break;
        case ParityMark:  // TODO, not in POSIX. CMSPAR
        case ParitySpace: // TODO, not in POSIX. CMSPAR
        default:
            throw ERROR_INFO("SerialPortPosix::Connect invalid parity");
            break;
        }
        attr.c_lflag &= ~ICANON; // Do not wait for a line delimiter. Work in noncanonical mode.
        attr.c_lflag &= ~( ECHO | ECHOE | ISIG | IEXTEN | NOFLSH | TOSTOP); // local modes
        attr.c_lflag |=  NOFLSH;
        attr.c_cc[VTIME] = (uint8_t)0; // timeout in deciseconds
        attr.c_cc[VMIN]  = (uint8_t)0; // minimum number of characters for noncanonical read

        unsigned int speed = B0;
        switch (baud) {
        case 9600:   speed =   B9600; break;
        case 19200:  speed =  B19200; break;
        case 38400:  speed =  B38400; break;
        case 57600:  speed =  B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        default:
            throw ERROR_INFO("SerialPortPosix::Connect unsupported baudrate");
            break;
        }
        if (cfsetispeed(&attr, speed) < 0 || cfsetospeed(&attr, speed) < 0) {
            throw ERROR_INFO("cfsetispeed failed");
        }

        if (tcsetattr(m_fd, TCSAFLUSH, &attr) < 0 ) {
            wxString errowWxs = wxString::Format("tcsetattr failed %s(%d)", strerror((int)errno), (int)errno);
            throw ERROR_INFO("SerialPortPosix::Connect " + errowWxs);
        }

        int ioctl_ret = 0;
        unsigned int argp;

        if ((ioctl_ret = ioctl(m_fd, TIOCMGET, &argp)) < 0) {
            throw ERROR_INFO("ioctl TIOCMGET");
        }
        if (useDTR) {
            argp |= TIOCM_DTR;
        }
        if (useRTS) {
            argp |= TIOCM_RTS;
        }
        if ((ioctl_ret = ioctl(m_fd, TIOCMSET, &argp)) < 0) {
            throw ERROR_INFO("ioctl TIOCMSET");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortPosix::Disconnect(void)
{
    bool bError = false;

    try
    {
#if defined (__APPLE__)
        if (tcdrain(m_fd) == -1) {
            fprintf(stderr,"Error waiting for drain - %s(%d).\n",strerror(errno), errno);
        }
        if (tcsetattr(m_fd, TCSANOW, &m_originalAttrs) == -1) {
            fprintf(stderr,"Error resetting tty attributes - %s(%d).\n",strerror(errno),errno);
        }
#endif
        if (close(m_fd) == -1) {
            throw ERROR_INFO("SerialPortPosix: close failed");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    m_fd = -1;

    return bError;
}

bool SerialPortPosix::SetReceiveTimeout(int timeoutMilliSeconds)
{
    bool bError = false;

    Debug.Write(wxString::Format("SerialPortPosix::SetReceiveTimeout %d ms\n", timeoutMilliSeconds));

    try
    {
        struct termios attr;

        if (tcgetattr(m_fd, &attr) < 0) {
            throw ERROR_INFO("tcgetattr failed");
        }
        int timeoutDeciSeconds = (timeoutMilliSeconds + 99) / 100;
        attr.c_cc[VTIME] = (uint8_t)timeoutDeciSeconds;
        if (tcsetattr(m_fd, TCSAFLUSH, &attr) < 0) {
            throw ERROR_INFO("tcsetattr failed");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortPosix::Send(const unsigned char *pData, unsigned int count)
{
    bool bError = false;

    try
    {
        Debug.AddBytes("SerialPortPosix::Send", pData, count);

        size_t rem = count;
        while (rem > 0)
        {
            ssize_t const nBytesWritten = write(m_fd, pData, rem);

            if (nBytesWritten < 0)
                throw ERROR_INFO("SerialPortPosix: write failed");

            rem -= nBytesWritten;
            pData += nBytesWritten;
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortPosix::Receive(unsigned char *pData, unsigned int count)
{
    bool bError = false;

    try
    {
        size_t rem = count;

        while (rem > 0)
        {
            ssize_t const receiveCount = read(m_fd, pData, rem);

            if (receiveCount == -1)
                throw ERROR_INFO("SerialPortPosix: read Failed");

            if (receiveCount == 0)
                break; // eof

            Debug.AddBytes("SerialPortPosix::Receive", pData, receiveCount);

            rem -= receiveCount;
            pData += receiveCount;
        }

        if (rem > 0) {
            throw ERROR_INFO("SerialPortPosix: " + wxString::Format(wxT("%i"), rem) + " remaining bytes to read at eof " + ", expected total of " + wxString::Format(wxT("%i"), count));
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool SerialPortPosix::SetRTS(bool asserted)
{
    return true; // TODO
}

bool SerialPortPosix::SetDTR(bool asserted)
{
    return true; // TODO
}

#endif // __linux__
