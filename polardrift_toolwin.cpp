/*
 *  polardrift_toolwin.cpp
 *  PHD Guiding
 *
 *  Created by Ken Self
 *  Copyright (c) 2017 Ken Self
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
 *    Neither the name of openphdguiding.org nor the names of its
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
#include "polardrift_tool.h"
#include "polardrift_toolwin.h"

#include <wx/gbsizer.h>
#include <wx/valnum.h>
#include <wx/textwrapper.h>

//==================================
BEGIN_EVENT_TABLE(PolarDriftToolWin, wxFrame)
EVT_CHOICE(ID_HEMI, PolarDriftToolWin::OnHemi)
EVT_BUTTON(ID_START, PolarDriftToolWin::OnStart)
EVT_BUTTON(ID_CLOSE, PolarDriftToolWin::OnCloseBtn)
EVT_CLOSE(PolarDriftToolWin::OnClose)
END_EVENT_TABLE()

wxWindow *PolarDriftTool::CreatePolarDriftToolWindow()
{
    if (!pCamera || !pCamera->Connected)
    {
        wxMessageBox(_("Please connect a camera first."));
        return 0;
    }

    // confirm that image scale is specified

    if (pFrame->GetCameraPixelScale() == 1.0)
    {
        bool confirmed = ConfirmDialog::Confirm(_(
            "The Polar Drift Align tool is most effective when PHD2 knows your guide\n"
            "scope focal length and camera pixel size.\n"
            "\n"
            "Enter your guide scope focal length on the Global tab in the Brain.\n"
            "Enter your camera pixel size on the Camera tab in the Brain.\n"
            "\n"
            "Would you like to run the tool anyway?"),
            "/rotate_tool_without_pixscale");

        if (!confirmed)
        {
            return 0;
        }
    }
    if (pFrame->pGuider->IsCalibratingOrGuiding())
    {
        wxMessageBox(_("Please wait till Calibration is done and stop guiding"));
        return 0;
    }

    return new PolarDriftToolWin();
}
bool PolarDriftTool::IsDrifting()
{
    PolarDriftToolWin *win = static_cast<PolarDriftToolWin *>(pFrame->pPolarDriftTool);
    return win->IsDrifting();
}
bool PolarDriftTool::WatchDrift()
{
    PolarDriftToolWin *win = static_cast<PolarDriftToolWin *>(pFrame->pPolarDriftTool);
    return win->WatchDrift();
}
void PolarDriftTool::PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale)
{
    PolarDriftToolWin *win = static_cast<PolarDriftToolWin *>(pFrame->pPolarDriftTool);
    return win->PaintHelper(dc, scale);
}

PolarDriftToolWin::PolarDriftToolWin()
: wxFrame(pFrame, wxID_ANY, _("Polar Drift Alignment"), wxDefaultPosition, wxDefaultSize,
wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxTAB_TRAVERSAL | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR)
{
    t0 = 0;
    sumt = sumt2 = sumx = sumx2 = sumy = sumy2 = sumtx = sumty = sumxy = 0.0;
    num = 0;
    offset = alpha = 0.0;
    s_drifting = false;
    
//Fairly convoluted way to get the camera size in pixels
    usImage *pCurrImg = pFrame->pGuider->CurrentImage();
    wxImage *pDispImg = pFrame->pGuider->DisplayedImage();
    double scalefactor = pFrame->pGuider->ScaleFactor();
    double xpx = pDispImg->GetWidth() / scalefactor;
    double ypx = pDispImg->GetHeight() / scalefactor;
    g_pxScale = pFrame->GetCameraPixelScale();
// Fullsize is easier but the camera simulator does not set this.
//    wxSize camsize = pCamera->FullSize;
    g_camWidth = pCamera->FullSize.GetWidth() == 0 ? xpx: pCamera->FullSize.GetWidth();

    g_camAngle = 0.0;
    if (pMount && pMount->IsConnected() && pMount->IsCalibrated())
    {
        g_camAngle = degrees(pMount->xAngle());
    }

    a_hemi = pConfig->Profile.GetInt("/PolarDriftTool/Hemisphere", 1);
    if (pPointingSource)
    {
        double lat, lon;
        if (!pPointingSource->GetSiteLatLong(&lat, &lon))
        {
            a_hemi = lat >= 0 ? 1 : -1;
        }
    }
    if (!pFrame->CaptureActive)
    {
        // loop exposures
        wxCommandEvent dummy;
        SetStatusText(_("Start Looping..."));
        pFrame->OnLoopExposure(dummy);
    }
    SetBackgroundColour(wxColor(0xcccccc));
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    // a vertical sizer holding everything
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    // a horizontal box sizer for the bitmap and the instructions
    wxBoxSizer *instrSizer = new wxBoxSizer(wxHORIZONTAL);
    c_instr = _(
        "Slew to near the Celestial Pole.\n"
        "Select a guide star on the main display.\n"
        "Click Start\n"
        "Wait for the display to stabilise\n"
        "Click Stop\n"
        "Adjust your mount's altitude and azimuth to place"
        "the guide star in its target circle\n"
        );

    w_instructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(240, 240), wxALIGN_LEFT);
#ifdef __WXOSX__
    w_instructions->SetFont(*wxSMALL_FONT);
#endif
    w_instructions->Wrap(-1);
    instrSizer->Add(w_instructions, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    topSizer->Add(instrSizer);

    // static box sizer holding the scope pointing controls
    wxStaticBoxSizer *sbSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Alignment Parameters")), wxVERTICAL);

    // a grid box sizer for laying out scope pointing the controls
    wxGridBagSizer *gbSizer = new wxGridBagSizer(0, 0);
    gbSizer->SetFlexibleDirection(wxBOTH);
    gbSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    wxStaticText *txt;

// First row of grid
    int gridRow = 0;
    txt = new wxStaticText(this, wxID_ANY, _("Camera arcsec/pixel"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxALL, 5);

    txt = new wxStaticText(this, wxID_ANY, _("Camera Angle"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 1), wxGBSpan(1, 1), wxALL, 5);

    txt = new wxStaticText(this, wxID_ANY, _("Hemisphere"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxALL, 5);

    // Next row of grid
    gridRow++;
    w_camScale = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    gbSizer->Add(w_camScale, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    w_camRot = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    gbSizer->Add(w_camRot, wxGBPosition(gridRow, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    wxArrayString hemi;
    hemi.Add(_("North"));
    hemi.Add(_("South"));
    w_hemiChoice = new wxChoice(this, ID_HEMI, wxDefaultPosition, wxDefaultSize, hemi);
    w_hemiChoice->SetToolTip(_("Select your hemisphere"));
    gbSizer->Add(w_hemiChoice, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxALL, 5);

    // Next row of grid
    gridRow++;

    w_start = new wxButton(this, ID_START, _("Start"), wxDefaultPosition, wxDefaultSize, 0);
    gbSizer->Add(w_start, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // Next row of grid
    gridRow++;
    w_close = new wxButton(this, ID_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0);
    gbSizer->Add(w_close, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // add grid bag sizer to static sizer
    sbSizer->Add(gbSizer, 1, wxALIGN_CENTER, 5);

    // add static sizer to top-level sizer
    topSizer->Add(sbSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // add some padding below the static sizer
    topSizer->Add(0, 3, 0, wxEXPAND, 3);

    w_notesLabel = new wxStaticText(this, wxID_ANY, _("Adjustment notes"), wxDefaultPosition, wxDefaultSize, 0);
    w_notesLabel->Wrap(-1);
    topSizer->Add(w_notesLabel, 0, wxEXPAND | wxTOP | wxLEFT, 8);

    w_notes = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 54), wxTE_MULTILINE);
    pFrame->RegisterTextCtrl(w_notes);
    topSizer->Add(w_notes, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    SetSizer(topSizer);
    w_statusBar = CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);

    Layout();
    topSizer->Fit(this);

    int xpos = pConfig->Global.GetInt("/PolarDriftTool/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/PolarDriftTool/pos.y", -1);
    MyFrame::PlaceWindowOnScreen(this, xpos, ypos);

    FillPanel();
}

PolarDriftToolWin::~PolarDriftToolWin()
{
    pFrame->pPolarDriftTool = NULL;
}

void PolarDriftToolWin::OnHemi(wxCommandEvent& evt)
{
    int i_hemi = w_hemiChoice->GetSelection() <= 0 ? 1 : -1;
    pConfig->Profile.SetInt("/PolarDriftTool/Hemisphere", i_hemi);
    if (i_hemi != a_hemi)
    {
        a_hemi = i_hemi;
    }
    FillPanel();
}

void PolarDriftToolWin::OnStart(wxCommandEvent& evt)
{
    if (s_drifting){ // STOP drifting
        s_drifting = false;
        SetStatusText(_("Polar Drift alignment stopped"));
        Debug.AddLine(wxString::Format("Polar Drift alignment stopped"));
        SetStatusText(wxString::Format("PA err(arcmin): %.1f Angle (deg): %.1f", offset*g_pxScale / 60, norm(-alpha, -180, 180)));
        FillPanel();
        return;
    }
    if (pFrame->pGuider->IsCalibratingOrGuiding())
    {
        SetStatusText(_("Please wait till Calibration is done and/or stop guiding"));
        return;
    }
    if (!pFrame->pGuider->IsLocked())
    {
        SetStatusText(_("Please select a star"));
        return;
    }
    if (pMount)
    {
        m_savePrimaryMountEnabled = pMount->GetGuidingEnabled();
        pMount->SetGuidingEnabled(false);
    }
    if (pSecondaryMount)
    {
        m_saveSecondaryMountEnabled = pSecondaryMount->GetGuidingEnabled();
        pSecondaryMount->SetGuidingEnabled(false);
    }

    m_guideOutputDisabled = true;
    num = 0;
    s_drifting = true;
    FillPanel();
    return;
}

void PolarDriftToolWin::OnCloseBtn(wxCommandEvent& evt)
{
    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/PolarDriftTool/pos.x", x);
    pConfig->Global.SetInt("/PolarDriftTool/pos.y", y);
    Debug.AddLine("Close PolarDriftTool");
    Destroy();
}

void PolarDriftToolWin::OnClose(wxCloseEvent& evt)
{
    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/PolarDriftTool/pos.x", x);
    pConfig->Global.SetInt("/PolarDriftTool/pos.y", y);
    Debug.AddLine("Close PolarDriftTool");
    Destroy();
}

void PolarDriftToolWin::FillPanel()
{
    w_instructions->SetLabel(c_instr);

    w_start->SetLabel(_("Start"));
    if (s_drifting) {
        w_start->SetLabel(_("Stop"));
    }
    w_hemiChoice->Enable(false);
    w_hemiChoice->SetSelection(a_hemi > 0 ? 0 : 1);
    w_camScale->SetValue(wxString::Format("%+.3f", g_pxScale));
    w_camRot->SetValue(wxString::Format("%+.3f", g_camAngle));
    Layout();
}

void PolarDriftToolWin::PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale)
{
    if (num < 2)
    {
        return;
    }
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    // Draw adjustment lines for placing the guide star in its correct position relative to the CoR
    // Blue (azimuth) and Red (altitude)
    dc.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
    dc.DrawLine(current.X*scale, current.Y*scale, target.X*scale, target.Y*scale);
    dc.DrawCircle(target.X*scale, target.Y*scale, 10*scale);
}

bool PolarDriftToolWin::WatchDrift()
{
    // Initially, assume an offset of 5.0 degrees of the camera from the CoR
    // Calculate how far to move in RA to get a detectable arc
    // Calculate the tangential ditance of that movement
    // Mark the starting position then rotate the mount
    double tnow = ::wxGetUTCTimeMillis().GetValue()/1000.0;
    current = pFrame->pGuider->CurrentPosition();
    num++;
    if (num <= 1)
    {
        sumt = sumt2 = sumx = sumx2 = sumy = sumy2 = sumtx = sumty = sumxy = 0.0;
        offset = alpha = 0.0;
        t0 = tnow;
    }
    tnow -= t0;
    sumt += tnow;
    sumt2 += tnow*tnow;
    sumx += current.X;
    sumx2 += current.X * current.X;
    sumy += current.Y;
    sumy2 += current.Y*current.Y;
    sumtx += tnow * current.X;
    sumty += tnow * current.Y;
    sumxy += current.X * current.Y;

    if (num <= 1) return true;
    const double factor = 24 * 3600 / 2 / M_PI;
    double xslope = (num*sumtx - sumt*sumx) / (num*sumt2 - sumt*sumt);
    double yslope = (num*sumty - sumt*sumy) / (num*sumt2 - sumt*sumt);

    double theta = degrees(atan2(yslope, xslope));
    // In the northern hemisphere the star rotates clockwise, in the southern hemisphere anti-clockwise
    // In NH the pole is to the right of the drift vector (-90 degrees) however in pixel terms (Y +ve down) it is to the left (+90 degrees)
    // So we multiply by a_hemi to get the correct direction

    alpha = theta + a_hemi * 90; // direction to the pole
    offset = sqrt(xslope*xslope + yslope*yslope)*factor;  //polar alignment error in pixels
    target = PHD_Point(current.X + offset*cos(radians(alpha)), current.Y + offset*(sin(radians(alpha))));

    Debug.AddLine(wxString::Format("Polar Drift: num %d t0 %.1f tnow %.1f Pos: %.1f,%.1f PA err(px): %.1f Angle (deg): %.1f", num, t0, tnow));
    Debug.AddLine(wxString::Format("Polar Drift: slopex %.1f slopey %.1f offset %.1f theta %1f alpha %.1f", xslope, yslope, offset, theta, alpha));
    SetStatusText(wxString::Format("Time %.1f PA err(arcmin): %.1f Angle (deg): %.1f", tnow, offset*g_pxScale/60, norm(-alpha,-180,180)));

    return true;
}
