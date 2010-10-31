/*
 *  config_INDI.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include "camera.h"
#include "scope.h"

#if defined (INDI_CAMERA) || defined (GUIDE_INDI)

#include "cam_INDI.h"
#include "tele_INDI.h"

#include "libindiclient/indi.h"
#include "libindiclient/indigui.h"

#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/combobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

struct indi_t *INDIClient = NULL;
long INDIport = 7624;
wxString INDIhost = _T("localhost");

#define POS(r, c)        wxGBPosition(r,c)
#define SPAN(r, c)       wxGBSpan(r,c)

class INDIConfig : public wxDialog
{
public:
    INDIConfig(wxWindow *parent);
    void SaveSettings();
private:
    wxComboBox *GetDevices(wxString str);
    wxTextCtrl *host;
    wxTextCtrl *port;
    wxComboBox *cam;
    wxTextCtrl *camport;
    wxComboBox *mount;
    wxTextCtrl *mountport;
};

wxComboBox *INDIConfig::GetDevices(wxString str)
{
    wxComboBox *combo;
    indi_list *isl;

    combo =  new wxComboBox(this, wxID_ANY, str);
    combo->Append(str);
    if (INDIClient) {
        for (isl = il_iter(INDIClient->devices); ! il_is_last(isl); isl = il_next(isl)) {
            struct indi_device_t *idev = (struct indi_device_t *)il_item(isl);
            wxString name = wxString::FromAscii(idev->name);
            if (name != str)
                combo->Append(wxString::FromAscii(idev->name));
        }
    }
    return combo;
}

INDIConfig::INDIConfig(wxWindow *parent) : wxDialog(parent, wxID_ANY, _T("INDI Configuration"))
{
    wxConfig *config = new wxConfig(_T("PHDGuiding"));
    wxGridBagSizer *gbs = new wxGridBagSizer(0, 20);
    wxBoxSizer *sizer;

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Hostname:")),
             POS(0, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    host = new wxTextCtrl(this, wxID_ANY, config->Read(_T("INDIhost"), _T("localhost")));
    gbs->Add(host, POS(0, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Port:")),
             POS(1, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    port = new wxTextCtrl(this, wxID_ANY, config->Read(_T("INDIport"), _T("7624")));
    gbs->Add(port, POS(1, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Camera Driver:")),
             POS(2, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    cam = GetDevices(config->Read(_T("INDIcam"), _T("")));
    gbs->Add(cam, POS(2, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Camera Port:")),
             POS(3, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    camport = new wxTextCtrl(this, wxID_ANY, config->Read(_T("INDIcam_port"), _T("")));
    gbs->Add(camport, POS(3, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Telescope Driver:")),
             POS(4, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    mount = GetDevices(config->Read(_T("INDImount"), _T("")));
    gbs->Add(mount, POS(4, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Telescope Port:")),
             POS(5, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    mountport = new wxTextCtrl(this, wxID_ANY, config->Read(_T("INDImount_port"), _T("")));
    gbs->Add(mountport, POS(5, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    sizer = new wxBoxSizer(wxVERTICAL) ;
    sizer->Add(gbs);
    sizer->Add(CreateButtonSizer(wxOK | wxCANCEL));
    SetSizer(sizer);
    sizer->SetSizeHints(this) ;
    sizer->Fit(this) ;
}

void INDIConfig::SaveSettings()
{
    wxConfig *config = new wxConfig(_T("PHDGuiding"));

    INDIhost = host->GetLineText(0);
    config->Write(_T("INDIhost"), INDIhost);

    port->GetLineText(0).ToLong(&INDIport);
    config->Write(_T("INDIport"), INDIport);

    Camera_INDI.indi_name = cam->GetValue();
    config->Write(_T("INDIcam"), Camera_INDI.indi_name);

    Camera_INDI.indi_port = camport->GetValue();
    config->Write(_T("INDIcami_port"), Camera_INDI.indi_port);

    INDIScope.indi_name = mount->GetValue();
    config->Write(_T("INDImount"), INDIScope.indi_name);

    INDIScope.serial_port = mountport->GetLineText(0);
    config->Write(_T("INDImount_port"), INDIScope.serial_port);
}

void MyFrame::OnINDIConfig(wxCommandEvent& WXUNUSED(event))
{
    INDIConfig *indiDlg = new INDIConfig(this);
    if (indiDlg->ShowModal() == wxID_OK) {
        indiDlg->SaveSettings();
    }
    indiDlg->Destroy();
}

void MyFrame::OnINDIDialog(wxCommandEvent& WXUNUSED(event))
{
    if (! INDIClient) {
        INDIClient = indi_init(INDIhost.ToAscii(), INDIport, "PHDGuiding");
        if (! INDIClient) {
            return;
        }
    }
    indigui_show_dialog(INDIClient);
}

#endif
