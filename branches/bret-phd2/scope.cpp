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

/*
#if 0
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
	if (DisableGuideOutput && (frame->pGuider->State >= STATE_GUIDING_LOCKED))  // Let you not actually send the commands during guiding
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

Scope::Scope(void)
{
    m_calibrationSteps = 0;

    int calDuration = pConfig->GetInt("/scope/CalibrationDuration", 750);

    SetParms(calDuration);
}

Scope::~Scope(void)
{
}


bool Scope::SetParms(int calibrationDuration)
{
    bool bError = false;

    try
    {
        if (calibrationDuration <= 0.0)
        {
            throw ERROR_INFO("invalid calibrationDuration");
        }

        m_calibrationDuration = calibrationDuration;

        pConfig->SetInt("/scope/CalibrationDuration", m_calibrationDuration);
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
        Debug.Write(wxString::Format("Scope::SetParms() caught an exception: %s\n", pErrorMsg));
    }

    Debug.Write(wxString::Format("Scope::SetParms() returns %d, m_calibrationDuration=%d\n", bError, m_calibrationDuration));

    return bError;
}

void MyFrame::OnConnectScope(wxCommandEvent& WXUNUSED(event)) {
//	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
//	wxMessageBox(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_log.txt"));
    Scope *pNewScope = NULL;

	if (pGuider->GetState() > STATE_SELECTED) return;
	if (CaptureActive) return;  // Looping an exposure already
	if (pScope->IsConnected()) pScope->Disconnect();

    if (false)
    {
        // this dummy if is here because otherwise we can't have the 
        // else if construct below, since we don't know which camera
        // will be first.
        //
        // With this here and always false, the rest can safely begin with
        // else if 
    }
#ifdef GUIDE_ASCOM
    else if (mount_menu->IsChecked(MOUNT_ASCOM)) {
        pNewScope = new ScopeASCOM();

        if (pNewScope->Connect())
        {
            SetStatusText("FAIL: ASCOM connection");
        }
        else
        {
			SetStatusText(_T("ASCOM connected"));
        }
	}
	#endif

	#ifdef GUIDE_GPUSB
    else if (mount_menu->IsChecked(MOUNT_GPUSB)) {
        pNewScope = new ScopeGpUsb();

		if (pNewScope->Connect()) {
			SetStatusText(_T("FAIL: GPUSB"));
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
            SetStatusText("FAIL: GPINT 3BC connection");
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
            SetStatusText("FAIL: GPINT 378 connection");
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
            SetStatusText("FAIL: GPINT 278 connection");
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
            SetStatusText("FAIL: GCUSB-ST4 connection");
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
            SetStatusText("FAIL: OnCamera connection");
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
            SetStatusText("FAIL: Voyager localhost");

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
            SetStatusText("FAIL: Equinox mount");
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
            SetStatusText("FAIL: EQMac mount");
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
            SetStatusText(_T("FAIL: INDI mount"));
        }
    }
#endif
	if (pNewScope && pNewScope->IsConnected()) {
        delete pScope;
        pScope = pNewScope;
		SetStatusText(_T("Mount connected"));
		SetStatusText(_T("Scope"),4);
        // now store the scope we selected so we can use it as the default next time.
        wxMenuItemList items = mount_menu->GetMenuItems();
        wxMenuItemList::iterator iter;

        for(iter = items.begin(); iter != items.end(); iter++)
        {
            wxMenuItem *pItem = *iter;

            if (pItem->IsChecked())
            {
                wxString value = pItem->GetItemLabelText();
                pConfig->SetString("/scope/LastMenuChoice", pItem->GetItemLabelText());
            }
        }
	}
	else
    {
		SetStatusText(_T("No scope"),4);
	}

    UpdateButtonsStatus();
}

bool Scope::BeginCalibration(Guider *pGuider)
{
    bool bError = false;

    try
    {

        if (!pScope->IsConnected() || !GuideCameraConnected) 
        {
            throw ERROR_INFO("Both camera and mount must be connected before you attempt to calibrate");
        }

        if (pGuider->GetState() != STATE_SELECTED) 
        { 
            // must have a star selected
            throw ERROR_INFO("Must have star selected");
        }

        if (!pGuider->LockPosition().IsValid()) 
        {
            throw ERROR_INFO("Must have a valid lock position");
        }

        m_bCalibrated = false;
        m_calibrationSteps = 0;
        m_backlashSteps = MAX_CALIBRATION_STEPS;
        m_calibrationStartingLocation = pGuider->CurrentPosition();
        m_calibrationDirection = NONE;
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
    }

    return bError;
}

wxString Scope::GetCalibrationStatus(double dX, double dY, double dist, double dist_crit)
{
    char directionName = '?';
    wxString sReturn;

    switch (m_calibrationDirection)
    {
        case NORTH:
            directionName = 'N';
            break;
        case SOUTH:
            directionName = 'S';
            break;
        case EAST:
            directionName = 'E';
            break;
        case WEST:
            directionName = 'W';
            break;
    }

    if (m_calibrationDirection != NONE)
    {
        if (m_calibrationDirection == NORTH && m_backlashSteps > 0)
        {
            frame->SetStatusText(wxString::Format(_T("Clear Backlash: %2d"), MAX_CALIBRATION_STEPS - m_calibrationSteps));
        }
        else
        {
            frame->SetStatusText(wxString::Format(_T("%c calibration: %2d"), directionName, m_calibrationSteps));
        }
        sReturn = wxString::Format(_T("dx=%4.1f dy=%4.1f dist=%4.1f (%4.1f)"),dX,dY,dist,dist_crit);
        Debug.Write(wxString::Format(_T("dx=%4.1f dy=%4.1f dist=%4.1f (%4.1f)\n"),dX,dY,dist,dist_crit));
    }

    return sReturn;
}

bool Scope::UpdateCalibrationState(Guider *pGuider)
{
    bool bError = false;

    try
    {
        if (m_calibrationDirection == NONE)
        {
            m_calibrationDirection = WEST;
            m_calibrationStartingLocation = pGuider->CurrentPosition();
        }

        double dX = m_calibrationStartingLocation.dX(pGuider->CurrentPosition());
        double dY = m_calibrationStartingLocation.dY(pGuider->CurrentPosition());
        double dist = m_calibrationStartingLocation.Distance(pGuider->CurrentPosition());
        double dist_crit = wxMax(CurrentGuideCamera->FullSize.GetHeight() * 0.05, MAX_CALIBRATION_DISTANCE);

        wxString statusMessage = GetCalibrationStatus(dX, dY, dist, dist_crit);

        /*
         * There are 3 sorts of motion that can happen during calibration. We can be:
         *   1. computing calibration data when moving WEST or NORTH
         *   2. returning to center after one of thoese moves (moving EAST or SOUTH)
         *   3. clearing dec backlash (before the NORTH move)
         *
         */

        if (m_calibrationDirection == NORTH && m_backlashSteps > 0)
        {
            // this is the "clearing dec backlash" case
            if (dist >= DEC_BACKLASH_DISTANCE)
            {
                assert(m_calibrationSteps == 0);
                m_calibrationSteps = 1;
                m_backlashSteps = 0;
                m_calibrationStartingLocation = pGuider->CurrentPosition();
            }
            else if (--m_backlashSteps <= 0)
            {
                wxMessageBox(_T("Unable to clear DEC backlash -- turning off Dec guiding"), _T("Alert"), wxOK | wxICON_ERROR);
#ifdef PREFS
                Dec_guide = DEC_OFF;
#endif
                m_calibrationDirection = NONE;
            }
        }
        else if (m_calibrationDirection == WEST || m_calibrationDirection == NORTH)
        {
            // this is the moving over in WEST or NORTH case
            //
            if (dist >= dist_crit) // have we moved far enough yet?
            {
                if (m_calibrationDirection == WEST)
                {
                    m_dRaAngle = m_calibrationStartingLocation.Angle(pGuider->CurrentPosition());
                    m_dRaRate = dist/(m_calibrationSteps*m_calibrationDuration);
                    m_calibrationDirection = EAST;

                    Debug.Write(wxString::Format("WEST calibration completes with angle=%.2f rate=%.2f\n", m_dRaAngle, m_dRaRate));
                }
                else
                {
                    assert(m_calibrationDirection == NORTH);
                    m_dDecAngle = m_calibrationStartingLocation.Angle(pGuider->CurrentPosition());
                    m_dDecRate = dist/(m_calibrationSteps*m_calibrationDuration);
                    m_calibrationDirection = SOUTH;

                    Debug.Write(wxString::Format("NORTH calibration completes with angle=%.2f rate=%.2f\n", m_dDecAngle, m_dDecRate));
                }
            }
            else if (m_calibrationSteps++ >= MAX_CALIBRATION_STEPS)
            {
                wchar_t *pDirection = m_calibrationDirection == NORTH ? pDirection = _T("Dec"): _T("RA");

                wxMessageBox(wxString::Format(_T("%s Calibration failed - Star did not move enough"), pDirection), _T("Alert"), wxOK | wxICON_ERROR);

                pGuider->SetState(STATE_UNINITIALIZED);

                throw ERROR_INFO("Calibrate failed");
            }
        }
        else
        {
            // this is the moving back in EAST or SOUTH case

            if(--m_calibrationSteps == 0)
            {
                if (m_calibrationDirection == EAST)
                {
                    m_calibrationDirection = NORTH;
                    dX = dY = dist = 0.0;
                    statusMessage = GetCalibrationStatus(dX, dY, dist, dist_crit);
                }
                else
                {
                    assert(m_calibrationDirection == SOUTH);
                    m_calibrationDirection = NONE;
                }
            }
        }

        if (m_calibrationDirection == NONE)
        {
            m_bCalibrated = true;
            pGuider->SetState(STATE_CALIBRATED);
            frame->SetStatusText(_T("calibration complete"),1);
        }
        else
        {
            frame->ScheduleGuide(m_calibrationDirection, m_calibrationDuration, statusMessage);
        }
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    return bError;
}

#if 0

    try
    {
        double dist;
        bool still_going;
        int iterations, i;
        double dist_crit;

        // Clear out any previous values
        m_bCalibrated = false;

        double ExpDur = frame->RequestedExposureDuration();
        frame->pGuider->SetState(STATE_CALIBRATING);

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
        frame->pGuider->FullFrameToDisplay();

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
			if( Abort != 0 )
				return false;

            frame->SetStatusText(wxString::Format(_T("W calibration: %d"),iterations+1));
            Guide(WEST,m_calibrationDuration);
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
            frame->pGuider->FullFrameToDisplay();
            dist = sqrt(dX*dX+dY*dY);
            iterations++;
            frame->SetStatusText(wxString::Format(_T("dx=%.1f dy=%.1f dist=%.1f (%.1f)"),dX,dY,dist,dist_crit),1);
            if (Log_Data) LogFile->AddLine(wxString::Format(_T("RA+ (west),%d,%.1f,%.1f,%.1f,%.1f"),iterations, dX,dY,StarX,StarY));
            if (iterations > 60) {
                wxMessageBox(_T("RA Calibration failed - Star did not move enough"),_T("Alert"),wxOK | wxICON_ERROR);
                frame->pGuider->SetState(STATE_UNINITIALIZED);
                frame->pGuider->Refresh();
                throw ERROR_INFO("RA calibration failed because star did not move enough");
            }
            if (dist > dist_crit) {
                m_dRaRate = dist / (double) (iterations * m_calibrationDuration);
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
			
			if( Abort != 0 )
				return false;
            
			frame->SetStatusText(wxString::Format(_T("E calibration: %d"),i+1));
            Guide(EAST,m_calibrationDuration);
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
            frame->pGuider->FullFrameToDisplay();
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
				if( Abort != 0 )
					return false;
                frame->SetStatusText(wxString::Format(_T("Clearing Dec backlash: %d"),iterations+1));
                Guide(NORTH,m_calibrationDuration);
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
                frame->pGuider->FullFrameToDisplay();
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
				if( Abort != 0 )
					return false;
                frame->SetStatusText(wxString::Format(_T("N calibration: %d"),iterations+1));
                Guide(NORTH,m_calibrationDuration);
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
                frame->pGuider->FullFrameToDisplay();
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
                    m_dDecRate = dist / (double) (iterations * m_calibrationDuration);
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
					if( Abort != 0 )
						return false;
                    frame->SetStatusText(wxString::Format(_T("S calibration: %d"),i+1));
                    Guide(SOUTH,m_calibrationDuration);
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
                    frame->pGuider->FullFrameToDisplay();
                }
            }
            if (Log_Data) LogFile->Write();
            LockX = StarX;  // re-sync star position
            LockY = StarY;

        }
        if (Log_Data) LogFile->Close();
        frame->SetStatusText(_T("Calibrated"));
        frame->SetStatusText(_T(""),1);
        frame->pGuider->SetState(STATE_CALIBRATED);
        m_bCalibrated = true;
        frame->SetStatusText(_T("Cal"),5);
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        if (Abort == 1) {
            frame->pGuider->SetState(STATE_UNINITIALIZED);
            frame->pGuider->Refresh(); 
        }
        bError = true;
    }

#endif
