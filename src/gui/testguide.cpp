/*
 *  testguide.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

class TestGuideDialog : public wxDialog
{
    wxButton *NButton1, *SButton1, *EButton1, *WButton1;
    wxButton *NButton2, *SButton2, *EButton2, *WButton2;
    wxSpinCtrlDouble *pulseDurationSpinCtrl;
    wxChoice *ditherTypeChoice;
    wxSpinCtrlDouble *ditherScaleSpinCtrl;
    wxCheckBox *raOnlyCheckBox;
public:
    TestGuideDialog();
    ~TestGuideDialog();
    wxSizer *InitMountControls();
    void OnButton(wxCommandEvent& evt);
    void OnReset(wxCommandEvent& evt);
    void OnDitherScaleChange(wxSpinDoubleEvent& evt);
    void OnRAOnlyChecked(wxCommandEvent& evt);
    void OnDither(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);
    void OnAppStateNotify(wxCommandEvent& evt);
    DECLARE_EVENT_TABLE()
};

enum
{
    ID_PULSEDURATION = 330001,
    ID_RESET,
    ID_DITHERTYPE,
    ID_DITHERSCALE,
    ID_RAONLY,
    ID_DITHER,
};

wxBEGIN_EVENT_TABLE(TestGuideDialog, wxDialog)
    EVT_BUTTON(MGUIDE1_UP,TestGuideDialog::OnButton)
    EVT_BUTTON(MGUIDE1_DOWN,TestGuideDialog::OnButton)
    EVT_BUTTON(MGUIDE1_RIGHT,TestGuideDialog::OnButton)
    EVT_BUTTON(MGUIDE1_LEFT,TestGuideDialog::OnButton)
    EVT_BUTTON(MGUIDE2_UP,TestGuideDialog::OnButton)
    EVT_BUTTON(MGUIDE2_DOWN,TestGuideDialog::OnButton)
    EVT_BUTTON(MGUIDE2_RIGHT,TestGuideDialog::OnButton)
    EVT_BUTTON(MGUIDE2_LEFT,TestGuideDialog::OnButton)
    EVT_BUTTON(ID_RESET, TestGuideDialog::OnReset)
    EVT_CLOSE(TestGuideDialog::OnClose)
    EVT_COMMAND(wxID_ANY, APPSTATE_NOTIFY_EVENT, TestGuideDialog::OnAppStateNotify)
    EVT_SPINCTRLDOUBLE(ID_DITHERSCALE, TestGuideDialog::OnDitherScaleChange)
    EVT_CHECKBOX(ID_RAONLY, TestGuideDialog::OnRAOnlyChecked)
    EVT_BUTTON(ID_DITHER, TestGuideDialog::OnDither)
wxEND_EVENT_TABLE()

wxSizer *TestGuideDialog::InitMountControls()
{
    wxSizer *sz1 = new wxBoxSizer(wxHORIZONTAL);

    sz1->Add(new wxStaticText(this, wxID_ANY, _("Guide Pulse Duration (ms):")),
        wxSizerFlags().Right().Border(wxRIGHT, 5).Align(wxALIGN_CENTER_VERTICAL));
    pulseDurationSpinCtrl = pFrame->MakeSpinCtrlDouble(this, ID_PULSEDURATION, wxEmptyString, wxDefaultPosition,
        wxSize(StringWidth(GetParent(), "00000"), -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 100.0, 5000.0, 100.0, 100.0);
    pulseDurationSpinCtrl->SetDigits(0);
    pulseDurationSpinCtrl->SetToolTip(_("Manual guide pulse duration (milliseconds)"));
    Mount *mnt = TheScope();
    int val = pConfig->Profile.GetInt("/ManualGuide/duration", mnt->CalibrationMoveSize());
    pulseDurationSpinCtrl->SetValue((double) val);
    sz1->Add(pulseDurationSpinCtrl, wxSizerFlags().Left().Border(wxRIGHT, 10).Align(wxALIGN_CENTER_VERTICAL));

    wxButton *btn = new wxButton(this, ID_RESET, _("Reset"));
    sz1->Add(btn, wxSizerFlags().Left().Border(wxRIGHT, 10).Align(wxALIGN_CENTER_VERTICAL));
    btn->SetToolTip(_("Reset the manual guide pulse duration to the default value. The default value is the calibration step size."));

    wxSizer *sz2 = new wxBoxSizer(wxHORIZONTAL);

    sz2->Add(new wxStaticText(this, wxID_ANY, _("Dither")), wxSizerFlags().Right().Border(wxRIGHT, 5).Align(wxALIGN_CENTER_VERTICAL));
    wxArrayString choices;
    choices.Add(_("MOVE1 (+/- 0.5)"));
    choices.Add(_("MOVE2 (+/- 1.0)"));
    choices.Add(_("MOVE3 (+/- 2.0)"));
    choices.Add(_("MOVE4 (+/- 3.0)"));
    choices.Add(_("MOVE5 (+/- 5.0)"));
    ditherTypeChoice = new wxChoice(this, ID_DITHERTYPE, wxDefaultPosition, wxDefaultSize, choices);
    ditherTypeChoice->Select(pConfig->Profile.GetInt("/ManualGuide/DitherType", 4) - 1);
    ditherTypeChoice->SetToolTip(_("Select the dither amount type. Imaging applications have the option of sending each of these dither amounts to PHD."));
    sz2->Add(ditherTypeChoice, wxSizerFlags().Left().Border(wxRIGHT, 10).Align(wxALIGN_CENTER_VERTICAL));

    sz2->Add(new wxStaticText(this, wxID_ANY, _("Scale")), wxSizerFlags().Right().Border(wxRIGHT, 5).Align(wxALIGN_CENTER_VERTICAL));
    ditherScaleSpinCtrl = pFrame->MakeSpinCtrlDouble(this, ID_DITHERSCALE, wxEmptyString, wxDefaultPosition,
        wxSize(StringWidth(GetParent(), "000.0"), -1), wxSP_ARROW_KEYS | wxALIGN_RIGHT, 0.1, 100.0, 1.0, 1.0);
    ditherScaleSpinCtrl->SetDigits(1);
    ditherScaleSpinCtrl->SetValue(pFrame->GetDitherScaleFactor());
    ditherScaleSpinCtrl->SetToolTip(_("Scale factor for dithering. The dither amount type is multiplied by this value to get the actual dither amount. Changing the value here affects both manual dithering and dithering from imaging applications connected to PHD."));
    sz2->Add(ditherScaleSpinCtrl, wxSizerFlags().Left().Border(wxRIGHT, 10).Align(wxALIGN_CENTER_VERTICAL));

    raOnlyCheckBox = new wxCheckBox(this, ID_RAONLY, _("RA Only"));
    sz2->Add(raOnlyCheckBox, wxSizerFlags().Left().Border(wxRIGHT, 10).Align(wxALIGN_CENTER_VERTICAL));
    raOnlyCheckBox->SetValue(pFrame->GetDitherRaOnly());
    raOnlyCheckBox->SetToolTip(_("Dither on RA axis only. Changing the value here affects both manual dithering and dithering from imaging applications connected to PHD."));

    btn = new wxButton(this, ID_DITHER, _("Dither"));
    btn->SetToolTip(_("Move the guider lock position a random amount on each axis, up to the maximum value determined by the dither type and the dither scale factor."));
    sz2->Add(btn, wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

    wxSizer *sz3 = new wxBoxSizer(wxVERTICAL);
    sz3->Add(sz1, wxSizerFlags().Border(wxALL, 10).Align(wxALIGN_CENTER_HORIZONTAL));
    sz3->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxSize(1, -1)),
        wxSizerFlags().Border(wxALL, 3).Align(wxALIGN_CENTER_VERTICAL).Expand());
    sz3->Add(sz2, wxSizerFlags().Border(wxALL, 10));

    return sz3;
}

TestGuideDialog::TestGuideDialog() :
    wxDialog(pFrame, wxID_ANY, _("Manual Guide"), wxPoint(-1,-1), wxSize(300,300))
{
    wxBoxSizer *pOuterSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *pWrapperSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Mount"));
    wxGridSizer *sizer = new wxGridSizer(3,3,0,0);
    static wxString AoLabels[] = {_("Up"), _("Down"), _("Right"), _("Left") };
    static wxString ScopeLabels[] = {_("North"), _("South"), _("East"), _("West") };

    bool usingAo = pSecondaryMount && pSecondaryMount->IsConnected();

    wxString *pLabels;

    if (usingAo)
    {
        pLabels = AoLabels;
        pWrapperSizer->GetStaticBox()->SetLabel(_("AO"));
    }
    else
    {
        pLabels = ScopeLabels;
    }

    // Build the buttons for the primary mount

    NButton1 = new wxButton(this, MGUIDE1_UP, pLabels[0], wxPoint(-1,-1),wxSize(-1,-1));
    SButton1 = new wxButton(this, MGUIDE1_DOWN, pLabels[1], wxPoint(-1,-1),wxSize(-1,-1));
    EButton1 = new wxButton(this, MGUIDE1_RIGHT, pLabels[2], wxPoint(-1,-1),wxSize(-1,-1));
    WButton1 = new wxButton(this, MGUIDE1_LEFT, pLabels[3], wxPoint(-1,-1),wxSize(-1,-1));

    sizer->AddStretchSpacer();
    sizer->Add(NButton1,wxSizerFlags().Expand().Border(wxALL,6));
    sizer->AddStretchSpacer();
    sizer->Add(WButton1,wxSizerFlags().Expand().Border(wxALL,6));
    sizer->AddStretchSpacer();
    sizer->Add(EButton1,wxSizerFlags().Expand().Border(wxALL,6));
    sizer->AddStretchSpacer();
    sizer->Add(SButton1,wxSizerFlags().Expand().Border(wxALL,6));

    pWrapperSizer->Add(sizer, wxSizerFlags().Center());
    if (!usingAo)
    {
        pWrapperSizer->Add(InitMountControls());
    }
    pOuterSizer->Add(pWrapperSizer,wxSizerFlags().Border(wxALL,3).Center().Expand());

    if (usingAo)
    {
        pWrapperSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Mount"));
        sizer = new wxGridSizer(3,3,0,0);

        pLabels = ScopeLabels;

        NButton2 = new wxButton(this, MGUIDE2_UP, pLabels[0], wxPoint(-1,-1),wxSize(-1,-1));
        SButton2 = new wxButton(this, MGUIDE2_DOWN, pLabels[1], wxPoint(-1,-1),wxSize(-1,-1));
        EButton2 = new wxButton(this, MGUIDE2_RIGHT, pLabels[2], wxPoint(-1,-1),wxSize(-1,-1));
        WButton2 = new wxButton(this, MGUIDE2_LEFT, pLabels[3], wxPoint(-1,-1),wxSize(-1,-1));

        sizer->AddStretchSpacer();
        sizer->Add(NButton2,wxSizerFlags().Expand().Border(wxALL,6));
        sizer->AddStretchSpacer();
        sizer->Add(WButton2,wxSizerFlags().Expand().Border(wxALL,6));
        sizer->AddStretchSpacer();
        sizer->Add(EButton2,wxSizerFlags().Expand().Border(wxALL,6));
        sizer->AddStretchSpacer();
        sizer->Add(SButton2,wxSizerFlags().Expand().Border(wxALL,6));

        pWrapperSizer->Add(sizer, wxSizerFlags().Center());
        pWrapperSizer->Add(InitMountControls());
        pOuterSizer->Add(pWrapperSizer,wxSizerFlags().Border(wxALL,3).Center().Expand());
    }

    pOuterSizer->AddSpacer(30);
    pOuterSizer->SetSizeHints(this);
    SetSizerAndFit(pOuterSizer);
}

TestGuideDialog::~TestGuideDialog(void)
{
    pFrame->pManualGuide = 0;
}

void TestGuideDialog::OnClose(wxCloseEvent& evt)
{
    int val = (int) floor(pulseDurationSpinCtrl->GetValue());
    pConfig->Profile.SetInt("/ManualGuide/duration", val);
    pConfig->Profile.SetInt("/ManualGuide/DitherType", ditherTypeChoice->GetSelection() + 1);
    Destroy();
}

void TestGuideDialog::OnReset(wxCommandEvent& evt)
{
    Mount *scope = TheScope();
    pulseDurationSpinCtrl->SetValue((double) scope->CalibrationMoveSize());
}

void TestGuideDialog::OnAppStateNotify(wxCommandEvent& evt)
{
    ditherScaleSpinCtrl->SetValue(pFrame->GetDitherScaleFactor());
    raOnlyCheckBox->SetValue(pFrame->GetDitherRaOnly());
}

void TestGuideDialog::OnDitherScaleChange(wxSpinDoubleEvent& evt)
{
    pFrame->SetDitherScaleFactor(ditherScaleSpinCtrl->GetValue());
}

void TestGuideDialog::OnRAOnlyChecked(wxCommandEvent& evt)
{
    pFrame->SetDitherRaOnly(raOnlyCheckBox->GetValue());
}

void TestGuideDialog::OnDither(wxCommandEvent& evt)
{
    int ditherType = ditherTypeChoice->GetSelection() + 1;
    wxString errMsg;
    PhdController::DitherCompat(MyFrame::GetDitherAmount(ditherType), &errMsg);
}

inline static void ManualMove(Mount *mount, GUIDE_DIRECTION dir, int dur)
{
    if (mount && mount->IsConnected())
    {
        if (mount->IsStepGuider())
        {
            dur = 1;
            Debug.Write(wxString::Format("Manual Guide: %s %d step(s)\n", mount->DirectionStr(dir), dur));
        }
        else
            Debug.Write(wxString::Format("Manual Guide: %s %d ms\n", mount->DirectionStr(dir), dur));

        pFrame->ScheduleManualMove(mount, dir, dur);
    }
}

void TestGuideDialog::OnButton(wxCommandEvent &evt)
{
    int duration = (int) floor(pulseDurationSpinCtrl->GetValue());

    switch (evt.GetId())
    {
        case MGUIDE1_UP:
            ManualMove(pMount, UP, duration);
            break;
        case MGUIDE1_DOWN:
            ManualMove(pMount, DOWN, duration);
            break;
        case MGUIDE1_RIGHT:
            ManualMove(pMount, RIGHT, duration);
            break;
        case MGUIDE1_LEFT:
            ManualMove(pMount, LEFT, duration);
            break;
        case MGUIDE2_UP:
            ManualMove(pSecondaryMount, UP, duration);
            break;
        case MGUIDE2_DOWN:
            ManualMove(pSecondaryMount, DOWN, duration);
            break;
        case MGUIDE2_RIGHT:
            ManualMove(pSecondaryMount, RIGHT, duration);
            break;
        case MGUIDE2_LEFT:
            ManualMove(pSecondaryMount, LEFT, duration);
            break;
    }
}

wxWindow *TestGuide::CreateManualGuideWindow()
{
    return new TestGuideDialog();
}

void TestGuide::ManualGuideUpdateControls()
{
    // notify the manual guide dialog to update its controls
    if (pFrame->pManualGuide)
    {
        wxCommandEvent event(APPSTATE_NOTIFY_EVENT, pFrame->GetId());
        event.SetEventObject(pFrame);
        wxPostEvent(pFrame->pManualGuide, event);
    }
}
