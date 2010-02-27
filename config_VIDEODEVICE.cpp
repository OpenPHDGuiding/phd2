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
#include <wx/arrstr.h>
#include <wx/string.h>
#include <wx/button.h>


V4LPropertiesDialog::V4LPropertiesDialog(V4LControlMap *controlMap)
	: wxDialog(frame, wxID_ANY, _T("Device Properties"), wxPoint(-1,-1), wxSize(500,300)) {

	wxStaticText *text;
	wxSpinCtrl *spinctrl;
	wxCheckBox *checkbox;
	wxChoice *choice;
	wxArrayString items;
	wxButton *button;

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2);

	this->controlMap = controlMap;

	V4LControlMap::iterator it;
	for (it=controlMap->begin(); it!=controlMap->end(); ++it) {
		int id = it->first;
		V4LControl *control = (V4LControl*)it->second;

		text = new wxStaticText(this, wxID_ANY, wxString(control->name, *wxConvCurrent));
		gridSizer->Add(text, wxSizerFlags().Expand().Proportion(2).Border(wxALL, 3));

		switch (control->type) {
			case V4L2_CTRL_TYPE_INTEGER:
				spinctrl = new wxSpinCtrl(this, id, _T("foo"), wxPoint(-1,-1), wxSize(75,-1), wxSP_ARROW_KEYS, control->min, control->max, control->defaultValue);
				spinctrlMap[id] = spinctrl;
				gridSizer->Add(spinctrl, wxSizerFlags().Proportion(1).Border(wxALL, 3));
				break;
			case V4L2_CTRL_TYPE_BOOLEAN:
				checkbox = new wxCheckBox(this, id, _T(""));
				checkboxMap[id] = checkbox;
				gridSizer->Add(checkbox, wxSizerFlags().Proportion(1).Border(wxALL, 3));
				break;
			case V4L2_CTRL_TYPE_MENU:
				items.Clear();
				for (int i=0; i<(control->max - control->min); i++) {
					items.Add(wxString(control->menu+MAXSIZE*i, *wxConvCurrent));
				}

				choice = new wxChoice(this, id, wxPoint(-1,-1), wxSize(75,-1), items);

				// FIXME
				choice->Enable(false);
				choiceMap[id] = choice;
				gridSizer->Add(choice, wxSizerFlags().Proportion(1).Border(wxALL, 3));
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

void V4LPropertiesDialog::onUpdate(wxCommandEvent& event) {
	V4LControlMap::iterator it;
	V4LControl *control;

	wxCheckBox *checkbox;
	wxSpinCtrl *spinctrl;

	CheckboxMap::iterator checkboxIt;
	SpinctrlMap::iterator spinctrlIt;

	for (it=controlMap->begin(); it!=controlMap->end(); ++it) {
		int id = it->first;
		V4LControl *control = (V4LControl*)it->second;

		switch (control->type) {
			case V4L2_CTRL_TYPE_INTEGER:
				if (spinctrlMap.end() != (spinctrlIt = spinctrlMap.find(id))) {
					spinctrl = spinctrlIt->second;

					control->value = spinctrl->GetValue();
				}
				break;
			case V4L2_CTRL_TYPE_BOOLEAN:
				if (checkboxMap.end() != (checkboxIt = checkboxMap.find(id))) {
					checkbox = checkboxIt->second;

					control->value = checkbox->GetValue();
				}
				break;
			default:
				return;
		}

		if (false == control->update()) {
			wxMessageDialog *dialog =
					new wxMessageDialog(NULL, wxString::Format(_T("Could not update '%s'!"), control->name), _T("Warning"), wxOK | wxICON_EXCLAMATION);

			dialog->Show();
		}
	}
}

void V4LPropertiesDialog::onReset(wxCommandEvent& event) {
	V4LControlMap::iterator it;
	V4LControl *control;

	wxCheckBox *checkbox;
	wxSpinCtrl *spinctrl;

	CheckboxMap::iterator checkboxIt;
	SpinctrlMap::iterator spinctrlIt;

	for (it=controlMap->begin(); it!=controlMap->end(); ++it) {
		int id = it->first;
		V4LControl *control = (V4LControl*)it->second;

		switch (control->type) {
			case V4L2_CTRL_TYPE_INTEGER:
				if (spinctrlMap.end() != (spinctrlIt = spinctrlMap.find(id))) {
					spinctrl = spinctrlIt->second;

					spinctrl->SetValue(control->defaultValue);
				}
				break;
			case V4L2_CTRL_TYPE_BOOLEAN:
				if (checkboxMap.end() != (checkboxIt = checkboxMap.find(id))) {
					checkbox = checkboxIt->second;

					checkbox->SetValue(control->defaultValue);
				}
				break;
			default:
				break;
		}
	}
}
