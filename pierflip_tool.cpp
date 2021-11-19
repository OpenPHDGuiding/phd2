/*
 *  pierflip_tool.cpp
 *  Open PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2018 Andy Galasso.
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
#include "pierflip_tool.h"

enum State
{
    ST_INTRO,
    ST_SLEW_1,
    ST_CALIBRATE_1,
    ST_SLEW_2,
    ST_CALIBRATE_2,
    ST_DONE,
};

struct PierFlipCalToolWin : public wxFrame
{
    wxTextCtrl *m_instructions;
    wxTextCtrl *m_dec;
    wxTextCtrl *m_ha;
    wxTextCtrl *m_pierSide;
    wxSizer *m_scopePosCtrls;
    wxButton *m_restart;
    wxButton *m_next;
    wxTimer m_timer;
    wxStatusBar *m_status;

    State m_state;
    bool m_calibration_started;
    Calibration m_firstCal;
    bool m_result;
    wxString m_resultError;

    void OnRestartClick(wxCommandEvent& event);
    void OnNextClick(wxCommandEvent& event);
    void DoOnTimer();
    void OnTimer(wxTimerEvent& event);
    void SetState(State state);
    void OnGuidingStateUpdated();

    PierFlipCalToolWin();
    ~PierFlipCalToolWin();
};

PierFlipCalToolWin::PierFlipCalToolWin()
    :
    wxFrame(pFrame, wxID_ANY, _("Meridian Flip Calibration Tool"), wxDefaultPosition, wxSize(334, 350),
            wxCAPTION|wxCLOSE_BOX|wxFRAME_NO_TASKBAR|wxTAB_TRAVERSAL)
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer *sz1 = new wxBoxSizer(wxVERTICAL);

    wxSize sz = GetTextExtent(_T("M"));

    m_instructions = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(31 * sz.GetWidth(), 19 * sz.GetHeight() / 2),
        wxTE_MULTILINE | wxTE_NO_VSCROLL | wxTE_READONLY | wxTE_WORDWRAP);
    sz1->Add(m_instructions, 0, wxALL|wxEXPAND, 5);

    wxFlexGridSizer *sz2 = new wxFlexGridSizer(3, 2, 0, 0);
    sz2->SetFlexibleDirection(wxBOTH);
    sz2->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    wxStaticText *label1 = new wxStaticText(this, wxID_ANY, _("Declination"));
    sz2->Add(label1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_dec = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    sz2->Add(m_dec, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText *label2 = new wxStaticText(this, wxID_ANY, _("Hour Angle"));
    sz2->Add(label2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_ha = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    sz2->Add(m_ha, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText *label3 = new wxStaticText(this, wxID_ANY, _("Pier Side"));
    sz2->Add(label3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_pierSide = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    sz2->Add(m_pierSide, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    m_scopePosCtrls = sz2;

    sz1->Add(sz2, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer *sz3 = new wxBoxSizer(wxHORIZONTAL);

    m_restart = new wxButton(this, wxID_ANY, _("Start over"), wxDefaultPosition, wxDefaultSize, 0);
    sz3->Add(m_restart, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    sz3->Add(0, 0, 1, wxEXPAND, 5);

    m_next = new wxButton(this, wxID_ANY, _("Calibrate"), wxDefaultPosition, wxDefaultSize, 0);
    sz3->Add(m_next, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxALL, 5);

    sz1->Add(sz3, 0, wxEXPAND, 5);

    SetSizer(sz1);
    Layout();

    m_timer.SetOwner(this, wxID_ANY);
    m_status = CreateStatusBar(1, 0, wxID_ANY);

    Centre(wxBOTH);

    // Connect Events
    m_restart->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PierFlipCalToolWin::OnRestartClick), nullptr, this);
    m_next->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PierFlipCalToolWin::OnNextClick), nullptr, this);
    Connect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(PierFlipCalToolWin::OnTimer));

    SetState(ST_INTRO);
}

PierFlipCalToolWin::~PierFlipCalToolWin()
{
    Debug.Write("PFT: closed\n");

    // Disconnect Events
    m_restart->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PierFlipCalToolWin::OnRestartClick), nullptr, this);
    m_next->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PierFlipCalToolWin::OnNextClick), nullptr, this);
    Disconnect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(PierFlipCalToolWin::OnTimer));

    if (pFrame->pierFlipToolWin == this)
        pFrame->pierFlipToolWin = nullptr;
}

static bool StartCalibration(wxString *err)
{
    SettleParams settle;
    settle.frames = 4;
    settle.settleTimeSec = 0;
    settle.timeoutSec = 90.0;
    settle.tolerancePx = 99.0;

    return PhdController::Guide(true /* recalibrate */, settle, wxRect(), err);
}

void PierFlipCalToolWin::OnRestartClick(wxCommandEvent& event)
{
    SetState(ST_SLEW_1);
}

void PierFlipCalToolWin::OnNextClick(wxCommandEvent& event)
{
    if (m_state == ST_INTRO)
    {
        SetState(ST_SLEW_1);
    }
    else if (m_state == ST_SLEW_1 || m_state == ST_SLEW_2)
    {
        wxString error;
        if (StartCalibration(&error))
            SetState(m_state == ST_SLEW_1 ? ST_CALIBRATE_1 : ST_CALIBRATE_2);
        else
        {
            Debug.Write(wxString::Format("PFT: start calibration failed: %s\n", error));
            m_status->SetStatusText(error);
        }
    }
    else if (m_state == ST_DONE)
    {
        Debug.Write(wxString::Format("PFT: apply result: %d\n", m_result));

        // apply setting
        TheScope()->SetCalibrationFlipRequiresDecFlip(m_result);

        // close the window
        Destroy();
    }
}

enum Color
{
    GREY,
    RED,
    YELLOW,
    GREEN,
};

static void SetBg(wxTextCtrl *ctrl, Color c)
{
    static wxColor R(237, 88, 88);
    static wxColor Y(237, 237, 88);
    static wxColor G(88, 237, 88);

    const wxColor& cl = c == GREY ? wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW) :
        c == RED ? R : c == YELLOW ? Y : G;

    if (ctrl->SetBackgroundColour(cl))
        ctrl->Refresh();
}

void PierFlipCalToolWin::SetState(State state)
{
    Debug.Write(wxString::Format("PFT: set state %d\n", state));

    m_state = state;
    m_calibration_started = false;

    // instructions
    wxString s;
    switch (state)
    {
    case ST_INTRO:
        s = _("This tool will automatically determine the correct value for the setting "
            "'Reverse Dec output after meridian flip'.\n\n"
            "The procedure requires two calibrations -- one with the telescope on the East "
            "side of the pier, and one on the West. You will be instructed to slew the "
            "telescope when needed.\n\n"
            "Click Next to begin");
        m_next->SetLabel(_("Next"));
        m_scopePosCtrls->ShowItems(false);
        Fit();
        break;
    case ST_SLEW_1:
        s = _("Point the telescope in the direction of the intersection of the meridian and the celestial "
            "equator, near Hour Angle = 0 and Declination = 0.\n\nClick Calibrate when ready.");
        m_next->SetLabel(_("Calibrate"));
        m_status->SetStatusText(wxEmptyString);
        if (!m_scopePosCtrls->AreAnyItemsShown())
        {
            m_scopePosCtrls->ShowItems(true);
            Fit();
        }
        break;
    case ST_CALIBRATE_1:
        s = _("Wait for the first calibration to complete.");
        m_status->SetStatusText(wxString::Format(_("Calibrating on the %s side of pier"),
            Scope::PierSideStrTr(pPointingSource->SideOfPier())));
        break;
    case ST_SLEW_2:
        s = wxString::Format(_("Slew the telescope to force a meridian flip - the scope should move to the %s side of the pier, still pointing near Dec = 0."),
            Scope::PierSideStrTr(m_firstCal.pierSide == PIER_SIDE_EAST ? PIER_SIDE_WEST : PIER_SIDE_EAST)) +
            _T("\n\n") +
            _("Point the telescope in the direction of the intersection of the meridian and the celestial "
            "equator, near Hour Angle = 0 and Declination = 0.\n\nClick Calibrate when ready.");
        m_status->SetStatusText(wxEmptyString);
        break;
    case ST_CALIBRATE_2:
        s = _("Wait for the second calibration to complete.");
        m_status->SetStatusText(wxString::Format(_("Calibrating on the %s side of pier"),
            Scope::PierSideStrTr(pPointingSource->SideOfPier())));
        break;
    case ST_DONE:
        if (m_resultError.empty())
        {
            s = _("Meridian flip calibration completed sucessfully.") +
                _T("\n\n") +
                wxString::Format(_("The correct setting for 'Reverse Dec output after meridian flip' "
                "for this mount is: %s"),
                m_result ? _("enabled") : _("disabled")) +
                _T("\n\n") +
                _("Click Apply to accept the setting");
        }
        else
        {
            s = _("Meridian flip calibration failed.") + _T("\n\n") +
                m_resultError + _T("\n\n") +
                _("Resolve any calibration issues and try again");
        }
        m_next->SetLabel(_("Apply"));
        m_status->SetStatusText(wxEmptyString);
        break;
    }
    m_instructions->SetValue(s);

    // pier side color
    switch (state)
    {
    case ST_INTRO:
    case ST_SLEW_1:
    case ST_CALIBRATE_1:
        SetBg(m_pierSide, GREY);
        break;
    default:
        break;
    }

    // start over button enable/disable
    {
        bool enable;
        switch (state)
        {
        case ST_INTRO:
        case ST_SLEW_1:
        case ST_CALIBRATE_1:
        case ST_CALIBRATE_2:
            enable = false;
            break;
        case ST_SLEW_2:
        case ST_DONE:
            enable = true;
            break;
        }
        m_restart->Enable(enable);
    }

    // next button enabled/disabled
    {
        int enable = -1;
        switch (state)
        {
        case ST_INTRO:
            enable = 1;
            break;
        case ST_CALIBRATE_1:
        case ST_CALIBRATE_2:
            enable = 0;
            break;
        case ST_DONE:
            enable = m_resultError.empty();
            break;
        default:
            break;
        }
        if (enable != -1)
            m_next->Enable(enable ? true : false);
    }

    m_timer.Stop();

    if (m_state != ST_INTRO)
    {
        DoOnTimer();
        m_timer.Start(1000);
    }
}

void PierFlipCalToolWin::DoOnTimer()
{
    double ra, dec, lst, ha;
    bool err = pPointingSource ? pPointingSource->GetCoordinates(&ra, &dec, &lst) : true;
    if (err)
    {
        dec = ha = 999.;
        m_dec->SetValue(_("Unknown"));
        m_ha->SetValue(_("Unknown"));
    }
    else
    {
        ha = norm(lst - ra, -12.0, 12.0);
        m_dec->SetValue(wxString::Format("%+.1f%s", dec, DEGREES_SYMBOL));
        m_ha->SetValue(wxString::Format("%+.2fh", ha));
    }

    // dec color = green if < 30, yellow if < Scope::DEC_COMP_LIMIT, red otherwise
    double absdec = fabs(dec);
    if (absdec < 30.)
        SetBg(m_dec, GREEN);
    else if (absdec < degrees(Scope::DEC_COMP_LIMIT))
        SetBg(m_dec, YELLOW);
    else
        SetBg(m_dec, RED);

    // ha color: green or yellow
    double absha = fabs(ha);
    if (absha < 2.5)
        SetBg(m_ha, GREEN);
    else
        SetBg(m_ha, YELLOW);

    PierSide ps = pPointingSource ? pPointingSource->SideOfPier() : PIER_SIDE_UNKNOWN;
    m_pierSide->SetValue(Scope::PierSideStrTr(ps));

    int enable = -1;
    switch (m_state)
    {
    case ST_SLEW_1:
        enable = ps != PIER_SIDE_UNKNOWN && absdec < degrees(Scope::DEC_COMP_LIMIT);
        break;
    case ST_SLEW_2:
        enable = ps != PIER_SIDE_UNKNOWN && ps != m_firstCal.pierSide;
        SetBg(m_pierSide, enable ? GREEN : RED);
        enable = enable && absdec < degrees(Scope::DEC_COMP_LIMIT);
        break;
    default:
        break;
    }
    if (enable != -1)
        m_next->Enable(enable ? true : false);
}

void PierFlipCalToolWin::OnTimer(wxTimerEvent&)
{
   DoOnTimer();
}

void PierFlipCalToolWin::OnGuidingStateUpdated()
{
    if (m_state != ST_CALIBRATE_1 && m_state != ST_CALIBRATE_2)
        return;

    if (m_calibration_started && !pFrame->pGuider->IsCalibratingOrGuiding())
    {
        // calibration stopped
        Debug.Write("PFT: calibration stopped\n");
        SetState(m_state == ST_CALIBRATE_1 ? ST_SLEW_1 : ST_SLEW_2);
        return;
    }

    if (pFrame->pGuider->IsCalibrating() && !m_calibration_started)
    {
        Debug.Write("PFT: calibration started\n");
        m_calibration_started = true;
    }

    if (m_calibration_started)
    {
        // guiding
        if (TheScope()->IsCalibrated() && !PhdController::IsSettling())
        {
            // calibrated and settle done
            if (m_state == ST_CALIBRATE_1)
            {
                Debug.Write("PFT: calibrate1 done, start looping\n");
                m_firstCal = TheScope()->MountCal();
                pFrame->StartLooping();
                SetState(ST_SLEW_2);
            }
            else if (m_state == ST_CALIBRATE_2)
            {
                const Calibration& cal2 = TheScope()->MountCal();
                double dx = norm_angle(m_firstCal.xAngle - cal2.xAngle);

                Debug.Write(wxString::Format("PFT: deltaRA = %.1f deg\n", degrees(dx)));

                m_resultError.clear();

                // dx should be close to 180 degrees
                if (fabs(dx) < radians(150))
                {
                    Debug.Write("PFT: deltaRA too small!\n");
                    m_resultError = _("The RA calibration angles varied by an unexpected amount.");
                }
                else
                {
                    // dy should be close to 0 or 180 degrees
                    double dy = norm_angle(m_firstCal.yAngle - cal2.yAngle);

                    Debug.Write(wxString::Format("PFT: deltaDec = %.1f deg\n", degrees(dy)));

                    if (fabs(dy) < radians(30))
                        m_result = false;
                    else if (fabs(dy) > radians(150))
                        m_result = true;
                    else
                    {
                        Debug.Write("PFT: deltaDec not definitive!\n");
                        m_resultError = _("The declination calibration angles varied by an unexpected amount.");
                    }
                }
                SetState(ST_DONE);
            }
        }
    }
}

bool PierFlipTool::CanRunTool(wxString *error)
{
    if (TheAO())
    {
        Debug.Write("PFT: called when AO present\n");
        *error = _("The meridian flip calibration tool requires an equipment profile without an AO");
        return false;
    }
    if (!TheScope())
    {
        Debug.Write("PFT: called when no mount present\n");
        *error = _("The meridian flip calibration tool requires a mount. "
            "Click the Connect Equipment button to select your mount.");
        return false;
    }
    if (TheScope()->GetDecGuideMode() == DEC_GUIDE_MODE::DEC_NONE)
    {
        Debug.Write("PFT: called when dec guiding disabled\n");
        *error = _("The meridian flip calibration tool cannnot be run with Declination guiding disabled. "
            "If your mount can guide in Declination, set your Dec guide mode to Auto and try again.");
        return false;
    }
    return true;
}

void PierFlipTool::ShowPierFlipCalTool()
{
    if (!pFrame->pierFlipToolWin)
    {
        Debug.Write("PFT: opened\n");
        pFrame->pierFlipToolWin = new PierFlipCalToolWin();
    }

    pFrame->pierFlipToolWin->Show();
}

void PierFlipTool::UpdateUIControls()
{
    PierFlipCalToolWin *win = static_cast<PierFlipCalToolWin *>(pFrame->pierFlipToolWin);
    win->OnGuidingStateUpdated();
}
