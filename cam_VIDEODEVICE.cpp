/* cam_VIDEODEVICE.cpp
 *  PHD Guiding
 *
 *  Created by Wolfgang Birkfellner, Steffen Elste.
 *  Copyright (c) 2009, 2010 Wolfgang Birkfellner, Steffen Elste.
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
#include "config_VIDEODEVICE.h"

extern "C" {
#include <libudev.h>
}

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!


WX_DEFINE_OBJARRAY(DeviceInfoArray);


#define V4LSUBSYSTEM "video4linux"


/*------------------------------------------------------------------------------*/

Camera_VIDEODEVICEClass::Camera_VIDEODEVICEClass() {
	Connected = FALSE;
	Name=_T("Linux V4L2 device");

	HasGuiderOutput = false;	// Do we have an ST4 port?
	HasGainControl = false;		// Can we adjust gain?
	HasPropertyDialog = false;	// Configuration dialog

	deviceInfoArray.Clear();
	controlMap.clear();
}

/*------------------------------------------------------------------------------*/

bool Camera_VIDEODEVICEClass::Connect() {
// Connect to camera -- returns true on error
	int width = 0;
	int height = 0;

	if (0 < queryCameraControls())
		HasPropertyDialog = true;

	camera = new linuxvideodevice((const char*)device.mb_str());
	if (NULL == camera || 0 == camera->openvideodevice(&width, &height)) {
		// If connection failed return true
		return true;
	}

	if (frame->mount_menu->IsChecked(MOUNT_CAMERA)) {
		// User wants to use an onboard guide port - connect.
		// (Should be smarter - does cam have one?)
		ScopeConnected = MOUNT_CAMERA;
		frame->SetStatusText(_T("Scope"),4);
	}

   	// Take care of resetting FullSize if needed
	FullSize = wxSize(width, height);

	fprintf(stderr, "Frame: %dx%d\n", FullSize.GetWidth(), FullSize.GetHeight());

	Connected = true;  // Set global flag for being connected
	return false;
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
	camera->shutdownvideodevice();
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;

    if (fd >= 0)
        v4l2_close(fd);

    HasPropertyDialog = false;

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
	camera->get_frame(duration);
	for (pixls=0; pixls < (xsize*ysize); pixls++) {
		img.ImageData[pixls]=camera->getPixel(pixls);
	}

	unsigned short *dptr = img.ImageData;  // Pointer to data block PHD is expecting

	// DownloadImage(dptr,img.Npixels);
	if (HaveDark && recon)
		Subtract(img,CurrentDarkFrame);

	return false;
}

void Camera_VIDEODEVICEClass::ShowPropertyDialog() {
	V4LPropertiesDialog* propertiesDialog = new V4LPropertiesDialog(&controlMap);

	propertiesDialog->Show();
}

bool Camera_VIDEODEVICEClass::ProbeDevices() {
	struct udev *udev;
	struct udev_device *udev_device;
	struct udev_enumerate *udev_enumerate;
	struct udev_list_entry *device, *property;

	// ... we don't want to end up with multiple entries for the same device
	deviceInfoArray.Empty();

    udev = udev_new();
    if (udev == NULL) {
    	fprintf(stderr, "udev_new() failed\n");
    	return false;
    }

    udev_enumerate = udev_enumerate_new(udev);
    if (udev_enumerate == NULL) {
    	fprintf(stderr, "udev_enumerate_new() failed\n");
    	return false;
    }

    if (0 == udev_enumerate_add_match_subsystem(udev_enumerate, V4LSUBSYSTEM) &&
    		0 == udev_enumerate_scan_devices(udev_enumerate)) {

    	udev_list_entry_foreach(device, udev_enumerate_get_list_entry(udev_enumerate)) {
    		const char *syspath = udev_list_entry_get_name(device);

    		if (NULL != syspath) {
        		if (NULL != (udev_device = udev_device_new_from_syspath(udev, syspath))) {
    				// Found a suitable device - new (array) entry
    				DeviceInfo deviceInfo = DeviceInfo();

        			udev_list_entry_foreach(property, udev_device_get_properties_list_entry(udev_device)) {
        				wxString value = wxString(udev_list_entry_get_value(property), *wxConvCurrent);

        				// We only need a couple of all the properties present
        				if (0 == strcmp("DEVNAME", udev_list_entry_get_name(property))) {
        					deviceInfo.setDeviceName(value.Strip());
        				}

        				if (0 == strcmp("ID_VENDOR_ID", udev_list_entry_get_name(property))) {
        					deviceInfo.setVendorId(value.Strip());
        				}

        				if (0 == strcmp("ID_MODEL_ID", udev_list_entry_get_name(property))) {
        					deviceInfo.setModelId(value.Strip());
        				}

        				if (0 == strcmp("ID_V4L_PRODUCT", udev_list_entry_get_name(property))) {
        					deviceInfo.setProduct(value.Strip());
        				}
        			}

        			// FIXME - how about some debugging info

        			// Add to the array
        			deviceInfoArray.Add(deviceInfo);

            		udev_device_unref(udev_device);
        		}
    		}
       	}
    } else {
    	fprintf(stderr, "No matching devices found!\n");
    }

    udev_enumerate_unref(udev_enumerate);
    udev_unref(udev);

    // Well, this looks weird - if deviceInfoArray.IsEmpty() returns 'true' there are NO DEVICES ... negate the return value
    return (!deviceInfoArray.IsEmpty());
}

DeviceInfo* Camera_VIDEODEVICEClass::GetDeviceAtIndex(int index) {

	if (0 > index || deviceInfoArray.GetCount() <= index)
		return NULL;

	return &(deviceInfoArray.Item(index));
}

wxArrayString& Camera_VIDEODEVICEClass::GetProductArray(wxArrayString& devices) {
	for (int i=0; i<deviceInfoArray.GetCount(); i++) {
		devices.Add(deviceInfoArray.Item(i).getProduct());
	}

	return devices;
}

int Camera_VIDEODEVICEClass::queryCameraControls() {
	struct v4l2_capability cap;
	struct v4l2_queryctrl ctrl;

	// Shouldn't happen ...
    if (0 > (fd = v4l2_open((const char*)device.mb_str(), O_RDWR, 0))) {
    	fprintf(stderr, "Error opening %s\n", (const char*)device.mb_str());
    	return -1;
    }

    // This is rather unlikely as well ...
    if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
    	fprintf(stderr, "Not a V4L(2) device: %s\n", (const char*)device.mb_str());
    	return -1;
    }

	// Check all the standard controls
	for (int i=V4L2_CID_BASE; i<V4L2_CID_LASTP1; i++) {
		ctrl.id = i;
		if (0 == v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl)) {
	    	if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
	    		continue;

	    	addControl(ctrl);
		}
	}

	// Check any custom controls
	for (int i=V4L2_CID_PRIVATE_BASE; ; i++) {
		ctrl.id = i;
		if (0 == v4l2_ioctl(fd, VIDIOC_QUERYCTRL, &ctrl)) {
	    	if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
	    		continue;

			addControl(ctrl);
		} else {
			break;
		}
	}

    return controlMap.size();
}

/*------------------------------------------------------------------------------*/

void Camera_VIDEODEVICEClass::addControl(struct v4l2_queryctrl &ctrl) {

	if (controlMap.find(ctrl.id) == controlMap.end()) {
		// No element with that key exists in the map ...
	    switch(ctrl.type) {
	        case V4L2_CTRL_TYPE_INTEGER:
	        case V4L2_CTRL_TYPE_BOOLEAN:
	        case V4L2_CTRL_TYPE_MENU:
	        	controlMap[ctrl.id] = new V4LControl(fd, ctrl);
	            break;
	        case V4L2_CTRL_TYPE_BUTTON:
	        case V4L2_CTRL_TYPE_INTEGER64:
	        case V4L2_CTRL_TYPE_CTRL_CLASS:
	        default:
	            break;
	    }
	}
}
