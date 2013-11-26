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
#include <wx/fs_mem.h>
#include <wx/html/htmlwin.h>

// Utility function to create a row in the table - each row has four elements: <label> <value> <label> <value>
static wxString TableRow (wxString sLabel1, wxString sVal1, wxString sLabel2, wxString sVal2)
{
    wxString sRslt = "<tr>"
          "<td style=\"width: 136px;\">" + sLabel1 + "</td>"
          "<td style=\"width: 173px;\">" + sVal1 + "</td>"
          "<td style=\"width: 168px;\">" + sLabel2 + "</td>"
          "<td style=\"width: 134px;\">" + sVal2 + "</td>"
        "</tr>";
    return (sRslt);
}

CalrestoreDialog::CalrestoreDialog() :
    wxDialog(pFrame, wxID_ANY, _("Restore calibration data"), wxDefaultPosition, wxSize(800, 400), wxCAPTION | wxCLOSE_BOX)
{
    wxString sCamAngle;
    wxString sXAngle;
    wxString sYAngle;
    wxString sTimestamp;
    wxString sPierSide;
    double dXRate;
    double dYRate;
    double dDeclination;
    bool bDecEstimated = false;
    wxString sPixelSize = wxString::Format("%.1f u", pConfig->Profile.GetDouble ("/camera/pixelsize", 1.0));
    double dImageScale = pFrame->GetCameraPixelScale();
    int iFocalLength = pConfig->Profile.GetInt ("/frame/focallength", 0);;

    // Get the other values we need from the config file entry
    sTimestamp = pConfig->Profile.GetString("/scope/calibration/timestamp", wxEmptyString);
    wxString prefix = "/" + pMount->GetMountClassName() + "/calibration/";
    dXRate = pConfig->Profile.GetDouble(prefix + "xRate", 1.0) *  1000.0;       // pixels per millisecond
    dYRate = pConfig->Profile.GetDouble(prefix + "yRate", 1.0) * 1000.0;
    double xAngle = pConfig->Profile.GetDouble(prefix + "xAngle", 0.0) * 180.0/M_PI;
    if (xAngle < 0.0) xAngle += 360.0;
    sCamAngle = wxString::Format("%0.1f deg", xAngle);
    dDeclination = pConfig->Profile.GetDouble(prefix + "declination", 0.0);
    if (dDeclination == 0)
    {
        dDeclination = floor(acos(dXRate/dYRate) * 180.0/M_PI);        // cos(dec) = Dec_Rate/RA_Rate
        bDecEstimated = true;
    }

    int iSide = pConfig->Profile.GetInt(prefix + "pierSide", PIER_SIDE_UNKNOWN);
    sPierSide = iSide == PIER_SIDE_EAST ? _("East") :
        iSide == PIER_SIDE_WEST ? _("West") : _("Unknown");

    // Create the vertical sizer we're going to need
    wxBoxSizer *pVSizer = new wxBoxSizer(wxVERTICAL);
    // Create an in-memory file that can be displayed as a table of values
    // Use TableEntry to populate each row - avoid inline strings in order to support localization
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    wxString sHTML_Leadin = "<html>" "<body style=\"width: 658px;\">" "<table style=\"text-align: left; width: 645px;\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\">" "<tbody>";
    wxString sHTML_Wrapup =  "</tbody>" "</table>" "<br>" "</body>" "</html>";

    wxString sHTML_Final = sHTML_Leadin;
    sHTML_Final += TableRow (_("Timestamp:"), sTimestamp, _("Camera angle:"), sCamAngle) +
                   TableRow (_("RA rate:"), wxString::Format("%0.3f arc-sec/sec",dXRate * dImageScale), _("Dec rate:"), wxString::Format("%0.3f arc-sec/sec",dYRate * dImageScale)) +
                   TableRow ("", wxString::Format("%0.3f px/sec",dXRate), "", wxString::Format("%0.3f px/sec",dYRate)) +
                   TableRow (_("Guider focal length:"), wxString::Format("%d mm", iFocalLength), _("Guider pixel size:"), sPixelSize) +
                   TableRow (_("Side of pier:"), sPierSide, _("Declination ") + (bDecEstimated ? _("(estimated):") : _("(from mount)")), wxString::Format("%0.0f deg", dDeclination));
    sHTML_Final += sHTML_Wrapup;
    wxMemoryFSHandler::AddFile ("cal_data.html", sHTML_Final);
    // Pull the in-memory file into an HTML viewer control
    wxHtmlWindow *pHtml;
    pHtml = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(658, 130), wxHW_SCROLLBAR_AUTO);
    pHtml->SetBorders(0);
    pHtml->LoadPage("memory:cal_data.html");
    pHtml->SetSize(pHtml->GetInternalRepresentation()->GetWidth(), pHtml->GetInternalRepresentation()->GetHeight());

    pVSizer->Add(pHtml, wxSizerFlags(0).Border(wxALL, 20));

    // Now deal with the buttons
    wxBoxSizer *pButtonSizer = new wxBoxSizer( wxHORIZONTAL );

    wxButton *pRestore = new wxButton( this, wxID_OK, _("Restore") );
    pButtonSizer->Add(
        pRestore,
        wxSizerFlags(0).Align(0).Border(wxALL, 20));
    pButtonSizer->Add(
        CreateButtonSizer(wxCANCEL),
        wxSizerFlags(0).Align(0).Border(wxALL, 20));

     //position the buttons centered with no border
     pVSizer->Add(
        pButtonSizer,
        wxSizerFlags(0).Center() );
    SetSizerAndFit (pVSizer);
}

CalrestoreDialog::~CalrestoreDialog(void)
{
}
