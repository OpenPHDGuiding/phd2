/*
 *  config_VIDEODEVICE.cpp
 *  PHD Guiding
 *
 *  Created by Steffen Elste
 *  Copyright (c) 2010 Steffen Elste.
 *  All rights reserved.
 *
 *  This source code is distributed under the following "BSD" license
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
#include "cam_VIDEODEVICE.h"
#include "config_VIDEODEVICE.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <libv4l2.h>
#include <libv4lconvert.h>

#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/config.h>


#define PHDGUIDING "PHDGuiding"


DECLARE_EVENT_TYPE(wxEVT_V4L_UPDATE, wxID_ANY)
DECLARE_EVENT_TYPE(wxEVT_V4L_RESET, wxID_ANY)

DEFINE_EVENT_TYPE(wxEVT_V4L_UPDATE)
DEFINE_EVENT_TYPE(wxEVT_V4L_RESET)


V4LPropertiesDialog::V4LPropertiesDialog(V4LControlMap &map)
	: wxDialog(frame, wxID_ANY, _T("Device Properties"), wxPoint(-1,-1), wxSize(500,300)),
	  controlMap(map) {

	wxStaticText *text;
	BooleanControl *booleanControl;
	IntegerControl *integerControl;
	MenueControl *menueControl;

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2);

	V4LControlMap::iterator it;
	for (it=controlMap.begin(); it!=controlMap.end(); ++it) {
		int id = it->first;
		V4LControl *control = (V4LControl*)it->second;

		text = new wxStaticText(this, wxID_ANY, control->name);
		gridSizer->Add(text, wxSizerFlags().Expand().Proportion(2).Border(wxALL, 3));

		switch (control->type) {
			case V4L2_CTRL_TYPE_BOOLEAN:
				booleanControl = new BooleanControl(this, id, _T(""));
				booleanControl->SetValue(0 != control->value);

				gridSizer->Add(booleanControl, wxSizerFlags().Proportion(1).Border(wxALL, 3));
				Connect(id, wxEVT_V4L_UPDATE, wxCommandEventHandler(BooleanControl::onUpdate), NULL, booleanControl);
				Connect(id, wxEVT_V4L_RESET, wxCommandEventHandler(BooleanControl::onReset), NULL, booleanControl);
				break;
			case V4L2_CTRL_TYPE_INTEGER:
				integerControl = new IntegerControl(this, id);
				integerControl->SetSize(wxSize(75,-1));
				integerControl->SetRange(control->min, control->max);
				integerControl->SetValue(control->value);

				gridSizer->Add(integerControl, wxSizerFlags().Proportion(1).Border(wxALL, 3));
				Connect(id, wxEVT_V4L_UPDATE, wxCommandEventHandler(IntegerControl::onUpdate), NULL, integerControl);
				Connect(id, wxEVT_V4L_RESET, wxCommandEventHandler(IntegerControl::onReset), NULL, integerControl);
				break;
			case V4L2_CTRL_TYPE_MENU:
				menueControl = new MenueControl(this, id, wxPoint(-1,-1), wxSize(75,-1), control->choices);

				gridSizer->Add(menueControl, wxSizerFlags().Proportion(1).Border(wxALL, 3));
				Connect(id, wxEVT_V4L_UPDATE, wxCommandEventHandler(MenueControl::onUpdate), NULL, menueControl);
				Connect(id, wxEVT_V4L_RESET, wxCommandEventHandler(MenueControl::onReset), NULL, menueControl);
				break;
			default:
				break;
		}
	}

	wxButton *applyButton = new wxButton(this, wxID_APPLY);
	wxButton *resetButton = new wxButton(this, wxID_RESET, _T("Reset"));

	Connect(wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(V4LPropertiesDialog::onUpdate));
	Connect(wxID_RESET, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(V4LPropertiesDialog::onReset));

	hbox->Add(resetButton, 1);
	hbox->Add(applyButton, 1, wxLEFT, 5);

	vbox->Add(gridSizer, 1);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	SetSizer(vbox);
	vbox->SetSizeHints(this);
}

void V4LPropertiesDialog::onUpdate(wxCommandEvent& WXUNUSED(event)) {
	V4LControlMap::iterator it;
	for (it=controlMap.begin(); it!=controlMap.end(); ++it) {
		int id = it->first;

		wxCommandEvent customEvent(wxEVT_V4L_UPDATE, id);
		GetEventHandler()->ProcessEvent(customEvent);
	}

}

void V4LPropertiesDialog::onReset(wxCommandEvent& WXUNUSED(event)) {
	V4LControlMap::iterator it;
	for (it=controlMap.begin(); it!=controlMap.end(); ++it) {
		int id = it->first;

		wxCommandEvent customEvent(wxEVT_V4L_RESET, id);
		GetEventHandler()->ProcessEvent(customEvent);
	}

}

void MyFrame::OnSaveSettings(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog *dialog = NULL;
	wxConfig *config = NULL;

	if (NULL != (config = new wxConfig(_T(PHDGUIDING)))) {
		if (true == Camera_VIDEODEVICE.saveSettings(config)) {
			dialog = new wxMessageDialog(frame, _T("Settings successfully saved"), _T("Save Settings"), wxOK | wxICON_INFORMATION);
		} else {
			dialog = new wxMessageDialog(frame, _T("Error saving settings"), _T("Save Settings"), wxOK | wxICON_ERROR);
		}
	} else {
		dialog = new wxMessageDialog(frame, _T("Error creating/opening settings"), _T("Save Settings"), wxOK | wxICON_ERROR);
	}

	dialog->ShowModal();

	if (NULL != dialog)
		dialog->Destroy();
}

void MyFrame::OnRestoreSettings(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog *dialog = NULL;
	wxConfig *config = NULL;

	if (NULL != (config = new wxConfig(_T(PHDGUIDING)))) {
		if (false == config->Exists(_T("camera")) || false == config->Exists(_T("vendorid")) || false == config->Exists(_T("modelid"))) {
			dialog = new wxMessageDialog(frame, _T("Cannot restore from incomplete settings"), _T("Restore Settings"), wxOK | wxICON_ERROR);
		} else {
			wxString cameraFromConfig, vendorFromConfig, modelFromConfig;

			config->Read(_T("camera"), &cameraFromConfig);
			config->Read(_T("vendorid"), &vendorFromConfig);
			config->Read(_T("modelid"), &modelFromConfig);

			// This is not completely fool-proof ...
			if (0 == Camera_VIDEODEVICE.Name.CmpNoCase(cameraFromConfig) && 0 == Camera_VIDEODEVICE.GetVendor().CmpNoCase(vendorFromConfig) && 0 == Camera_VIDEODEVICE.GetModel().CmpNoCase(modelFromConfig)) {
				if (true == Camera_VIDEODEVICE.restoreSettings(config)) {
					dialog = new wxMessageDialog(frame, _T("Settings successfully restored"), _T("Restore Settings"), wxOK | wxICON_INFORMATION);
				} else {
					dialog = new wxMessageDialog(frame, _T("Error restoring settings"), _T("Restore Settings"), wxOK | wxICON_INFORMATION);
				}
			} else {
				wxString message = _T("Cannot restore settings\n");

				message += _T("Device currently in use: ") + Camera_VIDEODEVICE.Name + _T(" - ") + Camera_VIDEODEVICE.GetVendor() + _T(":") + Camera_VIDEODEVICE.GetModel() + _T("\n");
				message += _T("Device from settings: ") + cameraFromConfig + _T(" - ") + vendorFromConfig + _T(":") + modelFromConfig;

				dialog = new wxMessageDialog(frame, message, _T("Restore Settings"), wxOK | wxICON_INFORMATION);
			}
		}
	}

	dialog->ShowModal();

	if (NULL != dialog)
		dialog->Destroy();
}


BooleanControl::BooleanControl(wxWindow* parent, wxWindowID id, const wxString& label)
	: wxCheckBox(parent, id, label) {

	// Empty
}

void BooleanControl::onUpdate(wxCommandEvent& WXUNUSED(event)) {
	V4LControl *control = (V4LControl*)Camera_VIDEODEVICE.getV4LControl(GetId());

	if (NULL != control) {
		control->value = GetValue();

		if (false == control->update()) {
			wxMessageDialog *dialog =
					new wxMessageDialog(NULL, _T("Could not update ") + control->name, _T("Warning"), wxOK | wxICON_EXCLAMATION);

			dialog->Show();
		}
	}
}

void BooleanControl::onReset(wxCommandEvent& WXUNUSED(event)) {
	V4LControl *control = (V4LControl*)Camera_VIDEODEVICE.getV4LControl(GetId());

	if (NULL != control) {
		if (false == control->reset()) {
			wxMessageDialog *dialog =
					new wxMessageDialog(NULL, _T("Could not reset ") + control->name, _T("Warning"), wxOK | wxICON_EXCLAMATION);

			dialog->Show();
		} else {
			SetValue(0 != control->defaultValue);
		}
	}
}


IntegerControl::IntegerControl(wxWindow* parent, wxWindowID id)
	: wxSpinCtrl(parent, id) {

	// Empty
}

void IntegerControl::onUpdate(wxCommandEvent& WXUNUSED(event)) {
	V4LControl *control = (V4LControl*)Camera_VIDEODEVICE.getV4LControl(GetId());

	if (NULL != control) {
		control->value = GetValue();

		if (false == control->update()) {
			wxMessageDialog *dialog =
					new wxMessageDialog(NULL, _T("Could not update ") + control->name, _T("Warning"), wxOK | wxICON_EXCLAMATION);

			dialog->Show();
		}
	}
}

void IntegerControl::onReset(wxCommandEvent& WXUNUSED(event)) {
	V4LControl *control = (V4LControl*)Camera_VIDEODEVICE.getV4LControl(GetId());

	if (NULL != control) {
		if (false == control->reset()) {
			wxMessageDialog *dialog =
					new wxMessageDialog(NULL, _T("Could not reset ") + control->name, _T("Warning"), wxOK | wxICON_EXCLAMATION);

			dialog->Show();
		} else {
			SetValue(control->defaultValue);
		}
	}
}

MenueControl::MenueControl(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, const wxArrayString& choices)
	: wxChoice(parent, id, pos, size, choices) {

	// Empty
}

void MenueControl::onUpdate(wxCommandEvent& WXUNUSED(event)) {
	V4LControl *control = (V4LControl*)Camera_VIDEODEVICE.getV4LControl(GetId());

	if (NULL != control) {
		// Entries for 'MENU'-type controls don't necessarily start at index '0'
		control->value = GetSelection() + control->min;

		if (false == control->update()) {
			wxMessageDialog *dialog =
					new wxMessageDialog(NULL, _T("Could not update ") + control->name, _T("Warning"), wxOK | wxICON_EXCLAMATION);

			dialog->Show();
		}
	}
}

void MenueControl::onReset(wxCommandEvent& WXUNUSED(event)) {
	V4LControl *control = (V4LControl*)Camera_VIDEODEVICE.getV4LControl(GetId());

	if (NULL != control) {
		if (false == control->reset()) {
			wxMessageDialog *dialog =
					new wxMessageDialog(NULL, _T("Could not reset ") + control->name, _T("Warning"), wxOK | wxICON_EXCLAMATION);

			dialog->Show();
		} else {
			SetSelection(control->defaultValue - control->min);
		}
	}
}
