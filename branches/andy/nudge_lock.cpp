/*
 *  nudge_lock.cpp
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
#include "nudge_lock.h"
#include "comet_tool.h"

#include <wx/valnum.h>

struct NudgeLockDialog : public wxDialog
{
    wxToggleButton *stayOnTop;
    wxButton *upButton, *downButton, *leftButton, *rightButton;
    wxSlider *nudgeAmountSlider;
    wxCheckBox *stickyLockPos;
    wxStaticText *nudgeAmountText;
    wxTextCtrl *lockPosCtrlX, *lockPosCtrlY;
    wxButton *updateLockPosButton;
    wxButton *saveLockPosButton;
    wxButton *restoreLockPosButton;

    bool lockPosIsValid;
    double lockPosX, lockPosY;

    NudgeLockDialog();
    ~NudgeLockDialog();

    void OnStayOnTopToggled(wxCommandEvent& event);
    void OnButton(wxCommandEvent& evt);
    void OnNudgeAmountSlider(wxCommandEvent& evt);
    void OnStickyChecked(wxCommandEvent& evt);
    void OnSetLockPosClicked(wxCommandEvent& evt);
    void OnSaveLockPosClicked(wxCommandEvent& evt);
    void OnRestoreLockPosClicked(wxCommandEvent& evt);
    void OnAppStateNotify(wxCommandEvent& evt);
    void OnClose(wxCloseEvent& evt);

    void UpdateSliderLabel();
    void UpdateLockPosCtrls();

    DECLARE_EVENT_TABLE()
};

enum
{
    ID_UP_BTN = 1001,
    ID_DOWN_BTN,
    ID_LEFT_BTN,
    ID_RIGHT_BTN,
    ID_STAY_ON_TOP,
    ID_NUDGE_AMOUNT,
    ID_STICKY,
    ID_SET_LOCK_POS,
    ID_SAVE_LOCK_POS,
    ID_RESTORE_LOCK_POS,
};

wxBEGIN_EVENT_TABLE(NudgeLockDialog, wxDialog)
    EVT_TOGGLEBUTTON(ID_STAY_ON_TOP, NudgeLockDialog::OnStayOnTopToggled)
    EVT_BUTTON(ID_UP_BTN, NudgeLockDialog::OnButton)
    EVT_BUTTON(ID_DOWN_BTN, NudgeLockDialog::OnButton)
    EVT_BUTTON(ID_LEFT_BTN, NudgeLockDialog::OnButton)
    EVT_BUTTON(ID_RIGHT_BTN, NudgeLockDialog::OnButton)
    EVT_SLIDER(ID_NUDGE_AMOUNT, NudgeLockDialog::OnNudgeAmountSlider)
    EVT_CHECKBOX(ID_STICKY, NudgeLockDialog::OnStickyChecked)
    EVT_BUTTON(ID_SET_LOCK_POS, NudgeLockDialog::OnSetLockPosClicked)
    EVT_BUTTON(ID_SAVE_LOCK_POS, NudgeLockDialog::OnSaveLockPosClicked)
    EVT_BUTTON(ID_RESTORE_LOCK_POS, NudgeLockDialog::OnRestoreLockPosClicked)
    EVT_COMMAND(wxID_ANY, APPSTATE_NOTIFY_EVENT, NudgeLockDialog::OnAppStateNotify)
    EVT_CLOSE(NudgeLockDialog::OnClose)
wxEND_EVENT_TABLE()

static double NudgeIncrements[] = { 0.01, 0.03, 0.1, 0.3, 1, 3, 10, };

static int IncrIdx(double incr)
{
    for (unsigned int i = 0; i < WXSIZEOF(NudgeIncrements); i++)
    {
        if (fabs(incr - NudgeIncrements[i]) < 0.0001)
            return i;
    }
    return 0;
}

NudgeLockDialog::NudgeLockDialog()
    : wxDialog(pFrame, wxID_ANY, _("Adjust Lock Position"), wxPoint(-1,-1), wxSize(300,300))
{
    stayOnTop = new wxToggleButton(this, ID_STAY_ON_TOP, wxEmptyString, wxDefaultPosition, wxSize(18, 18));
    stayOnTop->SetToolTip(_("Always on top"));

    upButton = new wxButton(this, ID_UP_BTN, _("Up"));
    downButton = new wxButton(this, ID_DOWN_BTN, _("Down"));
    leftButton = new wxButton(this, ID_LEFT_BTN, _("Left"));
    rightButton = new wxButton(this, ID_RIGHT_BTN, _("Right"));

    wxGridSizer *sz1 = new wxGridSizer(3, 3, 0, 0);

    sz1->AddStretchSpacer();
    sz1->Add(upButton, wxSizerFlags().Expand().Border(wxALL,1));
    sz1->AddStretchSpacer();

    sz1->Add(leftButton, wxSizerFlags().Expand().Border(wxALL,1));
    sz1->AddStretchSpacer();
    sz1->Add(rightButton, wxSizerFlags().Expand().Border(wxALL,1));

    sz1->AddStretchSpacer();
    sz1->Add(downButton, wxSizerFlags().Expand().Border(wxALL,1));

    wxBoxSizer *sz0 = new wxBoxSizer(wxHORIZONTAL);
    sz0->AddStretchSpacer();
    sz0->Add(sz1);
    sz0->AddStretchSpacer();
    sz0->Add(stayOnTop, wxSizerFlags().Right());

    wxSizer *sz2 = new wxBoxSizer(wxHORIZONTAL);

    sz2->Add(new wxStaticText(this, wxID_ANY, _("Step")), wxSizerFlags().Right().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

    double incr = pConfig->Global.GetDouble("/NudgeLock/Amount", NudgeIncrements[2]);
    int idx = IncrIdx(incr);
    nudgeAmountSlider = new wxSlider(this, ID_NUDGE_AMOUNT, idx, 0, WXSIZEOF(NudgeIncrements) - 1, wxDefaultPosition, wxSize(100, -1));
    nudgeAmountSlider->SetToolTip(_("Adjust how far the lock position moves when you click the Up/Down/Left/Right buttons"));
    sz2->Add(nudgeAmountSlider, wxSizerFlags().Expand().Border(wxALL, 0));

    nudgeAmountText = new wxStaticText(this, wxID_ANY, _T("1.234"));
    sz2->Add(nudgeAmountText, wxSizerFlags().Border(wxLEFT, 5).Align(wxALIGN_CENTER_VERTICAL));

    stickyLockPos = new wxCheckBox(this, ID_STICKY, _("Sticky Lock Position"));
    stickyLockPos->SetToolTip(_("Sticky lock position will not follow the star when guiding is stopped and restarted, or after calibration completes"));
    sz2->Add(0, 0, 1, wxEXPAND, 5);
    sz2->Add(stickyLockPos, wxSizerFlags().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

    wxSizer *sz3 = new wxBoxSizer(wxHORIZONTAL);
    sz3->Add(new wxStaticText(this, wxID_ANY, _("Lock Pos:")), wxSizerFlags().Right().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

    wxFloatingPointValidator<double> valX(2, &lockPosX, wxNUM_VAL_ZERO_AS_BLANK);
    valX.SetMin(0.0);
    lockPosCtrlX = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, StringSize(this, _T("12345.67"), 10), 0, valX);
    lockPosCtrlX->SetToolTip(_("Lock position X coordinate"));
    sz3->Add(lockPosCtrlX, wxSizerFlags().Border(wxALL, 0).Align(wxALIGN_CENTER_VERTICAL));

    wxFloatingPointValidator<double> valY(2, &lockPosY, wxNUM_VAL_ZERO_AS_BLANK);
    valY.SetMin(0.0);
    lockPosCtrlY = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, StringSize(this, _T("12345.67"), 10), 0, valY);
    lockPosCtrlY->SetToolTip(_("Lock position Y coordinate"));
    sz3->Add(lockPosCtrlY, wxSizerFlags().Border(wxLEFT, 5).Align(wxALIGN_CENTER_VERTICAL));

    wxSize s1 = StringSize(this, _("Set"), 10);
    wxSize s2 = StringSize(this, _("Save"), 10);
    wxSize s3 = StringSize(this, _("Restore"), 10);
    size_t longest = wxMax(s1.GetX(), s2.GetX());
    longest = wxMax(longest, s3.GetX());

    wxSize btnsize(longest, -1);
    updateLockPosButton = new wxButton(this, ID_SET_LOCK_POS, _("Set"), wxDefaultPosition, btnsize);
    updateLockPosButton->SetToolTip(_("Set the lock position to the entered coordinates"));
    sz3->Add(updateLockPosButton, wxSizerFlags().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
    saveLockPosButton = new wxButton(this, ID_SAVE_LOCK_POS, _("Save"), wxDefaultPosition, btnsize);
    saveLockPosButton->SetToolTip(_("Save the current lock position so it can be restored later"));
    sz3->Add(saveLockPosButton, wxSizerFlags().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
    restoreLockPosButton = new wxButton(this, ID_RESTORE_LOCK_POS, _("Restore"), wxDefaultPosition, btnsize);
    restoreLockPosButton->SetToolTip(_("Restore the saved lock position"));
    sz3->Add(restoreLockPosButton, wxSizerFlags().Border(wxRIGHT, 5).Align(wxALIGN_CENTER_VERTICAL));

    wxBoxSizer *outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(sz0, wxSizerFlags().Border(wxALL,3).Expand());
    outerSizer->Add(sz2, wxSizerFlags().Border(wxALL,3).Expand());
    outerSizer->Add(sz3, wxSizerFlags().Border(wxALL,3));

    UpdateSliderLabel();
    UpdateLockPosCtrls();

    outerSizer->SetSizeHints(this);
    SetSizerAndFit(outerSizer);

    int xpos = pConfig->Global.GetInt("/NudgeLock/pos.x", -1);
    int ypos = pConfig->Global.GetInt("/NudgeLock/pos.y", -1);
    MyFrame::PlaceWindowOnScreen(this, xpos, ypos);
}

NudgeLockDialog::~NudgeLockDialog(void)
{
    pFrame->pNudgeLock = 0;
}

void NudgeLockDialog::UpdateSliderLabel()
{
    int idx = nudgeAmountSlider->GetValue();
    double val = NudgeIncrements[idx];
    nudgeAmountText->SetLabel(wxString::Format("%.2f px", val));
}

void NudgeLockDialog::UpdateLockPosCtrls()
{
    const PHD_Point& pos = pFrame->pGuider->LockPosition();
    lockPosIsValid = pos.IsValid();
    if (lockPosIsValid)
    {
        lockPosX = pos.X;
        lockPosY = pos.Y;
    }
    else
    {
        lockPosX = 0.0;
        lockPosY = 0.0;
    }
    TransferDataToWindow();
    lockPosCtrlX->Enable(lockPosIsValid);
    lockPosCtrlY->Enable(lockPosIsValid);
    updateLockPosButton->Enable(lockPosIsValid);
    saveLockPosButton->Enable(lockPosIsValid);
    restoreLockPosButton->Enable(lockPosIsValid);
    stickyLockPos->SetValue(pFrame->pGuider->LockPosIsSticky());
}

static bool UpdateLockPos(const PHD_Point& newPos)
{
    if (pFrame->pGuider->IsValidLockPosition(newPos))
    {
        pFrame->pGuider->SetLockPosition(newPos);
        pFrame->Refresh();
        CometTool::NotifyUpdateLockPos();
        return true;
    }
    return false;
}

static void DoMove(double dx, double dy)
{
    PHD_Point newPos = pFrame->pGuider->LockPosition();
    newPos.X += dx;
    newPos.Y += dy;
    UpdateLockPos(newPos);
}

void NudgeLockDialog::OnStayOnTopToggled(wxCommandEvent& event)
{
    long style = GetWindowStyle();
    if (event.IsChecked())
        SetWindowStyle(style | wxSTAY_ON_TOP);
    else
        SetWindowStyle(style & ~wxSTAY_ON_TOP);
}

void NudgeLockDialog::OnButton(wxCommandEvent &evt)
{
    if (!lockPosIsValid)
        return;

    int idx = nudgeAmountSlider->GetValue();
    double incr = NudgeIncrements[idx];

    switch (evt.GetId())
    {
        case ID_UP_BTN:
            DoMove(0.0, -incr);
            break;
        case ID_DOWN_BTN:
            DoMove(0.0, +incr);
            break;
        case ID_RIGHT_BTN:
            DoMove(+incr, 0.0);
            break;
        case ID_LEFT_BTN:
            DoMove(-incr, 0.0);
            break;
    }
}

void NudgeLockDialog::OnNudgeAmountSlider(wxCommandEvent& evt)
{
    UpdateSliderLabel();

    int idx = nudgeAmountSlider->GetValue();
    double val = NudgeIncrements[idx];
    pConfig->Global.SetDouble("/NudgeLock/Amount", val);
}

void NudgeLockDialog::OnStickyChecked(wxCommandEvent& evt)
{
    bool sticky = evt.IsChecked();
    pFrame->pGuider->SetLockPosIsSticky(sticky);
    pConfig->Global.SetBoolean("/StickyLockPosition", sticky);
    pFrame->tools_menu->FindItem(EEGG_STICKY_LOCK)->Check(sticky);
}

void NudgeLockDialog::OnSetLockPosClicked(wxCommandEvent& evt)
{
    TransferDataFromWindow();
    PHD_Point newPos(lockPosX, lockPosY);
    if (!UpdateLockPos(newPos))
    {
        UpdateLockPosCtrls();
    }
}

void NudgeLockDialog::OnSaveLockPosClicked(wxCommandEvent& evt)
{
    const PHD_Point& pos = pFrame->pGuider->LockPosition();
    if (pos.IsValid())
    {
        pConfig->Profile.SetDouble("/NudgeLock/SavedLockPosX", pos.X);
        pConfig->Profile.SetDouble("/NudgeLock/SavedLockPosY", pos.Y);
    }
}

void NudgeLockDialog::OnRestoreLockPosClicked(wxCommandEvent& evt)
{
    double x = pConfig->Profile.GetDouble("/NudgeLock/SavedLockPosX", -1.0);
    double y = pConfig->Profile.GetDouble("/NudgeLock/SavedLockPosY", -1.0);
    if (x < 0.0 || y < 0.0)
        return;

    if (ConfirmDialog::Confirm(wxString::Format(_("Set lock position to saved value (%.2f,%.2f)?"), x, y),
        "/RestoreSavedLockPosOK", _("Restore saved Lock Pos")))
    {
        UpdateLockPos(PHD_Point(x, y));
    }
}

void NudgeLockDialog::OnAppStateNotify(wxCommandEvent& evt)
{
    UpdateLockPosCtrls();
}

void NudgeLockDialog::OnClose(wxCloseEvent& evt)
{
    // save the window position
    int x, y;
    GetPosition(&x, &y);
    pConfig->Global.SetInt("/NudgeLock/pos.x", x);
    pConfig->Global.SetInt("/NudgeLock/pos.y", y);

    evt.Skip();
}

wxWindow *NudgeLockTool::CreateNudgeLockToolWindow()
{
    return new NudgeLockDialog();
}

void NudgeLockTool::UpdateNudgeLockControls()
{
    // notify nudge lock tool to update its controls
    if (pFrame && pFrame->pNudgeLock)
    {
        wxCommandEvent event(APPSTATE_NOTIFY_EVENT, pFrame->GetId());
        event.SetEventObject(pFrame);
        wxPostEvent(pFrame->pNudgeLock, event);
    }
}
