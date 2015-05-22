/*
 *  confirm_dialog.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso.
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

ConfirmDialog::ConfirmDialog(const wxString& prompt, const wxString& title, const wxString& affirmLabel, const wxString& negativeLabel)
    : wxDialog(pFrame, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    dont_ask_again = new wxCheckBox(this, wxID_ANY, _("Don't ask again"));
    wxStaticText *txt = new wxStaticText(this, wxID_ANY, prompt);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(txt, wxSizerFlags(0).Border(wxALL, 10));
    sizer->Add(dont_ask_again, wxSizerFlags(0).Border(wxALL, 10));

    wxBoxSizer *topLevelSizer = new wxBoxSizer(wxVERTICAL);
    topLevelSizer->Add(sizer, wxSizerFlags(0).Expand());

    // Let CreateButtonSizer create platform-neutral OK/Cancel buttons
    // with correct yes/no and EndModal event behavior - then relabel
    // the buttons if client wants something other than OK and Cancel

    topLevelSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags(0).Expand().Border(wxALL, 10));
    if (!affirmLabel.IsEmpty())
        FindWindow(wxID_OK)->SetLabel(affirmLabel);
    if (!negativeLabel.IsEmpty())
        FindWindow(wxID_CANCEL)->SetLabel(negativeLabel);

    SetSizerAndFit(topLevelSizer);
}

ConfirmDialog::~ConfirmDialog(void)
{
}

static wxString ConfigKey(const wxString& name)
{
    return "/Confirm" + name;
}

bool ConfirmDialog::Confirm(const wxString& prompt, const wxString& config_key, const wxString& affirmLabel, const wxString& negativeLabel, const wxString& title_arg)
{
    wxString key(ConfigKey(config_key));

    bool skip_confirm = pConfig->Global.GetBoolean(key, false);
    if (skip_confirm)
        return true;

    wxString title(title_arg);
    if (title.IsEmpty())
        title = _("Confirm");

    ConfirmDialog dlg(prompt, title, affirmLabel, negativeLabel);
    if (dlg.ShowModal() == wxID_OK)
    {
        if (dlg.dont_ask_again->IsChecked())
            pConfig->Global.SetBoolean(key, true);
        return true;
    }

    return false;
}

bool ConfirmDialog::Confirm(const wxString& prompt, const wxString& config_key, const wxString& title_arg)
{
    return Confirm(prompt, config_key, wxEmptyString, wxEmptyString, title_arg);
}

void ConfirmDialog::ResetAllDontAskAgain(void)
{
    pConfig->Global.DeleteGroup("/Confirm");
}
