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
#include "drift_tool.h"
#include "manualcal_dialog.h"
#include "calreview_dialog.h"
#include "nudge_lock.h"
#include "comet_tool.h"

void MyFrame::OnEEGG(wxCommandEvent& evt)
{
    if (evt.GetId() == EEGG_RESTORECAL || evt.GetId() == EEGG_REVIEWCAL)
    {
        wxString savedCal = pConfig->Profile.GetString("/scope/calibration/timestamp", wxEmptyString);
        bool restoring = evt.GetId() == EEGG_RESTORECAL;
        if (savedCal.IsEmpty())
        {
            wxMessageBox(_("There is no calibration data available."));
            return;
        }

        if (pMount && pMount->IsConnected())
        {
            if (restoring)                                                  // Restore dialog is modal
            {
                CalRestoreDialog dlg(this);
                dlg.ShowModal();
            }
            else
            {
                if (pCalReviewDlg)                                          // Review dialog is non-modal
                    pCalReviewDlg->Destroy();
                pCalReviewDlg = new CalReviewDialog(this);
                pCalReviewDlg->Show();

            }
        }
        else
        {
            wxMessageBox(_("Please connect a mount first."));
        }
    }
    else if (evt.GetId() == EEGG_MANUALCAL)
    {
        if (pMount)
        {
            Calibration cal;
            cal.xRate  = pMount->xRate();
            cal.yRate  = pMount->yRate();
            cal.xAngle = pMount->xAngle();
            cal.yAngle = pMount->yAngle();
            cal.declination = pPointingSource->GetGuidingDeclination();
            cal.pierSide = pPointingSource->SideOfPier();
            cal.rotatorAngle = Rotator::RotatorPosition();

            if (!pMount->IsCalibrated())
            {
                cal.xRate       = 1.0;
                cal.yRate       = 1.0;
                cal.xAngle      = 0.0;
                cal.yAngle      = M_PI / 2.;
                cal.declination = 0.0;
            }

            ManualCalDialog manualcal(cal);
            if (manualcal.ShowModal () == wxID_OK)
            {
                manualcal.GetValues(&cal);
                pMount->SetCalibration(cal);
            }
        }
    }
    else if (evt.GetId() == EEGG_CLEARCAL)
    {
        wxString devicestr = "";
        if (!(pGuider && pGuider->IsCalibratingOrGuiding()))
            if (pMount)
            {
                if (pMount->IsStepGuider())
                    devicestr = _("AO");
                else
                    devicestr = _("Mount");
            }
            if (pSecondaryMount)
            {
                devicestr += _(", Mount");
            }
            if (devicestr.Length() > 0)
            {
                if (wxMessageBox(wxString::Format(_("%s calibration will be cleared - calibration will be re-done when guiding is started."), devicestr),
                    _("Clear Calibration"), wxOK | wxCANCEL) == wxOK)
                {
                    if (pMount)
                        pMount->ClearCalibration();
                    if (pSecondaryMount)
                        pSecondaryMount->ClearCalibration();
                    Debug.AddLine("User cleared calibration on " + devicestr);
                }
            }
    }
    else if (evt.GetId() == EEGG_FLIPRACAL)
    {
        Mount *mount = pSecondaryMount ? pSecondaryMount : pMount;

        if (mount)
        {
            double xorig = degrees(mount->xAngle());
            double yorig = degrees(mount->yAngle());

            Debug.AddLine("User-requested FlipRACal");

            if (FlipRACal())
            {
                wxMessageBox(_("Failed to flip RA calibration"));
            }
            else
            {
                double xnew = degrees(mount->xAngle());
                double ynew = degrees(mount->yAngle());
                wxMessageBox(wxString::Format(_("RA calibration angle flipped: (%.2f, %.2f) to (%.2f, %.2f)"),
                    xorig, yorig, xnew, ynew));
            }
        }
    }
    else if (evt.GetId() == EEGG_MANUALLOCK)
    {
        if (!pFrame->pNudgeLock)
            pFrame->pNudgeLock = NudgeLockTool::CreateNudgeLockToolWindow();

        pFrame->pNudgeLock->Show();
    }
    else if (evt.GetId() == EEGG_STICKY_LOCK)
    {
        bool sticky = evt.IsChecked();
        pFrame->pGuider->SetLockPosIsSticky(sticky);
        pConfig->Global.SetBoolean("/StickyLockPosition", sticky);
        NudgeLockTool::UpdateNudgeLockControls();
    }
    else
    {
        evt.Skip();
    }
}

void MyFrame::OnDriftTool(wxCommandEvent& WXUNUSED(evt))
{
    if (!pDriftTool)
    {
        pDriftTool = DriftTool::CreateDriftToolWindow();
    }

    if (pDriftTool)
    {
        pDriftTool->Show();
    }
}

void MyFrame::OnCometTool(wxCommandEvent& WXUNUSED(evt))
{
    if (!pCometTool)
    {
        pCometTool = CometTool::CreateCometToolWindow();
    }

    if (pCometTool)
    {
        pCometTool->Show();
    }
}
