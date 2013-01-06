/*
 *  cam_ascom.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2009-2010 Craig Stark.
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
#ifndef ASCOMCAMDEF
#define ASCOMCAMDEF

#ifdef __WINDOWS__
#ifdef _WIN7
#import "file:c:\Program Files (x86)\Common Files\ASCOM\Interface\AscomMasterInterfaces.tlb"
#else
#import "file:c:\Program Files\Common Files\ASCOM\Interface\AscomMasterInterfaces.tlb"
#endif
#import "file:C:\\Windows\\System32\\ScrRun.dll" \
	no_namespace \
	rename("DeleteFile","DeleteFileItem") \
	rename("MoveFile","MoveFileItem") \
	rename("CopyFile","CopyFileItem") \
	rename("FreeSpace","FreeDriveSpace") \
	rename("Unknown","UnknownDiskType") \
	rename("Folder","DiskFolder")

//
// Creates COM "smart pointer" _com_ptr_t named _ChooserPtr
//
#import "progid:DriverHelper.Chooser" \
	rename("Yield","ASCOMYield") \
	rename("MessageBox","ASCOMMessageBox")

using namespace AscomInterfacesLib;
using namespace DriverHelper;

#include <wx/msw/ole/oleutils.h>
#include <objbase.h>
#include <ole2ver.h>

#endif

class Camera_ASCOMClass : public GuideCamera, protected ASCOM_COMMON {
public:
	bool	CaptureFull(int duration, usImage& img, bool recon);	// Captures a full-res shot
	bool	Connect();
	bool	Disconnect();

	bool	PulseGuideScope(int direction, int duration);
	bool	Color;
	Camera_ASCOMClass(); 
private:
#ifdef __WINDOWS__
	ICameraPtr pCam;
#endif
};

class Camera_ASCOMLateClass : public GuideCamera, protected ASCOM_COMMON {
public:
	bool	CaptureFull(int duration, usImage& img, bool recon);	// Captures a full-res shot
	bool	Connect();
	bool	Disconnect();

	bool	PulseGuideScope(int direction, int duration);
	bool	Color;
	Camera_ASCOMLateClass(); 
private:
#ifdef __WINDOWS__
	IDispatch *ASCOMDriver;
	DISPID dispid_setxbin, dispid_setybin;  // Frequently used IDs
	DISPID dispid_startx, dispid_starty;
	DISPID dispid_numx, dispid_numy;
	DISPID dispid_startexposure, dispid_stopexposure;
	DISPID dispid_imageready, dispid_imagearray;
	DISPID dispid_setupdialog, dispid_camerastate;
	DISPID dispid_ispulseguiding, dispid_pulseguide;
	DISPID dispid_cooleron,dispid_setccdtemperature;
	bool ASCOM_SetBin(int mode);
	bool ASCOM_SetROI(int startx, int starty, int numx, int numy);
	bool ASCOM_StartExposure(double duration, bool light);
	bool ASCOM_StopExposure();
	bool ASCOM_ImageReady(bool& ready);
	bool ASCOM_Image(usImage& Image, bool subframe);
	bool ASCOM_IsMoving();
	int DriverVersion;

#endif
};

#endif  //ASCOMCAMDEF
