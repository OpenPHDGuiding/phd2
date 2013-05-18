/*
 *  eegg.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2007-2010 Craig Stark.
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
#include "scope.h"

void TestGuide() {

	wxMessageBox(_T("W RA+")); wxTheApp->Yield(); GuideScope(WEST,2000); wxTheApp->Yield();
	wxMessageBox(_T("N Dec+"));  wxTheApp->Yield(); GuideScope(NORTH,2000);wxTheApp->Yield();
	wxMessageBox(_T("E RA-"));  wxTheApp->Yield(); GuideScope(EAST,2000);wxTheApp->Yield();
	wxMessageBox(_T("S Dec-"));  wxTheApp->Yield(); GuideScope(SOUTH,2000);wxTheApp->Yield();
	wxMessageBox(_T("Done"));
}

void MyFrame::OnEEGG(wxCommandEvent &evt) {

	if ((evt.GetId() == EEGG_TESTGUIDEDIR) && (ScopeConnected))
		TestGuide();
	else if (evt.GetId() == EEGG_RANDOMMOTION) {
		RandomMotionMode = !RandomMotionMode;
		wxMessageBox(wxString::Format(_T("Random motion mode set to %d"),(int) RandomMotionMode));
	}
	else if (evt.GetId() == EEGG_MANUALCAL) {
		wxString tmpstr;
		tmpstr = wxGetTextFromUser(_T("Enter parameter (e.g. 0.005)"), _T("RA rate"), wxString::Format(_T("%.4f"),RA_rate));
		if (tmpstr.IsEmpty()) return;
		tmpstr.ToDouble(&RA_rate); // = 0.0035;
		tmpstr = wxGetTextFromUser(_T("Enter parameter (e.g. 0.005)"), _T("Dec rate"), wxString::Format(_T("%.4f"),Dec_rate));
		if (tmpstr.IsEmpty()) return;
		tmpstr.ToDouble(&Dec_rate); // = 0.0035;
		tmpstr = wxGetTextFromUser(_T("Enter parameter (e.g. 0.5)"), _T("RA angle"), wxString::Format(_T("%.3f"),RA_angle));
		if (tmpstr.IsEmpty()) return;
		tmpstr.ToDouble(&RA_angle); // = 0.0035;
		tmpstr = wxGetTextFromUser(_T("Enter parameter (e.g. 2.1)"), _T("Dec angle"), wxString::Format(_T("%.3f"),Dec_angle));
		if (tmpstr.IsEmpty()) return;
		tmpstr.ToDouble(&Dec_angle); // = 0.0035;
		Calibrated = true;
		SetStatusText(_T("Cal"),5);
	}
	else if (evt.GetId() == EEGG_CLEARCAL) {
		Calibrated=false; // clear calibration
		SetStatusText(_T("No cal"),5);
	}
	else if (evt.GetId() == EEGG_FLIPRACAL) {
		if (!Calibrated) 
			return;
		double orig=RA_angle;
		RA_angle = RA_angle + 3.14;
		if (RA_angle > 3.14)
			RA_angle = RA_angle - 6.28;
		wxMessageBox(wxString::Format(_T("RA calibration angle flipped: %.2f to %.2f"),orig,RA_angle));
	}
	else if (evt.GetId() == EEGG_MANUALLOCK) {
		if (!ScopeConnected || !GuideCameraConnected || !Calibrated)
			return;
		if (canvas->State > STATE_SELECTED) return;  // must not be calibrating or guiding already
		
		if (evt.IsChecked()) {
			wxString tmpstr;
			tmpstr = wxGetTextFromUser(_T("Enter x-lock position (or 0 for center)"), _T("X-lock position"));
			if (tmpstr.IsEmpty()) return;
			ManualLock = true;
			tmpstr.ToDouble(&LockX); 
			LockX = fabs(LockX);
			if (LockX < 0.0001) {
				LockX = CurrentGuideCamera->FullSize.GetWidth() / 2;
				LockY = CurrentGuideCamera->FullSize.GetHeight() / 2;
			}
			else {
				tmpstr = wxGetTextFromUser(_T("Enter y-lock position"), _T("Y-lock position"));
				if (tmpstr.IsEmpty()) return;
				tmpstr.ToDouble(&LockY); 
				LockY = fabs(LockY);
			}
		}
		else {
			ManualLock = false;
		}
	}	
	else evt.Skip();

}
