/*
 *  drift_tool.h
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2013-2014 Andy Galasso
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

#include <wx/gbsizer.h>
#include <wx/valnum.h>

enum Phase
{
    PHASE_ADJUST_AZ,
    PHASE_ADJUST_ALT,
};

enum Mode
{
    MODE_IDLE,
    MODE_DRIFT,
    MODE_ADJUST,
};

enum CtrlIds
{
    ID_SLEW = 10001,
    ID_SAVE,
    ID_DRIFT,
    ID_ADJUST,
    ID_PHASE,
    ID_TIMER,
};

struct DriftToolWin : public wxFrame
{
    DriftToolWin();
    ~DriftToolWin();

    void UpdatePhaseState();
    void UpdateModeState();

    Phase m_phase;
    Mode m_mode;
    bool m_drifting;
    bool m_need_end_dec_drift;
    bool m_save_lock_pos_is_sticky;
    bool m_save_use_subframes;
    GraphLogClientWindow::GRAPH_MODE m_save_graph_mode;
    int m_save_graph_length;
    int m_save_graph_height;

    bool m_can_slew;
    bool m_slewing;
    PHD_Point m_siteLatLong;

    wxStaticBitmap *m_bmp;
    wxBitmap *m_azArrowBmp;
    wxBitmap *m_altArrowBmp;
    wxStaticText *m_instructions;
    wxTextCtrl *m_raCurrent;
    wxTextCtrl *m_decCurrent;
    wxSpinCtrl *m_raSlew;
    wxSpinCtrl *m_decSlew;
    wxButton *m_slew;
    wxButton *m_saveCoords;
    wxStaticText *m_notesLabel;
    wxTextCtrl *m_notes;
    wxButton *m_drift;
    wxButton *m_adjust;
    wxButton *m_phaseBtn;
    wxStatusBar *m_statusBar;
    wxTimer *m_timer;

    void EnableSlew(bool enable);

    void OnSlew(wxCommandEvent& evt);
    void OnSaveCoords(wxCommandEvent& evt);
    void OnNotesText(wxCommandEvent& evt);
    void OnDrift(wxCommandEvent& evt);
    void OnAdjust(wxCommandEvent& evt);
    void OnPhase(wxCommandEvent& evt);
    void OnAppStateNotify(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);
    void OnTimer(wxTimerEvent& evt);

    void UpdateScopeCoordinates(void);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DriftToolWin, wxFrame)
    EVT_BUTTON(ID_SLEW, DriftToolWin::OnSlew)
    EVT_BUTTON(ID_SAVE, DriftToolWin::OnSaveCoords)
    EVT_BUTTON(ID_DRIFT, DriftToolWin::OnDrift)
    EVT_BUTTON(ID_ADJUST, DriftToolWin::OnAdjust)
    EVT_BUTTON(ID_PHASE, DriftToolWin::OnPhase)
    EVT_COMMAND(wxID_ANY, APPSTATE_NOTIFY_EVENT, DriftToolWin::OnAppStateNotify)
    EVT_CLOSE(DriftToolWin::OnClose)
    EVT_TIMER(ID_TIMER, DriftToolWin::OnTimer)
END_EVENT_TABLE()

DriftToolWin::DriftToolWin()
    : wxFrame(pFrame, wxID_ANY, _("Drift Align"), wxDefaultPosition, wxDefaultSize,
              wxCAPTION|wxCLOSE_BOX|wxMINIMIZE_BOX|wxSYSTEM_MENU|wxTAB_TRAVERSAL|wxFRAME_FLOAT_ON_PARENT|wxFRAME_NO_TASKBAR),
        m_need_end_dec_drift(false),
        m_slewing(false)
{
    SetBackgroundColour(wxColor(0xcccccc));

    SetSizeHints(wxDefaultSize, wxDefaultSize);

    // a vertical sizer holding everything
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    // a horizontal box sizer for the bitmap and the instructions
    wxBoxSizer *instrSizer = new wxBoxSizer(wxHORIZONTAL);

#   include "icons/AzArrow.xpm"
    m_azArrowBmp = new wxBitmap(AzArrow);
#   include "icons/AltArrow.xpm"
    m_altArrowBmp = new wxBitmap(AltArrow);

    m_bmp = new wxStaticBitmap(this, wxID_ANY, *m_azArrowBmp, wxDefaultPosition, wxSize(80, 100));
    instrSizer->Add(m_bmp, 0, wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);

    m_instructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(300,90), wxALIGN_LEFT|wxST_NO_AUTORESIZE);
#ifdef __WXOSX__
    m_instructions->SetFont(*wxSMALL_FONT);
#endif
    m_instructions->Wrap(-1);
    instrSizer->Add(m_instructions, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    topSizer->Add(instrSizer);

    // static box sizer holding the scope pointing controls
    wxStaticBoxSizer *sbSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Scope Pointing")), wxVERTICAL);

    // a grid box sizer for laying out scope pointing the controls
    wxGridBagSizer *gbSizer = new wxGridBagSizer(0, 0);
    gbSizer->SetFlexibleDirection(wxBOTH);
    gbSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    wxStaticText *txt;

    txt = new wxStaticText(this, wxID_ANY, _("Meridian Offset (deg)"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 5);

    txt = new wxStaticText(this, wxID_ANY, _("Declination (deg)"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 5);

    txt = new wxStaticText(this, wxID_ANY, _("Current"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL, 5);

    m_raCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    gbSizer->Add(m_raCurrent, wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    m_decCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    gbSizer->Add(m_decCurrent, wxGBPosition(1, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    txt = new wxStaticText(this, wxID_ANY, _("Slew To"), wxDefaultPosition, wxDefaultSize, 0);
    txt->Wrap(-1);
    gbSizer->Add(txt, wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL, 5);

    m_raSlew = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -90, 90);
    gbSizer->Add(m_raSlew, wxGBPosition(2, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    m_decSlew = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -90, 90);
    gbSizer->Add(m_decSlew, wxGBPosition(2, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

    m_slew = new wxButton(this, ID_SLEW, _("Slew"), wxDefaultPosition, wxDefaultSize, 0);
    m_slew->SetToolTip(_("Click to slew to given coordinates."));
    gbSizer->Add(m_slew, wxGBPosition(2, 3), wxGBSpan(1, 1), wxALL, 5);

    wxString label = _("Save");
    wxSize sz(GetTextExtent(label));
    sz.SetHeight(-1);
    sz.IncBy(16, 0);
    m_saveCoords = new wxButton(this, ID_SAVE, label, wxDefaultPosition, sz, 0);
    m_saveCoords->SetToolTip(_("Click to save these coordinates as the default location for this axis adjustment."));
    gbSizer->Add(m_saveCoords, wxGBPosition(2, 4), wxGBSpan(1, 1), wxTOP | wxBOTTOM | wxRIGHT, 5);

    // add grid bag sizer to static sizer
    sbSizer->Add(gbSizer, 1, wxALIGN_CENTER, 5);

    // add static sizer to top-level sizer
    topSizer->Add(sbSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    // add some padding below the static sizer
    topSizer->Add(0, 3, 0, wxEXPAND, 3);

    m_notesLabel = new wxStaticText(this, wxID_ANY, _("Altitude adjustment notes"), wxDefaultPosition, wxDefaultSize, 0);
    m_notesLabel->Wrap(-1);
    topSizer->Add(m_notesLabel, 0, wxEXPAND|wxTOP|wxLEFT, 8);

    m_notes = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 54), wxTE_MULTILINE);
    pFrame->RegisterTextCtrl(m_notes);
    topSizer->Add(m_notes, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    m_notes->Bind(wxEVT_COMMAND_TEXT_UPDATED, &DriftToolWin::OnNotesText, this);

    // horizontal sizer for the buttons
    wxBoxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);

    // proportional pad on left of Drift button
    hSizer->Add(0, 0, 2, wxEXPAND, 5);

    m_drift = new wxButton(this, ID_DRIFT, _("Drift"), wxDefaultPosition, wxDefaultSize, 0);
    hSizer->Add(m_drift, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    // proportional pad on right of Drift button
    hSizer->Add(0, 0, 1, wxEXPAND, 5);

    m_adjust = new wxButton(this, ID_ADJUST, _("Adjust"), wxDefaultPosition, wxDefaultSize, 0);
    hSizer->Add(m_adjust, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    // proportional pad on right of Align button
    hSizer->Add(0, 0, 2, wxEXPAND, 5);

    m_phaseBtn = new wxButton(this, ID_PHASE, wxT("???"), wxDefaultPosition, wxDefaultSize, 0);
    hSizer->Add(m_phaseBtn, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    // add button sizer to top level sizer
    topSizer->Add(hSizer, 1, wxEXPAND|wxALL, 5);

    SetSizer(topSizer);

    m_statusBar = CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);

    Layout();
    topSizer->Fit(this);

    int xpos = pConfig->Global.GetInt("/DriftTool/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/DriftTool/pos.y", -1);
    MyFrame::PlaceWindowOnScreen(this, xpos, ypos);

    // can mount slew?
    m_can_slew = pPointingSource && pPointingSource->CanSlew();

    // get site lat/long from scope
    double lat, lon;
    m_siteLatLong.Invalidate();
    if (pPointingSource && !pPointingSource->GetSiteLatLong(&lat, &lon))
    {
        m_siteLatLong.SetXY(lat, lon);
    }

    m_timer = NULL;
    if (m_can_slew || (pPointingSource && pPointingSource->CanReportPosition()))
    {
        enum { SCOPE_POS_POLL_MS = 1500 };
        m_timer = new wxTimer(this, ID_TIMER);
        m_timer->Start(SCOPE_POS_POLL_MS, false /* continuous */);
    }

    // make sure graph window is showing
    if (!pFrame->pGraphLog->IsShown())
    {
        wxCommandEvent evt;
        evt.SetInt(1); // "Checked"
        pFrame->OnGraph(evt);
    }

    // graph must be showing RA/Dec
    m_save_graph_mode = pFrame->pGraphLog->SetMode(GraphLogClientWindow::MODE_RADEC);

    // resize graph scale
    m_save_graph_length = pFrame->pGraphLog->GetLength();
    pFrame->pGraphLog->SetLength(pConfig->Global.GetInt("/DriftTool/GraphLength", GraphLogWindow::DefaultMaxLength));
    m_save_graph_height = pFrame->pGraphLog->GetHeight();
    pFrame->pGraphLog->SetHeight(pConfig->Global.GetInt("/DriftTool/GraphHeight", GraphLogWindow::DefaultMaxHeight));
    pFrame->pGraphLog->Refresh();

    // we do not want sticky lock position enabled
    m_save_lock_pos_is_sticky = pFrame->pGuider->LockPosIsSticky();
    pFrame->pGuider->SetLockPosIsSticky(false);
    pFrame->tools_menu->FindItem(EEGG_STICKY_LOCK)->Check(false);

    m_save_use_subframes = pCamera->UseSubframes;

    m_phase = PHASE_ADJUST_AZ;
    m_mode = MODE_IDLE;
    UpdatePhaseState();
    UpdateModeState();
}

DriftToolWin::~DriftToolWin()
{
    delete m_timer;
    delete m_azArrowBmp;
    delete m_altArrowBmp;
    pFrame->pDriftTool = NULL;
}

void DriftToolWin::EnableSlew(bool enable)
{
    m_raSlew->Enable(enable);
    m_decSlew->Enable(enable);
    m_slew->Enable(enable && !m_slewing);
    m_saveCoords->Enable(enable);
}

static void LoadRADec(Phase phase, double *ra, double *dec)
{
    if (phase == PHASE_ADJUST_AZ)
    {
        *ra = pConfig->Global.GetDouble("/DriftTool/Az/SlewRAOfs", 0.0);
        *dec = pConfig->Global.GetDouble("/DriftTool/Az/SlewDec", 0.0);
    }
    else
    {
        *ra = pConfig->Global.GetDouble("/DriftTool/Alt/SlewRAOfs", -65.0);
        *dec = pConfig->Global.GetDouble("/DriftTool/Alt/SlewDec", 0.0);
    }
}

static void SaveRADec(Phase phase, double ra, double dec)
{
    if (phase == PHASE_ADJUST_AZ)
    {
        pConfig->Global.SetDouble("/DriftTool/Az/SlewRAOfs", ra);
        pConfig->Global.SetDouble("/DriftTool/Az/SlewDec", dec);
    }
    else
    {
        pConfig->Global.SetDouble("/DriftTool/Alt/SlewRAOfs", ra);
        pConfig->Global.SetDouble("/DriftTool/Alt/SlewDec", dec);
    }
}

void DriftToolWin::UpdatePhaseState()
{
    double ra, dec;
    LoadRADec(m_phase, &ra, &dec);
    m_raSlew->SetValue((int) floor(ra));
    m_decSlew->SetValue((int) floor(dec));

    if (m_phase == PHASE_ADJUST_AZ)
    {
        SetTitle(_("Drift Align - Azimuth Adjustment"));
        m_bmp->SetBitmap(*m_azArrowBmp);
        m_instructions->SetLabel(
            _("Slew to near the Meridian and the Equator.\n"
              "Press Drift to measure drift.\n"
              "Press Adjust and adjust your mount's azimuth.\n"
              "Repeat Drift/Adjust until alignment is complete.\n"
              "Then, click Altitude to begin Altitude adjustment."));
        m_notesLabel->SetLabel(_("Azimuth adjustment notes"));
        m_notes->SetValue(pConfig->Profile.GetString("/DriftTool/Az/Notes", wxEmptyString));
        m_phaseBtn->SetLabel(_("> Altitude"));
    }
    else
    {
        SetTitle(_("Drift Align - Altitude Adjustment"));
        m_bmp->SetBitmap(*m_altArrowBmp);
        m_instructions->SetLabel(
            _("Slew to a location near the Equator and the Eastern or Western horizon.\n"
              "Press Drift to measure drift.\n"
              "Press Adjust and adjust your mount's altitude.\n"
              "Repeat Drift/Adjust until alignment is complete.\n"
              "Click Azimuth to repeat Azimuth adjustment."));
        m_notesLabel->SetLabel(_("Altitude adjustment notes"));
        m_notes->SetValue(pConfig->Profile.GetString("/DriftTool/Alt/Notes", wxEmptyString));
        m_phaseBtn->SetLabel(_("< Azimuth"));
    }
}

void DriftToolWin::UpdateModeState()
{
    wxCommandEvent dummy;
    wxString idleStatus;

repeat:

    if (m_mode == MODE_DRIFT)
    {
        m_drift->Enable(false);
        m_adjust->Enable(true);
        EnableSlew(false);

        // restore subframes setting
        pCamera->UseSubframes = m_save_use_subframes;

        if (!m_drifting)
        {
            if (!pCamera->Connected ||
                !pMount || !pMount->IsConnected())
            {
                idleStatus = _("Please connect a camera and a mount");
                m_mode = MODE_IDLE;
                goto repeat;
            }

            if (!pMount->IsCalibrated())
            {
                idleStatus = _("Please calibrate before starting drift alignment");
                m_mode = MODE_IDLE;
                goto repeat;
            }

            if (!pFrame->CaptureActive)
            {
                // loop exposures
                SetStatusText(_("Start Looping..."));
                pFrame->OnLoopExposure(dummy);
                return;
            }

            switch (pFrame->pGuider->GetState())
            {
            case STATE_UNINITIALIZED:
            case STATE_CALIBRATED:
            case STATE_SELECTING:
                SetStatusText(_("Auto-selecting a star"));
                pFrame->OnAutoStar(dummy);
                return;
            case STATE_CALIBRATING_PRIMARY:
            case STATE_CALIBRATING_SECONDARY:
                if (!pMount->IsCalibrated())
                    SetStatusText(_("Waiting for calibration to complete..."));
                return;
            case STATE_SELECTED:
                SetStatusText(_("Start guiding..."));
                pFrame->OnGuide(dummy);
                return;
            case STATE_GUIDING:
                // turn of dec guiding
                if (!m_need_end_dec_drift)
                {
                    pMount->StartDecDrift();
                    m_need_end_dec_drift = true;
                }
                // clear graph data
                SetStatusText(_("Drifting... click Adjust when done drifting"));
                pFrame->pGraphLog->OnButtonClear(dummy);
                pFrame->pGraphLog->EnableTrendLines(true);
                m_drifting = true;
                return;
            case STATE_STOP:
                return;
            }
        }
    }
    else if (m_mode == MODE_ADJUST)
    {
        m_drift->Enable(true);
        m_adjust->Enable(false);
        m_drifting = false;
        EnableSlew(m_can_slew);
        SetStatusText(m_phase == PHASE_ADJUST_AZ ? _("Adjust azimuth, click Drift when done") :
            _("Adjust altitude, click Drift when done"));

        // use full frames for adjust phase
        pCamera->UseSubframes = false;

        if (pFrame->pGuider->IsGuiding())
        {
            // stop guiding but continue looping
            pFrame->OnLoopExposure(dummy);

            // Set the lock position to the where the star has drifted to. This will be the center of the polar align circle.
            pFrame->pGuider->SetLockPosition(pFrame->pGuider->CurrentPosition());
            pFrame->pGraphLog->Refresh();  // polar align circle is updated in graph window's OnPaint handler
        }
    }
    else // MODE_IDLE
    {
        m_drift->Enable(true);
        m_adjust->Enable(true);
        m_drifting = false;
        EnableSlew(m_can_slew);
        SetStatusText(idleStatus);

        // restore subframes setting
        pCamera->UseSubframes = m_save_use_subframes;

        if (pFrame->pGuider->IsGuiding())
        {
            // stop guiding but continue looping
            pFrame->OnLoopExposure(dummy);
        }
    }
}

void DriftToolWin::OnSlew(wxCommandEvent& evt)
{
    double raSlew = (double) m_raSlew->GetValue();
    double decSlew = (double) m_decSlew->GetValue();

    double cur_ra, cur_dec, cur_st;
    if (pPointingSource->GetCoordinates(&cur_ra, &cur_dec, &cur_st))
    {
        Debug.AddLine("Drift tool: slew failed to get scope coordinates");
        return;
    }

    wxBusyCursor busy;

    double slew_ra = cur_st + (raSlew * 24.0 / 360.0);
    if (slew_ra >= 24.0)
        slew_ra -= 24.0;
    else if (slew_ra < 0.0)
        slew_ra += 24.0;
    Debug.AddLine(wxString::Format("Drift tool slew from ra %.2f, dec %.1f to ra %.2f, dec %.1f", cur_ra, cur_dec, slew_ra, decSlew));
    m_slewing = true;
    m_slew->Enable(false);
    GetStatusBar()->PushStatusText(_("Slewing ..."));
    if (pPointingSource->SlewToCoordinates(slew_ra, decSlew))
    {
        GetStatusBar()->PopStatusText();
        m_slewing = false;
        m_slew->Enable(true);
        Debug.AddLine("Drift tool: slew failed");
    }
    SaveRADec(m_phase, raSlew, decSlew);
}

void DriftToolWin::OnSaveCoords(wxCommandEvent& evt)
{
    double raSlew = (double) m_raSlew->GetValue();
    double decSlew = (double) m_decSlew->GetValue();

    SaveRADec(m_phase, raSlew, decSlew);
    SetStatusText(_("Coordinates saved."));
}

void DriftToolWin::OnNotesText(wxCommandEvent& evt)
{
    if (m_phase == PHASE_ADJUST_AZ)
    {
        pConfig->Profile.SetString("/DriftTool/Az/Notes", m_notes->GetValue());
    }
    else
    {
        pConfig->Profile.SetString("/DriftTool/Alt/Notes", m_notes->GetValue());
    }
}

void DriftToolWin::OnDrift(wxCommandEvent& evt)
{
    m_mode = MODE_DRIFT;
    UpdateModeState();
}

void DriftToolWin::OnAdjust(wxCommandEvent& evt)
{
    m_mode = MODE_ADJUST;
    UpdateModeState();
}

void DriftToolWin::OnPhase(wxCommandEvent& evt)
{
    if (m_phase == PHASE_ADJUST_ALT)
        m_phase = PHASE_ADJUST_AZ;
    else
        m_phase = PHASE_ADJUST_ALT;

    UpdatePhaseState();

    if (m_mode != MODE_IDLE)
    {
        m_mode = MODE_IDLE;
        UpdateModeState();
    }
}

void DriftToolWin::OnAppStateNotify(wxCommandEvent& evt)
{
    UpdateModeState();
}

void DriftToolWin::OnClose(wxCloseEvent& evt)
{
    Debug.AddLine("Close DriftTool");

    if (m_need_end_dec_drift)
    {
        pMount->EndDecDrift();
        pFrame->pGraphLog->EnableTrendLines(false);
        m_need_end_dec_drift = false;
    }

    // restore graph mode
    pFrame->pGraphLog->SetMode(m_save_graph_mode);

    // restore graph scale
    pConfig->Global.SetInt("/DriftTool/GraphLength", pFrame->pGraphLog->GetLength());
    pFrame->pGraphLog->SetLength(m_save_graph_length);
    pConfig->Global.SetInt("/DriftTool/GraphHeight", pFrame->pGraphLog->GetHeight());
    pFrame->pGraphLog->SetHeight(m_save_graph_height);
    pFrame->pGraphLog->Refresh();

    // turn sticky lock position back on if we disabled it
    if (m_save_lock_pos_is_sticky)
    {
        pFrame->pGuider->SetLockPosIsSticky(true);
        pFrame->tools_menu->FindItem(EEGG_STICKY_LOCK)->Check(true);
    }

    // restore subframes setting
    pCamera->UseSubframes = m_save_use_subframes;

    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/DriftTool/pos.x", x);
    pConfig->Global.SetInt("/DriftTool/pos.y", y);

    // restore polar align circle correction factor
    pFrame->pGuider->SetPolarAlignCircleCorrection(1.0);

    Destroy();
}

void DriftToolWin::UpdateScopeCoordinates(void)
{
    if (!pMount)
        return;

    double ra_hrs, dec_deg, st_hrs;
    if (pPointingSource->GetCoordinates(&ra_hrs, &dec_deg, &st_hrs))
        return; // error
    double ra_ofs_deg = (ra_hrs - st_hrs) * (360.0 / 24.0);
    if (ra_ofs_deg > 180.0)
        ra_ofs_deg -= 360.0;
    if (ra_ofs_deg <= -180.0)
        ra_ofs_deg += 360.0;

    m_raCurrent->SetValue(wxString::Format("%+.f", ra_ofs_deg));
    m_decCurrent->SetValue(wxString::Format("%+.f", dec_deg));

    // update polar align circle radius
    if (m_siteLatLong.IsValid())
    {
        double correction;

        if (m_phase == PHASE_ADJUST_AZ)
        {
            // azimuth correction from "Star Offset Positioning for Polar Axis Alignment", Frank Barrett, 2/19/2010
            double dec_r = radians(dec_deg);
            if (fabs(dec_r) < Mount::DEC_COMP_LIMIT)
            {
                double alt_r = radians(90.0 - m_siteLatLong.X + dec_deg);
                correction = cos(alt_r) / cos(dec_r);
            }
            else
            {
                correction = 1.0;
            }
        }
        else
        {
            // altitude correction:
            //     convert scope coordinates (RA = a, Dec = d) to cartesian coords:
            //     x = cos a cos d
            //     y = sin a cos d
            //     z = sin d
            // altitude adjustment is rotation about x-axis, so correction factor is radius of
            // circle of projection of scope vector onto the plane of the meridian (y-z plane)
            //     r^2 = y^2 + z^2
            // substitute y and z above and use a = 90 - h   (h = hour angle)
            //
            //  -ag
            //
            double ha_r = radians(ra_ofs_deg);
            double cos_dec = cos(radians(dec_deg));
            double cos_ha = cos(ha_r);
            correction = sqrt(1. + cos_dec * cos_dec * (cos_ha * cos_ha - 1.));

            // drift rate for the altitude measurement is assumed to be measured at the horizon,
            // but rate decreases as we move away from the horizon - Measuring Polar Axis Alignment
            // Error, Frank Barrett 2nd Edition 2 / 19 / 2010, Equation (2)
            if (fabs(ra_ofs_deg) > 15.)
            {
                correction /= fabs(sin(ha_r));
            }
            else
            {
                correction = 1.0;
            }
        }

        pFrame->pGuider->SetPolarAlignCircleCorrection(correction);
    }
}

void DriftToolWin::OnTimer(wxTimerEvent& evt)
{
    UpdateScopeCoordinates();

    if (m_slewing)
    {
        if (!pPointingSource || !pPointingSource->Slewing())
        {
            m_slew->Enable(true);
            m_slewing = false;
            GetStatusBar()->PopStatusText(); // clear "slewing" message
        }
    }
}

wxWindow *DriftTool::CreateDriftToolWindow()
{
    // confirm that image scale is specified

    if (pFrame->GetCameraPixelScale() == 1.0)
    {
        bool confirmed = ConfirmDialog::Confirm(_(
            "The Drift Align tool is most effective when PHD2 knows your guide\n"
            "scope focal length and camera pixel size.\n"
            "\n"
            "Enter your guide scope focal length on the Global tab in the Brain.\n"
            "Enter your camera pixel size on the Camera tab in the Brain.\n"
            "\n"
            "Would you like to run the drift tool anyway?"),
                "/drift_tool_without_pixscale");

        if (!confirmed)
        {
            return 0;
        }
    }

    return new DriftToolWin();
}
