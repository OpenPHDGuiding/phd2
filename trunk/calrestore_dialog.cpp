/*
 *  calrestore_dialog.cpp
 *  PHD Guiding
 *
 *  Created by Bruce Waddington
 *  Copyright (c) 2013 Bruce Waddington
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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
#include "calrestore_dialog.h"

CalrestoreDialog::CalrestoreDialog() :
    wxDialog(pFrame, wxID_ANY, _("Restore calibration data"), wxDefaultPosition, wxSize(800, 400), wxCAPTION | wxCLOSE_BOX)
{
    wxString sCamAngle;
    wxString sXAngle;
    wxString sYAngle;
    wxString sTimestamp;
    wxString sPierSide;
    double dXRate = 0.0;
    double dYRate = 0.0;
    double dDeclination = 0.0;
    bool bDecEstimated = false;
    wxString sPixelSize = wxString::Format("%.1f u", pConfig->Profile.GetDouble ("/camera/pixelsize", 1.0));
    double dImageScale = pFrame->GetCameraPixelScale();
    int iFocalLength = pConfig->Profile.GetInt ("/frame/focallength", 0);;

    // Get the other values we need from the config file entry
    sTimestamp = pConfig->Profile.GetString("/scope/calibration/timestamp", wxEmptyString);

    if (pMount)
    {
        wxString prefix = "/" + pMount->GetMountClassName() + "/calibration/";
        dXRate = pConfig->Profile.GetDouble(prefix + "xRate", 1.0) * 1000.0;       // pixels per millisecond
        dYRate = pConfig->Profile.GetDouble(prefix + "yRate", 1.0) * 1000.0;
        double xAngle = degrees(pConfig->Profile.GetDouble(prefix + "xAngle", 0.0));
        if (xAngle < 0.0) xAngle += 360.0;
        sCamAngle = wxString::Format("%0.1f deg", xAngle);
        dDeclination = pConfig->Profile.GetDouble(prefix + "declination", 0.0);
        if (dDeclination == 0.0 && fabs(dYRate) > 0.00001 && fabs(dXRate / dYRate) <= 1.0)
        {
            dDeclination = degrees(acos(dXRate / dYRate));        // cos(dec) = Dec_Rate/RA_Rate
            bDecEstimated = true;
        }

        int iSide = pConfig->Profile.GetInt(prefix + "pierSide", PIER_SIDE_UNKNOWN);
        sPierSide = iSide == PIER_SIDE_EAST ? _("East") :
            iSide == PIER_SIDE_WEST ? _("West") : _("Unknown");
    }

    // Create the vertical sizer we're going to need
    wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
    wxGrid *pGrid = new wxGrid(this, wxID_ANY);
    pGrid->CreateGrid(4, 4);
    pGrid->SetRowLabelSize(1);
    pGrid->SetColLabelSize(1);
    pGrid->EnableEditing(false);

    int col = 0;
    int row = 0;
    pGrid->SetCellValue( _("Timestamp:"), row, col++);
    pGrid->SetCellValue(sTimestamp, row, col++);
    pGrid->SetCellValue(_("Camera angle:"), row, col++);
    pGrid->SetCellValue(sCamAngle, row, col++);
    row++;
    col = 0;
    pGrid->SetCellValue(_("RA rate:"), row, col++);
    pGrid->SetCellValue( wxString::Format("%0.3f ''/sec\n%0.3f px/sec",dXRate * dImageScale,dXRate), row, col++);
    pGrid->SetCellValue(_("Dec rate:"), row, col++);
    pGrid->SetCellValue(wxString::Format("%0.3f ''/sec\n%0.3f px/sec",dYRate * dImageScale,dYRate), row, col++);
    row++;
    col = 0;
    pGrid->SetCellValue(_("Guider focal length:"), row, col++);
    pGrid->SetCellValue(wxString::Format("%d mm", iFocalLength), row, col++);
    pGrid->SetCellValue(_("Guider pixel size:"), row, col++);
    pGrid->SetCellValue(sPixelSize, row, col++);
    row++;
    col = 0;
    pGrid->SetCellValue(_("Side of pier:"), row, col++);
    pGrid->SetCellValue(sPierSide, row, col++);
    pGrid->SetCellValue(_("Declination ") + (bDecEstimated ? _("(estimated):") : _("(from mount)")), row, col++);
    pGrid->SetCellValue(wxString::Format("%0.0f deg", dDeclination), row, col++);
    pGrid->AutoSize();
    pGrid->ClearSelection();

    pVSizer->Add(pGrid, wxSizerFlags(0).Border(wxALL, 20));

    // Now deal with the buttons
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *pRestore = new wxButton(this, wxID_OK, _("Restore"));
    pButtonSizer->Add(
        pRestore,
        wxSizerFlags(0).Align(0).Border(wxRIGHT | wxLEFT | wxBOTTOM, 10));
    pButtonSizer->Add(
        CreateButtonSizer(wxCANCEL),
        wxSizerFlags(0).Align(0).Border(wxRIGHT | wxLEFT | wxBOTTOM, 10));

    //position the buttons centered with no border
    pVSizer->Add(pButtonSizer, wxSizerFlags(0).Center());

    SetSizerAndFit(pVSizer);
}

CalrestoreDialog::~CalrestoreDialog(void)
{
}
