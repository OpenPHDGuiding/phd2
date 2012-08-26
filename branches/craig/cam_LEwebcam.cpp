/*
 *  cam_LEwebcam.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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
#include "time.h"
#include "image_math.h"
#include <wx/timer.h>
#include "cameras/ShoestringLXUSB_DLL.h"
#include "cam_LEwebcam.h"

/* ----Prototypes of Inp and Out32--- */
short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);

#ifdef __WINDOWS__
LPCWSTR MakeLPCWSTR(char* mbString)
{
	int len = strlen(mbString) + 1;
	wchar_t *ucString = new wchar_t[len];
	mbstowcs(ucString, mbString, len);
	return (LPCWSTR)ucString;
}
#endif

Camera_LEwebcamClass::Camera_LEwebcamClass() {
	Connected = FALSE;
//	HaveBPMap = FALSE;
//	NBadPixels=-1;
	Name=_T("Long exposure webcam");
	FullSize = wxSize(640,480);  // should be overwritten
	VFW_Window = NULL; Extra_Window=NULL; 
	HasPropertyDialog = true;
	HasDelayParam = true;
	HasPortNum = true;
}


bool Camera_LEwebcamClass::Connect() {
// returns true on error
//	bool retval;
	int ndevices, i, devicenum;
	wxSplitterWindow *dispwin;
	wxVideoCaptureWindow* capwin;

	if (!Extra_Window) {
		dispwin = new wxSplitterWindow(frame->canvas,-1);
		Extra_Window = dispwin;
	}
	else dispwin = Extra_Window;

	if (!VFW_Window) {
		capwin = new wxVideoCaptureWindow(dispwin,WIN_VFW,wxPoint(0,0),wxSize(640,480));
		VFW_Window = capwin;
	}
	else capwin = VFW_Window;

	dispwin->Show(false);
	//capwin->Create(frame);
	ndevices = capwin->GetDeviceCount();
	if (ndevices == 0) return true;
	devicenum = 1;
	if (ndevices > 1) { // multiple found -- get one from user
		wxArrayString devnames;
		for (i=0; i<ndevices; i++)
			devnames.Add(capwin->GetDeviceName(i));
		devicenum = wxGetSingleChoiceIndex(_T("Select capture device"),_T("Camera choice"),devnames);
		if (devicenum == -1)
			return true;
		else devicenum = devicenum + 1;
	}
	if (capwin->DeviceConnect(devicenum-1) == false)  // try to connect
		return true;

	if (VFW_Window->HasVideoFormatDialog()) {
		VFW_Window->VideoFormatDialog();
//		int w,h,bpp;
//		FOURCC fourcc;
//		VFW_Window->GetVideoFormat( &w,&h, &bpp, &fourcc );
//		FullSize = wxSize(w,h);
	}

	int w,h,bpp;
	FOURCC fourcc;
	capwin->GetVideoFormat( &w,&h, &bpp, &fourcc );
//	capwin->SetVideoFormat(640,480,-1,-1);
	FullSize=wxSize(w,h);   
	frame->SetStatusText(wxString::Format("%d x %d mode activated",w,h),1);
	LastPort = Port; // This needed to detect changes to/from the serial port
	if (Port == 0) {  // LXUSB
		if (!LXUSB_Open()) {
			wxMessageBox(_T("Cannot find LXUSB interface"),_T("Error"),wxOK | wxICON_ERROR);
			return true;
		}
		LXUSB_Reset();
		LXUSB_LEDOff();
	}
	else if (Port < 17) { // One of the serial COM ports
		BOOL fSuccess;
		char PortName[6];

		sprintf(PortName,"COM%d",Port);
		hCom = CreateFile( MakeLPCWSTR(PortName),
				GENERIC_READ | GENERIC_WRITE,
				0,    // must be opened with exclusive-access
				NULL, // no security attributes
				OPEN_EXISTING, // must use OPEN_EXISTING
				0,    // not overlapped I/O
				NULL  // hTemplate must be NULL for comm devices
				);
		if (hCom == INVALID_HANDLE_VALUE) {
			wxMessageBox(wxString::Format("Could not attach to %s"), PortName);
			return true;
		}
		else {  // Valid basic connection - configure
			GetCommState(hCom, &dcb);
			dcb.BaudRate = CBR_2400;     // set the baud rate
			dcb.ByteSize = 8;             // data size, xmit, and rcv
			dcb.Parity = NOPARITY;        // no parity bit
			dcb.StopBits = ONESTOPBIT;    // one stop bit
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
			fSuccess = SetCommState(hCom, &dcb);
			if (fSuccess) {
				EscapeCommFunction(hCom,CLRRTS);  // clear RTS line and DTR line
				EscapeCommFunction(hCom,CLRDTR);  
			}
			else {
				wxMessageBox(wxString::Format("Cannot configure ",PortName));
				return true;
			}
		}
	}

	Connected = true;
	return false;
}

bool Camera_LEwebcamClass::Disconnect() {
 	if (VFW_Window->IsDeviceConnected()) {
		VFW_Window->DeviceDisconnect();
	}
	Connected = false;
	CurrentGuideCamera = NULL;
	GuideCameraConnected = false;
	if (Port == 0) {
		LXUSB_Reset();
		LXUSB_Close();
	}
	else if (Port < 17) {
		CloseHandle(hCom);
	}
	VFW_Window = NULL;
//	Extra_Window = NULL;
	return false;
}

bool Camera_LEwebcamClass::ChangePorts() {
// User can actually change LE port component while camera connected for IO-based ports - take care of this
	bool OpenSerial = false;
	if ((LastPort < 17) && (Port < 17)) { // Change from one COM to another
		CloseHandle(hCom);
		OpenSerial=true;
	}
	else if ((LastPort < 17) && (Port > 17)) { // Change from Serial to Parallel
		CloseHandle(hCom);
	}
	else if ((LastPort > 17) && (Port < 17)) { // Change from Parallel to Serial
		OpenSerial=true;
	}
	// Nothing to do for Parallel/parallel changes
	if (OpenSerial) {
		BOOL fSuccess;
		char PortName[6];

		sprintf(PortName,"COM%d",Port);
		hCom = CreateFile( MakeLPCWSTR(PortName),
				GENERIC_READ | GENERIC_WRITE,
				0,    // must be opened with exclusive-access
				NULL, // no security attributes
				OPEN_EXISTING, // must use OPEN_EXISTING
				0,    // not overlapped I/O
				NULL  // hTemplate must be NULL for comm devices
				);
		if (hCom == INVALID_HANDLE_VALUE) {
			wxMessageBox(wxString::Format("Could not attach to %s"), PortName);
			return true;
		}
		else {  // Valid basic connection - configure
			GetCommState(hCom, &dcb);
			dcb.BaudRate = CBR_2400;     // set the baud rate
			dcb.ByteSize = 8;             // data size, xmit, and rcv
			dcb.Parity = NOPARITY;        // no parity bit
			dcb.StopBits = ONESTOPBIT;    // one stop bit
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
			fSuccess = SetCommState(hCom, &dcb);
			if (fSuccess) {
				EscapeCommFunction(hCom,CLRRTS);  // clear RTS line and DTR line
				EscapeCommFunction(hCom,CLRDTR);  
			}
			else {
				wxMessageBox(wxString::Format("Cannot configure ",PortName));
				return true;
			}
		}
	}
	return false;  // all OK
}

bool Camera_LEwebcamClass::CaptureFull(int duration, usImage& img, bool recon) {
	int xsize,ysize, i;
//	int NFrames = 0;
	xsize = FullSize.GetWidth();
	ysize = FullSize.GetHeight();
	unsigned short *dptr;
	unsigned char *imgdata; //, *ptr1, *ptr2, *ptr3;
//	bool still_going = true;
	bool AmpOff = true;
	wxImage cap_img;
	int amp_lead, read_delay, bulk_delay, final_delay;
	unsigned short reg;

	if (LastPort != Port) 
		if (ChangePorts()) return true;
	LastPort = Port;
	if (img.NPixels != (xsize*ysize)) {
		if (img.Init(xsize,ysize)) {
 			wxMessageBox(_T("Memory allocation error during capture"),wxT("Error"),wxOK | wxICON_ERROR);
			Disconnect();
			return true;
		}
	}
//	dptr = img.ImageData;
//	for (i=0; i<img.NPixels; i++, dptr++)
//		*dptr = (unsigned short) 0;



	amp_lead = 250;  // was 10
	read_delay = Delay;
	// Note: Data lines on the camera when using the parallel port are:
	// D0: Frame1
	// D1: unused
	// D2: Amp
	// D3: Shutter
	if (Port == 0) LXUSB_AllControlDeassert();
	else if (Port < 17) {
		EscapeCommFunction(hCom,SETDTR);
		EscapeCommFunction(hCom,SETRTS);
	}

	bulk_delay = duration - 500;  // 1-500ms, we run flat out.  More, we allow for aborts during
	if (bulk_delay < 0) bulk_delay = 0;
	final_delay = duration - bulk_delay;	// This is what is left over after the bulk delay
	if (AmpOff) { // start the exposure with the amp offWoAt t
		if (Port == 0) 
			LXUSB_SetAll(LXUSB_FRAME1_ASSERTED,LXUSB_FRAME2_ASSERTED,LXUSB_SHUTTER_ASSERTED,LXUSB_CCDAMP_ASSERTED,LXUSB_LED_ON_RED);
		else if (Port < 17) // Serial
			EscapeCommFunction(hCom,SETDTR);
		else {
			reg = Inp32(Port) & 0xF2; // get current state and clear off lower 4 bits, saving others (exp-start = 0000)
			Out32(Port,reg);
		}
	}
	else {
		if (Port == 0) 
			LXUSB_SetAll(LXUSB_FRAME1_ASSERTED,LXUSB_FRAME2_ASSERTED,LXUSB_SHUTTER_ASSERTED,LXUSB_CCDAMP_DEASSERTED,LXUSB_LED_ON_RED);
		else if (Port < 17) // Serial
			EscapeCommFunction(hCom,CLRDTR);
		else {
			reg = Inp32(Port) & 0xF2; // get current state and clear off lower 4 bits (exp-start = 00X0)
			reg = reg ^ 0x04; // but don't have the amp control (0100) so amp is "on" all the time
			Out32(Port,reg);
		}
	}

//	frame->SetStatusText(wxString::Format("%d %d %d",bulk_delay,final_delay,amp_lead),1);
//	wxTheApp->Yield();

	if (bulk_delay) {
		while (bulk_delay > 250) {
			SleepEx(245,true);

			bulk_delay = bulk_delay - 245;
			wxTheApp->Yield();
		}
		SleepEx(bulk_delay,false);
	}
//	frame->SetStatusText(wxString::Format("%d %d %d",bulk_delay,final_delay,amp_lead),1);
//	wxTheApp->Yield();
	SleepEx(final_delay - amp_lead,true);  // wait for last bit (or only bit) of exposure duration

	// switch amp back on
	if (Port == 0) {
		LXUSB_LEDGreen();  
		LXUSB_CCDAmpDeassert();
	}
	else if (Port < 17) // Serial
		EscapeCommFunction(hCom,CLRDTR);
	else {
		//reg = Inp32(Port) ^ 0x04; // get current state & turn amp on (XXXX X1XX)
		reg = Inp32(Port) & 0xF0;	reg = reg ^ 0x04;  // xxxx 01X0
		Out32(Port,reg);
	}
	SleepEx(amp_lead,false);

	if (Port == 0)// hit the frame xfer
		LXUSB_Frame1Deassert();
	else if (Port < 17) // Serial
		EscapeCommFunction(hCom,CLRRTS);
	else {
		//reg = Inp32(Port) & 0xFE; // get current state flip D0 off (XXXX XXX0)
		reg = Inp32(Port) & 0xF2; reg = reg ^ 0x05;	// XXXX 01X1
		Out32(Port,reg);
	}
	if (read_delay) SleepEx(read_delay,false);  // wait the short delay

	//VFW_Window->SnapshotTowxImage(cap_img);  // grab the image

	wxImage cap_img1,cap_img2, cap_img3;
	int sum1, sum2, sum3;
	unsigned char *ptr1, *ptr2, *ptr3;
	VFW_Window->SnapshotTowxImage(cap_img);  // grab the image
	cap_img1 = cap_img.Copy();
	//SleepEx(100,false);
	VFW_Window->SnapshotTowxImage(cap_img);  // grab the image
	cap_img2 = cap_img.Copy();
	//SleepEx(100,false);
	VFW_Window->SnapshotTowxImage(cap_img);  // grab the image
	cap_img3 = cap_img.Copy();

	if (Port == 0) LXUSB_LEDOff();
	if (Port < 17) {
		EscapeCommFunction(hCom,SETDTR);
		EscapeCommFunction(hCom,SETRTS);
	}
	sum1=sum2=sum3=0;
	ptr1 = cap_img1.GetData();
	ptr2 = cap_img2.GetData();
	ptr3 = cap_img3.GetData();
	for (i=0; i<img.NPixels; i+=10, ptr1+=30, ptr2+=30, ptr3+=30) {
		sum1 += *ptr1;
		sum2 += *ptr2;
		sum3 += *ptr3;
	}
	imgdata = cap_img1.GetData();
	if ((sum1 >= sum2) && (sum1 >= sum3))
		imgdata = cap_img1.GetData();
	else if ((sum2 >= sum1) && (sum2>=sum3)) 
		imgdata = cap_img2.GetData();
	else if ((sum3 >= sum1) && (sum3>=sum2)) 
		imgdata = cap_img3.GetData();
	//frame->SetStatusText(wxString::Format("%d %d %d",sum1/10000,sum2/10000,sum3/10000),1);
	

	//imgdata = cap_img.GetData();
	dptr = img.ImageData;
	for (i=0; i<img.NPixels; i++, dptr++, imgdata+=3) {
		*dptr = (unsigned short) (*imgdata + *(imgdata+1) + *(imgdata+2));
	}
	if (HaveDark && recon) Subtract(img,CurrentDarkFrame);

	return false;
}

/*bool Camera_LEwebcamClass::CaptureCrop(int duration, usImage& img) {

	return true;
}	// Captures a cropped portion
*/

void Camera_LEwebcamClass::ShowPropertyDialog() {
	if (VFW_Window->HasVideoSourceDialog()) VFW_Window->VideoSourceDialog();
}