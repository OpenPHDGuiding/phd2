/*
 *  cam_firewire_OSX.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2007-2010 Craig Stark.
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

#if defined (__APPLE__)
#include "phd.h"
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include <wx/textdlg.h>

// Deal with 8-bit vs. 16-bit??
// Take care of gain capability

bool DCAM_flush_mode = true;
bool DCAM_start_stop_mode = true;

#include "cam_firewire.h"

Camera_FirewireClass::Camera_FirewireClass() {
    Connected = false;
//  HaveBPMap = false;
//  NBadPixels=-1;
    Name=_T("The Imaging Source Firewire");
    FullSize = wxSize(1280,1024);
    HasGainControl = true;
    m_hasGuideOutput = false;
}

bool Camera_FirewireClass::Connect() {
    int err, CamNum;
    uint32_t i, NCams;
    dc1394camera_t **cameras;
    dc1394video_mode_t vidmode;

    err = dc1394_find_cameras(&cameras, &NCams);

    if ((err != DC1394_SUCCESS) && (err != DC1394_NO_CAMERA)) {
        wxMessageBox(_T("Error looking for Firewire / IEEE1394 cameras"));
        return true;
    }
    if (NCams == 0) {
        wxMessageBox(_T("No Firewire / IEEE1394 camera found"));
        return true;
    }

    // Choose camera
    CamNum = 0;
    if (NCams > 1) {
        wxArrayString CamNames;
        for (i=0; i<NCams; i++)
            CamNames.Add(wxString(cameras[i]->model));
        CamNum = wxGetSingleChoiceIndex(_T("Select Firewire camera"),("Camera name"),CamNames);
        if (CamNum == -1) {
            for (i=0; i<NCams; i++)
                dc1394_free_camera(cameras[i]);
            free(cameras);
            return true;
        }
    }
    camera = cameras[CamNum];

    // Free unused cameras
    for (i=0; i<NCams; i++)
        if (CamNum != i)
            dc1394_free_camera(cameras[i]);
    free(cameras);


    // Get the highest-res mono mode
    uint32_t w, h, max;
    dc1394video_modes_t video_modes;
    dc1394_video_get_supported_modes(camera,&video_modes);
    dc1394color_coding_t coding;
    bool HaveValid = false;

    w=h=max=0;
    for (i=0; i<video_modes.num; i++) {
        dc1394_get_image_size_from_video_mode(camera,video_modes.modes[i],&w,&h);
        dc1394_get_color_coding_from_video_mode(camera,video_modes.modes[i], &coding);
        if (coding == DC1394_COLOR_CODING_MONO8) {
            if ( (w*h) > max) {
                max = w*h;
                HaveValid = true;
                vidmode = video_modes.modes[i];
            }
        }
    }
    if (!HaveValid) {
        wxMessageBox(_T("Cannot find a suitable monochrome video mode"));
        dc1394_free_camera(camera);
        return true;
    }

    if (dc1394_video_set_framerate(camera,DC1394_FRAMERATE_7_5)) {
        wxMessageBox(_T("Cannot set to 7.5 FPS"));
        dc1394_free_camera(camera);
        return true;
    }

    // Set to 400Mbps mode
    dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);

    // Engage video mode
    dc1394_video_set_mode(camera,vidmode);

    // Setup DMA buffers for capture
    if (dc1394_capture_setup(camera,4,DC1394_CAPTURE_FLAGS_DEFAULT)) {
        wxMessageBox(_T("Cannot setup DMA buffers"));
        dc1394_free_camera(camera);
        return true;
    }

    wxStopWatch swatch;
    long t1;
    // Startup transmission to make sure we can do so
    swatch.Start();
    if (dc1394_video_set_transmission(camera, DC1394_ON) !=DC1394_SUCCESS) {
        wxMessageBox(_T("Cannot start transmission"));
        dc1394_free_camera(camera);
        return true;
    }
    dc1394switch_t status = DC1394_OFF;
    for (i=0; i<5; i++) {
        wxMilliSleep(50);
        dc1394_video_get_transmission(camera, &status);
        if (status != DC1394_OFF) break;
    }
    t1 = swatch.Time();
    if (i==5) {
        wxMessageBox(_T("Transmission failed to start"));
        dc1394_free_camera(camera);
    }
//  dc1394_video_set_transmission(camera, DC1394_OFF); // turn it back off for now

//  dc1394_feature_set_value(camera,DC1394_FEATURE_EXPOSURE,0);  // turn off auto-exposure

/*  wxMilliSleep(500);
    dc1394video_frame_t *vframe;

    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_WAIT, &vframe);
    dc1394_capture_enqueue(camera, vframe);
    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_WAIT, &vframe);
    dc1394_capture_enqueue(camera, vframe);
    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_WAIT, &vframe);
    dc1394_capture_enqueue(camera, vframe);
*/

    // Setup my params
    dc1394_get_image_size_from_video_mode(camera,vidmode,&w,&h);
    FullSize = wxSize((int)w, (int)h);
    Name = wxString(camera->model);

//  wxMessageBox(Name + wxString::Format(" - %d x %d - %ld ms",FullSize.GetWidth(),FullSize.GetHeight(),t1));
//  dc1394feature_info_t feature;

    // set shutter speed mode
    dc1394bool_t    Present;
    dc1394_feature_has_absolute_control(camera,DC1394_FEATURE_SHUTTER,&Present);
    if (Present != DC1394_TRUE) {
        wxMessageBox("Cannot use absolute values to set exposures.  Exposure durations will not be controlled properly");
    }
    dc1394_feature_set_mode(camera, DC1394_FEATURE_SHUTTER, DC1394_FEATURE_MODE_MANUAL);
    dc1394_feature_set_absolute_control(camera,DC1394_FEATURE_SHUTTER,DC1394_ON);
    dc1394_feature_set_absolute_value(camera,DC1394_FEATURE_SHUTTER,1.0);

    // Set gain to manual control
    dc1394_feature_set_mode(camera,DC1394_FEATURE_GAIN,DC1394_FEATURE_MODE_MANUAL);

    if (DCAM_start_stop_mode)
        dc1394_video_set_transmission(camera, DC1394_OFF);

/*  int ans = wxMessageBox("Enable flushing of buffer on each capture?",_T("Flush mode?"),wxYES_NO);
    if (ans == wxYES)
        DCAM_flush_mode = true;
    else
        DCAM_flush_mode = false;
*/
    Connected = true;
    return false;
}

void Camera_FirewireClass::InitCapture() {
    // Set gain
    uint32_t Min;
    uint32_t Max;
    uint32_t NewVal;

    dc1394_feature_get_boundaries(camera,DC1394_FEATURE_GAIN,&Min,&Max);
    NewVal = GuideCameraGain * (Max - Min) / 100 + Min;
    dc1394_feature_set_value(camera,DC1394_FEATURE_GAIN, NewVal);
}

bool Camera_FirewireClass::Disconnect() {
    if (camera) {
        dc1394_video_set_transmission(camera,DC1394_OFF);
        dc1394_free_camera(camera);
    }
    Connected = false;
    return false;
}

bool Camera_FirewireClass::Capture(int duration, usImage& img, int options, const wxRect& subframe)
{
    int xsize, ysize, i;
    unsigned short *dataptr;
    unsigned char *imgptr;
    dc1394video_frame_t *vframe;
    int err;
    static int programmed_dur = 1000;
    wxStopWatch swatch;

    xsize = FullSize.GetWidth();
    ysize = FullSize.GetHeight();

    if (img.NPixels != (xsize*ysize)) {
        if (img.Init(xsize,ysize)) {
            wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
            return true;
        }
    }
    swatch.Start();
    dataptr = img.ImageData;

    if (DCAM_start_stop_mode) {
        dc1394_video_set_transmission(camera, DC1394_ON);
        dc1394switch_t status = DC1394_OFF;
        for (i=0; i<5; i++) {
            wxMilliSleep(10);
            dc1394_video_get_transmission(camera, &status);
            if (status != DC1394_OFF) break;
        }
    }

    // Dequeue any existing
/*  dc1394_video_set_transmission(camera,DC1394_OFF);
    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &frame);
    if (pFrame->frames_behind) dc1394_capture_enqueue(camera, frame);
    while (pFrame->frames_behind) {
        dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &frame);
        dc1394_capture_enqueue(camera, frame);
    }*/
    if (duration != programmed_dur) {
        dc1394_feature_set_absolute_value(camera,DC1394_FEATURE_SHUTTER,(float) (duration) / 1000.0);
        programmed_dur = duration;
    }


    // Flush
    if (DCAM_flush_mode) {
        vframe = NULL;
        dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &vframe);
        if (vframe) dc1394_capture_enqueue(camera, vframe);
        dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &vframe);
        if (vframe) dc1394_capture_enqueue(camera, vframe);
        dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &vframe);
        if (vframe) dc1394_capture_enqueue(camera, vframe);
        dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &vframe);
        if (vframe) dc1394_capture_enqueue(camera, vframe);
    }

    /*
    while (vframe && vpFrame->frames_behind) {
        pFrame->SetStatusText(wxString::Format("%d %d %d", (int) vpFrame->size[0], (int) vpFrame->image_bytes, (int) vpFrame->frames_behind));
        dc1394_capture_enqueue(camera, vframe);
        dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &vframe);
    }*/

/*  dc1394_video_set_transmission(camera, DC1394_ON);
    dc1394switch_t status = DC1394_OFF;
    for (i=0; i<5; i++) {
        wxMilliSleep(50);
        dc1394_video_get_transmission(camera, &status);
        if (status != DC1394_OFF) break;
    }*/

/*
    // Grab a short frame
    err = dc1394_feature_set_absolute_value(camera,DC1394_FEATURE_SHUTTER,0.001);
    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_WAIT, &vframe);
    dc1394_capture_enqueue(camera, vframe);
    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_WAIT, &vframe);
    dc1394_capture_enqueue(camera, vframe);
    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_WAIT, &vframe);
    dc1394_capture_enqueue(camera, vframe);

*/

/*  wxMilliSleep(duration - 20);
    // Flush
    vframe = NULL;
    dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &vframe);
    if (vframe) dc1394_capture_enqueue(camera, vframe);
    while (vframe) {
        dc1394_capture_dequeue(camera,DC1394_CAPTURE_POLICY_POLL, &vframe);
        if (vframe) dc1394_capture_enqueue(camera, vframe);
    }
*/

    // grab the next frame
    if (dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &vframe) != DC1394_SUCCESS) {
        DisconnectWithAlert(_("Cannot get a frame from the queue"));
        return true;
    }
    imgptr = vframe->image;
//  pFrame->SetStatusText(wxString::Format("%d %d %d",(int) vpFrame->frames_behind, (int) vpFrame->size[0], (int) vpFrame->size[1]));
    for (i=0; i<img.NPixels; i++, dataptr++, imgptr++)
        *dataptr = (unsigned short) *imgptr;
    dc1394_capture_enqueue(camera, vframe);  // release this frame
//  pFrame->SetStatusText(wxString::Format("Behind: %lu Pos: %lu",vpFrame->frames_behind,vpFrame->id));
    if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

    if (DCAM_start_stop_mode)
        dc1394_video_set_transmission(camera,DC1394_OFF);

    return false;
}

bool Camera_FirewireClass::HasNonGuiCapture(void)
{
    return true;
}

#endif
