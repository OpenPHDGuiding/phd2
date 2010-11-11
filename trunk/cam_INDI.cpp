/*
 *  cam_INI.cpp
 *  PHD Guiding
 *
 *  Created by Geoffrey Hausheer.
 *  Copyright (c) 2009 Geoffrey Hausheer.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include "camera.h"

#ifdef INDI_CAMERA
#include "time.h"
#include "image_math.h"
#include "cam_INDI.h"

#include "libindiclient/indi.h"
#include "libindiclient/indigui.h"

extern long INDIport;
extern wxString INDIhost;
extern struct indi_t *INDIClient;

void camera_capture_cb(struct indi_prop_t *iprop, void *data)
{
    Camera_INDIClass *cb = (Camera_INDIClass *)(data);
    cb->blob_elem = indi_find_first_elem(iprop);
    indi_dev_enable_blob(iprop->idev, FALSE);
    printf("Got camera callback\n");
    cb->modal = false;
}

Camera_INDIClass::Camera_INDIClass() {
    Connected = FALSE;
    Name=_T("INDI Camera");
    HasPropertyDialog = true;
    FullSize = wxSize(640,480);
}

static int modal_timeout_cb(void * data)
{
    Camera_INDIClass *cb = (Camera_INDIClass *)(data);
    if(cb->modal) {
        cb->modal = false;
    }
    return FALSE;
}
static void connect_cb(struct indi_prop_t *iprop, void *data)
{
    Camera_INDIClass *cb = (Camera_INDIClass *)(data);
    cb->is_connected = (iprop->state == INDI_STATE_IDLE || iprop->state == INDI_STATE_OK) && indi_prop_get_switch(iprop, "CONNECT");
    printf("Camera connected state: %d\n", cb->is_connected);
    cb->CheckState();
}

static void new_prop_cb(struct indi_prop_t *iprop, void *callback_data)
{
    Camera_INDIClass *cb = (Camera_INDIClass *)(callback_data);
    return cb->NewProp(iprop);
}

void Camera_INDIClass::CheckState()
{
    if(has_blob && is_connected && (expose_prop || video_prop)) {
        if (! ready) {
            printf("Camera is ready\n");
            ready = true;
            if (modal) {
                modal = false;
            }
        }
    }
}

void Camera_INDIClass::NewProp(struct indi_prop_t *iprop)
{
    /* property to set exposure */
    if (iprop->type == INDI_PROP_BLOB) {
        printf("Found BLOB property for camera %s\n", iprop->idev->name);
        has_blob = 1;
        indi_prop_add_cb(iprop, (IndiPropCB)camera_capture_cb, this);
    }
    else if (strcmp(iprop->name, "CCD_EXPOSURE") == 0) {
        printf("Found CCD_EXPOSURE for camera %s\n", iprop->idev->name);
        expose_prop = iprop;
    }
    else if (strcmp(iprop->name, "CCD_FRAME") == 0) {
        printf("Found CCD_FRAME for camera %s\n", iprop->idev->name);
        frame_prop = iprop;
    }
    else if (strcmp(iprop->name, "CCD_FRAME_TYPE") == 0) {
        printf("Found CCD_FRAME_TYPE for camera %s\n", iprop->idev->name);
        frame_type_prop = iprop;
    }
    else if (strcmp(iprop->name, "CCD_BINNING") == 0) {
        printf("Found CCD_BINNING for camera %s\n", iprop->idev->name);
        binning_prop = iprop;
    }
    else if (strcmp(iprop->name, "VIDEO_STREAM") == 0) {
        printf("Found Video Camera %s\n", iprop->idev->name);
        video_prop = iprop;
    }
    else if (strcmp(iprop->name, "DEVICE_PORT") == 0 && indi_port.Length()) {
        indi_send(iprop, indi_prop_set_string(iprop, "PORT", indi_port.ToAscii()));
        indi_dev_set_switch(iprop->idev, "CONNECTION", "CONNECT", TRUE);
    }
    else if (strcmp(iprop->name, "CONNECTION") == 0) {
        printf("Found CONNECTION for camera %s\n", iprop->idev->name);
        indi_send(iprop, indi_prop_set_switch(iprop, "CONNECT", TRUE));
        indi_prop_add_cb(iprop, (IndiPropCB)connect_cb, this);
    }
    CheckState();
}

bool Camera_INDIClass::Connect() {
    wxLongLong msec;
    if (! INDIClient) {
        INDIClient = indi_init(INDIhost.ToAscii(), INDIport, "PHDGuiding");
        if (! INDIClient) {
            return true;
        }
    }
    if (indi_name.IsEmpty()) {
        printf("No INDI camera is set.  Please set INDIcam in the preferences file\n");
        return true;
    }
    indi_device_add_cb(INDIClient, indi_name.ToAscii(), (IndiDevCB)new_prop_cb, this);

    modal = true;
    msec = wxGetLocalTimeMillis();
    while(modal && wxGetLocalTimeMillis() - msec < 10 * 1000) {
        ::wxSafeYield();
    }
    modal = false;

    if(! ready)
        return true;
    return false;

}

bool Camera_INDIClass::Disconnect() {
    Connected = FALSE;
    CurrentGuideCamera = NULL;
    GuideCameraConnected = false;
    return false;
}

void Camera_INDIClass::ShowPropertyDialog() {
    indigui_show_dialog(INDIClient);
}

#if defined (__APPLE__)
#include "../cfitsio/fitsio.h"
#else
#include <fitsio.h>
#endif

bool Camera_INDIClass::ReadFITS(usImage& img) {
    int xsize, ysize;
    fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
    int hdutype, naxis;
    int nhdus=0;
    long fits_size[2];
    long fpixel[3] = {1,1,1};

    if (fits_open_memfile(&fptr,
            "",
            READONLY,
            ((void **)(&blob_elem->value.blob.data)),
            &(blob_elem->value.blob.size),
            0,
            NULL,
            &status) )
    {
        (void) wxMessageBox(wxT("Unsupported type or read error loading FITS file"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU) {
        (void) wxMessageBox(wxT("FITS file is not of an image"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }

    // Get HDUs and size
    fits_get_img_dim(fptr, &naxis, &status);
    fits_get_img_size(fptr, 2, fits_size, &status);
    xsize = (int) fits_size[0];
    ysize = (int) fits_size[1];
    fits_get_num_hdus(fptr,&nhdus,&status);
    if ((nhdus != 1) || (naxis != 2)) {
        (void) wxMessageBox(wxT("Unsupported type or read error loading FITS file"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    if (img.Init(xsize,ysize)) {
        wxMessageBox(wxT("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, NULL, img.ImageData, NULL, &status) ) { // Read image
        (void) wxMessageBox(wxT("Error reading data"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    fits_close_file(fptr,&status);
    return false;
}

bool Camera_INDIClass::ReadStream(usImage& img) {
    int xsize, ysize;
    unsigned char *inptr;
    unsigned short *outptr;
    struct indi_elem_t *elem;

    if (! frame_prop) {
        wxMessageBox(wxT("Failed to determine image dimensions"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    if (! (elem = indi_find_elem(frame_prop, "WIDTH"))) {
        wxMessageBox(wxT("Failed to determine image dimensions"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    xsize = elem->value.num.value;
    if (! (elem = indi_find_elem(frame_prop, "HEIGHT"))) {
        wxMessageBox(wxT("Failed to determine image dimensions"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    ysize = elem->value.num.value;
    if (img.Init(xsize,ysize)) {
        wxMessageBox(wxT("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    outptr = img.ImageData;
    inptr = (unsigned char *)blob_elem->value.blob.data;
    for (int i = 0; i < xsize * ysize; i++)
        *outptr ++ = *inptr++;
    return false;
}

bool Camera_INDIClass::CaptureFull(int duration, usImage& img, bool recon) {
//    unsigned short *dataptr;
//    int i;

    if (expose_prop) {
        printf("Exposing for %d(ms)\n", duration);
        indi_dev_enable_blob(expose_prop->idev, TRUE);
        indi_prop_set_number(expose_prop, "CCD_EXPOSURE_VALUE", duration / 1000.0);
        indi_send(expose_prop, NULL);
    } else {
        printf("Enabling video capture\n");
        indi_dev_enable_blob(expose_prop->idev, TRUE);
        indi_send(video_prop, indi_prop_set_switch(video_prop, "ON", TRUE));
    }
    modal = true;
    while (modal) {
        wxTheApp->Yield();
    }
    if (! expose_prop) {
        indi_send(video_prop, indi_prop_set_switch(video_prop, "OFF", TRUE));
    }

    if (strncmp(".fits",blob_elem->value.blob.fmt, 5) == 0) {
        printf("Processing fits file\n");
        return ReadFITS(img);
    } else if (strncmp(".stream", blob_elem->value.blob.fmt, 7) == 0) {
        printf("Processing stream file\n");
        return ReadStream(img);
    } else {
        wxMessageBox(wxT("Unknown image format: ") + wxString::FromAscii(blob_elem->value.blob.fmt),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
}

#endif
