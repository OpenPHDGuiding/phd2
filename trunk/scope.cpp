/*
 *  scope.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
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
#if defined (__WINDOWS__)
#include "ascom.h"
#endif

#if defined (GUIDE_GCUSBST4)
#include "GC_USBST4.h"
#endif

#include "image_math.h"
#include "wx/textfile.h"
#include "camera.h"
#include "scope.h"
#include "socket_server.h"

#ifdef GUIDE_GPUSB
#include "ShoeString.h"
#endif

#ifdef GUIDE_INDI
#include "tele_INDI.h"
#endif

void GuideScope(int direction, int duration) {
	if (DisableGuideOutput && (frame->canvas->State >= STATE_GUIDING_LOCKED))  // Let you not actually send the commands during guiding
		return;
	try {
        if ((ScopeConnected == MOUNT_CAMERA) && GuideCameraConnected && (CurrentGuideCamera->HasGuiderOutput))
			CurrentGuideCamera->PulseGuideScope(direction,duration);
    #ifdef GUIDE_GPUSB
		else if (ScopeConnected == MOUNT_GPUSB)
			GPUSB_PulseGuideScope(direction,duration);
    #endif
	#ifdef GUIDE_ASCOM
		else if (ScopeConnected == MOUNT_ASCOM)
			ASCOM_PulseGuideScope(direction,duration);
    #endif
    #ifdef GUIDE_PARALLEL
		else if (ScopeConnected == MOUNT_GPINT3BC)
			GPINT_PulseGuideScope(direction, duration, (short) 0x3BC);
		else if (ScopeConnected == MOUNT_GPINT378)
			GPINT_PulseGuideScope(direction, duration, (short) 0x378);
		else if (ScopeConnected == MOUNT_GPINT278)
			GPINT_PulseGuideScope(direction, duration, (short) 0x278);
    #endif
	#ifdef GUIDE_GCUSBST4
		else if (ScopeConnected == MOUNT_GCUSBST4)
			GCUSBST4_PulseGuideScope(direction,duration);
	#endif
	#ifdef GUIDE_NEB
		else if (ScopeConnected == MOUNT_NEB)
			ServerSendGuideCommand(direction,duration);
    #endif
    #ifdef GUIDE_VOYAGER
		else if (ScopeConnected == MOUNT_VOYAGER)
			Voyager_PulseGuideScope(direction,duration);
    #endif
    #ifdef GUIDE_EQUINOX
		else if (ScopeConnected == MOUNT_EQUINOX)
			Equinox_PulseGuideScope(direction,duration);
    #endif
    #ifdef GUIDE_INDI
        else if (ScopeConnected == MOUNT_INDI)
            INDI_PulseGuideScope(direction, duration);
    #endif
	}
	catch (...) {
		wxMessageBox(_T("Exception thrown while trying to send guide command"));
	}

}
void DisconnectScope() {
#if defined (GUIDE_ASCOM)
	if (ScopeDriverDisplay) {  // this defined by ASCOM
		ScopeDriverDisplay->Release();
		ScopeDriverDisplay = NULL;
	}
#endif
#if defined (GUIDE_GCUSBST4)
	if (ScopeConnected == MOUNT_GCUSBST4)
		GCUSBST4_Disconnect();
#endif
#if defined (GUIDE_GPUSB)
	if (ScopeConnected == MOUNT_GPUSB) {
		GPUSB_Disconnect();
	}
#endif
	// Nothing needed for GPINT

	ScopeConnected = 0;
}

void MyFrame::OnConnectScope(wxCommandEvent& WXUNUSED(event)) {
//	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
//	wxMessageBox(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_log.txt"));

	if (canvas->State > STATE_SELECTED) return;
	if (CaptureActive) return;  // Looping an exposure already
	if (ScopeConnected) DisconnectScope();

	#ifdef GUIDE_GPUSB
	if (mount_menu->IsChecked(MOUNT_GPUSB)) {
		if (!GPUSB_Connect()) {
			ScopeConnected = MOUNT_GPUSB;
			SetStatusText(_T("GPUSB connected"));
		}
		else {
			ScopeConnected = 0;
			SetStatusText(_T("GPUSB failed"));
		}
	}
	#endif

    #ifdef GUIDE_ASCOM
	else if (mount_menu->IsChecked(MOUNT_ASCOM)) {
		wxString ScopeID;
		if (!ASCOM_OpenChooser(ScopeID)) { // Put the ASCOM Chooser up and let user select scope
			if (ASCOM_ConnectScope(ScopeID)){
				ScopeConnected = 0;  //
				SetStatusText("ASCOM connection failed");
			}
			else
				ScopeConnected = MOUNT_ASCOM;
		}
	}
	#endif
	#ifdef GUIDE_PARALLEL
	else if (mount_menu->IsChecked(MOUNT_GPINT3BC)) {
		ScopeConnected = MOUNT_GPINT3BC;
		GPINT_Connect((short) 0x3BC);
		SetStatusText(_T("GPINT 3BC selected"));
	}
	else if (mount_menu->IsChecked(MOUNT_GPINT378)) {
		ScopeConnected = MOUNT_GPINT378;
		GPINT_Connect((short) 0x378);
		SetStatusText(_T("GPINT 378 selected"));
	}
	else if (mount_menu->IsChecked(MOUNT_GPINT278)) {
		ScopeConnected = MOUNT_GPINT278;
		GPINT_Connect((short) 0x278);
		SetStatusText(_T("GPINT 278 selected"));
	}
    #endif
    #ifdef GUIDE_GCUSBST4
	else if (mount_menu->IsChecked(MOUNT_GCUSBST4)) {
		if (GCUSBST4_Connect()) {
			ScopeConnected = MOUNT_GCUSBST4;
			SetStatusText(_T("USB ST4 selected"));
		}
		else {
			ScopeConnected = 0;
			SetStatusText(_T("USB ST4 failed"));
		}
	}
    #endif
    #ifdef GUIDE_ONBOARD
	else if (mount_menu->IsChecked(MOUNT_CAMERA)) {
		ScopeConnected = MOUNT_CAMERA;
	}
	#endif
	#ifdef GUIDE_NEB
	else if (mount_menu->IsChecked(MOUNT_NEB)) {
		if (SocketServer)
			ScopeConnected = MOUNT_NEB;
		else
			SetStatusText("Server not running");
	}
	#endif
	#ifdef GUIDE_VOYAGER
	else if (mount_menu->IsChecked(MOUNT_VOYAGER)) {
		if (!Voyager_Connect()) {
			ScopeConnected = MOUNT_VOYAGER;
		}
		else  {
			ScopeConnected = 0;
			SetStatusText(_T("Voyager mount failed"));
		}
	}
	#endif
    #ifdef GUIDE_EQUINOX
	else if (mount_menu->IsChecked(MOUNT_EQUINOX)) {
		if (!Equinox_Connect()) {
			ScopeConnected = MOUNT_EQUINOX;
		}
		else  {
			ScopeConnected = 0;
			SetStatusText(_T("Equinox mount failed"));
		}
	}
    #endif
    #ifdef GUIDE_INDI
    else if (mount_menu->IsChecked(MOUNT_INDI)) {
        if (!INDI_ScopeConnect()) {
            ScopeConnected = MOUNT_INDI;
        } else {
            ScopeConnected = 0;
            SetStatusText(_T("INDI mount failed"));
        }
    }
    #endif
	if (ScopeConnected) {
		SetStatusText(_T("Scope"),4);
		if (FoundStar) Guide_Button->Enable(true);
	}
	else {
		SetStatusText(_T("No scope"),4);
		Guide_Button->Enable(false);
	}
}

void CalibrateScope () {
	double dist;
	bool still_going;
	int iterations, i;
	double dist_crit;
//	wxTextFile* logfile;

	if (!ScopeConnected || !GuideCameraConnected) {
		return;
	}
	if (frame->canvas->State != STATE_SELECTED) return;  // must have a star selected
	if (!FoundStar) return;  // Must have a star

	// Clear out any previous values
	Calibrated = false;
	RA_rate = RA_angle = Dec_rate = Dec_angle = 0.0;

	frame->SetExpDuration();
	frame->canvas->State = STATE_CALIBRATING;
	// Get starting point / frame
	if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame))
		Abort = 1;
	if (Abort) { frame->canvas->State = STATE_NONE; frame->canvas->Refresh(); return; }
//	if (HaveDark) Subtract(CurrentFullFrame,CurrentDarkFrame);
	if (NR_mode == NR_2x2MEAN)
		QuickLRecon(CurrentFullFrame);
	else if (NR_mode == NR_3x3MEDIAN)
		Median3(CurrentFullFrame);
	FindStar(CurrentFullFrame); // Get starting position
	LockX = StarX;
	LockY = StarY;
	frame->canvas->FullFrameToDisplay();

	still_going = true;
	iterations = 0;
	dist_crit = (double) CurrentGuideCamera->FullSize.GetHeight() * 0.05;
	if (dist_crit > 25.0) dist_crit = 25.0;
//	logfile = new wxTextFile(_T("PHD_log.txt"));
//	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
//	logfile = new wxTextFile(LogFName);
	// Do the RA+ calibration
	if (Log_Data) {
		if (LogFile->Exists()) LogFile->Open();
		else LogFile->Create();
		wxDateTime now = wxDateTime::Now();
		LogFile->AddLine(wxString::Format(_T("PHD Guide %s  -- "),VERSION) + now.FormatDate() + _T(" ") + now.FormatTime());
		LogFile->AddLine(_T("Calibration begun"));
		LogFile->AddLine(wxString::Format(_T("lock %.1f %.1f, star %.1f %.1f"),LockX,LockY,StarX,StarY));
		LogFile->AddLine(_T("Direction,Step,dx,dy,x,y"));
		LogFile->Write();
	}

	while (still_going) {
		frame->SetStatusText(wxString::Format(_T("W calibration: %d"),iterations+1));
		GuideScope(WEST,Cal_duration);
		if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame))
			Abort = 1;
//		if (HaveDark) Subtract(CurrentFullFrame,CurrentDarkFrame);
		if (NR_mode == NR_2x2MEAN)
			QuickLRecon(CurrentFullFrame);
		else if (NR_mode == NR_3x3MEDIAN)
			Median3(CurrentFullFrame);
		wxTheApp->Yield();
		if (Abort) { frame->canvas->State = STATE_NONE; frame->canvas->Refresh(); return; }
		if (NR_mode == NR_2x2MEAN)
			QuickLRecon(CurrentFullFrame);
		else if (NR_mode == NR_3x3MEDIAN)
			Median3(CurrentFullFrame);
		FindStar(CurrentFullFrame);
		frame->canvas->FullFrameToDisplay();
		dist = sqrt(dX*dX+dY*dY);
		iterations++;
		frame->SetStatusText(wxString::Format(_T("dx=%.1f dy=%.1f dist=%.1f (%.1f)"),dX,dY,dist,dist_crit),1);
		if (Log_Data) LogFile->AddLine(wxString::Format(_T("RA+ (west),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
//		if (Log_Data) LogFile->AddLine(wxString::Format(_T("RA+ (west) %d, dx= %.1f dy= %.1f x=%.1f y=%.1f"),iterations, dX,dY,StarX,StarY));
		if (iterations > 60) {
			wxMessageBox(_T("RA Calibration failed - Star did not move enough"),_T("Alert"),wxOK | wxICON_ERROR);
			frame->canvas->State = STATE_NONE;
			frame->canvas->Refresh();
			return;
		}
		if (dist > dist_crit) {
			RA_rate = dist / (double) (iterations * Cal_duration);
			//wxMessageBox(wxString::Format("atany_x = %.2f, atan2= %.2f, dx= %.1f dy= %.1f",atan(dY / dX),atan2(dX,dY), dX, dY),_T("info"));
			if (dX == 0.0) dX = 0.00001;
			if (dX > 0.0) RA_angle = atan(dY/dX);
			else if (dY >= 0.0) RA_angle = atan(dY/dX) + PI;
			else RA_angle = atan(dY/dX) - PI;
			still_going = false;
		}
	}
	frame->SetStatusText(wxString::Format(_T("rate=%.2f angle=%.2f"),RA_rate*1000,RA_angle),1);
	// Try to return back to the origin
	for (i=0; i<iterations; i++) {
		frame->SetStatusText(wxString::Format(_T("E calibration: %d"),i+1));
		GuideScope(EAST,Cal_duration);
		if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame))
			Abort = 1;
		if (Abort) { frame->canvas->State = STATE_NONE; frame->canvas->Refresh(); return; }
//		if (HaveDark) Subtract(CurrentFullFrame,CurrentDarkFrame);
		if (NR_mode == NR_2x2MEAN)
			QuickLRecon(CurrentFullFrame);
		else if (NR_mode == NR_3x3MEDIAN)
			Median3(CurrentFullFrame);
		wxTheApp->Yield();
		FindStar(CurrentFullFrame);
		if (Log_Data) LogFile->AddLine(wxString::Format(_T("RA- (east),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
//		if (Log_Data) LogFile->AddLine(wxString::Format(_T("RA- (east) %d, x=%.1f y=%.1f"),iterations, StarX,StarY));
		frame->canvas->FullFrameToDisplay();
	}
	if (Log_Data) LogFile->Write();
	LockX = StarX;  // re-sync star position
	LockY = StarY;

	// Do DEC if pref is set for it
	if (Dec_guide) {
		still_going = true;
		bool in_backlash = true;
		iterations = 0;
		while (in_backlash) {
			frame->SetStatusText(wxString::Format(_T("Clearing Dec backlash: %d"),iterations+1));
			GuideScope(NORTH,Cal_duration);
			if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame))
				Abort = 1;
			if (Abort) { frame->canvas->State = STATE_NONE; frame->canvas->Refresh(); return; }
//			if (HaveDark) Subtract(CurrentFullFrame,CurrentDarkFrame);
			if (NR_mode == NR_2x2MEAN)
				QuickLRecon(CurrentFullFrame);
			else if (NR_mode == NR_3x3MEDIAN)
				Median3(CurrentFullFrame);
			wxTheApp->Yield();
			FindStar(CurrentFullFrame);
			frame->canvas->FullFrameToDisplay();
			dist = sqrt(dX*dX+dY*dY);
			iterations++;
			if (abs(dist) >= 3.0) in_backlash = false;
			else if (iterations > 80) {
				wxMessageBox(_T("Can't seem to get star to move in Dec - turning off Dec guiding"),_T("Alert"),wxOK | wxICON_ERROR);
				still_going = false;
				in_backlash = false;
				Dec_guide = DEC_OFF;
				if (Log_Data) LogFile->AddLine(_T("Dec guiding failed during backlash removal - turned off"));
			}

		}
		LockX = StarX;  // re-sync star position
		LockY = StarY;
		iterations = 0;
		while (still_going && Dec_guide) { // do Dec +
			frame->SetStatusText(wxString::Format(_T("N calibration: %d"),iterations+1));
			GuideScope(NORTH,Cal_duration);
			if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame))
				Abort = 1;
			if (Abort) { frame->canvas->State = STATE_NONE; frame->canvas->Refresh(); return; }
//			if (HaveDark) Subtract(CurrentFullFrame,CurrentDarkFrame);
			if (NR_mode == NR_2x2MEAN)
				QuickLRecon(CurrentFullFrame);
			else if (NR_mode == NR_3x3MEDIAN)
				Median3(CurrentFullFrame);
			wxTheApp->Yield();
			FindStar(CurrentFullFrame);
			frame->canvas->FullFrameToDisplay();
			dist = sqrt(dX*dX+dY*dY);
			iterations++;
			frame->SetStatusText(wxString::Format(_T("dx=%.1f dy=%.1f dist=%.1f (%.1f)"),dX,dY,dist,dist_crit),1);
//			if (Log_Data) LogFile->AddLine(wxString::Format(_T("Dec+ (north) %d, dx= %.1f dy= %.1f x=%.1f y=%.1f"),iterations, dX,dY,StarX,StarY));
			if (Log_Data) LogFile->AddLine(wxString::Format(_T("Dec+ (north),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
			if (iterations > 60) {
				wxMessageBox(_T("Dec Calibration failed - turning off Dec guiding"),_T("Alert"),wxOK | wxICON_ERROR);
				still_going = false;
				Dec_guide = DEC_OFF;
				if (Log_Data) LogFile->AddLine(_T("Dec guiding failed during North cal - turned off"));
//				frame->canvas->State = STATE_NONE;
//				frame->canvas->Refresh();
				break;
//				return;
			}
			if (dist > dist_crit) {
				Dec_rate = dist / (double) (iterations * Cal_duration);
				//wxMessageBox(wxString::Format("atany_x = %.2f, atan2= %.2f, dx= %.1f dy= %.1f",atan(dY / dX),atan2(dX,dY), dX, dY),_T("info"));
				if (dX == 0.0) dX = 0.00001;
				if (dX > 0.0) Dec_angle = atan(dY/dX);
				else if (dY >= 0.0) Dec_angle = atan(dY/dX) + PI;
				else Dec_angle = atan(dY/dX) - PI;
				still_going = false;
			}
		}
		// Return (or nearly so) if we've not turned it off...
		if (Dec_guide) {
			for (i=0; i<iterations; i++) {
				frame->SetStatusText(wxString::Format(_T("S calibration: %d"),i+1));
				GuideScope(SOUTH,Cal_duration);
				if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame))
					Abort = 1;
				if (Abort) { frame->canvas->State = STATE_NONE; frame->canvas->Refresh(); return; }
	//			if (HaveDark) Subtract(CurrentFullFrame,CurrentDarkFrame);
				if (NR_mode == NR_2x2MEAN)
					QuickLRecon(CurrentFullFrame);
				else if (NR_mode == NR_3x3MEDIAN)
					Median3(CurrentFullFrame);
				wxTheApp->Yield();
				FindStar(CurrentFullFrame);
//				if (Log_Data) LogFile->AddLine(wxString::Format(_T("Dec- (south) %d, x=%.1f y=%.1f"),iterations, StarX,StarY));
				if (Log_Data) LogFile->AddLine(wxString::Format(_T("Dec- (south),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
				frame->canvas->FullFrameToDisplay();
			}
		}
		if (Log_Data) LogFile->Write();
		LockX = StarX;  // re-sync star position
		LockY = StarY;

	}
	if (Log_Data) LogFile->Close();
//	delete logfile;
	frame->SetStatusText(_T("Calibrated"));
	frame->SetStatusText(_T(""),1);
	frame->canvas->State = STATE_SELECTED;
	Calibrated = true;
	frame->SetStatusText(_T("Cal"),5);
//	frame->Recal_Checkbox->SetValue(false);
}

