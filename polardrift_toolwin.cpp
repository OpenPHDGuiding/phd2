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
EVT_CHECKBOX(ID_MIRROR, PolarDriftToolWin::OnMirror)
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
void PolarDriftTool::PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale)
{
    PolarDriftToolWin *win = static_cast<PolarDriftToolWin *>(pFrame->pPolarDriftTool);
    if (win)
    {
        win->PaintHelper(dc, scale);
    }
}
bool PolarDriftTool::UpdateState()
{
    PolarDriftToolWin *win = static_cast<PolarDriftToolWin *>(pFrame->pPolarDriftTool);
    if (win && win->IsDrifting())
    {
        // Rotate the mount in RA a bit
        if (!win->WatchDrift())
        {
            return false;
        }
    }
    return true;
}

PolarDriftToolWin::PolarDriftToolWin()
: wxFrame(pFrame, wxID_ANY, _("Polar Drift Alignment"), wxDefaultPosition, wxDefaultSize,
wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxTAB_TRAVERSAL | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR | wxRESIZE_BORDER )
{
    m_t0 = 0;
    m_sumt = m_sumt2 = m_sumx = m_sumx2 = m_sumy = m_sumy2 = m_sumtx = m_sumty = m_sumxy = 0.0;
    m_num = 0;
    m_offset = m_alpha = 0.0;
    m_drifting = false;

    m_pxScale = pFrame->GetCameraPixelScale();
// Fullsize is easier but the camera simulator does not set this.
//    wxSize camsize = pCamera->FullSize;
//    g_camWidth = pCamera->FullSize.GetWidth() == 0 ? xpx: pCamera->FullSize.GetWidth();

//    g_camAngle = 0.0;
//    if (pMount && pMount->IsConnected() && pMount->IsCalibrated())
//    {
//        g_camAngle = degrees(pMount->xAngle());
//    }

    m_hemi = pConfig->Profile.GetInt("/PolarDriftTool/Hemisphere", 1);
    if (pPointingSource)
    {
        double lat, lon;
        if (!pPointingSource->GetSiteLatLong(&lat, &lon))
        {
            m_hemi = lat >= 0 ? 1 : -1;
        }
    }
    m_mirror = pConfig->Profile.GetInt("/PolarDriftTool/Mirror", 1);
    if (!pFrame->CaptureActive)
    {
        // loop exposures
        SetStatusText(_("Start Looping..."));
        pFrame->StartLoopingInteractive(_T("PolarDrift:start"));
    }
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    // a vertical sizer holding everything
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    // a horizontal box sizer for the bitmap and the instructions
    wxBoxSizer *instrSizer = new wxBoxSizer(wxHORIZONTAL);
    c_instr = _(
        "Slew to near the Celestial Pole.\n"
        "Make sure tracking is ON.\n"
        "Select a guide star on the main display.\n"
        "Click Start.\n"
        "Wait for the display to stabilise.\n"
        "Click Stop.\n"
        "Adjust your mount's altitude and azimuth to place "
        "the guide star in its target circle.\n"
        );

    m_instructionsText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(280, 240), wxALIGN_LEFT);
#ifdef __WXOSX__
    m_instructionsText->SetFont(*wxSMALL_FONT);
#endif
    m_instructionsText->Wrap(-1);
    instrSizer->Add(m_instructionsText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

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
    txt = new wxStaticText(this, wxID_ANY, _("Hemisphere"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxALL, 5);

    wxArrayString hemi;
    hemi.Add(_("North"));
    hemi.Add(_("South"));
    m_hemiChoice = new wxChoice(this, ID_HEMI, wxDefaultPosition, wxDefaultSize, hemi);
    m_hemiChoice->SetToolTip(_("Select your hemisphere"));
    gbSizer->Add(m_hemiChoice, wxGBPosition(gridRow, 1), wxGBSpan(1, 1), wxALL, 5);

    // Next row of grid
    gridRow++;
    m_mirrorCheck = new wxCheckBox(this, ID_MIRROR, _("Mirror image"));
    gbSizer->Add(m_mirrorCheck, wxGBPosition(gridRow, 0), wxGBSpan(1, 1), wxALL | wxALIGN_BOTTOM, 5);
    m_mirrorCheck->SetValue(m_mirror==-1?true:false);
    m_mirrorCheck->SetToolTip(_("The image is mirrored e.g. from OAG"));

    m_startButton = new wxButton(this, ID_START, _("Start"), wxDefaultPosition, wxDefaultSize, 0);
    gbSizer->Add(m_startButton, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // Next row of grid
    gridRow++;
    m_closeButton = new wxButton(this, ID_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0);
    gbSizer->Add(m_closeButton, wxGBPosition(gridRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    // add grid bag sizer to static sizer
    sbSizer->Add(gbSizer, 1, wxALIGN_CENTER, 5);

    // add static sizer to top-level sizer
    topSizer->Add(sbSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

    // add some padding below the static sizer
    topSizer->Add(0, 3, 0, wxEXPAND, 3);

    m_notesLabel = new wxStaticText(this, wxID_ANY, _("Adjustment notes"), wxDefaultPosition, wxDefaultSize, 0);
    m_notesLabel->Wrap(-1);
    topSizer->Add(m_notesLabel, 0, wxEXPAND | wxTOP | wxLEFT, 8);

    w_notesText = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 54), wxTE_MULTILINE);
    pFrame->RegisterTextCtrl(w_notesText);
    topSizer->Add(w_notesText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    SetSizer(topSizer);
    m_statusBar = CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);
    const int sbw[3] = { -2, -1, -1 };
    m_statusBar->SetFieldsCount(3, sbw);

    Layout();
    topSizer->Fit(this);

    int x = pConfig->Global.GetInt("/PolarDriftTool/pos.x", -1);
    int y = pConfig->Global.GetInt("/PolarDriftTool/pos.y", -1);
    MyFrame::PlaceWindowOnScreen(this, x, y);

    x = pConfig->Global.GetInt("/PolarDriftTool/size.x", -1);
    y = pConfig->Global.GetInt("/PolarDriftTool/size.y", -1);
    SetSize(x, y);

    FillPanel();
}

PolarDriftToolWin::~PolarDriftToolWin()
{
    pFrame->pPolarDriftTool = NULL;
}

void PolarDriftToolWin::OnHemi(wxCommandEvent& evt)
{
    int i_hemi = m_hemiChoice->GetSelection() <= 0 ? 1 : -1;
    pConfig->Profile.SetInt("/PolarDriftTool/Hemisphere", i_hemi);
    if (i_hemi != m_hemi)
    {
        m_hemi = i_hemi;
    }
    FillPanel();
}

void PolarDriftToolWin::OnMirror(wxCommandEvent& evt)
{
    int i_mirror = m_mirrorCheck->IsChecked()? -1: 1;
    pConfig->Profile.SetInt("/PolarDriftTool/Mirror", i_mirror);
    if (i_mirror != m_mirror)
    {
        m_mirror =  i_mirror;
    }
    FillPanel();
}

void PolarDriftToolWin::OnStart(wxCommandEvent& evt)
{
    if (m_drifting){ // STOP drifting
        m_drifting = false;
        SetStatusText(_("Polar Drift alignment stopped"));
        Debug.AddLine(wxString::Format("Polar Drift alignment stopped"));
        SetStatusText(wxString::Format(_("PA err(arcmin): %.1f Angle (deg): %.1f"), m_offset*m_pxScale / 60, norm(-m_alpha, -180, 180)));
        FillPanel();
        if (pMount)
        {
            pMount->SetGuidingEnabled(m_savePrimaryMountEnabled);
        }
        if (pSecondaryMount)
        {
            pSecondaryMount->SetGuidingEnabled(m_saveSecondaryMountEnabled);
        }

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
    m_num = 0;
    m_drifting = true;
    FillPanel();
    return;
}

void PolarDriftToolWin::OnCloseBtn(wxCommandEvent& evt)
{
    wxCloseEvent dummy;
    OnClose(dummy);
}

void PolarDriftToolWin::OnClose(wxCloseEvent& evt)
{
    if (m_drifting) // STOP drifting before closing
    {
        wxCommandEvent dummy;
        OnStart(dummy);
    }
    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/PolarDriftTool/pos.x", x);
    pConfig->Global.SetInt("/PolarDriftTool/pos.y", y);

    // save the window size
    GetSize(&x, &y);
    pConfig->Global.SetInt("/PolarDriftTool/size.x", x);
    pConfig->Global.SetInt("/PolarDriftTool/size.y", y);

    Debug.AddLine("Close PolarDriftTool");
    Destroy();
}

void PolarDriftToolWin::FillPanel()
{
    m_instructionsText->SetLabel(c_instr);

    m_startButton->SetLabel(_("Start"));
    if (m_drifting) {
        m_startButton->SetLabel(_("Stop"));
    }
    m_hemiChoice->Enable(true);
    if (pPointingSource)
    {
        double lat, lon;
        if (!pPointingSource->GetSiteLatLong(&lat, &lon))
        {
            m_hemi = lat >= 0 ? 1 : -1;
            m_hemiChoice->Enable(false);
        }
    }
    m_hemiChoice->SetSelection(m_hemi > 0 ? 0 : 1);
//    w_camScale->SetValue(wxString::Format("%+.3f", m_pxScale));
//    w_camRot->SetValue(wxString::Format("%+.3f", g_camAngle));
    Layout();
}

void PolarDriftToolWin::PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale)
{
    if (m_num < 2)
    {
        return;
    }
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    // Draw adjustment lines for placing the guide star in its correct position relative to the CoR
    // Blue (azimuth) and Red (altitude)
    dc.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
    dc.DrawLine(m_current.X*scale, m_current.Y*scale, m_target.X*scale, m_target.Y*scale);
    dc.DrawCircle(m_target.X*scale, m_target.Y*scale, 10*scale);
}

bool PolarDriftToolWin::WatchDrift()
{
    // Monitor the drift of the selected star
    // Calculate a least squares fit of the drift agains time along each sensor axis
    // Calculate the tangential ditance of that movement
    // Mark the starting position then rotate the mount
    double tnow = ::wxGetUTCTimeMillis().GetValue()/1000.0;
    m_current = pFrame->pGuider->CurrentPosition();
    m_num++;
    if (m_num <= 1)
    {
        m_sumt = m_sumt2 = m_sumx = m_sumx2 = m_sumy = m_sumy2 = m_sumtx = m_sumty = m_sumxy = 0.0;
        m_offset = m_alpha = 0.0;
        m_t0 = tnow;
    }
    tnow -= m_t0;
    m_sumt += tnow;
    m_sumt2 += tnow*tnow;
    m_sumx += m_current.X;
    m_sumx2 += m_current.X * m_current.X;
    m_sumy += m_current.Y;
    m_sumy2 += m_current.Y*m_current.Y;
    m_sumtx += tnow * m_current.X;
    m_sumty += tnow * m_current.Y;
    m_sumxy += m_current.X * m_current.Y;

    if (m_num <= 1) return true;
    const double factor = 24 * 3600 / 2 / M_PI; // approx 13751: seconds per radian
    double xslope = (m_num*m_sumtx - m_sumt*m_sumx) / (m_num*m_sumt2 - m_sumt*m_sumt);
    double yslope = (m_num*m_sumty - m_sumt*m_sumy) / (m_num*m_sumt2 - m_sumt*m_sumt);

    double theta = degrees(atan2(yslope, xslope));
    // In the northern hemisphere the star rotates clockwise, in the southern hemisphere anti-clockwise
    // In NH the pole is to the right of the drift vector (-90 degrees) however in pixel terms (Y +ve down) it is to the left (+90 degrees)
    // So we multiply by m_hemi to get the correct direction

    m_alpha = theta + m_hemi * 90 * m_mirror; // direction to the pole
    m_offset = hypot(xslope, yslope)*factor;  //polar alignment error in pixels
    m_target = PHD_Point(m_current.X + m_offset*cos(radians(m_alpha)), m_current.Y + m_offset*(sin(radians(m_alpha))));

    Debug.AddLine(wxString::Format("Polar Drift: m_hemi %d m_mirror %d m_pxScale %.1f", m_hemi, m_mirror, m_pxScale));
    Debug.AddLine(wxString::Format("Polar Drift: m_num %d m_t0 %.1f tnow %.1f m_current(X,Y): %.1f,%.1f", m_num, m_t0, tnow,
        m_current.X, m_current.Y));
    Debug.AddLine(wxString::Format("Polar Drift: slope(X,Y) %.4f,%.4f m_offset %.1f theta %.1f m_alpha %.1f", xslope, yslope, m_offset, theta, m_alpha));
    Debug.AddLine(wxString::Format("Polar Drift: m_target(X,Y) %.1f,%.1f", m_target.X, m_target.Y));
    SetStatusText(wxString::Format(_("Time %.fs"), tnow), 0);
    SetStatusText(wxString::Format(_("PA Err: %.f min"), m_offset*m_pxScale / 60), 1);
    SetStatusText(wxString::Format(_("Angle: %.f deg"), norm(-m_alpha, -180, 180)), 2);

    return true;
}
