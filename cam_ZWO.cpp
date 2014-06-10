/*
*  cam_ZWO.cpp
*  PHD Guiding
*
*  Created by Robin Glover.
*  Copyright (c) 2014 Robin Glover.
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
#include "cam_ZWO.h"
#include "cameras/AsiCamera.h"

class CaptureThread : public wxThread
{
	unsigned char* _buffer;
	int _bufferSize;
	int _maxWait;
public:
	CaptureThread(unsigned char* buffer, int buffersize, int maxWait) : wxThread(wxTHREAD_JOINABLE)
	{
		_buffer = buffer;
		_bufferSize = buffersize;
		_maxWait = maxWait;
	}

	void* Entry()
	{
		bool gotFrame = getImageData(_buffer, _bufferSize, _maxWait);
		return gotFrame ? (void*)1 : (void*)0;
	}
};

Camera_ZWO::Camera_ZWO()
{
	Connected = false;
	m_hasGuideOutput = true;
	//	HasGainControl = isAvailable(CONTROL_GAIN);
	HasGainControl = true; // really ought to ask the open camera, but all known ZWO cameras have gain
}

Camera_ZWO::~Camera_ZWO()
{
}

bool Camera_ZWO::Connect()
{

	if (!openCamera(0))
	{
		wxMessageBox(_T("Failed to open ZWO ASI Camera."), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	if (!initCamera())
	{
		wxMessageBox(_T("Failed to initialize ZWO ASI Camera."), _("Error"), wxOK | wxICON_ERROR);
		return true;
	}

	bool color = isColorCam();
	bool supportsRGB = isImgTypeSupported(IMG_RGB24);

	FullSize.x = getMaxWidth();
	FullSize.y = getMaxHeight();

	PixelSize = getPixelSize();

	if (HasGainControl)
	{
		GuideCameraGain = (getMax(CONTROL_GAIN) + getMin(CONTROL_GAIN)) / 2;
	}

	Connected = true;

	if (isAvailable(CONTROL_BANDWIDTHOVERLOAD))
		setValue(CONTROL_BANDWIDTHOVERLOAD, getMin(CONTROL_BANDWIDTHOVERLOAD), false);

	return false;
}

bool Camera_ZWO::Disconnect()
{
	closeCamera();

	Connected = false;
	return false;
}


bool Camera_ZWO::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
	int exposureUS = duration * 1000;

	int xsize = getMaxWidth();
	int ysize = getMaxHeight();

	if (img.NPixels != (xsize*ysize)) {
		if (img.Init(xsize, ysize)) {
			pFrame->Alert(_("Memory allocation error during capture"));
			Disconnect();
			return true;
		}
	}

	setStartPos(0, 0);

	setImageFormat(xsize, ysize, 1, IMG_Y8);
	startCapture();
	
	setValue(CONTROL_EXPOSURE, exposureUS, false);
	setValue(CONTROL_GAIN, GuideCameraGain, false);

	int bufSize = xsize * ysize;
	unsigned char* buffer = new unsigned char[bufSize];

	// The getImageData is synchronous, so need to run it in a thread to avoid freezing the UI
	CaptureThread ct(buffer, bufSize, duration * 2 + 1000);
	ct.Run();
	while (ct.IsAlive())
	{
		// Yield() frequently to keep the message pump running and the UI responsive
		wxMilliSleep(1);
		wxGetApp().Yield();
	}

	bool gotFrame = ct.Wait() != NULL;


	if (!gotFrame)
		return true;

	unsigned char* src = buffer;
	unsigned short* dest = img.ImageData;
	for (int y = 0; y<ysize; y++) 
	{
		for (int x = 0; x<xsize; x++, src++, dest++) 
		{ 
			*dest = (unsigned short)*src;
		}
	}

	delete[] buffer;

	if (recon) SubtractDark(img);

	return false;
}

GuideDirections GetDirection(int direction)
{
	switch (direction)
	{
	default:
	case NORTH:
		return guideNorth;
	case EAST:
		return guideEast;
	case WEST:
		return guideWest;
	case SOUTH:
		return guideSouth;

	}
	
}

bool  Camera_ZWO::ST4PulseGuideScope(int direction, int duration)
{
	pulseGuide(GetDirection(direction), duration);

	return false;
}

void  Camera_ZWO::ClearGuidePort()
{
	pulseGuide(guideNorth, 0);
}