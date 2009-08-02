/*
 *  cam_INI.cpp
 *  PHD Guiding
 *
 *  Created by Geoffrey Hausheer.
 *  Copyright (c) 2009 Geoffrey Hausheer.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
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
#include "time.h"
#include "image_math.h"
#include "cam_INDI.h"

#include <gtk/gtk.h>
extern "C" {
#include "libindi/indi.h"
#include "libindi/indigui.h"
}

struct indi_t *indi_object = NULL;

void camera_capture_cb(struct indi_prop_t *iprop, void *data)
{
    Camera_INDIClass *cb = (Camera_INDIClass *)(data);
    cb->blob_elem = indi_find_elem(iprop, "CCD1");
    printf("Got camera callback\n");
    gtk_main_quit();
}

Camera_INDIClass::Camera_INDIClass() {
	Connected = FALSE;
//	HaveBPMap = FALSE;
//	NBadPixels=-1;
//	ConnectedModel = 1;
	Name=_T("INDI Camera");
	HasPropertyDialog = true;
	FullSize = wxSize(640,480);
//	FullSize = wxSize(1360,1024);
}

int try_camera_connect_wrapper(void *data)
{
    Camera_INDIClass *cb = (Camera_INDIClass *)(data);
    return cb->TryConnect();
}
int Camera_INDIClass::TryConnect(void)
{
    struct indi_device_t *idev;
    struct indi_prop_t *iprop;
    char name[] = "CCD Simulator";

    if(--connect_count == 0) {
        if(modal)
            gtk_main_quit();
        return FALSE;
    }

    idev = indi_find_device(indi, name);
    if (! idev) {
        printf("Couldn't locate INDI camera device %s\n", name);
        return TRUE;
    }

    /* property to set exposure */
    if (! (expose_prop = indi_find_prop(idev, "EXPOSE_DURATION"))) {
        printf("Couldn't locate INDI property 'EXPOSE_DURATION'\n");
        return TRUE;
    }

    if (! (iprop = indi_find_prop(idev, "Video"))) {
        printf("Couldn't locate INDI property 'Video'\n");
        return TRUE;
    } else {
        indi_prop_add_cb(iprop, camera_capture_cb, this);
    }
    if (! (iprop = indi_find_prop(idev, "CONNECTION"))) {
        printf("Couldn't find camera property 'CONNECT'\n");
        return TRUE;
    }
    //indi_prop_add_cb(iprop, connect_cb, camera);
    indi_dev_set_switch(idev, "CONNECTION", "CONNECT", TRUE);
    if(modal)
        gtk_main_quit();
    return FALSE;
}

bool Camera_INDIClass::Connect() {
    connect_count = 5;
    modal = 0;

    if (! indi_object) {
        indi_object = indi_init(NULL, NULL);
        if (! indi_object) {
            return true;
        }
    }

    indi = indi_object;
    if(TryConnect()) {
        modal = 1;
        g_timeout_add_seconds(1, (GSourceFunc)try_camera_connect_wrapper, this);
        gtk_main();
    }
    if(! connect_count)
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
    indigui_show_dialog(indi, 0);
}

#if defined (__WINDOWS__)
#include "../fits/fitsio.h"
#elsif defined (__APPLE__)
#include "../cfitsio/fitsio.h"
#else
#include <fitsio.h>
#endif

bool Camera_INDIClass::CaptureFull(int duration, usImage& img, bool recon) {
	int xsize, ysize;
//	unsigned short *dataptr;
//	int i;
	fitsfile *fptr;  // FITS file pointer
	int status = 0;  // CFITSIO status value MUST be initialized to zero!
	int hdutype, naxis;
	int nhdus=0;
	long fits_size[2];
	long fpixel[3] = {1,1,1};

    indi_dev_enable_blob(expose_prop->idev, TRUE);
    printf("Exposing for %d(ms)\n", duration);
    indi_prop_set_number(expose_prop, "EXPOSE_S", duration / 1000.0);
    indi_send(expose_prop, NULL);
    gtk_main();

	if (!fits_open_memfile(&fptr,
            "",
            READONLY,
            ((void **)(&blob_elem->value.blob.data)),
            &(blob_elem->value.blob.size),
            0,
            NULL,
            &status) )
    {
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
	}
    return false;
}

