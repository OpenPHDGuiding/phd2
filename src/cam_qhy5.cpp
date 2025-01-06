/*
 *  cam_qhy5.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2012 Craig Stark.
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

#if defined(CAM_QHY5)

# include <libusb.h>
# include "camera.h"
# include "image_math.h"

# include "cam_qhy5.h"

# define QHY5_MATRIX_WIDTH 1558
# define QHY5_MATRIX_HEIGHT 1048
# define QHY5_BUFFER_SIZE (QHY5_MATRIX_WIDTH * (QHY5_MATRIX_HEIGHT + 2))

# define QHY5_IMAGE_WIDTH 1280
# define QHY5_IMAGE_HEIGHT 1024

# define QHY5_VID 0x16c0
# define QHY5_PID 0x296d
# define STORE_WORD_BE(var, val)                                                                                               \
     *(var) = ((val) >> 8) & 0xff;                                                                                             \
     *((var) + 1) = (val) & 0xff

static unsigned char reg[19];
static int gain_lut[74] = { 0x000, 0x004, 0x005, 0x006, 0x007, 0x008, 0x009, 0x00A, 0x00B, 0x00C, 0x00D, 0x00E, 0x00F,
                            0x010, 0x011, 0x012, 0x013, 0x014, 0x015, 0x016, 0x017, 0x018, 0x019, 0x01A, 0x01B, 0x01C,
                            0x01D, 0x01E, 0x01F, 0x051, 0x052, 0x053, 0x054, 0x055, 0x056, 0x057, 0x058, 0x059, 0x05A,
                            0x05B, 0x05C, 0x05D, 0x05E, 0x05F, 0x6CE, 0x6CF, 0x6D0, 0x6D1, 0x6D2, 0x6D3, 0x6D4, 0x6D5,
                            0x6D6, 0x6D7, 0x6D8, 0x6D9, 0x6DA, 0x6DB, 0x6DC, 0x6DD, 0x6DE, 0x6DF, 0x6E0, 0x6E1, 0x6E2,
                            0x6E3, 0x6E4, 0x6E5, 0x6E6, 0x6E7, 0x6FC, 0x6FD, 0x6FE, 0x6FF };

static libusb_device_handle *m_handle = NULL;

static bool s_libusb_init_done;

static bool init_libusb()
{
    if (s_libusb_init_done)
        return false;
    int ret = libusb_init(0);
    if (ret != 0)
        return true;
    s_libusb_init_done = true;
    return false;
}

static void uninit_libusb()
{
    if (s_libusb_init_done)
    {
        libusb_exit(0);
        s_libusb_init_done = false;
    }
}

CameraQHY5::CameraQHY5()
{
    Connected = FALSE;
    FullSize = wxSize(QHY5_IMAGE_WIDTH, QHY5_IMAGE_HEIGHT);
    m_hasGuideOutput = true;
    HasGainControl = true;
    RawBuffer = NULL;
    Name = _T("QHY 5");
}

CameraQHY5::~CameraQHY5()
{
    uninit_libusb();
}

wxByte CameraQHY5::BitsPerPixel()
{
    return 8;
}

bool CameraQHY5::Connect(const wxString& camId)
{
    if (init_libusb())
    {
        wxMessageBox(_("Could not initialize USB library"), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    m_handle = libusb_open_device_with_vid_pid(NULL, QHY5_VID, QHY5_PID);
    if (m_handle == NULL)
    {
        wxMessageBox(_T("Libusb failed to open camera QHY5."), _("Error"), wxOK | wxICON_ERROR);
        return true;
    }

    if (libusb_kernel_driver_active(m_handle, 0))
        libusb_detach_kernel_driver(m_handle, 0);

    libusb_set_configuration(m_handle, 1);
    libusb_claim_interface(m_handle, 0);

    if (RawBuffer)
        delete[] RawBuffer;

    RawBuffer = new unsigned char[QHY5_BUFFER_SIZE];

    Connected = true;
    return false;
}

bool CameraQHY5::ST4PulseGuideScope(int direction, int duration)
{
    int result = -1;
    int reg = 0;
    int32_t dur[2] = { -1, -1 };

    // Max guide pulse is 2.54s -- 255 keeps it on always
    if (duration > 2540)
        duration = 2540;

    switch (direction)
    {
    case WEST:
        reg = 0x80;
        dur[0] = duration;
        break; // 0111 0000
    case NORTH:
        reg = 0x40;
        dur[1] = duration;
        break; // 1011 0000
    case SOUTH:
        reg = 0x20;
        dur[1] = duration;
        break; // 1101 0000
    case EAST:
        reg = 0x10;
        dur[0] = duration;
        break; // 1110 0000
    default:
        return true; // bad direction passed in
    }
    result = libusb_control_transfer(m_handle, 0x42, 0x10, 0, reg, (unsigned char *) dur, sizeof(dur), 5000);
    wxMilliSleep(duration + 10);
    return result < 0 ? true : false;
}

void CameraQHY5::InitCapture() { }

bool CameraQHY5::Disconnect()
{
    libusb_release_interface(m_handle, 0);
    libusb_close(m_handle);
    m_handle = NULL;

    Connected = false;
    if (RawBuffer)
        delete[] RawBuffer;
    RawBuffer = NULL;

    return false;
}

bool CameraQHY5::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    // Only does full frames still
    // static int last_dur = 0;
    static int last_gain = 60;
    static int first_time = 1;
    unsigned char *bptr;
    unsigned short *dptr;
    int x, y;
    int xsize = FullSize.GetWidth();
    int ysize = FullSize.GetHeight();
    int op_height = FullSize.GetHeight();
    // bool firstimg = true;
    unsigned char buffer[2]; // for debug purposes
    int offset, value, index;
    int gain, gain_val, gain_lut_sz = (int) (sizeof(gain_lut) / sizeof(int));
    int ret, result;

    if (img.Init(xsize, ysize))
    {
        DisconnectWithAlert(CAPT_FAIL_MEMORY);
        return true;
    }

    if (GuideCameraGain != last_gain)
    {
        op_height -= (op_height % 4);
        offset = (QHY5_MATRIX_HEIGHT - op_height) / 2;
        index = (QHY5_MATRIX_WIDTH * (op_height + 26)) >> 16;
        value = (QHY5_MATRIX_WIDTH * (op_height + 26)) & 0xffff;
        gain = (int) (73. * (GuideCameraGain / 100.));
        if (gain >= gain_lut_sz)
            gain = gain_lut_sz - 1;
        if (gain < 0)
            gain = 0;
        gain_val = gain_lut[gain];
        STORE_WORD_BE(reg + 0, gain_val);
        STORE_WORD_BE(reg + 2, gain_val);
        STORE_WORD_BE(reg + 4, gain_val);
        STORE_WORD_BE(reg + 6, gain_val);
        STORE_WORD_BE(reg + 8, offset);
        STORE_WORD_BE(reg + 10, 0);
        STORE_WORD_BE(reg + 12, op_height - 1);
        STORE_WORD_BE(reg + 14, 0x0521);
        STORE_WORD_BE(reg + 16, op_height + 25);
        reg[18] = 0xcc;
        if (libusb_control_transfer(m_handle, 0x42, 0x13, value, index, reg, sizeof(reg), 5000))
        {
            wxMilliSleep(2);
            if (libusb_control_transfer(m_handle, 0x42, 0x14, 0x31a5, 0, reg, 0, 5000))
            {
                wxMilliSleep(1);
                if (libusb_control_transfer(m_handle, 0x42, 0x16, 0, first_time, reg, 0, 5000))
                {
                    first_time = 0;
                }
            }
        }

        last_gain = GuideCameraGain;
    }

    index = duration >> 16;
    value = duration & 0xffff;
    buffer[0] = 0;
    buffer[1] = 100;
    libusb_control_transfer(m_handle, 0xc2, 0x12, value, index, buffer, 2, 5000);

    /* wait for exposure end */
    wxMilliSleep(duration);

    ret = libusb_bulk_transfer(m_handle, 0x82, RawBuffer, QHY5_BUFFER_SIZE, &result, 20000);
    if (ret < 0)
    {
        pFrame->Alert(_T("Failed to read image: libusb_bulk_transfer() failed."));
        return true;
    }

    if (result != QHY5_BUFFER_SIZE)
    {
        pFrame->Alert(_T("Failed to read image."));
        return true;
    }

    // bptr = RawBuffer;
    //  Load and crop from the 800 x 525 image that came in
    dptr = img.ImageData;
    for (y = 0; y < ysize; y++)
    {
        bptr = RawBuffer + QHY5_MATRIX_WIDTH * y + 20;
        for (x = 0; x < xsize; x++, bptr++, dptr++) // CAN SPEED THIS UP
        {
            *dptr = (unsigned short) *bptr;
        }
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(img);

    return false;
}

#endif
