/*
 *  comet_tool.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2014 Andy Galasso
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
#include "comet_tool.h"
#include "nudge_lock.h"

#include <wx/valnum.h>

struct CometToolWin : public wxDialog
{
    wxToggleButton *m_enable;
    wxStaticText *m_xLabel;
    wxSpinCtrlDouble *m_xRate;
    wxStaticText *m_yLabel;
    wxSpinCtrlDouble *m_yRate;
    wxRadioBox *m_units;
    wxRadioBox *m_axes;
    wxButton *m_start;
    wxButton *m_stop;
    wxTextCtrl *m_status;

    bool m_training;
    wxTimer m_timer;

    PHD_Point m_startPos;
    wxLongLong_t m_startTime;

    CometToolWin();
    ~CometToolWin();

    void OnClose(wxCloseEvent& event);
    void OnAppStateNotify(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnEnableToggled(wxCommandEvent& event);
    void OnSpinCtrlX(wxSpinDoubleEvent& event);
    void OnSpinCtrlY(wxSpinDoubleEvent& event);
    void OnUnits(wxCommandEvent& event);
    void OnAxes(wxCommandEvent& event);
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);

    void UpdateGuiderShift();
    void CalcRate();
    void UpdateStatus();
};

static wxString TITLE = wxTRANSLATE("Comet Tracking");
static wxString TITLE_TRAINING = wxTRANSLATE("Comet Tracking - Training Active");
static wxString TITLE_ACTIVE = wxTRANSLATE("Comet Tracking - Active");

CometToolWin::CometToolWin()
    : wxDialog(pFrame, wxID_ANY, wxGetTranslation(TITLE), wxPoint(-1,-1), wxSize(300,300)),
      m_training(false),
      m_timer(this)
{
    SetSizeHints( wxDefaultSize, wxDefaultSize );

    m_enable = new wxToggleButton(this, wxID_ANY, _("Enable"), wxDefaultPosition, wxDefaultSize, 0);
    m_enable->SetToolTip(_("Toggle comet tracking on or off."));

    m_xLabel = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(20,-1), wxALIGN_RIGHT);
    m_xLabel->Wrap(-1);
    m_yLabel = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(20, -1), wxALIGN_RIGHT);
    m_yLabel->Wrap(-1);

    m_xRate = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -5000., +5000., 0., 1.);
    m_xRate->SetToolTip(_("Comet tracking rate"));
    m_yRate = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -5000., +5000., 0., 1.);
    m_yRate->SetToolTip(_("Comet tracking rate"));

    wxString m_unitsChoices[] = { _("Pixels/hr"), _("Arcsec/hr") };
    int m_unitsNChoices = sizeof(m_unitsChoices) / sizeof(wxString);
    m_units = new wxRadioBox(this, wxID_ANY, _("Units"), wxDefaultPosition, wxDefaultSize, m_unitsNChoices, m_unitsChoices, 1, wxRA_SPECIFY_ROWS);
    m_units->SetSelection(1);
    m_units->SetToolTip(_("Tracking rate units"));

    wxString m_axesChoices[] = { _("Camera (X/Y)"), _("Mount (RA/Dec)") };
    int m_axesNChoices = sizeof(m_axesChoices) / sizeof(wxString);
    m_axes = new wxRadioBox(this, wxID_ANY, _("Axes"), wxDefaultPosition, wxDefaultSize, m_axesNChoices, m_axesChoices, 1, wxRA_SPECIFY_ROWS);
    m_axes->SetSelection(1);
    m_axes->SetToolTip(_("Tracking rate axes"));

    m_start = new wxButton(this, wxID_ANY, _("Start"), wxDefaultPosition, wxDefaultSize, 0);
    m_start->SetToolTip(_("Start training the tracking rate."));
    m_stop = new wxButton(this, wxID_ANY, _("Stop"), wxDefaultPosition, wxDefaultSize, 0);
    m_stop->SetToolTip(_("Stop training"));
    m_stop->Enable(false);

    // Use a text ctrl for status, wxStaticText flickers. Adding the wxTE_NO_VSCROLL style also causes the control to flicker on Windows 7.
    long style = wxSTATIC_BORDER | wxTE_MULTILINE /*| wxTE_NO_VSCROLL*/;
    m_status = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(400, 60), style);
    m_status->Enable(false);

    wxBoxSizer *xSizer = new wxBoxSizer(wxHORIZONTAL);
    xSizer->Add(0, 0, 1, wxEXPAND, 5);
    xSizer->Add(m_xLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    xSizer->Add(m_xRate, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    xSizer->Add(0, 0, 1, wxEXPAND, 5);

    wxBoxSizer *ySizer = new wxBoxSizer(wxHORIZONTAL);
    ySizer->Add(0, 0, 1, wxEXPAND, 5);
    ySizer->Add(m_yLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    ySizer->Add(m_yRate, 0, wxALL, 5);
    ySizer->Add(0, 0, 1, wxEXPAND, 5);

    wxStaticBoxSizer *ratesSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Rates")), wxVERTICAL);
    ratesSizer->Add(xSizer, 1, wxEXPAND, 5);
    ratesSizer->Add(ySizer, 1, wxEXPAND, 5);
    ratesSizer->Add(m_units, 0, wxALL, 5);
    ratesSizer->Add(m_axes, 0, wxALL, 5);

    wxBoxSizer *startStopSizer = new wxBoxSizer(wxHORIZONTAL);
    startStopSizer->Add(0, 0, 1, wxEXPAND, 5);
    startStopSizer->Add(m_start, 0, wxALL, 5);
    startStopSizer->Add(m_stop, 0, wxALL, 5);
    startStopSizer->Add(0, 0, 1, wxEXPAND, 5);

    wxStaticBoxSizer *trainingSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Rate Training")), wxVERTICAL);
    trainingSizer->Add(startStopSizer, 1, wxEXPAND, 5);
    trainingSizer->Add(m_status, 0, wxALL, 5);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_enable, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5);
    topSizer->Add(ratesSizer, 1, wxEXPAND, 5);
    topSizer->Add(trainingSizer, 0, wxEXPAND, 5);

    SetSizer(topSizer);
    Layout();
    topSizer->Fit(this);

    // Connect Events
    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(CometToolWin::OnClose));
    Connect(APPSTATE_NOTIFY_EVENT, wxCommandEventHandler(CometToolWin::OnAppStateNotify));
    Connect(wxEVT_TIMER, wxTimerEventHandler(CometToolWin::OnTimer));
    m_enable->Connect(wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(CometToolWin::OnEnableToggled), NULL, this);
    m_xRate->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(CometToolWin::OnSpinCtrlX), NULL, this);
    m_yRate->Connect(wxEVT_SPINCTRLDOUBLE, wxSpinDoubleEventHandler(CometToolWin::OnSpinCtrlY), NULL, this);
    m_units->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CometToolWin::OnUnits), NULL, this);
    m_axes->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(CometToolWin::OnAxes), NULL, this);
    m_start->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CometToolWin::OnStart), NULL, this);
    m_stop->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CometToolWin::OnStop), NULL, this);

    int xpos = pConfig->Global.GetInt("/CometTool/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/CometTool/pos.y", -1);
    if (xpos == -1 || ypos == -1)
        Centre(wxBOTH);
    else
        Move(xpos, ypos);

    UpdateStatus();

    wxCommandEvent dummy;
    OnAppStateNotify(dummy); // init controls
}

CometToolWin::~CometToolWin(void)
{
    pFrame->pCometTool = 0;
}

static void SetEnabledState(CometToolWin* win, bool active)
{
    if (active)
    {
        win->SetTitle(wxGetTranslation(TITLE_ACTIVE));
        win->m_enable->SetLabel(_("Disable"));
    }
    else
    {
        win->SetTitle(wxGetTranslation(TITLE));
        win->m_enable->SetLabel(_("Enable"));
    }
}

void CometToolWin::OnEnableToggled(wxCommandEvent& event)
{
    bool active = m_enable->GetValue();
    pFrame->pGuider->EnableLockPosShift(active);
    SetEnabledState(this, active);
}

void CometToolWin::UpdateGuiderShift()
{
    PHD_Point rate(m_xRate->GetValue(), m_yRate->GetValue());
    GRAPH_UNITS units = m_units->GetSelection() == 0 ? UNIT_PIXELS : UNIT_ARCSEC;
    bool isMountCoords = m_axes->GetSelection() == 1;
    pFrame->pGuider->SetLockPosShiftRate(rate, units, isMountCoords);
}

void CometToolWin::OnSpinCtrlX(wxSpinDoubleEvent& event)
{
    UpdateGuiderShift();
}

void CometToolWin::OnSpinCtrlY(wxSpinDoubleEvent& event)
{
    UpdateGuiderShift();
}

void CometToolWin::OnUnits(wxCommandEvent& event)
{
    UpdateGuiderShift();
}

void CometToolWin::OnAxes(wxCommandEvent& event)
{
    UpdateGuiderShift();
}

void CometToolWin::OnStart(wxCommandEvent& event)
{
    if (!pFrame->pGuider->IsGuiding())
        return;

    if (!pFrame->pNudgeLock)
        pFrame->pNudgeLock = NudgeLockTool::CreateNudgeLockToolWindow();
    pFrame->pNudgeLock->Show();

    m_startPos = pFrame->pGuider->LockPosition();
    m_startTime = ::wxGetUTCTimeMillis().GetValue();

    pFrame->pGuider->EnableLockPosShift(true);

    m_start->Enable(false);
    m_stop->Enable(true);
    m_units->Enable(false);
    m_axes->Enable(false);

    m_timer.Start(500);
    m_training = true;

    SetTitle(wxGetTranslation(TITLE_TRAINING));
    UpdateStatus();
}

void CometToolWin::OnStop(wxCommandEvent& event)
{
    m_timer.Stop();
    m_training = false;

    m_start->Enable(pFrame->pGuider->IsGuiding());
    m_stop->Enable(false);
    m_units->Enable(true);
    m_axes->Enable(true);

    SetTitle(wxGetTranslation(TITLE));
    UpdateStatus();
}

void CometToolWin::OnTimer(wxTimerEvent& WXUNUSED(evt))
{
    UpdateStatus();
}

void CometToolWin::CalcRate()
{
    double dt = (double) (::wxGetUTCTimeMillis().GetValue() - m_startTime) / 3600000.0; // hours
    PHD_Point rate = (pFrame->pGuider->LockPosition() - m_startPos) / dt;
    pFrame->pGuider->SetLockPosShiftRate(rate, UNIT_PIXELS, false);
}

void CometToolWin::UpdateStatus()
{
    if (m_training)
    {
        m_status->SetValue(wxString::Format(_("Training, elapsed time %lus.\nUse the "
            "\"Adjust Lock Position\" controls to center the comet\nin the imaging "
            "camera and click Stop to complete training."),
            (long)((::wxGetUTCTimeMillis().GetValue() - m_startTime) / 1000)));
    }
    else
    {
        m_status->SetValue(_("Center the comet in the imaging camera.\nSelect a guide "
            "star and start Guiding.\nThen, click Start to begin training."));
    }
}

void CometToolWin::OnAppStateNotify(wxCommandEvent& WXUNUSED(event))
{
    const LockPosShiftParams& shift = pFrame->pGuider->GetLockPosShiftParams();

    m_enable->SetValue(shift.shiftEnabled);
    SetEnabledState(this, m_enable->GetValue());

    m_xRate->SetValue(shift.shiftRate.X);
    m_yRate->SetValue(shift.shiftRate.Y);

    if (shift.shiftIsMountCoords)
    {
        m_axes->SetSelection(1);
        m_xLabel->SetLabel(_("RA"));
        m_yLabel->SetLabel(_("Dec"));
    }
    else
    {
        m_axes->SetSelection(0);
        m_xLabel->SetLabel(_("X"));
        m_yLabel->SetLabel(_("Y"));
    }

    if (shift.shiftUnits == UNIT_PIXELS)
    {
        m_units->SetSelection(0);
    }
    else
    {
        m_units->SetSelection(1);
    }

    if (m_training)
    {
        if (!pFrame->pGuider->IsGuiding())
        {
            // if guiding stopped, stop training
            wxCommandEvent dummy;
            OnStop(dummy);
        }
    }
    else
    {
        m_start->Enable(pFrame->pGuider->IsGuiding());
    }
}

void CometToolWin::OnClose(wxCloseEvent& evt)
{
    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/CometTool/pos.x", x);
    pConfig->Global.SetInt("/CometTool/pos.y", y);

    evt.Skip();
}

wxWindow *CometTool::CreateCometToolWindow()
{
    return new CometToolWin();
}

void CometTool::NotifyUpdateLockPos()
{
    if (pFrame && pFrame->pCometTool)
    {
        CometToolWin *win = static_cast<CometToolWin *>(pFrame->pCometTool);
        if (win->m_training)
            win->CalcRate();
    }
}

void CometTool::UpdateCometToolControls()
{
    // notify comet tool to update its controls
    if (pFrame && pFrame->pCometTool)
    {
        wxCommandEvent event(APPSTATE_NOTIFY_EVENT, pFrame->GetId());
        event.SetEventObject(pFrame);
        wxPostEvent(pFrame->pCometTool, event);
    }
}
