/*
 *  drift_tool.h
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2013 Andy Galasso
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

    bool m_can_slew;
    bool m_slewing;

    double m_raSlewVal;
    double m_decSlewVal;

    wxStaticText *m_title;
    wxStaticText *m_instructions;
    wxTextCtrl *m_raCurrent;
    wxTextCtrl *m_decCurrent;
    wxTextCtrl *m_raSlew;
    wxTextCtrl *m_decSlew;
    wxButton *m_slew;
    wxButton *m_drift;
    wxButton *m_adjust;
    wxButton *m_phaseBtn;
    wxStatusBar *m_statusBar;
    wxTimer *m_timer;

    void EnableSlew(bool enable);

    void OnSlew(wxCommandEvent& evt);
    void OnDrift(wxCommandEvent& evt);
    void OnAdjust(wxCommandEvent& evt);
    void OnPhase(wxCommandEvent& evt);
    void OnAppStateNotify(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);
    void OnTimer(wxTimerEvent& evt);

    void ShowScopeCoordinates(void);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DriftToolWin, wxFrame)
    EVT_BUTTON(ID_SLEW, DriftToolWin::OnSlew)
    EVT_BUTTON(ID_DRIFT, DriftToolWin::OnDrift)
    EVT_BUTTON(ID_ADJUST, DriftToolWin::OnAdjust)
    EVT_BUTTON(ID_PHASE, DriftToolWin::OnPhase)
    EVT_COMMAND(wxID_ANY, APPSTATE_NOTIFY_EVENT, DriftToolWin::OnAppStateNotify)
    EVT_CLOSE(DriftToolWin::OnClose)
    EVT_TIMER(ID_TIMER, DriftToolWin::OnTimer)
END_EVENT_TABLE()

DriftToolWin::DriftToolWin()
    : wxFrame(pFrame, wxID_ANY, _("Drift Align Tool"), wxDefaultPosition, wxDefaultSize, wxCAPTION|wxCLOSE_BOX|wxMINIMIZE_BOX|wxSYSTEM_MENU|wxTAB_TRAVERSAL|wxFRAME_FLOAT_ON_PARENT),
        m_need_end_dec_drift(false),
        m_slewing(false)
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer* bSizer1;
    bSizer1 = new wxBoxSizer(wxVERTICAL);

    m_title = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_title->Wrap(-1);
    bSizer1->Add(m_title, 0, wxALIGN_CENTER|wxALL, 5);

    m_instructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(400,100), wxALIGN_LEFT|wxST_NO_AUTORESIZE);
    m_instructions->Wrap(-1);
    bSizer1->Add(m_instructions, 0, wxALL, 5);

    wxStaticBoxSizer* sbSizer1;
    sbSizer1 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Scope Pointing")), wxVERTICAL);

    wxGridBagSizer* gbSizer2;
    gbSizer2 = new wxGridBagSizer(0, 0);
    gbSizer2->SetFlexibleDirection(wxBOTH);
    gbSizer2->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    wxStaticText* staticText3;
    staticText3 = new wxStaticText(this, wxID_ANY, _("Meridian Offset (deg)"), wxDefaultPosition, wxDefaultSize, 0);
    staticText3->Wrap(-1);
    gbSizer2->Add(staticText3, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 5);

    wxStaticText* staticText4;
    staticText4 = new wxStaticText(this, wxID_ANY, _("Declination (deg)"), wxDefaultPosition, wxDefaultSize, 0);
    staticText4->Wrap(-1);
    gbSizer2->Add(staticText4, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 5);

    wxStaticText* staticText5;
    staticText5 = new wxStaticText(this, wxID_ANY, _("Current"), wxDefaultPosition, wxDefaultSize, 0);
    staticText5->Wrap(-1);
    gbSizer2->Add(staticText5, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL, 5);

    m_raCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    gbSizer2->Add(m_raCurrent, wxGBPosition(1, 1), wxGBSpan(1, 1), wxALL, 5);

    m_decCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    gbSizer2->Add(m_decCurrent, wxGBPosition(1, 2), wxGBSpan(1, 1), wxALL, 5);

    wxStaticText* staticText6;
    staticText6 = new wxStaticText(this, wxID_ANY, _("Slew To"), wxDefaultPosition, wxDefaultSize, 0);
    staticText6->Wrap(-1);
    gbSizer2->Add(staticText6, wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL, 5);

    wxFloatingPointValidator<double> ra_validator(0, &m_raSlewVal, wxNUM_VAL_DEFAULT);
    ra_validator.SetRange(-90.0, 90.0);
    m_raSlew = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0L, ra_validator);
    gbSizer2->Add(m_raSlew, wxGBPosition(2, 1), wxGBSpan(1, 1), wxALL, 5);

    wxFloatingPointValidator<double> dec_validator(0, &m_decSlewVal, wxNUM_VAL_DEFAULT);
    ra_validator.SetRange(-90.0, 90.0);
    m_decSlew = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0L, dec_validator);
    gbSizer2->Add(m_decSlew, wxGBPosition(2, 2), wxGBSpan(1, 1), wxALL, 5);

    m_slew = new wxButton(this, ID_SLEW, _("Slew"), wxDefaultPosition, wxDefaultSize, 0);
    gbSizer2->Add(m_slew, wxGBPosition(2, 3), wxGBSpan(1, 1), wxALL, 5);

    sbSizer1->Add(gbSizer2, 1, wxALIGN_CENTER, 5);

    bSizer1->Add(sbSizer1, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    bSizer1->Add(0, 30, 0, wxEXPAND, 5);

    wxBoxSizer* bSizer2;
    bSizer2 = new wxBoxSizer(wxHORIZONTAL);

    bSizer2->Add(0, 0, 2, wxEXPAND, 5);

    m_drift = new wxButton(this, ID_DRIFT, _("Drift"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(m_drift, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    bSizer2->Add(0, 0, 1, wxEXPAND, 5);

    m_adjust = new wxButton(this, ID_ADJUST, _("Adjust"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(m_adjust, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    bSizer2->Add(0, 0, 2, wxEXPAND, 5);

    m_phaseBtn = new wxButton(this, ID_PHASE, wxT("???"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer2->Add(m_phaseBtn, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    bSizer1->Add(bSizer2, 1, wxEXPAND|wxALL, 5);

    bSizer1->Add(0, 0, 1, wxEXPAND, 5);

    SetSizer(bSizer1);

    Layout();
    bSizer1->Fit(this);

    m_statusBar = CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);

    int xpos = pConfig->Global.GetInt("/DriftTool/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/DriftTool/pos.y", -1);
    if (xpos == -1 || ypos == -1)
        Centre(wxBOTH);
    else
        Move(xpos, ypos);

    // can mount slew?
    m_can_slew = pMount && pMount->CanSlew();

    m_timer = NULL;
    if (m_can_slew)
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

    m_phase = PHASE_ADJUST_AZ;
    m_mode = MODE_IDLE;
    UpdatePhaseState();
    UpdateModeState();
}

DriftToolWin::~DriftToolWin()
{
    delete m_timer;
    pFrame->pDriftTool = NULL;
}

void DriftToolWin::EnableSlew(bool enable)
{
    m_raSlew->Enable(enable);
    m_decSlew->Enable(enable);
    m_slew->Enable(enable && !m_slewing);
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
    LoadRADec(m_phase, &m_raSlewVal, &m_decSlewVal);
    TransferDataToWindow();

    if (m_phase == PHASE_ADJUST_AZ)
    {
        m_title->SetLabel(_("Azimuth Adjustment"));
        m_instructions->SetLabel(_("Instructions:\r"
            "  Slew to near the Meridian and the Equator.\r"
            "  Press Drift to measure drift.\r"
            "  Press Adjust and adjust your mount's azimuth.\r"
            "  Repeat Drift/Adjust until alignment is complete.\r"
            "  Then, click Altitude to begin Altitude adjustment."));
        m_phaseBtn->SetLabel(_("> Altitude"));
    }
    else
    {
        m_title->SetLabel(_("Altitude Adjustment"));
        m_instructions->SetLabel(_("Instructions:\r"
            "  Slew to a location near the Equator and the Eastern or Western horizon.\r"
            "  Press Drift to measure drift.\r"
            "  Press Adjust and adjust your mount's altitude.\r"
            "  Repeat Drift/Adjust until alignment is complete.\r"
            "  Click Azimuth to repeat Azimuth adjustment."));
        m_phaseBtn->SetLabel(_("> Azimuth"));
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

        if (pFrame->pGuider->GetState() == STATE_GUIDING)
        {
            // stop guiding but continue looping
            pFrame->OnLoopExposure(dummy);
        }
    }
    else // MODE_IDLE
    {
        m_drift->Enable(true);
        m_adjust->Enable(true);
        m_drifting = false;
        EnableSlew(m_can_slew);
        SetStatusText(idleStatus);

        if (pFrame->pGuider->GetState() == STATE_GUIDING)
        {
            // stop guiding but continue looping
            pFrame->OnLoopExposure(dummy);
        }
    }
}

void DriftToolWin::OnSlew(wxCommandEvent& evt)
{
    if (Validate() && TransferDataFromWindow())
    {
        double cur_ra, cur_dec, cur_st;
        if (pMount->GetCoordinates(&cur_ra, &cur_dec, &cur_st))
        {
            Debug.AddLine("Drift tool: slew failed to get scope coordinates");
            return;
        }
        double slew_ra = cur_st + (m_raSlewVal * 24.0 / 360.0);
        if (slew_ra >= 24.0)
            slew_ra -= 24.0;
        else if (slew_ra < 0.0)
            slew_ra += 24.0;
        Debug.AddLine(wxString::Format("Drift tool slew from ra %.2f, dec %.1f to ra %.2f, dec %.1f", cur_ra, cur_dec, slew_ra, m_decSlewVal));
        m_slewing = true;
        m_slew->Enable(false);
        GetStatusBar()->PushStatusText(_("Slewing ..."));
        if (pMount->SlewToCoordinates(slew_ra, m_decSlewVal))
        {
            GetStatusBar()->PopStatusText();
            m_slewing = false;
            m_slew->Enable(true);
            Debug.AddLine("Drift tool: slew failed");
        }
        SaveRADec(m_phase, m_raSlewVal, m_decSlewVal);
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

    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/DriftTool/pos.x", x);
    pConfig->Global.SetInt("/DriftTool/pos.y", y);

    evt.Skip();
}

void DriftToolWin::ShowScopeCoordinates(void)
{
    if (!pMount)
        return;
    double ra, dec, st;
    if (pMount->GetCoordinates(&ra, &dec, &st))
        return; // error
    double ra_deg = (ra - st) * (360.0 / 24.0);
    if (ra_deg > 180.0)
        ra_deg -= 360.0;
    if (ra_deg <= -180.0)
        ra_deg += 360.0;

    m_raCurrent->SetValue(wxString::Format("%+.f", ra_deg));
    m_decCurrent->SetValue(wxString::Format("%+.f", dec));
}

void DriftToolWin::OnTimer(wxTimerEvent& evt)
{
    ShowScopeCoordinates();

    if (m_slewing)
    {
        if (!pMount || !pMount->Slewing())
        {
            m_slew->Enable(true);
            m_slewing = false;
            GetStatusBar()->PopStatusText(); // clear "slewing" message
        }
    }
}

wxWindow *DriftTool::CreateDriftToolWindow()
{
    return new DriftToolWin();
}
