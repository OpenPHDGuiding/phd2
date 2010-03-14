/*
 *  cam_VIDEODEVICE.h
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
 *
 */

#ifndef CAM_VIDEODEVICE_H_INCLUDED
#define CAM_VIDEODEVICE_H_INCLUDED

#include "cameras/linuxvideodevice.h"

#include <wx/dynarray.h>


class DeviceInfo;


WX_DECLARE_OBJARRAY(DeviceInfo, DeviceInfoArray);


// ToDo - Refactor into separate header and source file maybe
class DeviceInfo {
public:
	DeviceInfo() {}

	wxString getProduct() { return product; }
	void setProduct(const char* string) { product = wxString(string, *wxConvCurrent); }
	void setProduct(wxString string) { product = string; }

	wxString getDeviceName() { return deviceName; }
	void setDeviceName(const char* string) { deviceName = wxString(string, *wxConvCurrent); }
	void setDeviceName(wxString string) { deviceName = string; }

	wxString getBus() { return bus; }
	void setBus(const char* string) { bus = wxString(string, *wxConvCurrent); }
	void setBus(wxString string) { bus = string; }

	wxString getDriver() { return driver; }
	void setDriver(const char* string) { driver = wxString(string, *wxConvCurrent); }
	void setDriver(wxString string) { driver = string; }

	wxString getVendorId() { return vendorId; }
	void setVendorId(const char* string) { vendorId = wxString(string, *wxConvCurrent); }
	void setVendorId(wxString string) { vendorId = string; }

	wxString getModelId() { return modelId; }
	void setModelId(const char* string) { modelId = wxString(string, *wxConvCurrent); }
	void setModelId(wxString string) { modelId = string; }

private:
	wxString product;
	wxString deviceName;
	wxString bus, driver;
	wxString vendorId, modelId;
};


class Camera_VIDEODEVICEClass : public GuideCamera {
public:
	Camera_VIDEODEVICEClass();

	bool	CaptureFull(int duration, usImage& img, bool recon);	// Captures a full-res shot
	bool	Connect();		// Opens up and connects to cameras
	bool	Disconnect();
	void	InitCapture() { return; }
	bool    PulseGuideScope(int direction, int duration);

	void	ShowPropertyDialog();

	bool	ProbeDevices();
	size_t	NumberOfDevices() { return deviceInfoArray.GetCount(); }
	DeviceInfo*	GetDeviceAtIndex(int index);
	wxArrayString& GetProductArray(wxArrayString& devices);

	wxString GetDevice() { return device; }
	void	SetDevice(wxString string) { device = string; }

	wxString GetVendor() { return vendor; }
	void	SetVendor(wxString string) { vendor = string; }

	wxString GetModel() { return model; }
	void	SetModel(wxString string) { model = string; }

	const linuxvideodevice* getCamera() { return camera; }

	bool saveSettings(wxConfig *config);
	bool restoreSettings(wxConfig *config);

private:
	linuxvideodevice *camera;

	wxString device;
	wxString vendor, model;
	DeviceInfoArray deviceInfoArray;
	V4LControlMap controlMap;
};

extern Camera_VIDEODEVICEClass Camera_VIDEODEVICE;

#endif // CAM_VIDEODEVICE_H_INCLUDED
