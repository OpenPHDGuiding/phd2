/*
 *  usimage.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
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


bool usImage::Init(int xsize, int ysize) {
// Allocates space for image and sets params up
// returns true on error
	if (ImageData) {
		delete[] ImageData;
		ImageData = NULL;
	}
	NPixels = xsize * ysize;
	Size = wxSize(xsize,ysize);
	Origin = wxPoint(0,0);
	Min = Max = Mean = 0;
	if (NPixels) {
		ImageData = new unsigned short[NPixels];
		if (!ImageData) return true;
	}
	return false;
}

void usImage::CalcStats() {
	int i, d;
	unsigned short *ptr;
	float f_mean;

	if ((!ImageData) || (!NPixels))
		return;

	ptr = ImageData;
	Min = 65535; Max = 0;
	f_mean = 0.0;
	if (Origin == wxPoint(0,0)) { // Full frame
		for (i=0; i<NPixels; i++, ptr++) {
			d = (int) ( *ptr);
			if (d < Min) Min = d;
			if (d > Max) Max = d;
			f_mean = f_mean + (float) d;
		}
		Mean = (int) (f_mean / (float) NPixels);
	}
	else { // Subframe
		int x, y;
		for (y=0; y<CROPYSIZE; y++) {
			ptr = ImageData + Origin.x + (Origin.y + y)*Size.GetWidth();
			for (x=0; x<CROPXSIZE; x++, ptr++) {
				d = (int) ( *ptr);
				if (d < Min) Min = d;
				if (d > Max) Max = d;
				f_mean = f_mean + (float) d;
			}
		}
		Mean = (int) (f_mean / (float) (CROPXSIZE * CROPYSIZE));
	}
}

bool usImage::Clean() {

	return false;
}

bool usImage::CopyToImage(wxImage **rawimg, int min, int max, double power) {
	wxImage	*img;
	unsigned char *ImgPtr;
	unsigned short *RawPtr;
	int i;
	float d;

//	Binsize = 1;
	img = *rawimg;
	if ((!img->Ok()) || (img->GetWidth() != Size.GetWidth()) || (img->GetHeight() != Size.GetHeight()) ) {  // can't reuse bitmap
		if (img->Ok()) {
			delete img;  // Clear out current image if it exists
			img = (wxImage *) NULL;
		}
		img = new wxImage(Size.GetWidth(), Size.GetHeight(), false);
	}
	ImgPtr = img->GetData();
	RawPtr = ImageData;
//	s_factor = (((float) Max - (float) Min) / 255.0);
	float range = (float) (Max - Min);

	if ((power == 1.0) || (range == 0.0)) {
		range = Max;  // Go 0-max
		for (i=0; i<NPixels; i++, RawPtr++ ) {
			d = ((float) (*RawPtr) / range) * 255.0;
			if (d < 0.0) d = 0.0;
			else if (d > 255.0) d = 255.0;
			*ImgPtr = (unsigned char) d;
			ImgPtr++;
			*ImgPtr = (unsigned char) d;
			ImgPtr++;
			*ImgPtr = (unsigned char) d;
			ImgPtr++;

		}
	}
	else {
		for (i=0; i<NPixels; i++, RawPtr++ ) {
			d = (float) (*RawPtr);
			d = pow((d - (float) Min) / range, (float) power) * 255.0;
			if (d < 0.0) d = 0.0;
			else if (d > 255.0) d = 255.0;
			*ImgPtr = (unsigned char) d;
			ImgPtr++;
			*ImgPtr = (unsigned char) d;
			ImgPtr++;
			*ImgPtr = (unsigned char) d;
			ImgPtr++;

		}
	}
	*rawimg = img;
	return false;
}

bool usImage::BinnedCopyToImage(wxImage **rawimg, int min, int max, double power) {
	wxImage	*img;
	unsigned char *ImgPtr;
	unsigned short *RawPtr;
	int x,y;
	float d;
	//, s_factor;
	int full_xsize, full_ysize;
	int use_xsize, use_ysize;

	full_xsize = Size.GetWidth();
	full_ysize = Size.GetHeight();
	use_xsize = full_xsize;
	use_ysize = full_ysize;
	if (use_xsize % 2) use_xsize--;
	if (use_ysize % 2) use_ysize--;
//	Binsize = 2;

	img = *rawimg;
	if ((!img->Ok()) || (img->GetWidth() != (full_xsize/2)) || (img->GetHeight() != (full_ysize/2)) ) {  // can't reuse bitmap
		if (img->Ok()) {
			delete img;  // Clear out current image if it exists
			img = (wxImage *) NULL;
		}
		img = new wxImage(full_xsize/2, full_ysize/2, false);
	}
	ImgPtr = img->GetData();
	RawPtr = ImageData;
//	s_factor = (((float) Max - (float) Min) / 255.0);
	float range = (float) (Max - Min);

	if ((power == 1.0) || (range == 0.0)) {
		range = Max;  // Go 0-max
		if (range == 0.0) range = 0.001;
		for (y=0; y<use_ysize; y+=2) {
			for (x=0; x<use_xsize; x+=2) {
				RawPtr = ImageData + x + y*full_xsize;
				d = (float) (*RawPtr + *(RawPtr+1) + *(RawPtr+full_xsize) + *(RawPtr+1+full_xsize)) / 4.0;
				d = (d / range) * 255.0;
				*ImgPtr = (unsigned char) d;
				ImgPtr++;
				*ImgPtr = (unsigned char) d;
				ImgPtr++;
				*ImgPtr = (unsigned char) d;
				ImgPtr++;
			}
		}
	}
	else {
		for (y=0; y<use_ysize; y+=2) {
			for (x=0; x<use_xsize; x+=2) {
				RawPtr = ImageData + x + y*full_xsize;
				d = (float) (*RawPtr + *(RawPtr+1) + *(RawPtr+full_xsize) + *(RawPtr+1+full_xsize)) / 4.0;
				d = pow((d - (float) Min) / range, (float) power) * 255.0;
				if (d < 0.0) d = 0.0;
				else if (d > 255.0) d = 255.0;
				*ImgPtr = (unsigned char) d;
				ImgPtr++;
				*ImgPtr = (unsigned char) d;
				ImgPtr++;
				*ImgPtr = (unsigned char) d;
				ImgPtr++;
			}
		}
	}
	*rawimg = img;
	return false;
}
