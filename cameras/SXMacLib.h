/***************************************************************************\
 
 Copyright (c) 2004 David Schmenk
 
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

//#include <MacTypes.h>
typedef unsigned short UInt16;
typedef unsigned char Boolean;
typedef unsigned long UInt32;
typedef unsigned char UInt8;

/***************************************************************************\
 *                                                                           *
 *                       USB basic control macros                            *
 *                                                                           *
 \***************************************************************************/
/*
 * CCD color representation.
 *  Packed colors allow individual sizes up to 16 bits.
 *  2X2 matrix bits are represented as:
 *      0 1
 *      2 3
 */
#define SXCCD_COLOR_PACKED_RGB          0x8000
#define SXCCD_COLOR_PACKED_BGR          0x4000
#define SXCCD_COLOR_PACKED_RED_SIZE     0x0F00
#define SXCCD_COLOR_PACKED_GREEN_SIZE   0x00F0
#define SXCCD_COLOR_PACKED_BLUE_SIZE    0x000F
#define SXCCD_COLOR_MATRIX_ALT_EVEN     0x2000
#define SXCCD_COLOR_MATRIX_ALT_ODD      0x1000
#define SXCCD_COLOR_MATRIX_2X2          0x0000
#define SXCCD_COLOR_MATRIX_RED_MASK     0x0F00
#define SXCCD_COLOR_MATRIX_GREEN_MASK   0x00F0
#define SXCCD_COLOR_MATRIX_BLUE_MASK    0x000F
#define SXCCD_COLOR_MONOCHROME          0x0FFF
/*
 * Caps bit definitions.
 */
#define SXCCD_CAPS_STAR2K               0x01
#define SXCCD_CAPS_COMPRESS             0x02
#define SXCCD_CAPS_EEPROM               0x04
#define SXCCD_CAPS_GUIDER               0x08
/*
 * CCD command options.
 */
#define SXCCD_EXP_FLAGS_FIELD_ODD       1
#define SXCCD_EXP_FLAGS_FIELD_EVEN      2
#define SXCCD_EXP_FLAGS_FIELD_BOTH      (SXCCD_EXP_FLAGS_FIELD_EVEN|SXCCD_EXP_FLAGS_FIELD_ODD)
#define SXCCD_EXP_FLAGS_FIELD_MASK      SXCCD_EXP_FLAGS_FIELD_BOTH
#define SXCCD_EXP_FLAGS_NOBIN_ACCUM     4
#define SXCCD_EXP_FLAGS_NOWIPE_FRAME    8
#define SXCCD_EXP_FLAGS_TDI             32
#define SXCCD_EXP_FLAGS_NOCLEAR_FRAME   64
/*
 * Serial port queries.
 */
#define SXCCD_SERIAL_PORT_AVAIL_OUTPUT  0
#define SXCCD_SERIAL_PORT_AVAIL_INPUT   1

/*
 * Limits.
 */
#define	SXCCD_MAX_CAMS                 	20

/*
 * CCD parameters.
 */
struct sxccd_params_t
{
    UInt16 hfront_porch;
    UInt16 hback_porch;
    UInt16 width;
    UInt16 vfront_porch;
    UInt16 vback_porch;
    UInt16 height;
    float  pix_width;  // in microns
    float  pix_height; // in microns
    UInt16 color_matrix;
    UInt8   bits_per_pixel;
    UInt8   num_serial_ports;
    UInt8   extra_caps;
    UInt8   vclk_delay;
};

/*
 * Prototypes.
 */
#ifdef __cplusplus
extern "C" {
#endif
    
    Boolean   sxReset(void *device);
    Boolean   sxClearPixels(void *device, UInt16 flags, UInt16 camIndex);
    Boolean   sxLatchPixels(void *device, UInt16 flags, UInt16 camIndex, UInt16 xoffset, UInt16 yoffset, UInt16 width, UInt16 height, UInt16 xbin, UInt16 ybin);
    Boolean   sxExposePixels(void *device, UInt16 flags, UInt16 camIndex, UInt16 xoffset, UInt16 yoffset, UInt16 width, UInt16 height, UInt16 xbin, UInt16 ybin, UInt32 msec);
    UInt32      sxReadPixels(void *device, UInt16 *pixels, UInt32 count);
    Boolean   sxSetShutter(void *device, UInt16 state);
    Boolean   sxSetTimer(void *device, UInt32 msec);
    UInt32      sxGetTimer(void *device);
    Boolean   sxGetCameraParams(void *device, UInt16 camIndex, struct sxccd_params_t *params);
    Boolean   sxSetSTAR2000(void *device, UInt8 star2k);
    Boolean   sxSetSerialPort(void *device, UInt16 portIndex, UInt16 property, UInt32 value);
    UInt16      sxGetSerialPort(void *device, UInt16 portIndex, UInt16 property);
    Boolean   sxWriteSerialPort(void *device, UInt16 camIndex, UInt16 flush, UInt16 count, UInt8 *data);
    Boolean   sxReadSerialPort(void *device, UInt16 camIndex, UInt16 count, UInt8 *data);
    UInt16      sxGetCameraModel(void *device);
    UInt32      sxGetFirmwareVersion(void *device);
    UInt32      sxOpen(void** handles);
    void        sxClose(void *device);
    Boolean   sxSetCooler(void *device, UInt8 SetStatus, UInt16 SetTemp, UInt8 *RetStatus, UInt16 *RetTemp );
    
#ifdef SXCCD_DANGEROUS
    boolean_t   sxSetCameraParams(void *device, UInt16 camIndex, struct sxccd_params_t *params);
    boolean_t   sxSetCameraModel(void *device, UInt16 model);
    boolean_t   sxWriteEEPROM(void *device, UInt16 address, UInt16 count, UInt18 *data, UInt16 admin_code);
    boolean_t   sxReadEEPROM(void *device, UInt16 address, UInt16 count, UInt18 *data);
#endif
    
    // SCT additions
    
    typedef enum {
        sxDeviceTypeCamera = 1,
        sxDeviceTypeFilterWheel = 2
    } sxDeviceType;
    
    typedef struct sxDeviceInfo { // forward declare and provide accessors ? always going to be compiled in so probably not worth it...
        UInt16 pid;
        const char* name;
        sxDeviceType type;
    } sxDeviceInfo;
    
    const sxDeviceInfo* sxLookupDeviceInfo(UInt16 vid,UInt16 pid);
    
    int sxOpen2(void** sxHandles,const int sxHandlesCount); // bitmap param to filter which kind of device to open ?
    
    void* sxOpenByModel(UInt16 nModelNumber);
    
    void sxSetTimeoutMS(UInt32 timeoutMS);
    
#ifdef __cplusplus
}
#endif

#endif // __APPLE__ || __linux__
