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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <IOKit/serial/IOSerialKeys.h>

static kern_return_t createSerialIterator(io_iterator_t *serialIterator)
{
    kern_return_t   kernResult;
    mach_port_t     masterPort;
    CFMutableDictionaryRef  classesToMatch;
    
    if ((kernResult = IOMasterPort(0, &masterPort)) != KERN_SUCCESS)
    {
        printf("IOMasterPort returned %d\n", kernResult);
        return kernResult;
    }
    
    if ((classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
    {
        printf("IOServiceMatching returned NULL\n");
        return kernResult;
    }
    
    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey),CFSTR(kIOSerialBSDAllTypes));
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
    
    io_iterator_t iterator;
    kern_return_t result = createSerialIterator(&iterator);
    if (result == KERN_SUCCESS){
        io_object_t port;
        while ((port = IOIteratorNext(iterator)) != 0){
            const char* name = getRegistryString(port,kIOCalloutDeviceKey);
            if (name){
                ret.Add(name);
            }
            IOObjectRelease(port);
        }
        IOObjectRelease(iterator);
    }
    
    return ret;
}

bool SerialPortMac::Connect(const wxString& portName, int baud, int dataBits, int stopBits, PARITY Parity, bool useRTS, bool useDTR)
{
    const char* path = portName.ToStdString().c_str();
    
    m_PortFID = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_PortFID == -1){
        fprintf(stderr,"Failed to open device at %s - %s(%d).\n",path,strerror(errno),errno);
        return true;
    }
    
    if (ioctl(m_PortFID, TIOCEXCL) == -1){
        fprintf(stderr,"Error setting TIOCEXCL on %s - %s(%d).\n",path,strerror(errno),errno);
        close(m_PortFID); m_PortFID = 0;
        return true;
    }
    
    if (fcntl(m_PortFID, F_SETFL, 0) == -1){
        fprintf(stderr,"Error clearing O_NONBLOCK %s - %s(%d).\n",path,strerror(errno),errno);
        close(m_PortFID); m_PortFID = 0;
        return true;
    }

    if (tcgetattr(m_PortFID, &m_originalAttrs) == -1){
        fprintf(stderr,"Error getting tty attributes %s - %s(%d).\n",path,strerror(errno),errno);
        close(m_PortFID); m_PortFID = 0;
        return true;
    }

    struct termios attrs = m_originalAttrs;
    
    cfmakeraw(&attrs);
    attrs.c_cc[VMIN] = 1;
    attrs.c_cc[VTIME] = 10;

    switch (baud) {
        case 9600:
            cfsetspeed(&attrs, B9600);
            break;
        default:
            fprintf(stderr,"Unrecognised baud %d.\n",baud);
            close(m_PortFID); m_PortFID = 0;
            return true;
    }

    switch (dataBits) {
        case 8:
            attrs.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr,"Unrecognised dataBits %d.\n",dataBits);
            close(m_PortFID); m_PortFID = 0;
            return true;
    }
    
    switch (Parity) {
        case SerialPort::ParityNone:
            attrs.c_cflag &= ~PARENB;
            break;
        default:
            fprintf(stderr,"Unrecognised parity %d.\n",Parity);
            close(m_PortFID); m_PortFID = 0;
            return true;
    }

    if (stopBits > 1){
        attrs.c_cflag |= CSTOPB;
    }
    else {
        attrs.c_cflag &= ~CSTOPB;
    }
    
    if (tcsetattr(m_PortFID,TCSANOW,&attrs) == -1){
        printf("Error setting tty attributes %s - %s(%d).\n",path,strerror(errno),errno);
        close(m_PortFID); m_PortFID = 0;
        return true;
    }

    int handshake = 0;
    if (useRTS){
        handshake |= TIOCM_RTS;
    }
    if (useDTR){
        handshake |= TIOCM_DTR;
    }
    if (ioctl(m_PortFID,TIOCMSET,&handshake) == -1){
        printf("Error setting handshake %s - %s(%d).\n",path,strerror(errno),errno);
        close(m_PortFID); m_PortFID = 0;
        return true;
    }

    return false;
}

bool SerialPortMac::Disconnect(void) {
    bool bError = false;
    
    if (m_PortFID > 0) {
        if (tcdrain(m_PortFID) == -1){
            fprintf(stderr,"Error waiting for drain - %s(%d).\n",strerror(errno), errno);
        }
        if (tcsetattr(m_PortFID, TCSANOW, &m_originalAttrs) == -1){
            fprintf(stderr,"Error resetting tty attributes - %s(%d).\n",strerror(errno),errno);
        }
        if (close(m_PortFID)){
            fprintf(stderr,"Error closing port - %s(%d).\n",strerror(errno),errno);
            bError = true;
        }
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
    if (m_PortFID < 1) {
        return true;
    }
    
    const ssize_t writeCount = write(m_PortFID, pData, count); // todo; retry ?
    return !(writeCount == count);
}

bool SerialPortMac::Receive(unsigned char *pData, unsigned count)
{
    if (m_PortFID < 1) {
        return true;
    }
    
    do {
        const ssize_t readCount = read(m_PortFID, pData, count);
        if (readCount == -1){
            fprintf(stderr,"Error writing to port - %s(%d).\n",strerror(errno),errno);
            return true;
        }
        count -= readCount;
        pData += readCount;
    } while(count > 0);
    
    return !(count == 0);
}

bool SerialPortMac::SetRTS(bool asserted)
{
    return true;
}

bool SerialPortMac::SetDTR(bool asserted)
{
    return true;
}

#endif // _APPLE_
