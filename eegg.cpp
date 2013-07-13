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

void TestGuide() {

#ifdef BRET_TODO
    wxMessageBox(_("W RA+")); wxGetApp().Yield(); pMount->Guide(WEST,2000); wxGetApp().Yield();
    wxMessageBox(_("N Dec+"));  wxGetApp().Yield(); pMount->Guide(NORTH,2000);wxGetApp().Yield();
    wxMessageBox(_("E RA-"));  wxGetApp().Yield(); pMount->Guide(EAST,2000);wxGetApp().Yield();
    wxMessageBox(_("S Dec-"));  wxGetApp().Yield(); pMount->Guide(SOUTH,2000);wxGetApp().Yield();
    wxMessageBox(_("Done"));
#endif
}

static void load_calibration(Mount *mnt)
{
    if (!mnt)
        return;
    wxString prefix = "/" + mnt->GetMountClassName() + "/calibration/";
    if (!pConfig->HasEntry(prefix + "timestamp"))
        return;
    double xRate = pConfig->GetDouble(prefix + "xRate", 1.0);
    double yRate = pConfig->GetDouble(prefix + "yRate", 1.0);
    double xAngle = pConfig->GetDouble(prefix + "xAngle", 0.0);
    double yAngle = pConfig->GetDouble(prefix + "yAngle", M_PI/2.0);
    double declination = pConfig->GetDouble(prefix + "declination", 0.0);
    mnt->SetCalibration(xAngle, yAngle, xRate, yRate, declination);
}

void MyFrame::OnEEGG(wxCommandEvent &evt)
{
    if (evt.GetId() == EEGG_TESTGUIDEDIR)
    {
        if (!pMount->IsConnected())
        {
            wxMessageBox(_("Please connect a Mount to Manual Guide."));
            return;
        }
        TestGuide();
    }
    else if (evt.GetId() == EEGG_RANDOMMOTION)
    {
        RandomMotionMode = !RandomMotionMode;
        wxMessageBox(wxString::Format(_T("Random motion mode set to %d"),(int) RandomMotionMode));
    }
    else if (evt.GetId() == EEGG_RESTORECAL)
    {
        wxString savedCal = pConfig->GetString("/scope/calibration/timestamp", wxEmptyString);
        if (savedCal.IsEmpty())
        {
            wxMessageBox(_("There is no calibration data available."));
            return;
        }

        int answer = wxMessageBox("Load calibration data from " + savedCal + "?", "Load Calibration Data", wxYES_NO);
        if (answer == wxYES)
        {
            load_calibration(pMount);
            load_calibration(pSecondaryMount);
        }
    }
    else if (evt.GetId() == EEGG_MANUALCAL)
    {
        double xRate  = pMount->xRate();
        double yRate  = pMount->yRate();
        double xAngle = pMount->xAngle();
        double yAngle = pMount->yAngle();
        double declination = pMount->GetDeclination();

        if (!pMount->IsCalibrated())
        {
            xRate       = 1.0;
            yRate       = 1.0;
            xAngle      = 0.0;
            yAngle      = M_PI/2;
            declination = 0.0;
        }

        wxString tmpstr;
        tmpstr = wxGetTextFromUser(_("Enter parameter (e.g. 0.005)"), _("RA rate"), wxString::Format(_T("%.4f"),xRate));
        if (tmpstr.IsEmpty()) return;
        tmpstr.ToDouble(&xRate); // = 0.0035;

        tmpstr = wxGetTextFromUser(_("Enter parameter (e.g. 0.005)"), _("Dec rate"), wxString::Format(_T("%.4f"),yRate));
        if (tmpstr.IsEmpty()) return;
        tmpstr.ToDouble(&yRate); // = 0.0035;

        tmpstr = wxGetTextFromUser(_("Enter parameter (e.g. 0.5)"), _("RA angle"), wxString::Format(_T("%.3f"),xAngle));
        if (tmpstr.IsEmpty()) return;
        tmpstr.ToDouble(&xAngle); // = 0.0035;

        tmpstr = wxGetTextFromUser(_("Enter parameter (e.g. 2.1)"), _("Dec angle"), wxString::Format(_T("%.3f"),yAngle));
        if (tmpstr.IsEmpty()) return;
        tmpstr.ToDouble(&yAngle); // = 0.0035;

        tmpstr = wxGetTextFromUser(_("Enter parameter (e.g. 2.1)"), _("Declination"), wxString::Format(_T("%.3f"),declination));
        if (tmpstr.IsEmpty()) return;
        tmpstr.ToDouble(&declination); // = 0.0035;

        pMount->SetCalibration(xAngle, yAngle, xRate, yRate, declination);
    }
    else if (evt.GetId() == EEGG_CLEARCAL)
    {
        pMount->ClearCalibration(); // clear calibration
    }
    else if (evt.GetId() == EEGG_FLIPRACAL)
    {
        Mount *mount = pSecondaryMount ? pSecondaryMount : pMount;
        double orig = mount->xAngle();

        if (FlipRACal())
        {
            wxMessageBox(_("Failed to flip RA calibration"));
        }
        else
        {
            double xAngle = mount->xAngle();
            wxMessageBox(wxString::Format(_("RA calibration angle flipped: %.2f to %.2f"), orig, xAngle));
        }
    }
    else if (evt.GetId() == EEGG_MANUALLOCK)
    {
        if (!pMount->IsConnected() || !pCamera && !pCamera->Connected || !pMount->IsCalibrated())
        {
            wxMessageBox(_("Entering manual calibration requires a camera and mount to be connected and calibrated."));
            return;
        }
        if (pGuider->GetState() > STATE_SELECTED)
        {
            wxMessageBox(_("Entering manual calibration cannot be done while calibrating or guiding."));
            return;  // must not be calibrating or guiding already
        }

        double LockX=0.0, LockY=0.0;
        PHD_Point curLock = pFrame->pGuider->LockPosition();
        wxString tmpstr;
        if (curLock.IsValid())
            tmpstr = wxString::Format("%.3f", curLock.X);
        do
        {
            tmpstr = wxGetTextFromUser(_("Enter x-lock position, or 0 for center"), _("X-lock position"), tmpstr);
        } while (!tmpstr.IsEmpty() && !tmpstr.ToDouble(&LockX));
        if (tmpstr.IsEmpty())
            return;
        LockX = fabs(LockX);
        if (LockX < 0.0001) {
            LockX = pCamera->FullSize.GetWidth() / 2;
            LockY = pCamera->FullSize.GetHeight() / 2;
        }
        else {
            tmpstr = "";
            if (curLock.IsValid())
                tmpstr = wxString::Format("%.3f", curLock.Y);
            do {
                tmpstr = wxGetTextFromUser(_("Enter y-lock position"), _("Y-lock position"), tmpstr);
            } while (!tmpstr.IsEmpty() && !tmpstr.ToDouble(&LockY));
            if (tmpstr.IsEmpty())
                return;
            LockY = fabs(LockY);
        }
        pFrame->pGuider->SetLockPosition(PHD_Point(LockX, LockY));
        pFrame->pGuider->SetLockPosIsSticky(true);
        pFrame->tools_menu->FindItem(EEGG_STICKY_LOCK)->Check(true);
    }
    else if (evt.GetId() == EEGG_STICKY_LOCK)
    {
        bool sticky = evt.IsChecked();
        pFrame->pGuider->SetLockPosIsSticky(sticky);
    }
    else
    {
        evt.Skip();
    }
}



void MyFrame::OnDriftTool(wxCommandEvent& WXUNUSED(ect)) {

}
