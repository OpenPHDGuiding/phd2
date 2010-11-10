/*
 *  cam_simulator.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
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
#include "time.h"
#include "image_math.h"
#include "cam_simulator.h"

#define SIMMODE 1   // 1=FITS, 2=BMP, 3=Generate

Camera_SimClass::Camera_SimClass() {
	Connected = FALSE;
//	HaveBPMap = FALSE;
//	NBadPixels=-1;
//	ConnectedModel = 1;
	Name=_T("Simulator");
	FullSize = wxSize(640,480);
	HasGuiderOutput = true;

//	FullSize = wxSize(1360,1024);
}

bool Camera_SimClass::Connect() {
//	wxMessageBox(wxGetCwd());
	Connected = TRUE;
	return false;
}

bool Camera_SimClass::Disconnect() {
	Connected = FALSE;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	return false;
}


#if defined (__APPLE__)
#include "../cfitsio/fitsio.h"
#else
#include <fitsio.h>
#endif

#if SIMMODE==1
bool Camera_SimClass::CaptureFull(int duration, usImage& img, bool recon) {
	int xsize, ysize;
//	unsigned short *dataptr;
//	int i;
	fitsfile *fptr;  // FITS file pointer
	int status = 0;  // CFITSIO status value MUST be initialized to zero!
	int hdutype, naxis;
	int nhdus=0;
	long fits_size[2];
	long fpixel[3] = {1,1,1};
//	char keyname[15];
//	char keystrval[80];

#if defined (__APPLE__)
	if ( !fits_open_file(&fptr, "/Users/stark/dev/PHD/image_02.fit", READONLY, &status) ) {
#else
	if ( !fits_open_diskfile(&fptr, "simimage.fit", READONLY, &status) ) {
#endif
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
		   (void) wxMessageBox(_T("Unsupported type or read error loading FITS file"),wxT("Error"),wxOK | wxICON_ERROR);
		   return true;
		}
		if (img.Init(xsize,ysize)) {
			wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, NULL, img.ImageData, NULL, &status) ) { // Read image
			(void) wxMessageBox(_T("Error reading data"),wxT("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		fits_close_file(fptr,&status);
	}
	return false;

}
#endif

#if SIMMODE==2
bool Camera_SimClass::CaptureFull(int duration, usImage& img) {
	int xsize, ysize;
	wxImage disk_image;
	unsigned short *dataptr;
	unsigned char *imgptr;
	int i;

	bool retval = disk_image.LoadFile("/Users/stark/dev/PHD/simimage.bmp");
	if (!retval) {
		wxMessageBox(_T("Cannot load simulated image"));
		return true;
	}
	xsize = disk_image.GetWidth();
	ysize = disk_image.GetHeight();
	if (img.Init(xsize,ysize)) {
 		wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
		return true;
	}

	dataptr = img.ImageData;
	imgptr = disk_image.GetData();
	for (i=0; i<img.NPixels; i++, dataptr++, imgptr++) {
		*dataptr = (unsigned short) *imgptr;
		imgptr++; imgptr++;
	}
	QuickLRecon(img);
	return false;

}
#endif

#if SIMMODE==3
bool Camera_SimClass::CaptureFull(int duration, usImage& img, bool recon) {
	int xPE = 4;
	int yPE = 2;
	int Noise = 4;
	int star_x[20],star_y[20], inten[20];
	unsigned short *dataptr, newval;
//	float newval, *dataptr;
	int r1, r2;
	int i;
	int exptime, gain, offset;
	int start_time;
	int xsize, ysize;


	exptime = duration;
	gain = 30;
	offset = 100;
   start_time = clock();
	srand(2);
	xsize = FullSize.GetWidth();
	ysize = FullSize.GetHeight();
	for (i=0; i<20; i++) {  // Place stars in same random location each time away from edges
		star_x[i] = rand() % xsize;
		star_y[i] = rand() % ysize;
		if (star_x[i] < 12) star_x[i] = star_x[i] + 12;
		if (star_y[i] < 12) star_y[i] = star_y[i] + 12;
		if (star_x[i] > (xsize - 12)) star_x[i] = star_x[i] - 12;
		if (star_y[i] > (ysize - 12)) star_y[i] = star_y[i] - 12;
		inten[i] = rand() % 100;
	}
	srand( (unsigned) clock() );

	// OK, now move the stars by some random bit
	r1 = rand() % Noise - Noise / 2; r1 = 0;
	r2 = rand() % Noise - Noise / 2; r2 = 0;
//	frame->SetStatusText(wxString::Format ("%d %.2f",start_time,sin((double) start_time / 47750.0)));
	for (i=0; i<20; i++) {
		star_x[i] = star_x[i] + r1 + (int) (sin((double) start_time / 47750.0) * xPE);
		star_y[i] = star_y[i] + r2 + (int) (sin((double) start_time / 47750.0 + 100.0) * yPE);
	}
//	frame->SetStatusText(wxString::Format(_T("%d,%d,%d   %d,%d,%d"),star_x[0],star_y[0],inten[0],star_x[1],star_y[1],inten[1]));
	if (img.NPixels != (xsize*ysize) ) {
		if (img.Init(xsize,ysize)) {
			wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
			return true;
		}
	}
	dataptr = img.ImageData;
	for (i=0; i<img.NPixels; i++, dataptr++)  // put in base noise
		*dataptr = (unsigned short) ((float) gain/10.0 * offset*exptime/100 + (rand() % (gain *  100)));
	dataptr = img.ImageData;

	for (i=0; i<20; i++) {
		newval = inten[i] * exptime * gain + (int) ((float) gain/10.0 * (float) offset * (float) exptime/100.0 + (rand() % (gain * 100)));
		*(dataptr + star_y[i]*xsize + star_x[i]) = newval;
		*(dataptr + (star_y[i]+1)*xsize + star_x[i]) = newval * 0.5;
		*(dataptr + (star_y[i]-1)*xsize + star_x[i]) = newval * 0.5;
		*(dataptr + (star_y[i])*xsize + (star_x[i]+1)) = newval * 0.5;
		*(dataptr + (star_y[i])*xsize + (star_x[i]-1)) = newval * 0.5;
		*(dataptr + (star_y[i]+1)*xsize + (star_x[i]+1)) = newval * 0.3;
		*(dataptr + (star_y[i]+1)*xsize + (star_x[i]-1)) = newval * 0.3;
		*(dataptr + (star_y[i]-1)*xsize + (star_x[i]+1)) = newval * 0.3;
		*(dataptr + (star_y[i]-1)*xsize + (star_x[i]-1)) = newval * 0.3;
	}
	if (exptime>100) {
		int t=0;
		while (t<exptime) {
			wxMilliSleep(100);
			wxTheApp->Yield(true);
			t=t+100;

		}
	}
	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);

	return false;
}
#endif

#if SIMMODE == 4
bool Camera_SimClass::CaptureFull(int duration, usImage& img, bool recon) {
	int xsize, ysize;
	//	unsigned short *dataptr;
	//	int i;
	fitsfile *fptr;  // FITS file pointer
	int status = 0;  // CFITSIO status value MUST be initialized to zero!
	int hdutype, naxis;
	int nhdus=0;
	long fits_size[2];
	long fpixel[3] = {1,1,1};
	//	char keyname[15];
	//	char keystrval[80];
	static int frame = 0;
	static int step = 1;
	char fname[256];
	sprintf(fname,"/Users/stark/dev/PHD/simimg/DriftSim_%d.fit",frame);
	if ( !fits_open_file(&fptr, fname, READONLY, &status) ) {
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
			(void) wxMessageBox(wxString::Format("Unsupported type or read error loading FITS file %d %d",nhdus,naxis),wxT("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		if (img.Init(xsize,ysize)) {
			wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, NULL, img.ImageData, NULL, &status) ) { // Read image
			(void) wxMessageBox(_T("Error reading data"),wxT("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		fits_close_file(fptr,&status);
		frame = frame + step;
		if (frame > 440) {
			step = -1;
			frame = 439;
		}
		else if (frame < 0) {
			step = 1;
			frame = 1;
		}

	}
	return false;
	
}
#endif

