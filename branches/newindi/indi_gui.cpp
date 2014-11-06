/*
 *    indi_gui.cpp
 *    PHD Guiding
 * 
 *    Copyright(c) 2009 Geoffrey Hausheer. All rights reserved.
 *    
 *    Redraw for libindi/baseclient by Patrick Chevalley
 *    Copyright (c) 2014 Patrick Chevalley
 *    All rights reserved.
 *  
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2.1 of the License, or (at your option) any later version.
 * 
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 * 
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *  
 *  Contact Information: gcx@phracturedblue.com <Geoffrey Hausheer>
 *******************************************************************************/

#ifdef _WIN32
#pragma warning(disable: 4996)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "indi_gui.h"

#define POS(r, c)        wxGBPosition(r,c)
#define SPAN(r, c)       wxGBSpan(r,c)

WX_DECLARE_STRING_HASH_MAP( void *, ptrHash );

static const char indi_state[4][6] = {
   "Idle",
   "Ok",
   "Busy",
   "Alert",
};

class IndiStatus : public wxLed
{
public:
   IndiStatus(wxWindow *parent, wxWindowID id, IPState state) : wxLed(parent, id)
	{
		SetState(state);
		Enable();
	}
	void SetState(int state)
	{
		switch(state) {
			case IPS_IDLE:  SetColor("808080"); break;
			case IPS_OK:    SetColor("008000"); break;
			case IPS_BUSY:  SetColor("FFFF00"); break;
			case IPS_ALERT: SetColor("FF0000"); break;
		}
		SetToolTip(wxString::FromAscii(indi_state[state]));
	}
};

class IndiProp
{
public:
	ptrHash		ctrl;
	ptrHash		entry;
	IndiStatus	*state;
	wxStaticText	*name;
	wxPanel		*page;
	wxGridBagSizer  *gbs;
};

class IndiDev
{
public:
	ptrHash		group;
	wxNotebook	*page;
};

enum {
	SWITCH_CHECKBOX,
	SWITCH_BUTTON,
	SWITCH_COMBOBOX,
};

enum {
	ID_Save = 1,
};

BEGIN_EVENT_TABLE(IndiGui, wxFrame)
EVT_CLOSE(IndiGui::OnQuit)
EVT_MENU(ID_Save, IndiGui::SaveDialog)
END_EVENT_TABLE()

void IndiGui::MakeDevicePage(INDI::BaseDevice *dp)
{
/*	IndiDev *indiDev = new IndiDev();
	wxPanel *panel = new wxPanel(parent_notebook);
	indiDev->page = new wxNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
	wxBoxSizer *nb_sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(nb_sizer);
	nb_sizer->Add(indiDev->page, 1,  wxEXPAND | wxALL);
	parent_notebook->AddPage(panel, wxString::FromAscii(idev->device->getDeviceName()));
	idev->window = indiDev;
	sizer->Layout();
	panel->Fit();*/
}


void IndiGui::UpdateWidget(INDI::Property *property)
{
/*	void *value;
	int n;
	ISRule switch_type;

	if (property->getType() == INDI_SWITCH)
	   switch_type = property->getSwitch()->r;

	
	   
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);

		value = ((IndiProp *)iprop->widget)->ctrl[wxString::FromAscii(elem->name)];
		switch (property->getType()) {
		case INDI_TEXT:
			{
			for (n = 0; n < property->getText()->ntp; n++ ) {
			   
			  wxStaticText *ctrl = (wxStaticText *)value;
			  wxString str(elem->value.str, wxConvUTF8);
			  ctrl->SetLabel(str);
			}  
			break;
			}
		case INDI_PROP_NUMBER:
			{
			wxStaticText *ctrl = (wxStaticText *)value;
			ctrl->SetLabel(wxString::Format(wxT("%f"), elem->value.num.value));
			break;
			}
		case INDI_PROP_SWITCH:
			{
			switch (switch_type) {
			case SWITCH_BUTTON:
				{
				wxToggleButton *ctrl = (wxToggleButton *)value;
				ctrl->SetValue(elem->value.set ? true : false);
				break;
				}
			case SWITCH_CHECKBOX:
				{
				wxCheckBox *ctrl = (wxCheckBox *)value;
				ctrl->SetValue(elem->value.set ? true : false);
				break;
				}
			case SWITCH_COMBOBOX:
				{
				if (elem->value.set) {
					int choice = (long)value;
					wxChoice *combo = (wxChoice *)((IndiProp *)iprop->widget)->gbs->FindItemAtPosition(POS(0,0))->GetWindow();
					combo->SetSelection(choice);
				}
				break;
				}
			}
			break;
			}
		}
	
	((IndiProp *)iprop->widget)->state->SetState(iprop->state);

	//Display any message
	ShowMessage(iprop->message);
	iprop->message[0] = 0;
	*/
}


void IndiGui::ShowMessage(const char *message)
{
/*	if (message && strlen(message) > 0) {
		time_t curtime;
		char timestr[30];
		struct tm time_loc;

		curtime = time(NULL);
#ifdef _WIN32
		localtime_s(&time_loc, &curtime);
#else
		localtime_r(&curtime, &time_loc);
#endif
		strftime(timestr, sizeof(timestr), "%b %d %T: ", &time_loc);
		textbuffer->SetInsertionPoint(0);
		textbuffer->WriteText(wxString::FromAscii(timestr));
		textbuffer->WriteText(wxString::FromAscii(message));
		textbuffer->WriteText(_T("\n"));
	}*/
}

void IndiGui::SetButtonEvent(wxCommandEvent & event)
{
/*	indi_list *isl;
	wxTextCtrl *entry;
	wxButton *button = (wxButton *)event.GetEventObject();
	struct indi_prop_t *iprop = (struct indi_prop_t *)button->GetClientData();

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		entry = (wxTextCtrl *)((IndiProp *)iprop->widget)->entry[wxString::FromAscii(elem->name)];
		switch (iprop->type) {
		case INDI_PROP_TEXT:
			strncpy(elem->value.str, entry->GetLineText(0).mb_str(wxConvUTF8), sizeof(elem->value.str));
			printf("Text: %s\n", elem->value.str);
			entry->Clear();
			break;
		case INDI_PROP_NUMBER:
			entry->GetLineText(0).ToDouble(&elem->value.num.value);
			entry->Clear();
			break;
		}
	}
	indi_send(iprop, NULL);*/
}


void IndiGui::SetComboboxEvent(wxCommandEvent & event)
{
/*	indi_list *isl;
	wxChoice *combo = (wxChoice *)event.GetEventObject();
	struct indi_prop_t *iprop = (struct indi_prop_t *)combo->GetClientData();
	int choice = combo->GetSelection();

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		int value = (long)((IndiProp *)iprop->widget)->ctrl[wxString::FromAscii(elem->name)];
		if (value == choice) {
			if (! elem->value.set) {
				elem->value.set = 1;
				indi_send(elem->iprop, elem);
			}
			return;
		}
	}
	*/
}

void IndiGui::SetCheckboxEvent(wxCommandEvent & event)
{
/*	wxCheckBox *button = (wxCheckBox *)event.GetEventObject();
	struct indi_elem_t *elem = (struct indi_elem_t *)button->GetClientData();
	elem->value.set = button->GetValue();
	indi_send(elem->iprop, elem);*/
}

void IndiGui::SetToggleButtonEvent(wxCommandEvent & event)
{
/*	wxToggleButton *button = (wxToggleButton *)event.GetEventObject();
	struct indi_elem_t *elem = (struct indi_elem_t *)button->GetClientData();
	elem->value.set = button->GetValue();
	indi_send(elem->iprop, elem);*/
}

int IndiGui::GetSwitchType(INDI::Property *property)
{
/*	int num_props = il_length(iprop->elems);

	if (iprop->rule == INDI_RULE_ANYOFMANY)
		return SWITCH_CHECKBOX;
	
	if (num_props <= 4)
		return SWITCH_BUTTON;
*/
	return SWITCH_COMBOBOX;
}

void IndiGui::CreateSwitchWidget(INDI::Property *property, int num_props)
{
/*	int guitype = GetSwitchType(iprop);

	switch (guitype) {
		case SWITCH_COMBOBOX: CreateSwitchCombobox(iprop, num_props); break;
		case SWITCH_CHECKBOX: CreateSwitchCheckbox(iprop, num_props); break;
		case SWITCH_BUTTON:   CreateSwitchButton(iprop, num_props);   break;
	}*/
}

void IndiGui::CreateSwitchCombobox(INDI::Property *property, int num_props)
{
/*	wxChoice *combo;
	wxPanel *p;
	wxGridBagSizer *gbs;
	wxString *choices = new wxString[num_props];
	int i = 0;
	int idx = 0;
	indi_list *isl;

	p = ((IndiProp *)iprop->widget)->page;
	gbs = ((IndiProp *)iprop->widget)->gbs;
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl)) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		if(elem->value.set)
			idx = i;
		((IndiProp *)iprop->widget)->ctrl[wxString::FromAscii(elem->name)] = (void *) (intptr_t)i;
		choices[i++] = wxString::FromAscii(elem->label);
	}
	combo = new wxChoice(p, wxID_ANY, wxDefaultPosition, wxDefaultSize, num_props, choices);
	combo->SetSelection(idx);
	combo->SetClientData(iprop);
	Connect(combo->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
		        wxCommandEventHandler(IndiGui::SetComboboxEvent));
	gbs->Add(combo, POS(0, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);

	delete [] choices;*/
}

void IndiGui::CreateSwitchCheckbox(INDI::Property *property, int /*num_props*/)
{
/*	wxPanel *p;
	wxGridBagSizer *gbs;
	int pos = 0;
	indi_list *isl;

	p = ((IndiProp *)iprop->widget)->page;
	gbs = ((IndiProp *)iprop->widget)->gbs;
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		wxCheckBox *button = new wxCheckBox(p, wxID_ANY, wxString::FromAscii(elem->label));
		((IndiProp *)iprop->widget)->ctrl[wxString::FromAscii(elem->name)] = button;
		if (elem->value.set)
			button->SetValue(true);
		button->SetClientData(elem);
		Connect(button->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
		        wxCommandEventHandler(IndiGui::SetCheckboxEvent));
		gbs->Add(button, POS(pos / 4, pos % 4), SPAN(1, 1), wxALIGN_LEFT | wxALL);
	}*/
}

void IndiGui::CreateSwitchButton(INDI::Property *property, int /*num_props*/)
{
	/*wxPanel *p;
	wxGridBagSizer *gbs;
	int pos = 0;
	indi_list *isl;

	p = ((IndiProp *)iprop->widget)->page;
	gbs = ((IndiProp *)iprop->widget)->gbs;
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		wxToggleButton *button = new wxToggleButton(p, wxID_ANY, wxString::FromAscii(elem->label));
		((IndiProp *)iprop->widget)->ctrl[wxString::FromAscii(elem->name)] = button;
		if (elem->value.set)
			button->SetValue(true);
		button->SetClientData(elem);
		Connect(button->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
		        wxCommandEventHandler(IndiGui::SetToggleButtonEvent));
		gbs->Add(button, POS(0, pos), SPAN(1, 1), wxALIGN_LEFT | wxALL);
		
	}*/
}

void IndiGui::CreateTextWidget(INDI::Property *property, int /*num_props*/)
{
/*	int pos = 0;
	wxStaticText *value;
	wxTextCtrl *entry;
	wxButton *button;
	wxPanel *p;
	wxGridBagSizer *gbs;
	indi_list *isl;

	p = ((IndiProp *)iprop->widget)->page;
	gbs = ((IndiProp *)iprop->widget)->gbs;
	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		gbs->Add(new wxStaticText(p, wxID_ANY, wxString::FromAscii(elem->label)),
		         POS(pos, 0), SPAN(1, 1),
		         wxALIGN_LEFT | wxALL);

		value = new wxStaticText(p, wxID_ANY, wxString::FromAscii(elem->value.str));
		((IndiProp *)iprop->widget)->ctrl[wxString::FromAscii(elem->name)] = value;
		gbs->Add(value, POS(pos, 1), SPAN(1, 1),
		         wxALIGN_LEFT | wxALL);
		if (iprop->permission != INDI_RO) {
			entry = new wxTextCtrl(p, wxID_ANY);
			((IndiProp *)iprop->widget)->entry[wxString::FromAscii(elem->name)] = entry;
			gbs->Add(entry, POS(pos, 2), SPAN(1, 1),
			         wxALIGN_LEFT | wxEXPAND | wxALL);
		}
	}
	if (iprop->permission != INDI_RO) {
		button = new wxButton(p, wxID_ANY, _T("Set"));
		button->SetClientData(iprop);
		Connect(button->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
		        wxCommandEventHandler(IndiGui::SetButtonEvent));
		gbs->Add(button, POS(0, 3), SPAN(pos, 1),
		         wxALIGN_LEFT | wxALL);
	}*/
}

void IndiGui::CreateNumberWidget(INDI::Property *property, int /*num_props*/)
{
/*	int pos = 0;
	wxStaticText *value;
	wxTextCtrl *entry;
	wxButton *button;
	wxPanel *p;
	wxGridBagSizer *gbs;
	indi_list *isl;

	p = ((IndiProp *)iprop->widget)->page;
	gbs = ((IndiProp *)iprop->widget)->gbs;

	for (isl = il_iter(iprop->elems); ! il_is_last(isl); isl = il_next(isl), pos++) {
		struct indi_elem_t *elem = (struct indi_elem_t *)il_item(isl);
		gbs->Add(new wxStaticText(p, wxID_ANY, wxString::FromAscii(elem->label)),
		         POS(pos, 0), SPAN(1, 1),
		         wxALIGN_LEFT | wxALL);

		value = new wxStaticText(p, wxID_ANY, wxString::Format(_T("%f"), elem->value.num.value));
		((IndiProp *)iprop->widget)->ctrl[wxString::FromAscii(elem->name)] = value;
		gbs->Add(value, POS(pos, 1), SPAN(1, 1),
		         wxALIGN_LEFT | wxALL);
		if (iprop->permission != INDI_RO) {
			entry = new wxTextCtrl(p, wxID_ANY);
			((IndiProp *)iprop->widget)->entry[wxString::FromAscii(elem->name)] = entry;
			gbs->Add(entry, POS(pos, 2), SPAN(1, 1),
			         wxALIGN_LEFT | wxEXPAND | wxALL);
		}
	}
	if (iprop->permission != INDI_RO) {
		button = new wxButton(p, wxID_ANY, _T("Set"));
		button->SetClientData(iprop);
		Connect(button->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
		        wxCommandEventHandler(IndiGui::SetButtonEvent));
		gbs->Add(button, POS(0, 3), SPAN(pos, 1),
		         wxALIGN_LEFT | wxALL);
	}*/
}

void IndiGui::CreateLightWidget(INDI::Property * /*iprop*/, int /*num_props*/)
{
}

void IndiGui::CreateBlobWidget(INDI::Property * /*iprop*/, int /*num_props*/)
{
}

void IndiGui::BuildPropWidget(INDI::Property *property, wxPanel *parent)
{
/*	IndiProp *indiProp = new IndiProp();
	int num_props;

	indiProp->page = parent;
	indiProp->gbs  = new wxGridBagSizer(0, 20);
	
	indiProp->state = new IndiStatus(parent, wxID_ANY, iprop->state);
	indiProp->name  = new wxStaticText(parent, wxID_ANY, wxString::FromAscii(iprop->name));

	iprop->widget = indiProp;
 	num_props = il_length(iprop->elems);

	switch (iprop->type) {
	case INDI_PROP_TEXT:
		CreateTextWidget(iprop, num_props);
		break;
	case INDI_PROP_SWITCH:
		CreateSwitchWidget(iprop, num_props);
		break;
	case INDI_PROP_NUMBER:
		CreateNumberWidget(iprop, num_props);
		break;
	case INDI_PROP_LIGHT:
		CreateLightWidget(iprop, num_props);
		break;
	case INDI_PROP_BLOB:
		CreateBlobWidget(iprop, num_props);
		break;
	}
	indiProp->gbs->Layout();*/
}

void IndiGui::AddProp(INDI::BaseDevice *dp, const wxString groupname, INDI::Property *property)
{
/*	wxPanel *page;
	wxGridBagSizer *gbs;
	int next_free_row;
	IndiDev *indiDev = (IndiDev *)idev->window;
	
	page = (wxPanel *)indiDev->group[groupname];
	if (! page) {
		page = new wxPanel(indiDev->page);
		indiDev->page->AddPage(page, groupname);
		page->SetSizer(new wxGridBagSizer(0, 20));
		((IndiDev *)idev->window)->group[groupname] = page;
	}
	gbs = (wxGridBagSizer *)page->GetSizer();
	next_free_row = gbs->GetRows();

	BuildPropWidget(iprop, page);
	gbs->Add(((IndiProp *)iprop->widget)->state,
	         POS(next_free_row, 0), SPAN(1, 1),
	         wxALIGN_LEFT | wxALL);
	gbs->Add(((IndiProp *)iprop->widget)->name,
	         POS(next_free_row, 1), SPAN(1, 1),
	         wxALIGN_LEFT | wxALL);
	gbs->Add(((IndiProp *)iprop->widget)->gbs,
	         POS(next_free_row, 2), SPAN(1, 1),
	         wxALIGN_LEFT | wxEXPAND | wxALL);
	gbs->Layout();
	page->Fit();
	panel->Fit();
	page->Show();
	indiDev->page->Fit();
	indiDev->page->Layout();
	indiDev->page->Show();*/
}


void IndiGui::DeleteProp(INDI::Property *property)
{
/*	IndiProp *prop = (IndiProp*)iprop->widget;
	IndiDev *indiDev = (IndiDev *)iprop->idev->window;

	for (int y = 0; y < prop->gbs->GetRows(); y++) {
		for (int x = 0; x < prop->gbs->GetCols(); x++) {
			wxGBSizerItem *item = prop->gbs->FindItemAtPosition(POS(y, x));
			if (item)
				item->GetWindow()->Destroy();
		}
	}
	if (prop->name)
		prop->name->Destroy();
	if (prop->state)
		prop->state->Destroy();
	if (prop->page->GetChildren().GetCount() == 0) {
		for (unsigned int i = 0; i < indiDev->page->GetPageCount(); i++) {
			if (prop->page == indiDev->page->GetPage(i)) {
				indiDev->group.erase(indiDev->page->GetPageText(i));
				indiDev->page->DeletePage(i);
				break;
			}
		}
	}
	delete prop;*/
}


IndiGui::IndiGui() : wxFrame((wxFrame *)wxTheApp->GetTopWindow(), wxID_ANY,
                             _("INDI Options"),
                             wxDefaultPosition, wxSize(640, 400))
{
//	indi = _indi;
	wxMenu *menuFile = new wxMenu;

	menuFile->Append( ID_Save, _T("&Save Settings...") );
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, _T("&File") );
	SetMenuBar( menuBar );

	panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DOUBLE | wxTAB_TRAVERSAL);
	sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);
	parent_notebook = new wxNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
	sizer->Add(parent_notebook, 0, wxEXPAND | wxALL);
	textbuffer = new wxTextCtrl(panel, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	sizer->Add(textbuffer, 1, wxEXPAND | wxALL);
}

void IndiGui::SaveDialog(wxCommandEvent& WXUNUSED(event))
{
/*	IndiSave *saveDlg = new IndiSave(this, _T("Save Options"), indi);
	if (saveDlg->ShowModal() == wxID_OK) {
		saveDlg->SetSave();
		ic_update_props(indi->config);
	}
	saveDlg->Destroy();*/
}

void IndiGui::OnQuit(wxCloseEvent& WXUNUSED(event))
{
	if (child_window) {
		Show(false);
	} else {
		Destroy();
	}
}
		
