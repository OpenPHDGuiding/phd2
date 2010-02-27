/*
 *  cam_template.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2009 Craig Stark.
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
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, TemplateRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include "camera.h"
#include "image_math.h"
#include "cam_template.h"

Camera_TemplateClass::Camera_TemplateClass() {
	Connected = FALSE;
	Name=_T("Template Camera");
	FullSize = wxSize(1280,1024);  // Current size of a full frame
	HasGuiderOutput = true;  // Do we have an ST4 port?
	HasGainControl = true;  // Can we adjust gain?
}



bool Camera_TemplateClass::Connect() {
// Connect to camera -- returns true on error
	if (frame->mount_menu->IsChecked(MOUNT_CAMERA)) {  // User wants to use an onboard guide port - connect.  (Should be smarter - does cam have one?)
		ScopeConnected = MOUNT_CAMERA;
		frame->SetStatusText(_T("Scope"),4);
	}

	if (1) {  // If connection failed, pop up dialog and return true
		wxMessageBox(_T("No camera"));
		return true;
	}
	// Take care of clearing guide port, allocating any ram for a buffer, etc.

    // Take care of resetting FullSize if needed
    // FullSize = wxSize(X,Y);

	Connected = true;  // Set global flag for being connected

	return false;
}

bool Camera_TemplateClass::PulseGuideScope(int direction, int duration) {
    // Guide cam in a direction for a given duration (msec)
	switch (direction) {
		case WEST:  break;
		case NORTH:  break;
		case SOUTH:  break;
		case EAST: 	break;
		default: return true; // bad direction passed in
	}

	// Start guide pulse

	// Wait duration to make sure guide pulse is done so we don't clash (don't do this is guide cmd isn't threaded
	wxMilliSleep(duration + 10);
	return false;
}

/*void Camera_TemplateClass::ClearGuidePort() {
}
*/

/*void Camera_TemplateClass::InitCapture() {
// This gets called when you change parameters (gain, exp. duration etc).  Do any cam re-programming needed.  This isn't
// always needed as some cams have you program everything each time.  Define here and undefine in .h file if needed.

}
*/

bool Camera_TemplateClass::Disconnect() {

    // Cam disconnect routine

//	delete [] buffer;
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	return false;
}

bool Camera_TemplateClass::CaptureFull(int duration, usImage& img, bool recon) {
// Capture a full frame and stick it into img -- if recon is true, do any frame recon needed.
	int xsize = FullSize.GetWidth();
	int ysize = FullSize.GetHeight();


	if (img.Init(xsize,ysize)) {
 		wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
		Disconnect();
		return true;
	}

    // Start camera exposure
    //	ThreadedExposure(duration, buffer);
	if (duration > 100) {  // Long-ish exposure -- nicely wait until near the end
		wxMilliSleep(duration - 100);
		wxTheApp->Yield();
	}
	while (0)  // Poll the camera to see if it's still exposing / downloading (i.e., check until image ready)
		wxMilliSleep(50);

	unsigned short *dptr = img.ImageData;  // Pointer to data block PHD is expecting

	// DownloadImage(dptr,img.Npixels);
	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);


	// Do quick L recon to remove bayer array if color sensor in Bayer matrix
    //	QuickLRecon(img);
	return false;
}

