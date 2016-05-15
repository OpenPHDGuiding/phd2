/***************************************************************************\
 Slight modification by Craig Stark to compile as a MSVC DLL
 - Continued modifications by Craig Stark to support newer SX cams
 - Modified by Deneys Maartens to work under Linux, using libusb
 
 Copyright (c) 2003 David Schmenk
 All rights reserved.
 
 sxOpen() routine derived from code written by John Hyde, Intel Corp.
 USB Design by Example
 
 Copyright (C) 2001,  Intel Corporation
 All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, and/or sell copies of the Software, and to permit persons
 to whom the Software is furnished to do so, provided that the above
 copyright notice(s) and this permission notice appear in all copies of
 the Software and that both the above copyright notice(s) and this
 permission notice appear in supporting documentation.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
 Except as contained in this notice, the name of a copyright holder
 shall not be used in advertising or otherwise to promote the sale, use
 or other dealings in this Software without prior written authorization
 of the copyright holder.
 
 \***************************************************************************/

#if defined (__APPLE__) || defined (__linux__)

#include "SXMacLib.h"

#include <libusb.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/*
 * Control request fields.
 */
#define USB_REQ_TYPE                0
#define USB_REQ                     1
#define USB_REQ_VALUE_L             2
#define USB_REQ_VALUE_H             3
#define USB_REQ_INDEX_L             4
#define USB_REQ_INDEX_H             5
#define USB_REQ_LENGTH_L            6
#define USB_REQ_LENGTH_H            7
#define USB_REQ_DATA                8
#define USB_REQ_DIR(r)              ((r)&(1<<7))
#define USB_REQ_DATAOUT             0x00
#define USB_REQ_DATAIN              0x80
#define USB_REQ_KIND(r)             ((r)&(3<<5))
#define USB_REQ_VENDOR              (2<<5)
#define USB_REQ_STD                 0
#define USB_REQ_RECIP(r)            ((r)&31)
#define USB_REQ_DEVICE              0x00
#define USB_REQ_IFACE               0x01
#define USB_REQ_ENDPOINT            0x02
#define USB_DATAIN                  0x80
#define USB_DATAOUT                 0x00
/*
 * CCD camera control commands.
 */
#define SXUSB_GET_FIRMWARE_VERSION  255
#define SXUSB_ECHO                  0
#define SXUSB_CLEAR_PIXELS          1
#define SXUSB_READ_PIXELS_DELAYED   2
#define SXUSB_READ_PIXELS           3
#define SXUSB_SET_TIMER             4
#define SXUSB_GET_TIMER             5
#define SXUSB_RESET                 6
#define SXUSB_SET_CCD               7
#define SXUSB_GET_CCD               8
#define SXUSB_SET_STAR2K            9
#define SXUSB_WRITE_SERIAL_PORT     10
#define SXUSB_READ_SERIAL_PORT      11
#define SXUSB_SET_SERIAL            12
#define SXUSB_GET_SERIAL            13
#define SXUSB_CAMERA_MODEL          14
#define SXUSB_LOAD_EEPROM           15
// 16 - Set A2D Configuration registers
// 17 - Set A2D Red Offset & Gain registers
// 18 - IOE_7 triggers timed exposure
#define SXUSB_SET_A2D               16
#define SXUSB_RED_A2D               17
#define SXUSB_READ_PIXELS_GATED     18
// 19 - Sends firmware build number available from version 1.16
#define SXUSB_BUILD_NUMBER          19

// USB commands only useable on cameras with cooler control
// 30 - Sets cooler "set Point" & reports current cooler temperature
// 31 - Reports cooler temperature
#define SXUSB_COOLER_CONTROL        30
#define SXUSB_COOLER                30
#define SXUSB_COOLER_TEMPERATURE    31

// USB commands only useable on cameras with shutter control
// Check "Caps" bits in t_sxccd_params
// 32 - Controls shutter & spare cooler MCU port bits
// 33 - returns shutter status  & spare cooler MCU port bits
//      also sets shutter delay period
#define SXUSB_SHUTTER_CONTROL       32
#define SXUSB_SHUTTER               32
#define SXUSB_READ_I2CPORT          33

#define ENDPOINT_OUT 1
#define ENDPOINT_IN 2
#define VENDOR_ID 0x1278

static UInt32 ioTimeout = 0;

static inline Boolean WriteFile(void* sxHandle,
                                UInt8 *packet,
                                size_t length,
                                UInt32 *transferred,
                                void* ignored)
{
    int actualLength;
    int ret = libusb_bulk_transfer(sxHandle,
                                     ENDPOINT_OUT | LIBUSB_ENDPOINT_OUT,
                                     packet,
                                     length,
                                     &actualLength,
                                     ioTimeout);
   *transferred = actualLength;
   return 0 == ret;
}

static inline Boolean ReadFile(void* sxHandle,
                               UInt8 *packet,
                               size_t length,
                               UInt32 *transferred,
                               void* ignored)

{
    int actualLength;
    int ret = libusb_bulk_transfer(sxHandle,
                                   ENDPOINT_IN | LIBUSB_ENDPOINT_IN,
                                   packet,
                                   length,
                                   &actualLength,
                                   ioTimeout);
   *transferred = actualLength;
   return 0 == ret;
}

Boolean sxReset(void* sxHandle)
{
    UInt8 setup_data[8];
    UInt32 written;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_RESET;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

Boolean sxClearPixels(void* sxHandle, UInt16 flags, UInt16 camIndex)
{
    UInt8 setup_data[8];
    UInt32 written;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_CLEAR_PIXELS;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)flags;
    setup_data[USB_REQ_VALUE_H ] = flags >> 8;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

Boolean sxLatchPixels(void* sxHandle, UInt16 flags, UInt16 camIndex, UInt16 xoffset, UInt16 yoffset, UInt16 width, UInt16 height, UInt16 xbin, UInt16 ybin)
{
    UInt8 setup_data[18];
    UInt32 written;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_READ_PIXELS;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)flags;
    setup_data[USB_REQ_VALUE_H ] = flags >> 8;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 10;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = xoffset & 0xFF;
    setup_data[USB_REQ_DATA + 1] = xoffset >> 8;
    setup_data[USB_REQ_DATA + 2] = yoffset & 0xFF;
    setup_data[USB_REQ_DATA + 3] = yoffset >> 8;
    setup_data[USB_REQ_DATA + 4] = width & 0xFF;
    setup_data[USB_REQ_DATA + 5] = width >> 8;
    setup_data[USB_REQ_DATA + 6] = height & 0xFF;
    setup_data[USB_REQ_DATA + 7] = height >> 8;
    setup_data[USB_REQ_DATA + 8] = (UInt8)xbin;
    setup_data[USB_REQ_DATA + 9] = (UInt8)ybin;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

Boolean sxExposePixels(void* sxHandle, UInt16 flags, UInt16 camIndex, UInt16 xoffset, UInt16 yoffset, UInt16 width, UInt16 height, UInt16 xbin, UInt16 ybin, UInt32 msec)
{
    UInt8 setup_data[22];
    UInt32 written;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_READ_PIXELS_DELAYED;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)flags;
    setup_data[USB_REQ_VALUE_H ] = flags >> 8;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 14;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = xoffset & 0xFF;
    setup_data[USB_REQ_DATA + 1] = xoffset >> 8;
    setup_data[USB_REQ_DATA + 2] = yoffset & 0xFF;
    setup_data[USB_REQ_DATA + 3] = yoffset >> 8;
    setup_data[USB_REQ_DATA + 4] = width & 0xFF;
    setup_data[USB_REQ_DATA + 5] = width >> 8;
    setup_data[USB_REQ_DATA + 6] = height & 0xFF;
    setup_data[USB_REQ_DATA + 7] = height >> 8;
    setup_data[USB_REQ_DATA + 8] = (UInt8)xbin;
    setup_data[USB_REQ_DATA + 9] = (UInt8)ybin;
    setup_data[USB_REQ_DATA + 10] = (UInt8)msec;
    setup_data[USB_REQ_DATA + 11] = (UInt8)(msec >> 8);
    setup_data[USB_REQ_DATA + 12] = (UInt8)(msec >> 16);
    setup_data[USB_REQ_DATA + 13] = (UInt8)(msec >> 24);
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

Boolean sxExposePixelsGated(void* sxHandle, UInt16 flags, UInt16 camIndex, UInt16 xoffset, UInt16 yoffset, UInt16 width, UInt16 height, UInt16 xbin, UInt16 ybin, UInt32 msec)
{
    UInt8 setup_data[22];
    UInt32 written;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_READ_PIXELS_GATED;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)flags;
    setup_data[USB_REQ_VALUE_H ] = flags >> 8;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 14;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = xoffset & 0xFF;
    setup_data[USB_REQ_DATA + 1] = xoffset >> 8;
    setup_data[USB_REQ_DATA + 2] = yoffset & 0xFF;
    setup_data[USB_REQ_DATA + 3] = yoffset >> 8;
    setup_data[USB_REQ_DATA + 4] = width & 0xFF;
    setup_data[USB_REQ_DATA + 5] = width >> 8;
    setup_data[USB_REQ_DATA + 6] = height & 0xFF;
    setup_data[USB_REQ_DATA + 7] = height >> 8;
    setup_data[USB_REQ_DATA + 8] = (UInt8)xbin;
    setup_data[USB_REQ_DATA + 9] = (UInt8)ybin;
    setup_data[USB_REQ_DATA + 10] = (UInt8)msec;
    setup_data[USB_REQ_DATA + 11] = (UInt8)(msec >> 8);
    setup_data[USB_REQ_DATA + 12] = (UInt8)(msec >> 16);
    setup_data[USB_REQ_DATA + 13] = (UInt8)(msec >> 24);
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &written, NULL));
}

UInt32 sxReadPixels(void* sxHandle, UInt16 *pixels, UInt32 count)
{
    UInt32 bytes_read;
    const UInt32 byteCount = count * 2;
    
    if (ReadFile(sxHandle, (UInt8 *)pixels, byteCount, &bytes_read, NULL)){
        if (bytes_read != byteCount){
            fprintf(stderr,"SXMacLib: Error reading pixels, expected %ld but got %ld\n",byteCount,bytes_read);
        }
        return bytes_read;
    }
    
    fprintf(stderr,"SXMacLib: Error reading pixels\n");

    return 0;
}

Boolean sxSetTimer(void* sxHandle, UInt32 msec)
{
    UInt8 setup_data[12];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_SET_TIMER;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 4;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = (UInt8)msec;
    setup_data[USB_REQ_DATA + 1] = (UInt8)(msec >> 8);
    setup_data[USB_REQ_DATA + 2] = (UInt8)(msec >> 16);
    setup_data[USB_REQ_DATA + 3] = (UInt8)(msec >> 24);
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

UInt32 sxGetTimer(void* sxHandle)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_GET_TIMER;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 4;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 4, &bytes_rw, NULL);
    return (setup_data[0] | (setup_data[1] << 8) | (setup_data[2] << 16) | (setup_data[3] << 24));
}

Boolean sxSetCameraParams(void* sxHandle, UInt16 camIndex, struct sxccd_params_t *params)
{
    UInt8 setup_data[23];
    UInt32 bytes_rw;
    UInt16 fixpt;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_SET_CCD;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)camIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 15;
    setup_data[USB_REQ_LENGTH_H] = 0;
    setup_data[USB_REQ_DATA + 0] = (UInt8)(params->hfront_porch);
    setup_data[USB_REQ_DATA + 1] = (UInt8)(params->hback_porch);
    setup_data[USB_REQ_DATA + 2] = (UInt8)params->width;
    setup_data[USB_REQ_DATA + 3] = params->width >> 8;
    setup_data[USB_REQ_DATA + 4] = (UInt8)(params->vfront_porch);
    setup_data[USB_REQ_DATA + 5] = (UInt8)(params->vback_porch);
    setup_data[USB_REQ_DATA + 6] = (UInt8)(params->height);
    setup_data[USB_REQ_DATA + 7] = params->height >> 8;
    fixpt = (UInt16)(params->pix_width * 256);
    setup_data[USB_REQ_DATA + 8] = (UInt8)fixpt;
    setup_data[USB_REQ_DATA + 9] = fixpt >> 8;
    fixpt = (UInt16)(params->pix_height * 256);
    setup_data[USB_REQ_DATA + 10] = (UInt8)fixpt;
    setup_data[USB_REQ_DATA + 11] = fixpt >> 8;
    setup_data[USB_REQ_DATA + 12] = (UInt8)(params->color_matrix);
    setup_data[USB_REQ_DATA + 13] = params->color_matrix >> 8;
    setup_data[USB_REQ_DATA + 14] = params->vclk_delay;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

Boolean sxGetCameraParams(void* sxHandle, UInt16 camIndex, struct sxccd_params_t *params)
{
    UInt32 bytes_rw, status;
    
    UInt8 send_data[8];
    memset(send_data, 0, sizeof(send_data));

    UInt8 params_data[17];
    memset(params_data, 0, sizeof(params_data));

    send_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    send_data[USB_REQ         ] = SXUSB_GET_CCD;
    send_data[USB_REQ_VALUE_L ] = 0;
    send_data[USB_REQ_VALUE_H ] = 0;
    send_data[USB_REQ_INDEX_L ] = (UInt8)camIndex;
    send_data[USB_REQ_INDEX_H ] = 0;
    send_data[USB_REQ_LENGTH_L] = sizeof(params_data);
    send_data[USB_REQ_LENGTH_H] = 0;
    
    status = WriteFile(sxHandle, send_data, sizeof(send_data), &bytes_rw, NULL);
    if (!status || bytes_rw != sizeof(send_data)){
        fprintf(stderr,"SXMacLib: Error writing data in sxGetCameraParams, wrote %d bytes\n",(unsigned int)bytes_rw);
        return false;
    }
    
    status = ReadFile(sxHandle, params_data, sizeof(params_data), &bytes_rw, NULL);
    if (!status || bytes_rw != sizeof(params_data)){
        fprintf(stderr,"SXMacLib: Error reading data in sxGetCameraParams, read %d bytes\n",(unsigned int)bytes_rw);
        return false;
    }
    
    params->hfront_porch     = params_data[0];
    params->hback_porch      = params_data[1];
    params->width            = params_data[2]   | (params_data[3]  << 8);
    params->vfront_porch     = params_data[4];
    params->vback_porch      = params_data[5];
    params->height           = params_data[6]   | (params_data[7]  << 8);
    params->pix_width        = (float)((params_data[8]  | (params_data[9]  << 8)) / 256.0);
    params->pix_height       = (float)((params_data[10] | (params_data[11] << 8)) / 256.0);
    params->color_matrix     = params_data[12]  | (params_data[13] << 8);
    params->bits_per_pixel   = params_data[14];
    params->num_serial_ports = params_data[15];
    params->extra_caps       = params_data[16];

    return true;
}

Boolean sxSetCooler(void* sxHandle, UInt8 SetStatus, UInt16 SetTemp, UInt8 *RetStatus, UInt16 *RetTemp )
{
    //  Cooler Temperatures are sent & received as 2 byte unsigned integers.
    //  Temperatures are represented in degrees Kelvin times 10
    //  Resolution is 0.1 degree so a Temperature of -22.3 degC = 250.7 degK is represented by the integer 2507
    //  Cooler Temperature = (Temperature in degC + 273) * 10.
    //  Actual Temperature in degC = (Cooler temperature - 2730)/10
    //  Note there is a latency of one reading when changing the cooler status.
    //  The new status is returned on the next reading.
    //  Maximum Temperature is +35.0degC and minimum temperature is -40degC
    
    UInt8 setup_data[8];
    UInt32 status, bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR;
    setup_data[USB_REQ         ] = SXUSB_COOLER;
    setup_data[USB_REQ_VALUE_L ] = SetTemp & 0xFF;
    setup_data[USB_REQ_VALUE_H ] = (SetTemp >> 8) & 0xFF;
    if (SetStatus)
        setup_data[USB_REQ_INDEX_L ] = 1;
    else
        setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, 8, &bytes_rw, NULL);
    if (bytes_rw) {
        status = ReadFile(sxHandle, setup_data, 3, &bytes_rw, NULL);
        *RetTemp = (setup_data[1] * 256) + setup_data[0];
        if (setup_data[2])
            *RetStatus = 1;
        else
            *RetStatus = 0;
        status = 1;
    }
    else
        status = 0;
    return (status);
}

Boolean sxSetShutter(void* sxHandle, UInt16 state )
{
    // Sets the mechanical shutter to open (state=0) or closed (state=1)
    // No error checking if the shutter does not exist
    
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR;
    setup_data[USB_REQ         ] = SXUSB_SHUTTER;
    setup_data[USB_REQ_VALUE_L ] = 0;
    if (state)
        setup_data[USB_REQ_VALUE_H ] = 128;
    else
        setup_data[USB_REQ_VALUE_H ] = 64;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, 8, &bytes_rw, NULL);
    if (bytes_rw) {
        ReadFile(sxHandle, setup_data, 2, &bytes_rw, NULL);
        if (bytes_rw)
            return (setup_data[1] * 256) + setup_data[0];
        else
            return 0;
    }
    else
        return 0;
}

Boolean sxSetSTAR2000(void* sxHandle, UInt8 star2k)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_SET_STAR2K;
    setup_data[USB_REQ_VALUE_L ] = star2k;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

Boolean sxSetSerialPort(void* sxHandle, UInt16 portIndex, UInt16 property, UInt32 value)
{
    (void)sxHandle;
    (void)portIndex;
    (void)property;
    (void)value;
    return (1);
}

UInt16 sxGetSerialPort(void* sxHandle, UInt16 portIndex, UInt16 property)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_GET_SERIAL;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)property;
    setup_data[USB_REQ_VALUE_H ] = property >> 8;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)portIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 2;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 2, &bytes_rw, NULL);
    return (setup_data[0] | (setup_data[1] << 8));
}

Boolean sxWriteSerialPort(void* sxHandle, UInt16 portIndex, UInt16 flush, UInt16 count, UInt8 *data)
{
    UInt8 setup_data[72];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_WRITE_SERIAL_PORT;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)flush;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)portIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = (UInt8)count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    memcpy(setup_data + USB_REQ_DATA, data, count);
    return (WriteFile(sxHandle, setup_data, USB_REQ_DATA + count, &bytes_rw, NULL));
}

Boolean sxReadSerialPort(void* sxHandle, UInt16 portIndex, UInt16 count, UInt8 *data)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_READ_SERIAL_PORT;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)portIndex;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = (UInt8)count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    return (ReadFile(sxHandle, data, count, &bytes_rw, NULL));
}

Boolean sxSetCameraModel(void* sxHandle, UInt16 model)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_CAMERA_MODEL;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)model;
    setup_data[USB_REQ_VALUE_H ] = model >> 8;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 0;
    setup_data[USB_REQ_LENGTH_H] = 0;
    return (WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL));
}

UInt16 sxGetCameraModel(void* sxHandle)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw = 0;
    UInt16 model = 0xffff;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_CAMERA_MODEL;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 2;
    setup_data[USB_REQ_LENGTH_H] = 0;
    if(!WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL)){
        fprintf(stderr,"SXMacLib: Error writing in sxGetCameraModel, wrote %d bytes\n",(unsigned int)bytes_rw);
    }
    else if(!ReadFile(sxHandle, setup_data, 2, &bytes_rw, NULL)){
        fprintf(stderr,"SXMacLib: Error reading in sxGetCameraModel, read %d bytes\n",(unsigned int)bytes_rw);
    }
    else {
        model = (setup_data[0] | (setup_data[1] << 8));
    }
    
    return model;
}

Boolean sxWriteEEPROM(void* sxHandle, UInt16 address, UInt16 count, UInt8 *data, UInt16 admin_code)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAOUT;
    setup_data[USB_REQ         ] = SXUSB_LOAD_EEPROM;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)address;
    setup_data[USB_REQ_VALUE_H ] = address >> 8;
    setup_data[USB_REQ_INDEX_L ] = (UInt8)admin_code;
    setup_data[USB_REQ_INDEX_H ] = admin_code >> 8;
    setup_data[USB_REQ_LENGTH_L] = (UInt8)count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    memcpy(setup_data + USB_REQ_DATA, data, count);
    return (WriteFile(sxHandle, setup_data, USB_REQ_DATA + count, &bytes_rw, NULL));
}

Boolean sxReadEEPROM(void* sxHandle, UInt16 address, UInt16 count, UInt8 *data)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_LOAD_EEPROM;
    setup_data[USB_REQ_VALUE_L ] = (UInt8)address;
    setup_data[USB_REQ_VALUE_H ] = address >> 8;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = (UInt8)count;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    return (ReadFile(sxHandle, data, count, &bytes_rw, NULL));
}

UInt32 sxGetFirmwareVersion(void* sxHandle)
{
    unsigned char setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_GET_FIRMWARE_VERSION;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 4;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 4, &bytes_rw, NULL);
    return ((UInt32)setup_data[0] | ((UInt32)setup_data[1] << 8) | ((UInt32)setup_data[2] << 16) | ((UInt32)setup_data[3] << 24));
}

UInt16 sxGetBuildNumber(void* sxHandle)
{
    UInt8 setup_data[8];
    UInt32 bytes_rw;
    
    setup_data[USB_REQ_TYPE    ] = USB_REQ_VENDOR | USB_REQ_DATAIN;
    setup_data[USB_REQ         ] = SXUSB_BUILD_NUMBER;
    setup_data[USB_REQ_VALUE_L ] = 0;
    setup_data[USB_REQ_VALUE_H ] = 0;
    setup_data[USB_REQ_INDEX_L ] = 0;
    setup_data[USB_REQ_INDEX_H ] = 0;
    setup_data[USB_REQ_LENGTH_L] = 4;
    setup_data[USB_REQ_LENGTH_H] = 0;
    WriteFile(sxHandle, setup_data, sizeof(setup_data), &bytes_rw, NULL);
    ReadFile(sxHandle, setup_data, 4, &bytes_rw, NULL);
    return ((UInt32)setup_data[0] | (UInt32)(setup_data[1] << 8));
}

static libusb_context *ctx = NULL;

static Boolean isSXCamera(struct libusb_device_descriptor* desc)
{
    if (desc){
        const sxDeviceInfo* info = sxLookupDeviceInfo(desc->idVendor,desc->idProduct);
        if (info && info->type == sxDeviceTypeCamera) {
            printf("SXMacLib: Found SX device Vendor/Product %04x/%04x\n", desc->idVendor, desc->idProduct);
            return true;
        }
        printf("SXMacLib: Skip device Vendor/Product %04x/%04x\n", desc->idVendor, desc->idProduct);
    }
    return false;
}

UInt32 sxOpen(void** sxHandles)
{
    int i, num_dev;
    int ret = 0;
    int count = 0;
    libusb_device **device;
    
    if (!ctx) {
        ret = libusb_init(&ctx);
        printf("Initialising SXMacLib\n");
    }
    if (ret) {
        goto out;
    }
    
    num_dev = libusb_get_device_list(ctx, &device);
    if (num_dev < 0) {
        goto out;
    }
    
    for (i = 0; i < num_dev; i++) {
        struct libusb_device_descriptor desc;
        ret = libusb_get_device_descriptor(device[i], &desc);
        if (ret < 0) {
            goto err_desc;
        }
        if (isSXCamera(&desc)) {
            libusb_device_handle* handle;

            ret = libusb_open(device[i], &handle);
            if (0 == ret) {

#if defined (__linux__)
                if (libusb_kernel_driver_active(handle, 0) == 1) {
                    ret = libusb_detach_kernel_driver(handle, 0);
                    if (ret == 0) {
                        printf("SXMacLib: Kernel driver detached.\n");
                    } else {
                        fprintf(stderr,"SXMacLib: Error detaching kernel driver.\n");
                    }
                }

                struct libusb_config_descriptor *config;
                ret = libusb_get_config_descriptor(device[i], 0, &config);
                if (ret == 0) {
                        printf("SXMacLib: Config descriptor read.\n");
                    } else {
                        fprintf(stderr,"SXMacLib: Error reading config descriptor.\n");
                }
                int interface = config->interface->altsetting->bInterfaceNumber;
#else
                // passing bInterfaceNumber doesn't work on a Mac so leave it as 0 as it was before.
                // This is probably a off by one error and we should be passing in config->interface->altsetting->bInterfaceNumber - 1
                const int interface = 0;
#endif
                
                ret = libusb_claim_interface(handle, interface);
                if (0 == ret) {
                    
                    const int model = sxGetCameraModel(handle);
                    if (0xFFFF == model) {
                        /* somehow prompt user for model? */
                    }
                    else {
                        
                        //printf("model = %0x\n", model);
                        sxHandles[count] = handle;
                        count++;
                    }
                } else {
                    fprintf(stderr,"SXMacLib: libusb_claim_interface error %s\n", libusb_error_name(ret));
                }
            }
            
            /* Lets look for a maximum of SXCCD_MAX_CAMS Devices */
            if (count >= SXCCD_MAX_CAMS) {
                break;
            }
        }
    }
    
    libusb_free_device_list(device, 1);
out:
    return count;
err_desc:
    count = 0;
    goto out;
}

void sxClose(void* sxHandle)
{
    if (sxHandle){
        libusb_close(sxHandle);
    }
}

// SCT additions

static sxDeviceInfo infos[] = {
    {0x0507,"Lodestar",sxDeviceTypeCamera},
    {0x0509,"Superstar",sxDeviceTypeCamera},
    {0x0517,"CoStar",sxDeviceTypeCamera},
    {0x0525,"Ultrastar",sxDeviceTypeCamera},
    {0x0398,"H814c",sxDeviceTypeCamera},
    {0x0198,"H814",sxDeviceTypeCamera},
    {0x0105,"SXVF-M5",sxDeviceTypeCamera},
    {0x0305,"SXVF-M5C",sxDeviceTypeCamera},
    {0x0107,"SXVF-M7",sxDeviceTypeCamera},
    {0x0308,"SXVF-M8C",sxDeviceTypeCamera},
    {0x0109,"SXVF-M9",sxDeviceTypeCamera},
    {0x0325,"SXVR-M25C",sxDeviceTypeCamera},
    {0x0326,"SXVR-M26C",sxDeviceTypeCamera},
    {0x0128,"SXVR-H18",sxDeviceTypeCamera},
    {0x0126,"SXVR-H16",sxDeviceTypeCamera},
    {0x0135,"SXVR-H35",sxDeviceTypeCamera},
    {0x0136,"SXVR-H36",sxDeviceTypeCamera},
    {0x0119,"SXVR-H9",sxDeviceTypeCamera},
    {0x0319,"SXVR-H9C",sxDeviceTypeCamera},
    {0x0194,"SX-694", sxDeviceTypeCamera},
    {0x0374,"SXVR-H674",sxDeviceTypeCamera},
    {0x0920,"Filter Wheel",sxDeviceTypeFilterWheel}
};

const sxDeviceInfo* sxLookupDeviceInfo(UInt16 vid,UInt16 pid)
{
    if (VENDOR_ID == vid){
        int i;
        for (i = 0; i < sizeof(infos)/sizeof(infos[0]); ++i){
            if (infos[i].pid == pid){
                return &infos[i];
            }
        }
    }
    return NULL;
}

int sxOpen2(void** sxHandles,const int sxHandlesCount)
{
    int i, num_dev;
    int ret = 0;
    int count = 0;
    libusb_device **device;
    
    if (!ctx) {
        ret = libusb_init(&ctx);
    }
    if (ret) {
        goto out;
    }
    
    num_dev = libusb_get_device_list(ctx, &device);
    if (num_dev < 0) {
        goto out;
    }
    
    for (i = 0; i < num_dev; i++) {
        
        struct libusb_device_descriptor desc;
        ret = libusb_get_device_descriptor(device[i], &desc);
        if (ret < 0) {
            goto err_desc;
        }
        
        if (isSXCamera(&desc)) {
            
            // check to see if we've already got this device in the array
            Boolean found = false;
            int j;
            const uint8_t bus = libusb_get_bus_number(device[i]);
            const uint8_t address = libusb_get_device_address(device[i]);
            for (j = 0; j < sxHandlesCount && !found; ++j){
                libusb_device_handle* handle = (libusb_device_handle*)sxHandles[j];
                if (handle){
                    libusb_device* device = libusb_get_device(handle);
                    if (device){
                        found = (bus == libusb_get_bus_number(device) && address == libusb_get_device_address(device));
                    }
                }
            }
            
            if (!found){
                
                libusb_device_handle* handle;
                ret = libusb_open(device[i], &handle);
                if (0 == ret) {
                    
                    ret = libusb_claim_interface(handle, 0);
                    if (0 == ret) {
                        
                        const int model = sxGetCameraModel(handle);
                        if (0xFFFF == model) {
                            /* somehow prompt user for model? */
                        }
                        else {
                            
                            //printf("model = %0x\n", model);
                            
                            // find next free slot
                            int j = 0;
                            for (; j < sxHandlesCount; ++j){
                                if (!sxHandles[j]){
                                    sxHandles[j] = handle;
                                    break;
                                }
                            }
                            /* Lets look for a maximum of SXCCD_MAX_CAMS Devices */
                            if (j >= SXCCD_MAX_CAMS) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    libusb_free_device_list(device, 1);
    
    // calculate the count
    int j;
    for (j = 0; j < sxHandlesCount; ++j){
        if (sxHandles[j]) ++count;
    }
    
out:
    return count;
err_desc:
    count = 0;
    goto out;
}


////////////////////////////////////////////////////////////////////////////////////////
// Open the first Sx camera found with the matching model number
void* sxOpenByModel(UInt16 nModelNumber)
{
    int i, num_dev;
    int ret = 0;
    libusb_device **device;
    
    if (!ctx)
        ret = libusb_init(&ctx);
    
    if (ret)
        return NULL;
    
    num_dev = libusb_get_device_list(ctx, &device);
    if (num_dev < 0) {
        return NULL;
    }
    
    // Look through the devices and find a vendor ID for SX, check for the
    // specified model number
    for (i = 0; i < num_dev; i++) {
        
        struct libusb_device_descriptor desc;
        ret = libusb_get_device_descriptor(device[i], &desc);
        
        // Something bad has happend, bail out, bail out...
        if (ret < 0)
            continue;
        
        // If it's a starlight Xpress device, try and open and see what the
        if(desc.idVendor == VENDOR_ID) {
            
            libusb_device_handle* handle;
            ret = libusb_open(device[i], &handle);
            if (0 != ret)
                continue;
            
#ifndef SB_LINUX_BUILD
            ret = libusb_claim_interface(handle, 0);
            if (0 != ret)
                continue;
#endif
            // Look for the requested model, or the color version of it
            const int model = sxGetCameraModel(handle);
            if (nModelNumber == model || (nModelNumber + 0x80) == model){
                libusb_free_device_list(device, 1);
                return handle;
            }
            
            // Return to the pile
            libusb_close(handle);
        }
    }
    
    libusb_free_device_list(device, 1);
    
    return NULL;
}

void sxSetTimeoutMS(UInt32 timeoutMS)
{
    printf("SXMacLib: setting timeout to %ldms\n",timeoutMS);
    ioTimeout = timeoutMS;
}

#endif // __APPLE__

