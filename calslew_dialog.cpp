/*
*  calslew_dialog.cpp
*  PHD Guiding
*
*  Created by Bruce Waddington
*  Copyright (c) 2023 Bruce Waddington
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
#include "calslew_dialog.h"

enum {defBestDec = 0, defBestOffset = 5};

// Utility function to add the <label, input> pairs to a flexgrid
static void AddTableEntryPair(wxWindow *parent, wxFlexGridSizer *pTable, const wxString& label, wxWindow *pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "), wxPoint(-1, -1), wxSize(-1, -1));
    pTable->Add(pLabel, 1, wxALL, 5);
    pTable->Add(pControl, 1, wxALL, 5);
}

static wxSpinCtrl *NewSpinnerInt(wxWindow *parent, const wxSize& size, int val, int minval, int maxval, int inc,
    const wxString& tooltip)
{
    wxSpinCtrl *pNewCtrl = pFrame->MakeSpinCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size, wxSP_ARROW_KEYS, minval, maxval, val, _("Exposure time"));
    pNewCtrl->SetValue(val);
    pNewCtrl->SetToolTip(tooltip);
    return pNewCtrl;
}

static void MakeBold(wxControl *ctrl)
{
    wxFont font = ctrl->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    ctrl->SetFont(font);
}

void CalSlewDialog::GetCustomLocation(int* PrefHA, int* PrefDec, bool* SingleSide, bool* UsingDefaults) const
{
    *PrefHA = pConfig->Profile.GetInt ("/scope/CalSlew/TgtHA", defBestOffset);
    *PrefDec = pConfig->Profile.GetInt("/scope/CalSlew/TgtDec", defBestDec);
    *SingleSide = pConfig->Profile.GetBoolean("/scope/CalSlew/SingleSide", false);
    *UsingDefaults = (*PrefDec == defBestDec && *PrefHA == defBestOffset && !*SingleSide);
}

CalSlewDialog::CalSlewDialog()
    : wxDialog(pFrame, wxID_ANY, _("Calibration Slew"),
    wxDefaultPosition, wxSize(600, -1), wxCAPTION | wxCLOSE_BOX)
{
    wxStaticBoxSizer* currSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Current Position"));
    wxStaticBoxSizer* tgtSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Target Position"));
    wxFlexGridSizer* currPosSizer = new wxFlexGridSizer(1, 5, 5, 15);
    wxFlexGridSizer* targetPosSizer = new wxFlexGridSizer(1, 5, 5, 15);

    m_pExplanation = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(600, -1), wxALIGN_CENTER_HORIZONTAL);
    MakeBold(m_pExplanation);
    int textWidth = StringWidth(this, "000000000");
    m_pCurrOffset = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(textWidth, -1));
    m_pCurrDec = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(textWidth, -1));
    wxStaticBoxSizer* sizerCurrSOP = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Pointing"));
    m_pCurrWest = new wxRadioButton(this, wxID_ANY, _("West"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_pCurrEast = new wxRadioButton(this, wxID_ANY, _("East"));
    sizerCurrSOP->Add(m_pCurrWest);
    sizerCurrSOP->Add(m_pCurrEast);

    AddTableEntryPair(this, currPosSizer, _("Declination"), m_pCurrDec);
    AddTableEntryPair(this, currPosSizer, _("Meridian offset (degrees)"), m_pCurrOffset);
    currPosSizer->Add(sizerCurrSOP);
    currSizer->Add(currPosSizer);
    MakeBold(m_pCurrDec);
    MakeBold(m_pCurrOffset);

    int spinnerWidth = StringWidth(this, "0000");
    m_pTargetDec = NewSpinnerInt(this, wxSize(spinnerWidth, -1), 0, -50, 50, 5, _("Target declination for slew, as close to Dec = 0 as possible for your location \n(>=-20 and <= 20) recommended"));
    AddTableEntryPair(this, targetPosSizer, _("Declination"), m_pTargetDec);
    m_pTargetOffset = NewSpinnerInt(this, wxSize(spinnerWidth, -1), 10, 5, 50, 5, _("Target offset from central meridian, in degrees; east or west based on 'Pointing' buttons (less than 15 degrees recommended)"));
    AddTableEntryPair(this, targetPosSizer, _("Meridian offset (degrees)"), m_pTargetOffset);

    wxStaticBoxSizer* sizerTargetSOP = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Pointing"));
    m_pTargetWest = new wxRadioButton(this, wxID_ANY, _("West"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_pTargetWest->SetToolTip(_("Scope on the east side of pier, pointing west"));
    m_pTargetWest->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &CalSlewDialog::OnTargetWest, this);
    m_pTargetEast = new wxRadioButton(this, wxID_ANY, _("East"));
    m_pTargetEast->SetToolTip(_("Scope on west side of pier, pointing east"));
    m_pTargetEast->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &CalSlewDialog::OnTargetEast, this);
    sizerTargetSOP->Add(m_pTargetWest);
    sizerTargetSOP->Add(m_pTargetEast);
    targetPosSizer->Add(sizerTargetSOP);
    tgtSizer->Add(targetPosSizer);

    wxBoxSizer* midBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *pCustomBtn = new wxButton(this, wxID_ANY, _("Save custom values..."));
    pCustomBtn->SetToolTip(_("Saves a custom sky location if your site has restricted sky visibility and you can't calibrate at the recommended location"));
    pCustomBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSlewDialog::OnCustom, this);
    wxButton *pLoadBtn = new wxButton(this, wxID_ANY, _("Load custom values"));
    pLoadBtn->SetToolTip(_("Reloads a previously saved custom location and displays its values in the 'Target Position' fields"));
    pLoadBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSlewDialog::OnLoadCustom, this);
    wxButton* pRestoreBtn = new wxButton(this, wxID_ANY, _("Restore defaults"));
    pRestoreBtn->SetToolTip(_("Restores the 'Target Position' fields to show the recommended pointing location"));
    pRestoreBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSlewDialog::OnRestore, this);
    midBtnSizer->Add(pLoadBtn, wxSizerFlags().Center().Border(wxALL, 20));
    midBtnSizer->Add(pCustomBtn, wxSizerFlags().Center().Border(wxALL, 20));
    midBtnSizer->Add(pRestoreBtn, wxSizerFlags().Center().Border(wxALL, 20));

    m_pMessage = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(600, -1), wxALIGN_CENTER_HORIZONTAL);
    MakeBold(m_pMessage);
    m_pWarning = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxALIGN_CENTER_HORIZONTAL);
    MakeBold(m_pWarning);

    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pSlewBtn = new wxButton(this, wxID_ANY,_("Slew"));
    m_pSlewBtn->SetToolTip(_("Starts a slew to the target sky location. BE SURE the scope can be safely slewed"));
    m_pSlewBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSlewDialog::OnSlew, this);
    m_pCalibrateBtn = new wxButton(this, wxID_ANY, _("Calibrate"));
    m_pCalibrateBtn->SetToolTip(_("Starts the PHD2 calibration. This dialog window will close once the calibration has begun."));
    m_pCalibrateBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSlewDialog::OnCalibrate, this);
    wxButton* pCancelBtn = new wxButton(this, wxID_ANY, _("Cancel"));
    pCancelBtn->SetToolTip(_("Closes the dialog window without re-calibrating"));
    pCancelBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalSlewDialog::OnCancel, this);
    btnSizer->Add(m_pSlewBtn, wxSizerFlags().Border(wxALL, 20));
    btnSizer->Add(m_pCalibrateBtn, wxSizerFlags().Border(wxALL, 20));
    btnSizer->Add(pCancelBtn, wxSizerFlags().Border(wxALL, 20));

    wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
    vSizer->Add(m_pExplanation, wxSizerFlags().Center().Border(wxTop, 5));
    vSizer->Add(currSizer, wxSizerFlags().Center().Border(wxALL, 20));
    vSizer->Add(tgtSizer, wxSizerFlags().Center());
    vSizer->Add(midBtnSizer, wxSizerFlags().Center().Border(wxTOP,5));
    vSizer->Add(m_pMessage, wxSizerFlags().Center().Border(wxTOP, 15));
    vSizer->Add(m_pWarning, wxSizerFlags().Center().Border(wxTOP, 15));
    vSizer->Add(btnSizer, wxSizerFlags().Center().Border(wxTOP, 15));

    m_pTimer = new wxTimer(this, wxID_ANY);     // asynch updates to current position fields
    m_pTimer->SetOwner(this);
    this->Connect(wxEVT_TIMER, wxTimerEventHandler(CalSlewDialog::OnTimer), NULL, this);
    this->Bind(wxEVT_CLOSE_WINDOW, &CalSlewDialog::OnClose, this);
    InitializeUI(true);
    m_pCurrOffset->Enable(false);
    m_pCurrDec->Enable(false);
    m_pCurrWest->Enable(false);
    m_pCurrEast->Enable(false);
    SetAutoLayout(true);
    SetSizerAndFit(vSizer);
}

void CalSlewDialog::ShowError(const wxString& msg, bool fatal)
{
    m_pMessage->SetLabelText(msg);
    if (fatal)
    {
        m_pSlewBtn->Enable(false);
        m_pCalibrateBtn->Enable(false);

    }
}

void CalSlewDialog::ShowStatus(const wxString& msg)
{
    m_pMessage->SetLabelText(msg);
}

void CalSlewDialog::OnTimer(wxTimerEvent& evt)
{
    UpdateCurrentPosition(true);
}

void CalSlewDialog::UpdateCurrentPosition(bool fromTimer)
{
    double ra, dec, lst;
    double hourAngle;
    const int INVALID_DEC = 100;
    static double lastDec = INVALID_DEC;

    if (!pPointingSource->GetCoordinates(&ra, &dec, &lst))
    {
        hourAngle = norm(lst - ra, -12.0, 12.0);
        if (hourAngle < 0)
            m_pCurrEast->SetValue(true);
        else
            m_pCurrWest->SetValue(true);
        if (m_pCurrWest->GetValue() != m_pTargetWest->GetValue())
            m_pWarning->SetLabelText(_("MERIDIAN FLIP!"));
        else
            m_pWarning->SetLabelText(wxEmptyString);
        m_pCurrOffset->SetValue(wxString::Format("%.1f", abs(hourAngle * 15.0)));
        m_pCurrDec->SetValue(wxString::Format("%+.1f", dec));
        if (fromTimer)
        {
            if (fabs(lastDec - dec) > 10)
            {
                if (lastDec != INVALID_DEC)
                    ShowExplanationMsg(dec);
                lastDec = dec;
            }
        }
        else
            ShowExplanationMsg(dec);
    }
    else
    {
        ShowError(_("Mount can't report its pointing position"), true);
        if (m_pTimer)
            m_pTimer->Stop();
    }
}
// Return of 'true' means error occurred
bool CalSlewDialog::GetCalibPositionRecommendations(int* HA, int* Dec) const
{
    double hourAngle;
    int bestDec;
    int bestOffset;
    double lst;
    double ra;
    double dec;
    double eqAltitude;
    bool pointingEast = false;
    bool errorSeen = false;

    bestDec = defBestDec;
    bestOffset = defBestOffset;
    try
    {
        if (pPointingSource && pPointingSource->CanReportPosition())
        {
            if (pPointingSource->GetCoordinates(&ra, &dec, &lst))
            {
                throw ERROR_INFO("CalPositionRecommendations: Mount not reporting pointing position");
            }
            hourAngle = norm(lst - ra, -12.0, 12.0);
            if (hourAngle <= 0)
                bestOffset = -bestOffset;
            // Check that we aren't pointing down in the weeds at the default Dec location
            double lat, lon;
            if (pPointingSource->GetSiteLatLong(&lat, &lon))
                bestDec = 0;
            else if (lat >= 0)
            {
                eqAltitude = 90.0 - lat;
                if (eqAltitude < 30)        // far north location
                    bestDec += 30 - eqAltitude;
            }
            else
            {
                eqAltitude = 90 + lat;
                if (eqAltitude < 30)        // far south location
                    bestDec -= (30 - eqAltitude);
            }
            *HA = bestOffset;
            *Dec = bestDec;
            m_pSlewBtn->Enable(pPointingSource->CanSlew());
        }
        else
        {
            throw ERROR_INFO("CalPositionRecommendations: mount not connected or not reporting pointing info");
        }
    }
    catch (const wxString& msg)
    {
        POSSIBLY_UNUSED(msg);
        *HA = defBestOffset;
        *Dec = defBestDec;
        errorSeen = true;
    }
    return errorSeen;
}

void CalSlewDialog::ShowExplanationMsg(double dec)
{
    wxString slewCond;
    if (pPointingSource->CanSlew())
        slewCond = _("Use the 'slew' button to move the scope to a preferred position. ");
    else
        slewCond = _("Move the scope to a preferred position. ");
    if (fabs(dec) > 80)
    {
        m_pExplanation->SetLabelText(_("Calibration is likely to fail this close to the pole.\n") + slewCond);
    }
    else if (fabs(dec) > degrees(Scope::DEC_COMP_LIMIT))
    {
        m_pExplanation->SetLabelText(_("Declination compensation will not be effective if you calibrate within 30 degrees of the pole.\n") + slewCond);
    }
    else if (fabs(dec) > 20)
        m_pExplanation->SetLabelText(_("Calibration will be most accurate with the scope pointing closer to Dec = 0.\n") + slewCond);
    else
        m_pExplanation->SetLabelText(wxEmptyString);
}

void CalSlewDialog::InitializeUI(bool forceDefaults)
{
    if (pPointingSource && pPointingSource->CanReportPosition())
    {
        int bestDec;
        int bestOffset;
        bool singleSide = false;
        bool usingDefaults;
        double hourAngle;
        double ra;
        double dec;
        double lst;
        bool pointingEast = false;
        bool noLocationInfo = false;

        if (forceDefaults)
        {
            usingDefaults = true;
        }
        else
            CalSlewDialog::GetCustomLocation(&bestOffset,&bestDec, &singleSide, &usingDefaults);    // Get any custom preferences if present

        if (pPointingSource->GetCoordinates(&ra, &dec, &lst))
        {
            ShowError(_("Mount can't report its pointing position"), true);
            return;
        }

        ShowExplanationMsg(dec);
        hourAngle = norm(lst - ra, -12.0, 12.0);

        if (!usingDefaults)                                             // Custom pointing position
        {
            if (singleSide)                                             // Use specified E-W orientation
            {
                if (bestOffset <= 0)
                    m_pTargetEast->SetValue(true);
                else
                    m_pTargetWest->SetValue(true);
            }
            else
            {
                if (hourAngle <= 0)                                     // Use same E-W orientation as current pointing position
                    m_pTargetEast->SetValue(true);
                else
                    m_pTargetWest->SetValue(true);
            }
            bestOffset = abs(bestOffset);
            m_pTargetOffset->SetValue(wxString::Format("%d", bestOffset));
            m_pTargetDec->SetValue(wxString::Format("%d", bestDec));
        }
        else
        {
            if (!GetCalibPositionRecommendations(&bestOffset, &bestDec))
            {
                pointingEast = (bestOffset <= 0);

                m_pTargetOffset->SetValue(wxString::Format("%d", abs(bestOffset)));
                m_pTargetDec->SetValue(wxString::Format("%d", bestDec));
                if (pointingEast)
                    m_pTargetEast->SetValue(true);
                else
                    m_pTargetWest->SetValue(true);
            }
            else
            {
                ShowError(_("Mount can't report its pointing position"), true);
                return;
            }
        }           // end of default handling

        // Current position
        m_pCurrOffset->SetValue(wxString::Format("%.1f", abs(hourAngle * 15.0)));
        m_pCurrDec->SetValue(wxString::Format("%+.1f", dec));
        m_pCurrEast->SetValue(hourAngle <= 0);
        if (m_pCurrEast->GetValue() != m_pTargetEast->GetValue())
            m_pWarning->SetLabelText(_("MERIDIAN FLIP!"));
        else
            m_pWarning->SetLabelText(wxEmptyString);

        m_pTimer->Stop();
        m_pTimer->Start(1500, false /* continuous */);
        if (pPointingSource->CanSlew())
            ShowStatus(_("Adjust 'Target Position' values if needed for your location, then click 'Slew'"));
        else
            ShowStatus(_("Manually move the telescope to a Dec location near ") + std::to_string(bestDec));
    }
    else
    {
        if (!pPointingSource || !pPointingSource->IsConnected())
            ShowError(_("Mount is not connected"), true);
        else
            ShowError(_("Mount can't report its pointing position"), true);
    }

}

void CalSlewDialog::UpdateTargetPosition(int CustHA, int CustDec)
{
    m_pTargetOffset->SetValue(abs(CustHA));
    m_pTargetDec->SetValue(CustDec);
    m_pWarning->SetLabelText(wxEmptyString);
    if (CustHA <= 0)
    {
        m_pTargetEast->SetValue(true);
        if (m_pCurrWest->GetValue())
            m_pWarning->SetLabelText("MERIDIAN FLIP!");
    }
    else
    {
        m_pTargetWest->SetValue(true);
        if (m_pCurrEast->GetValue())
            m_pWarning->SetLabelText("MERIDIAN FLIP!");
    }
}

bool CalSlewDialog::PerformSlew(double ra, double dec)
{
    bool completed = false;
    if (pFrame->CaptureActive)
        pFrame->StopCapturing();

    if (pPointingSource->CanSlewAsync())
    {
        struct SlewInBg : public RunInBg
        {
            double ra, dec;
            SlewInBg(wxWindow *parent, double ra_, double dec_)
                : RunInBg(parent, _("Slew"), _("Slewing...")), ra(ra_), dec(dec_)
            {
                SetPopupDelay(100);
            }
            bool Entry()        //wxWidgets background thread method
            {
                bool err = pPointingSource->SlewToCoordinatesAsync(ra, dec);
                if (err)
                {
                    SetErrorMsg(_("Slew failed! Make sure scope is tracking at sidereal rate"));
                    return true;
                }
                while (pPointingSource->Slewing())
                {
                    wxMilliSleep(500);
                    if (IsCanceled())
                    {
                        pPointingSource->AbortSlew();
                        SetErrorMsg(_("Slew was cancelled"));
                        return true;
                    }
                }
                return false;
            }
        };
        SlewInBg bg(this, ra, dec);
        if (bg.Run())
        {
            ShowError(bg.GetErrorMsg(), false);
        }
        else
        {
            UpdateCurrentPosition(false);
            ShowExplanationMsg(dec);
            completed = true;
        }
    }
    else
    {
        wxBusyCursor busy;
        m_pSlewBtn->Enable(false);
        if (!pPointingSource->SlewToCoordinates(ra, dec))
        {
            ShowExplanationMsg(dec);
            ShowStatus(_("Click on 'calibrate' to start calibration or 'Cancel' to exit"));
            completed = true;
        }
        else
        {
            m_pSlewBtn->Enable(true);
            ShowError(_("Slew failed! Make sure scope is tracking at sidereal rate"), false);
            Debug.Write("Cal-slew: slew failed\n");
        }
    }
    return !completed;
}

void CalSlewDialog::OnSlew(wxCommandEvent& evt)
{
    double offsetSlew = (double) m_pTargetOffset->GetValue();
    double decSlew = (double) m_pTargetDec->GetValue();
    double slew_ra;
    bool clearBacklash = false;

    double cur_ra, cur_dec, cur_st;
    ShowStatus("");
    if (pPointingSource->GetCoordinates(&cur_ra, &cur_dec, &cur_st))
    {
        Debug.Write("Cal-slew: slew failed to get scope coordinates\n");
        ShowError("Could not get coordinates from mount!", true);
        return;
    }

    if (m_pTargetEast->GetValue())
        slew_ra = norm_ra(cur_st + (offsetSlew / 15.0));
    else
        slew_ra = norm_ra(cur_st - (offsetSlew / 15.0));

    Debug.Write(wxString::Format("Cal-slew: slew from ra %.2f, dec %.1f to ra %.2f, dec %.1f\n",
        cur_ra, cur_dec, slew_ra, decSlew));
    if (decSlew < cur_dec)         // scope will slew south regardless of north or south hemisphere
    {
        ShowStatus(_("Initial slew to approximate position"));
        if (!PerformSlew(slew_ra, decSlew - 1.0))
        {
            wxMilliSleep(500);
            ShowStatus(_("Final slew north to pre-clear Dec backlash"));
            if (!PerformSlew(slew_ra, decSlew))
                ShowStatus(_("Click on 'calibrate' to start calibration or 'Cancel' to exit"));
        }
    }
    else
    {
        ShowStatus(_("Slewing to target position"));
        if (!PerformSlew(slew_ra, decSlew))
            ShowStatus(_("Click on 'calibrate' to start calibration or 'Cancel' to exit"));
    }

}

void CalSlewDialog::OnCalibrate(wxCommandEvent& evt)
{
    SettleParams settle;
    wxString msg;
    settle.tolerancePx = 99.;
    settle.settleTimeSec = 1;
    settle.timeoutSec = 1;
    settle.frames = 1;

    if (pPointingSource->PreparePositionInteractive())
        return;
    if (PhdController::Guide(true, settle, wxRect(), &msg))
    {
        ShowStatus("Calibration started");
        wxDialog::Destroy();
    }
    else
        ShowError(_("Calibration could not start - suspend any imaging automation apps"), false);
}

void CalSlewDialog::OnTargetWest(wxCommandEvent& evt)
{
    if (m_pCurrEast->GetValue())
        m_pWarning->SetLabelText(_("MERIDIAN FLIP!"));
    else
        m_pWarning->SetLabelText("");
}

void CalSlewDialog::OnTargetEast(wxCommandEvent& evt)
{
    if (m_pCurrWest->GetValue())
        m_pWarning->SetLabelText("MERIDIAN FLIP!");
    else
        m_pWarning->SetLabelText("");
}

void CalSlewDialog::OnCancel(wxCommandEvent& evt)
{
    wxDialog::Destroy();
}

void CalSlewDialog::OnClose(wxCloseEvent& evt)
{
    wxDialog::Destroy();
}

void CalSlewDialog::OnRestore(wxCommandEvent& evt)
{
    InitializeUI(true);
}

void CalSlewDialog::OnLoadCustom(wxCommandEvent& evt)
{
    InitializeUI(false);
}

void CalSlewDialog::OnCustom(wxCommandEvent& evt)
{
    int ha = m_pTargetOffset->GetValue();
    int dec = m_pTargetDec->GetValue();
    bool sideEast = m_pTargetEast->GetValue() == 1;
    if (sideEast)
        ha = -ha;
    CalCustomDialog custDlg(this, ha, dec);
    custDlg.ShowModal();            // Dialog handles the UI updates
}

CalSlewDialog::~CalSlewDialog(void)
{
    delete m_pTimer;
    pFrame->pCalSlewDlg = NULL;
}

CalCustomDialog::CalCustomDialog(CalSlewDialog* Parent, int DefaultHA, int DefaultDec)
    : wxDialog(pFrame, wxID_ANY, _("Save Customized Calibration Position"),
    wxDefaultPosition, wxSize(474, -1), wxCAPTION | wxCLOSE_BOX)
{
    m_Parent = Parent;
    wxStaticBoxSizer* tgtSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Target Position"));
    wxFlexGridSizer* targetPosSizer = new wxFlexGridSizer(1, 5, 5, 15);

    int spinnerWidth = StringWidth(this, "0000");
    m_pTargetDec = NewSpinnerInt(this, wxSize(spinnerWidth, -1), DefaultDec, -50, 50, 5, _("Target declination for slew, as close to Dec = 0 as possible for your location"));
    AddTableEntryPair(this, targetPosSizer, _("Declination"), m_pTargetDec);
    m_pTargetOffset = NewSpinnerInt(this, wxSize(spinnerWidth, -1), abs(DefaultHA), 5, 50, 5, _("Target offset from central meridian, in degrees; east or west based on 'Pointing' buttons"));
    AddTableEntryPair(this, targetPosSizer, _("Meridian offset (degrees)"), m_pTargetOffset);

    wxStaticBoxSizer* sizerTargetSOP = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Pointing"));
    m_pTargetWest = new wxRadioButton(this, wxID_ANY, _("West"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_pTargetWest->SetToolTip(_("Scope on the east side of pier, pointing west"));
    m_pTargetWest->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &CalCustomDialog::OnTargetWest, this);
    m_pTargetEast = new wxRadioButton(this, wxID_ANY, _("East"));
    m_pTargetEast->SetToolTip(_("Scope on the west side of pier, pointing east"));
    m_pTargetEast->Bind(wxEVT_COMMAND_RADIOBUTTON_SELECTED, &CalCustomDialog::OnTargetEast, this);
    if (DefaultHA <= 0)
        m_pTargetEast->SetValue(true);
    else
        m_pTargetWest->SetValue(true);
    sizerTargetSOP->Add(m_pTargetWest);
    sizerTargetSOP->Add(m_pTargetEast);
    targetPosSizer->Add(sizerTargetSOP);
    tgtSizer->Add(targetPosSizer);

    m_pEastWestOnly = new wxCheckBox(this, wxID_ANY, wxEmptyString);
    m_pEastWestOnly->SetToolTip(_("Check this if calibration can only be done on a particular side of the meridian because of nearby obstructions"));
    if (m_pTargetWest->GetValue())
        m_pEastWestOnly->SetLabelText(_("Western sky only"));
    else
        m_pEastWestOnly->SetLabelText(_("Eastern sky only"));

    wxStaticText* pMessage = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(600, -1), wxALIGN_CENTER_HORIZONTAL);
    pMessage->SetLabelText(_("If your site location requires a unique sky position for calibration, \n") + 
        _("you can specify it here."));
    MakeBold(pMessage);

    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* OkBtn = new wxButton(this, wxID_ANY, _("Ok"));
    OkBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalCustomDialog::OnOk, this);
    wxButton* cancelBtn = new wxButton(this, wxID_ANY, _("Cancel"));
    cancelBtn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &CalCustomDialog::OnCancel, this);
    btnSizer->Add(OkBtn, wxSizerFlags().Border(wxALL, 20));
    btnSizer->Add(cancelBtn, wxSizerFlags().Border(wxALL, 20));

    wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);
    vSizer->Add(tgtSizer, wxSizerFlags().Center());
    vSizer->Add(m_pEastWestOnly, wxSizerFlags().Center().Border(wxTOP, 15));
    vSizer->Add(pMessage, wxSizerFlags().Center().Border(wxTOP, 15));
    vSizer->Add(btnSizer, wxSizerFlags().Center().Border(wxTOP, 15));

    SetAutoLayout(true);
    SetSizerAndFit(vSizer);
};

void CalCustomDialog::OnOk(wxCommandEvent& evt)
{
    int cDec = m_pTargetDec->GetValue();
    int cHA = m_pTargetOffset->GetValue();
    if (m_pTargetEast->GetValue())
        cHA = -cHA;
    pConfig->Profile.SetInt("/scope/CalSlew/TgtHA", cHA);
    pConfig->Profile.SetInt("/scope/CalSlew/TgtDec", cDec);
    pConfig->Profile.SetBoolean("/scope/CalSlew/SingleSide", m_pEastWestOnly->GetValue());
    m_Parent->UpdateTargetPosition(cHA, cDec);

    EndDialog(wxOK);
}

void CalCustomDialog::OnCancel(wxCommandEvent& evt)
{
    EndDialog(wxCANCEL);
}
void CalCustomDialog::OnTargetWest(wxCommandEvent& evt)
{
    m_pEastWestOnly->SetLabelText(_("Western sky only"));
}
void CalCustomDialog::OnTargetEast(wxCommandEvent& evt)
{
    m_pEastWestOnly->SetLabelText(_("Eastern sky only"));
}