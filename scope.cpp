/*
 *  scope.cpp
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

#include "image_math.h"
#include "wx/textfile.h"
#include "socket_server.h"

/*#if 0
#if defined (GUIDE_GCUSBST4)
#include "scope_GC_USBST4.h"
#endif

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
        if ((pScope->IsConnected() == MOUNT_CAMERA) && GuideCameraConnected && (CurrentGuideCamera->HasGuiderOutput))
			CurrentGuideCamera->PulseGuideScope(direction,duration);
    #ifdef GUIDE_GPUSB
		else if (pScope->IsConnected() == MOUNT_GPUSB)
			GPUSB_PulseGuideScope(direction,duration);
    #endif
	#ifdef GUIDE_ASCOM
		else if (pScope->IsConnected() == MOUNT_ASCOM)
			ASCOM_PulseGuideScope(direction,duration);
    #endif
    #ifdef GUIDE_GPINT
		else if (pScope->IsConnected() == MOUNT_GPINT3BC)
			GPINT_PulseGuideScope(direction, duration, (short) 0x3BC);
		else if (pScope->IsConnected() == MOUNT_GPINT378)
			GPINT_PulseGuideScope(direction, duration, (short) 0x378);
		else if (pScope->IsConnected() == MOUNT_GPINT278)
			GPINT_PulseGuideScope(direction, duration, (short) 0x278);
    #endif
	#ifdef GUIDE_GCUSBST4
		else if (pScope->IsConnected() == MOUNT_GCUSBST4)
			GCUSBST4_PulseGuideScope(direction,duration);
	#endif
	#ifdef GUIDE_NEB
		else if (pScope->IsConnected() == MOUNT_NEB)
			ServerSendGuideCommand(direction,duration);
    #endif
    #ifdef GUIDE_VOYAGER
		else if (pScope->IsConnected() == MOUNT_VOYAGER)
			Voyager_PulseGuideScope(direction,duration);
    #endif
    #ifdef GUIDE_EQUINOX
		else if (pScope->IsConnected() == MOUNT_EQUINOX)
			Equinox_PulseGuideScope(direction,duration, MOUNT_EQUINOX);
    #endif
	#ifdef GUIDE_EQUINOX
		else if (pScope->IsConnected() == MOUNT_EQMAC)
			Equinox_PulseGuideScope(direction,duration, MOUNT_EQMAC);
	#endif
	#ifdef GUIDE_INDI
        else if (pScope->IsConnected() == MOUNT_INDI)
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
	if (pScope->IsConnected() == MOUNT_GCUSBST4)
		GCUSBST4_Disconnect();
#endif
#if defined (GUIDE_GPUSB)
	if (pScope->IsConnected() == MOUNT_GPUSB) {
		GPUSB_Disconnect();
	}
#endif
	// Nothing needed for GPINT

	pScope->IsConnected() = 0;
}

#endif
*/

void MyFrame::OnConnectScope(wxCommandEvent& WXUNUSED(event)) {
//	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
//	wxMessageBox(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_log.txt"));
    Scope *pNewScope = NULL;

	if (canvas->State > STATE_SELECTED) return;
	if (CaptureActive) return;  // Looping an exposure already
	if (pScope->IsConnected()) pScope->Disconnect();

    #ifdef GUIDE_ASCOM
	else if (mount_menu->IsChecked(MOUNT_ASCOM)) {
        pNewScope = new ScopeASCOM();

        if (pNewScope->Connect())
        {
            SetStatusText("ASCOM connection failed");
        }
        else
        {
			SetStatusText(_T("ASCOM connected"));
        }
	}
	#endif

	#ifdef GUIDE_GPUSB
	if (mount_menu->IsChecked(MOUNT_GPUSB)) {
        pNewScope = new ScopeGpUsb();

		if (pNewScope->Connect()) {
			SetStatusText(_T("GPUSB failed"));
		}
		else {
			SetStatusText(_T("GPUSB connected"));
		}
	}
	#endif

	#ifdef GUIDE_GPINT
	else if (mount_menu->IsChecked(MOUNT_GPINT3BC)) {
        pNewScope = new ScopeGpInt((short) 0x3BC);

        if (pNewScope->Connect())
        {
            SetStatusText("GPINT 3BC connection failed");
        }
        else
        {
            SetStatusText(_T("GPINT 3BC selected"));
        }
	}
	else if (mount_menu->IsChecked(MOUNT_GPINT378)) {
        pNewScope = new ScopeGpInt((short) 0x378);

        if (pNewScope->Connect())
        {
            SetStatusText("GPINT 378 connection failed");
        }
        else
        {
            SetStatusText(_T("GPINT 378 selected"));
        }
	}
	else if (mount_menu->IsChecked(MOUNT_GPINT278)) {
        pNewScope = new ScopeGpInt((short) 0x278);

        if (pNewScope->Connect())
        {
            SetStatusText("GPINT 278 connection failed");
        }
        else
        {
            SetStatusText(_T("GPINT 278 selected"));
        }
	}
    #endif

    #ifdef GUIDE_GCUSBST4
	else if (mount_menu->IsChecked(MOUNT_GCUSBST4)) {
        ScopeGCUSBST4 *pGCUSBST4 = new ScopeGCUSBST4();
        pNewScope = pGCUSBST4;
        if (pNewScope->Connect())
        {
            SetStatusText("GCUSB-ST4 connection failed");
        }
        else
        {
            SetStatusText(_T("GCUSB-ST4 selected"));
        }
	}
    #endif

    #ifdef GUIDE_ONBOARD
	else if (mount_menu->IsChecked(MOUNT_CAMERA)) {
        pNewScope = new ScopeOnCamera();
        if (pNewScope->Connect())
        {
            SetStatusText("OnCamera connection failed");
        }
        else
        {
            SetStatusText(_T("OnCamera selected"));
        }
	}
	#endif
	#ifdef GUIDE_NEB
	else if (mount_menu->IsChecked(MOUNT_NEB)) {
		if (SocketServer)
			pScope->IsConnected() = MOUNT_NEB;
		else
			SetStatusText("Server not running");
	}
	#endif
	#ifdef GUIDE_VOYAGER
	else if (mount_menu->IsChecked(MOUNT_VOYAGER)) {
        ScopeVoyager *pVoyager = new ScopeVoyager();
        pNewScope = pVoyager;

        if (pNewScope->Connect())
        {
            SetStatusText("Voyager localhost failed");

            wxString IPstr = wxGetTextFromUser(_T("Enter IP address"),_T("Voyager not found on localhost"));

            // we have to use the ScopeVoyager pointer to pass the address to connect
            if (pVoyager->Connect(IPstr))
            {
                SetStatusText("Voyager IP failed");
            }
        }

        if (pNewScope->IsConnected())
        {
            SetStatusText(_T("Voyager selected"));
        }
	}
	#endif
    #ifdef GUIDE_EQUINOX
	else if (mount_menu->IsChecked(MOUNT_EQUINOX)) {
        pNewScope = new ScopeEquinox();

        if (pNewScope->Connect())
        {
            SetStatusText("Equinox mount failed");
        }
        else
        {
            SetStatusText(_T("Equinox connected"));
        }
	}
    #endif
	#ifdef GUIDE_EQMAC
	else if (mount_menu->IsChecked(MOUNT_EQMAC)) {
        ScopeEQMac *pEQMac = new ScopeEQMac();
        pNewScope = pEQMac;

        // must use pEquinox to pass an arument to connect
        if (pEQMac->Connect())
        {
            SetStatusText("EQMac mount failed");
        }
        else
        {
            SetStatusText(_T("EQMac connected"));
        }
	}
	#endif
	#ifdef GUIDE_INDI
    else if (mount_menu->IsChecked(MOUNT_INDI)) {
        if (!INDI_ScopeConnect()) {
            pScope->IsConnected() = MOUNT_INDI;
        } else {
            pScope->IsConnected() = 0;
            SetStatusText(_T("INDI mount failed"));
        }
    }
    #endif
	if (pNewScope && pNewScope->IsConnected()) {
        delete pScope;
        pScope = pNewScope;
		SetStatusText(_T("Mount connected"));
		SetStatusText(_T("Scope"),4);
		if (FoundStar) Guide_Button->Enable(true);
	}
	else {
		SetStatusText(_T("No scope"),4);
		Guide_Button->Enable(false);
	}
}

bool Scope::Calibrate(void) {
    bool bError = false;

    try
    {
        double dist;
        bool still_going;
        int iterations, i;
        double dist_crit;

        if (!pScope->IsConnected() || !GuideCameraConnected) {
            throw ERROR_INFO("Both camera and mount must be connected before you attempt to calibrate");
        }

        if (frame->canvas->State != STATE_SELECTED) { // must have a star selected
            throw ERROR_INFO("Must have star selected");
        }

        if (!FoundStar) {// Must have a star
            throw ERROR_INFO("Must have star found");
        }

        // Clear out any previous values
        m_bCalibrated = false;

        double ExpDur = frame->RequestedExposureDuration();
        frame->canvas->State = STATE_CALIBRATING;

        // Get starting point / frame
        if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame)) {
            Abort = 1;
            throw ERROR_INFO("CaptureFull failed above loop");
        }
        if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
            QuickLRecon(CurrentFullFrame);
        else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
            Median3(CurrentFullFrame);
        FindStar(CurrentFullFrame); // Get starting position
        LockX = StarX;
        LockY = StarY;
        frame->canvas->FullFrameToDisplay();

        still_going = true;
        iterations = 0;
        dist_crit = (double) CurrentGuideCamera->FullSize.GetHeight() * 0.05;
        if (dist_crit > 25.0) dist_crit = 25.0;
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
            Guide(WEST,Cal_duration);
            if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame)) {
                Abort = 1;
                throw ERROR_INFO("CaptureFull failed in W calibration");
            }
#if 0
            // Bret TODO: Understand why there were executed after a capture failed
            if (NR_mode == NR_2x2MEAN)
                QuickLRecon(CurrentFullFrame);
            else if (NR_mode == NR_3x3MEDIAN)
                Median3(CurrentFullFrame);
            wxTheApp->Yield();
#endif
            if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
                QuickLRecon(CurrentFullFrame);
            else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
                Median3(CurrentFullFrame);
            FindStar(CurrentFullFrame);
            frame->canvas->FullFrameToDisplay();
            dist = sqrt(dX*dX+dY*dY);
            iterations++;
            frame->SetStatusText(wxString::Format(_T("dx=%.1f dy=%.1f dist=%.1f (%.1f)"),dX,dY,dist,dist_crit),1);
            if (Log_Data) LogFile->AddLine(wxString::Format(_T("RA+ (west),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
            if (iterations > 60) {
                wxMessageBox(_T("RA Calibration failed - Star did not move enough"),_T("Alert"),wxOK | wxICON_ERROR);
                frame->canvas->State = STATE_NONE;
                frame->canvas->Refresh();
                throw ERROR_INFO("RA calibration failed because star did not move enough");
            }
            if (dist > dist_crit) {
                m_dRaRate = dist / (double) (iterations * Cal_duration);
                if (dX == 0.0) dX = 0.00001;
                if (dX > 0.0) m_dRaAngle = atan(dY/dX);
                else if (dY >= 0.0) m_dRaAngle = atan(dY/dX) + PI;
                else m_dRaAngle = atan(dY/dX) - PI;
                still_going = false;
            }
        }
        frame->SetStatusText(wxString::Format(_T("rate=%.2f angle=%.2f"),m_dRaRate*1000,m_dRaAngle),1);
        // Try to return back to the origin
        for (i=0; i<iterations; i++) {
            frame->SetStatusText(wxString::Format(_T("E calibration: %d"),i+1));
            Guide(EAST,Cal_duration);
            if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame)) {
                Abort = 1;
                throw ERROR_INFO("CaptureFull failed in E calibration");
            }
            if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
                QuickLRecon(CurrentFullFrame);
            else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
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
                Guide(NORTH,Cal_duration);
                if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame)){
                    Abort = 1;
                    throw ERROR_INFO("CaptureFull failed clearing Dec backlasth");
                }
                if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
                    QuickLRecon(CurrentFullFrame);
                else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
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
                Guide(NORTH,Cal_duration);
                if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame))
                {
                    Abort = 1;
                    throw ERROR_INFO("CaptureFull failed calibrating N");
                }
                if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
                    QuickLRecon(CurrentFullFrame);
                else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
                    Median3(CurrentFullFrame);
                wxTheApp->Yield();
                FindStar(CurrentFullFrame);
                frame->canvas->FullFrameToDisplay();
                dist = sqrt(dX*dX+dY*dY);
                iterations++;
                frame->SetStatusText(wxString::Format(_T("dx=%.1f dy=%.1f dist=%.1f (%.1f)"),dX,dY,dist,dist_crit),1);
                if (Log_Data) LogFile->AddLine(wxString::Format(_T("Dec+ (north),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
                if (iterations > 60) {
                    wxMessageBox(_T("Dec Calibration failed - turning off Dec guiding"),_T("Alert"),wxOK | wxICON_ERROR);
                    still_going = false;
                    Dec_guide = DEC_OFF;
                    if (Log_Data) LogFile->AddLine(_T("Dec guiding failed during North cal - turned off"));
                    break;
                }
                if (dist > dist_crit) {
                    m_dDecRate = dist / (double) (iterations * Cal_duration);
                    if (dX == 0.0) dX = 0.00001;
                    if (dX > 0.0) m_dDecAngle = atan(dY/dX);
                    else if (dY >= 0.0) m_dDecAngle = atan(dY/dX) + PI;
                    else m_dDecAngle = atan(dY/dX) - PI;
                    still_going = false;
                }
            }
            // Return (or nearly so) if we've not turned it off...
            if (Dec_guide) {
                for (i=0; i<iterations; i++) {
                    frame->SetStatusText(wxString::Format(_T("S calibration: %d"),i+1));
                    Guide(SOUTH,Cal_duration);
                    if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame)) {
                        Abort = 1;
                        throw ERROR_INFO("CaptureFull failed calibrating S");
                    }
                    if (GuideCameraPrefs::NR_mode == NR_2x2MEAN)
                        QuickLRecon(CurrentFullFrame);
                    else if (GuideCameraPrefs::NR_mode == NR_3x3MEDIAN)
                        Median3(CurrentFullFrame);
                    wxTheApp->Yield();
                    FindStar(CurrentFullFrame);
                    if (Log_Data) LogFile->AddLine(wxString::Format(_T("Dec- (south),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
                    frame->canvas->FullFrameToDisplay();
                }
            }
            if (Log_Data) LogFile->Write();
            LockX = StarX;  // re-sync star position
            LockY = StarY;

        }
        if (Log_Data) LogFile->Close();
        frame->SetStatusText(_T("Calibrated"));
        frame->SetStatusText(_T(""),1);
        frame->canvas->State = STATE_SELECTED;
        m_bCalibrated = true;
        frame->SetStatusText(_T("Cal"),5);
    }
    catch (char WXUNUSED(*ErrorMsg))
    {
        if (Abort == 1) {
            frame->canvas->State = STATE_NONE;
            frame->canvas->Refresh(); 
        }
        bError = true;
    }

    return bError;
}

