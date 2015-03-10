/***************************************************************************\

    Copyright (c) 2003 David Schmenk

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


#ifdef __cplusplus
#define DLL_EXPORT                    extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT                    __declspec(dllexport)
#endif

//#define DLL_EXPORT static __declspec(dllexport)

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
/* The version of this file we got from SX has SXCCD_MAX_CAMS as 2, but
   sxOpen() can return more than that! We'll set it to 20 to avoid stack
   overruns when we allocate arrays of HANDLE on the stack to pass to
   sxOpen() - ag 2015/03/06 */
#define SXCCD_MAX_CAMS                  20
struct t_sxccd_params
{
    USHORT hfront_porch;
    USHORT hback_porch;
    USHORT width;
    USHORT vfront_porch;
    USHORT vback_porch;
    USHORT height;
    float  pix_width;
    float  pix_height;
    USHORT color_matrix;
    BYTE   bits_per_pixel;
    BYTE   num_serial_ports;
    BYTE   extra_caps;
    BYTE   vclk_delay;
};
/*
 * Prototypes.
 */
DLL_EXPORT LONG sxReset(HANDLE sxHandle);
DLL_EXPORT LONG sxClearPixels(HANDLE sxHandle, USHORT flags, USHORT camIndex);
DLL_EXPORT LONG sxLatchPixels(HANDLE sxHandle, USHORT flags, USHORT camIndex, USHORT xoffset, USHORT yoffset, USHORT width, USHORT height, USHORT xbin, USHORT ybin);
DLL_EXPORT LONG sxExposePixels(HANDLE sxHandle, USHORT flags, USHORT camIndex, USHORT xoffset, USHORT yoffset, USHORT width, USHORT height, USHORT xbin, USHORT ybin, ULONG msec);
DLL_EXPORT LONG sxReadPixels(HANDLE sxHandle, USHORT *pixels, ULONG count);
DLL_EXPORT LONG sxSetShutter(HANDLE sxHandle, USHORT state);
DLL_EXPORT ULONG sxSetTimer(HANDLE sxHandle, ULONG msec);
DLL_EXPORT ULONG sxGetTimer(HANDLE sxHandle);
DLL_EXPORT ULONG sxGetCameraParams(HANDLE sxHandle, USHORT camIndex, struct t_sxccd_params *params);
DLL_EXPORT ULONG sxSetSTAR2000(HANDLE sxHandle, BYTE star2k);
DLL_EXPORT ULONG sxSetSerialPort(HANDLE sxHandle, USHORT portIndex, USHORT property, ULONG value);
DLL_EXPORT USHORT sxGetSerialPort(HANDLE sxHandle, USHORT portIndex, USHORT property);
DLL_EXPORT ULONG sxWriteSerialPort(HANDLE sxHandle, USHORT camIndex, USHORT flush, USHORT count, BYTE *data);
DLL_EXPORT ULONG sxReadSerialPort(HANDLE sxHandle, USHORT camIndex, USHORT count, BYTE *data);
DLL_EXPORT USHORT sxGetCameraModel(HANDLE sxHandle);
DLL_EXPORT ULONG sxReadEEPROM(HANDLE sxHandle, USHORT address, USHORT count, BYTE *data);
DLL_EXPORT ULONG sxGetFirmwareVersion(HANDLE sxHandle);
DLL_EXPORT int sxOpen(HANDLE *sxHandles);
DLL_EXPORT void sxClose(HANDLE sxHandle);
DLL_EXPORT ULONG sxSetCooler(HANDLE sxHandle, UCHAR SetStatus, USHORT SetTemp, UCHAR *RetStatus, USHORT *RetTemp );

#ifdef SXCCD_DANGEROUS
DLL_EXPORT ULONG sxSetCameraParams(HANDLE sxHandle, USHORT camIndex, struct t_sxccd_params *params);
DLL_EXPORT ULONG sxSetCameraModel(HANDLE sxHandle, USHORT model);
DLL_EXPORT ULONG sxWriteEEPROM(HANDLE sxHandle, USHORT address, USHORT count, BYTE *data, USHORT admin_code);
#endif
