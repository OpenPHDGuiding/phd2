/*
 *  cam_opencv.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2013 Craig Stark.
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
#ifdef OPENCV_CAMERA
#include "camera.h"
#include "cam_opencv.h"

using namespace cv;

Camera_OpenCVClass::Camera_OpenCVClass() {
	Connected = FALSE;
	Name=_T("OpenCV");
	FullSize = wxSize(640,480);
	HasGuiderOutput = false;

}

bool Camera_OpenCVClass::Connect() {
    CapDev = new VideoCapture(0);
    if (!CapDev->isOpened())
        return true;
 	Connected = TRUE;
	return false;
}

bool Camera_OpenCVClass::Disconnect() {
	Connected = FALSE;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
    delete CapDev;
	return false;
}


bool Camera_OpenCVClass::CaptureFull(int duration, usImage& img, bool recon) {
    wxStopWatch swatch;
    Mat captured_frame;
    int i, nframes;
   
    if (!CapDev) return true;
    
    // Grab at least one frame...
    CapDev->read(captured_frame);
    cvtColor(captured_frame,captured_frame,CV_RGB2GRAY);
    
    cv::Size sz = captured_frame.size();
	if (img.Init(sz.width,sz.height)) {
        wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
        return true;
    }
    for (i=0; i<img.NPixels; i++)
        img.ImageData[i] = 0;

    nframes = 0;
    unsigned char *dptr;
    dptr = captured_frame.data;
    while (swatch.Time() < duration) {
        nframes++;
        CapDev->read(captured_frame);
        cvtColor(captured_frame,captured_frame,CV_RGB2GRAY);
        dptr = captured_frame.data; 
        for (i=0; i<img.NPixels; i++)
            img.ImageData[i] += (unsigned int) dptr[i];
    }
//    frame->SetStatusText(wxString::Format("%d frames",nframes));
	return false;

}
#endif  // OPENCV_CAMERA

