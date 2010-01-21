/* cam_template.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2009 Craig Stark.
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
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, TemplateRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "phd.h"
#include "camera.h"
#include "image_math.h"
#include "cam_VIDEODEVICE.h"

#include <libhal.h>
#include <glib.h>


/*------------------------------------------------------------------------------*/

Camera_VIDEODEVICEClass::Camera_VIDEODEVICEClass() {
	Connected = FALSE;
	Name=_T("Linux V4L2 device");
//	FullSize = wxSize(lw,lh);  // Current size of a full frame
	HasGuiderOutput = false;     // Do we have an ST4 port?
	HasGainControl = false;      // Can we adjust gain?

	devicearray.Empty();
}

/*------------------------------------------------------------------------------*/

bool Camera_VIDEODEVICEClass::Connect() {
// Connect to camera -- returns true on error
	int width = 0;
	int height = 0;

	if (true == ProbeDevices(&devicearray)) {
		wxMessageBox(_T("No V4L2 devices found!"));
		return true;
	} else {
		if (true == devicearray.IsEmpty()) {
			// This is bad - no devices found!
			wxMessageBox(_T("No device found!"));
			return true;
		} else {
			wxString device;
			int index = -1;

			if (1 == devicearray.GetCount()) {
				// Easy - only one device found
				index = 0;
			} else {
				index = wxGetSingleChoiceIndex(_T("Select device"), _T("V4L2 devices"), devicearray);
			}

			fprintf(stderr, "Index: %d\n", index);

			if (-1 == index) {
				// Cancel
				return true;
			}

			// ToDo
			device = devicearray.Item(index).BeforeFirst(wxChar('-')).Trim();

			dummycam = new linuxvideodevice((const char*)device.mb_str());
			if (NULL == dummycam || 0 == dummycam->openvideodevice(&width, &height)) {
				// If connection failed, pop up dialog and return true
				wxMessageBox(_T("No camera"));
				return true;
			}
			if (frame->mount_menu->IsChecked(MOUNT_CAMERA)) {
				// User wants to use an onboard guide port - connect.
				// (Should be smarter - does cam have one?)
				ScopeConnected = MOUNT_CAMERA;
				frame->SetStatusText(_T("Scope"),4);
			}

			// Take care of clearing guide port, allocating any ram for a buffer, etc.

	    	// Take care of resetting FullSize if needed
			FullSize = wxSize(width, height);

			fprintf(stderr, "Frame: %dx%d\n", FullSize.GetWidth(), FullSize.GetHeight());

			Connected = true;  // Set global flag for being connected
			return false;
		}
	}
}

/*------------------------------------------------------------------------------*/

bool Camera_VIDEODEVICEClass::PulseGuideScope(int direction, int duration) {
	// Guide cam in a direction for a given duration (msec)
	switch (direction) {
		case WEST:  break;
		case NORTH:  break;
		case SOUTH:  break;
		case EAST: 	break;
		default: return true; // bad direction passed in
	}

	// Start guide pulse

	// Wait duration to make sure guide pulse is done so we don't clash 
	// (don't do this is guide cmd isn't threaded
	wxMilliSleep(duration + 10);
	return false;
}

/*------------------------------------------------------------------------------*/

/*void Camera_TemplateClass::ClearGuidePort() {
}
*/

/*void Camera_TemplateClass::InitCapture() {
// This gets called when you change parameters (gain, exp. duration etc).  Do any cam re-programming needed.  This isn't
// always needed as some cams have you program everything each time.  Define here and undefine in .h file if needed.
}
*/

/*------------------------------------------------------------------------------*/

bool Camera_VIDEODEVICEClass::Disconnect() {

	//  Cam disconnect routine
	dummycam->shutdownvideodevice();
	//  delete [] buffer;
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	return false;
}		

/*------------------------------------------------------------------------------*/

bool Camera_VIDEODEVICEClass::CaptureFull(int duration, usImage& img, bool recon) {
	// Capture a full frame and stick it into img -- if recon is true, do any frame recon needed.
	
	int xsize = FullSize.GetWidth();
	int ysize = FullSize.GetHeight();
	int pixls;

	if (img.Init(xsize,ysize)) {
		wxMessageBox(_T("Memory allocation error during capture"), wxT("Error"), wxOK | wxICON_ERROR);
		Disconnect();
		return true;
	}

	// Start camera exposure
	dummycam->get_frame(duration);	
	for (pixls=0; pixls < (xsize*ysize); pixls++) {
		img.ImageData[pixls]=dummycam->getPixel(pixls);	
	}
//    	ThreadedExposure(duration, buffer);
//	if (duration > 100) {  // Long-ish exposure -- nicely wait until near the end
//		wxMilliSleep(duration - 100);
//		wxTheApp->Yield();
//	}
//	while (0)  // Poll the camera to see if it's still exposing / downloading (i.e., check until image ready)
//		wxMilliSleep(50);
	unsigned short *dptr = img.ImageData;  // Pointer to data block PHD is expecting

	// DownloadImage(dptr,img.Npixels);
	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);


	// Do quick L recon to remove bayer array if color sensor in Bayer matrix
	// QuickLRecon(img);
	return false;
}

bool Camera_VIDEODEVICEClass::ProbeDevices(wxArrayString *array) {
	bool result = true;
	int i, fd, ok;
	int num_udis = 0;
	char **udis;
	DBusError error;
	LibHalContext *hal_ctx;

	fprintf(stderr, "Probing devices with HAL...\n");

	dbus_error_init(&error);
	hal_ctx = libhal_ctx_new();
	if (hal_ctx == NULL) {
		fprintf(stderr, "Could not create libhal context");
		dbus_error_free(&error);
		return result;
	}

	if (!libhal_ctx_set_dbus_connection(hal_ctx, dbus_bus_get(DBUS_BUS_SYSTEM,
			&error))) {
		fprintf(stderr, "libhal_ctx_set_dbus_connection: %s: %s", error.name, error.message);
		dbus_error_free(&error);
		return result;
	}

	if (!libhal_ctx_init(hal_ctx, &error)) {
		if (dbus_error_is_set(&error)) {
			fprintf(stderr, "libhal_ctx_init: %s: %s", error.name, error.message);
			dbus_error_free(&error);
		}
		fprintf(stderr, "Could not initialise connection to hald.\n"
			"Normally this means the HAL daemon (hald) is not running or not ready");
		return result;
	}

	udis = libhal_find_device_by_capability(hal_ctx, "video4linux", &num_udis,
			&error);

	if (dbus_error_is_set(&error)) {
		fprintf(stderr, "libhal_find_device_by_capability: %s: %s", error.name, error.message);
		dbus_error_free(&error);
		return result;
	}

	for (i = 0; i < num_udis; i++) {
		char *device;
		char *parent_udi = NULL;
		char *subsystem = NULL;
		char *product_name;
		struct v4l2_capability v2cap;
	    gint vendor_id = 0;
	    gint product_id = 0;
	    gchar *property_name = NULL;

		parent_udi = libhal_device_get_property_string(hal_ctx, udis[i],
				"info.parent", &error);
		if (dbus_error_is_set(&error)) {
			fprintf(stderr, "error getting parent for %s: %s: %s", udis[i], error.name, error.message);
			dbus_error_free(&error);
		}

		if (parent_udi != NULL) {
			subsystem = libhal_device_get_property_string(hal_ctx, parent_udi,
					"info.subsystem", NULL);
			if (subsystem == NULL)
				continue;

			property_name = g_strjoin(".", subsystem, "vendor_id", NULL);
			vendor_id = libhal_device_get_property_int(hal_ctx, parent_udi,
					property_name, &error);
			if (dbus_error_is_set(&error)) {
				fprintf(stderr, "error getting vendor id: %s: %s", error.name, error.message);
				dbus_error_free(&error);
			}
			g_free (property_name);

			property_name = g_strjoin(".", subsystem, "product_id", NULL);
			product_id = libhal_device_get_property_int(hal_ctx, parent_udi,
					property_name, &error);
			if (dbus_error_is_set(&error)) {
				fprintf(stderr, "error getting product id: %s: %s", error.name, error.message);
				dbus_error_free(&error);
			}
			g_free (property_name);

			libhal_free_string(subsystem);
			libhal_free_string(parent_udi);
		}

		fprintf(stderr, "Found device %04x:%04x, getting capabilities...\n", vendor_id, product_id);

		device = libhal_device_get_property_string(hal_ctx, udis[i],
				"video4linux.device", &error);

		if (dbus_error_is_set(&error)) {
			fprintf(stderr, "error getting V4L device for %s: %s: %s", udis[i], error.name, error.message);
			dbus_error_free(&error);
			continue;
		}

		/* vbi devices support capture capability too, but cannot be used,
		 * so detect them by device name */
		if (strstr(device, "vbi")) {
			fprintf(stderr, "Skipping vbi device: %s\n", device);
			libhal_free_string(device);
			continue;
		}

		if ((fd = open(device, O_RDONLY | O_NONBLOCK)) < 0) {
			fprintf(stderr, "Failed to open %s: %s", device, strerror(errno));
			libhal_free_string(device);
			continue;
		}

		ok = ioctl(fd, VIDIOC_QUERYCAP, &v2cap);
		if (ok < 0) {
			// No V4L2 device ... skip
			continue;
		} else {
			int cap = v2cap.capabilities;
			fprintf(stderr, "Detected v4l2 device: %s\n", v2cap.card);
			fprintf(stderr, "Driver: %s, version: %d\n", v2cap.driver, v2cap.version);
			fprintf(stderr, "Capabilities: 0x%08X\n", v2cap.capabilities);
			if (!(cap & V4L2_CAP_VIDEO_CAPTURE)) {
				fprintf(stderr, "Device %s seems to not have the capture capability, (radio tuner?)\n"
							"Removing it from device list.\n", device);
				libhal_free_string(device);
				close(fd);
				continue;
			}
			product_name = (char *) v2cap.card;

			Name.Clear();
			Name = wxString::From8BitData(g_strstrip(product_name));

			wxString Device = wxString::From8BitData(g_strstrip(device));

//			fprintf(stderr, "Found: %s %s\n",
//					(const char*)Name.mb_str(),
//					(const char*)wxString::From8BitData(g_strstrip(device)).mb_str());

			fprintf(stderr, "Found: %s at %s\n",
					(const char*)Name.mb_str(),
					(const char*)Device.mb_str());

			array->Add(Device + _T(" - ") + Name);

			result = false;
		}

		fprintf(stderr, "\n");

		libhal_free_string(device);
		close(fd);
	}

	libhal_free_string_array(udis);

	return result;
}

/*------------------------------------------------------------------------------*/

