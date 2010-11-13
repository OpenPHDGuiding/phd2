/* cam_VIDEODEVICE.cpp
 *  PHD Guiding
 *
 *  Created by Wolfgang Birkfellner, Steffen Elste.
 *  Copyright (c) 2009, 2010 Wolfgang Birkfellner, Steffen Elste.
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
#include "camera.h"

#ifdef V4L_CAMERA
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
}

/*------------------------------------------------------------------------------*/

bool Camera_VIDEODEVICEClass::Connect() {
// Connect to camera -- returns true on error
	int width = 0;
	int height = 0;

	camera = new linuxvideodevice((const char*)device.mb_str());
	if (NULL == camera || 0 == camera->openvideodevice(&width, &height)) {
		// If connection failed return true
		return true;
	}

	// Camera settings ...
	if (0 < camera->queryV4LControls()) {
		HasPropertyDialog = true;

		// FIXME - accessing the main application frame from here is a hack
		frame->Menubar->Enable(MENU_V4LSAVESETTINGS, true);
		frame->Menubar->Enable(MENU_V4LRESTORESETTINGS, true);
	}

	if (frame->mount_menu->IsChecked(MOUNT_CAMERA)) {
		// User wants to use an onboard guide port - connect.
		// (Should be smarter - does cam have one?)
		ScopeConnected = MOUNT_CAMERA;
		frame->SetStatusText(_T("Scope"),4);
	}

   	// Take care of resetting FullSize if needed
	FullSize = wxSize(width, height);

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
	// Better safe than sorry ...
	if (NULL != camera && true == Connected) {
	    //  Cam disconnect routine
		camera->shutdownvideodevice();
		Connected = false;

	    HasPropertyDialog = false;

		// FIXME - accessing the main application frame from here is a hack
		frame->Menubar->Enable(MENU_V4LSAVESETTINGS, false);
		frame->Menubar->Enable(MENU_V4LRESTORESETTINGS, false);
		frame->Setup_Button->Enable(false);

		delete camera;

		return false;
	}

	return true;
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

	//unsigned short *dptr = img.ImageData;  // Pointer to data block PHD is expecting

	// DownloadImage(dptr,img.Npixels);
	if (HaveDark && recon)
		Subtract(img,CurrentDarkFrame);

	return false;
}

void Camera_VIDEODEVICEClass::ShowPropertyDialog() {
	V4LPropertiesDialog* propertiesDialog = new V4LPropertiesDialog(camera->getV4LControlMap());

	propertiesDialog->ShowModal();

	propertiesDialog->Destroy();
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

const V4LControl* Camera_VIDEODEVICEClass::getV4LControl(int id) {
	return (NULL != camera ? camera->getV4LControl(id) : NULL);
}

/*------------------------------------------------------------------------------*/

bool Camera_VIDEODEVICEClass::saveSettings(wxConfig *config) {
	bool result = false;

	if (NULL != config) {
		V4LControlMap controlMap = camera->getV4LControlMap();
		V4LControlMap::iterator it;
		V4LControl *control;

		config->Write(_T("camera"), Name);
		config->Write(_T("vendorid"), vendor);
		config->Write(_T("modelid"), model);

		for (it=controlMap.begin(); it!=controlMap.end(); ++it) {
			int id = it->first;
			control = (V4LControl*)it->second;

			config->Write(wxString::Format(wxT("%i"), id), control->value);
		}

		config->Flush();

		result = true;
	}

	return result;
}

bool Camera_VIDEODEVICEClass::restoreSettings(wxConfig *config) {
	bool result = false;

	if (NULL != config) {
		V4LControlMap controlMap = camera->getV4LControlMap();
		V4LControlMap::iterator it;
		V4LControl *control;

		for (it=controlMap.begin(); it!=controlMap.end(); ++it) {
			int id = it->first;
			control = (V4LControl*)it->second;

			if (true == config->Exists(wxString::Format(wxT("%i"), id))) {
				config->Read(wxString::Format(wxT("%i"), id), &(control->value));

				// 'result' is true if all controls could be restored
				if (false == (result = control->update()))
					break;
			}
		}
	}

	return result;
}

#endif
