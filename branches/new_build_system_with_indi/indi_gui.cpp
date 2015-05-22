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

wxDEFINE_EVENT(INDIGUI_THREAD_NEWDEVICE_EVENT, wxThreadEvent);
wxDEFINE_EVENT(INDIGUI_THREAD_NEWPROPERTY_EVENT, wxThreadEvent);
wxDEFINE_EVENT(INDIGUI_THREAD_NEWNUMBER_EVENT, wxThreadEvent);
wxDEFINE_EVENT(INDIGUI_THREAD_NEWTEXT_EVENT, wxThreadEvent);
wxDEFINE_EVENT(INDIGUI_THREAD_NEWSWITCH_EVENT, wxThreadEvent);
wxDEFINE_EVENT(INDIGUI_THREAD_NEWMESSAGE_EVENT, wxThreadEvent);
wxDEFINE_EVENT(INDIGUI_THREAD_REMOVEPROPERTY_EVENT, wxThreadEvent);


BEGIN_EVENT_TABLE(IndiGui, wxDialog)
  EVT_CLOSE(IndiGui::OnQuit)
  EVT_THREAD(INDIGUI_THREAD_NEWDEVICE_EVENT, IndiGui::OnNewDeviceFromThread)
  EVT_THREAD(INDIGUI_THREAD_NEWPROPERTY_EVENT, IndiGui::OnNewPropertyFromThread)
  EVT_THREAD(INDIGUI_THREAD_NEWNUMBER_EVENT, IndiGui::OnNewNumberFromThread)
  EVT_THREAD(INDIGUI_THREAD_NEWTEXT_EVENT, IndiGui::OnNewTextFromThread)
  EVT_THREAD(INDIGUI_THREAD_NEWSWITCH_EVENT, IndiGui::OnNewSwitchFromThread)
  EVT_THREAD(INDIGUI_THREAD_NEWMESSAGE_EVENT, IndiGui::OnNewMessageFromThread)
  EVT_THREAD(INDIGUI_THREAD_REMOVEPROPERTY_EVENT, IndiGui::OnRemovePropertyFromThread)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////
// Functions running in the INDI client thread
//////////////////////////////////////////////////////////////////////

void IndiGui::newDevice(INDI::BaseDevice *dp)
{
   wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, INDIGUI_THREAD_NEWDEVICE_EVENT);
   event->SetExtraLong((long) dp);
   wxQueueEvent(this, event);
}

void IndiGui::newProperty(INDI::Property *property)
{
    wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, INDIGUI_THREAD_NEWPROPERTY_EVENT);
    event->SetExtraLong((long) property);
    wxQueueEvent(this, event);
}

void IndiGui::newNumber(INumberVectorProperty *nvp)
{
    wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, INDIGUI_THREAD_NEWNUMBER_EVENT);
    event->SetExtraLong((long) nvp);
    wxQueueEvent(this, event);
}

void IndiGui::newSwitch(ISwitchVectorProperty *svp)
{
   wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, INDIGUI_THREAD_NEWSWITCH_EVENT);
   event->SetExtraLong((long) svp);
   wxQueueEvent(this, event);
}

void IndiGui::newText(ITextVectorProperty *tvp)
{
   wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, INDIGUI_THREAD_NEWTEXT_EVENT);
   event->SetExtraLong((long) tvp);
   wxQueueEvent(this, event);
}

void IndiGui::newMessage(INDI::BaseDevice *dp, int messageID)
{
   wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, INDIGUI_THREAD_NEWMESSAGE_EVENT);
#ifdef INDI_PRE_1_0_0   
   const char *msg = dp->messageQueue(messageID);
#else   
   const char *msg = dp->messageQueue(messageID).c_str(); // http://sourceforge.net/p/indi/code/1803/
#endif   
   event->SetExtraLong((long) msg);
   wxQueueEvent(this, event);
}

void IndiGui::removeProperty(INDI::Property *property)
{
   wxString devname =  wxString::FromAscii(property->getDeviceName());
   wxString groupname =  wxString::FromAscii(property->getGroupName());
   wxString propname =  wxString::FromAscii(property->getName());
   IndiDev *indiDev = (IndiDev *)devlist[devname];
   if (!indiDev) return;
   IndiProp *indiProp = (IndiProp *)indiDev->properties[propname];
   if (!indiProp) return;
   
   wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD, INDIGUI_THREAD_REMOVEPROPERTY_EVENT);
   event->SetExtraLong((long) indiProp);
   wxQueueEvent(this, event);
}

//////////////////////////////////////////////////////////////////////

void IndiGui::ConnectServer(wxString INDIhost, long INDIport)
{
    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
    connectServer();
}

void IndiGui::serverConnected()
{
    setBLOBMode(B_NEVER, "", NULL);
    ready = true;
}

void IndiGui::serverDisconnected(int exit_code)
{
   // connection to server is lost, destroy ourself 
   if (ready) Destroy();
}

void IndiGui::OnNewDeviceFromThread(wxThreadEvent& event)
{
    INDI::BaseDevice *dp = (INDI::BaseDevice *) event.GetExtraLong();
    //printf("newdevice from thread %s \n",dp->getDeviceName());
    wxString devname =  wxString::FromAscii(dp->getDeviceName());
    IndiDev *indiDev = new IndiDev();
    wxPanel *panel = new wxPanel(parent_notebook);
    indiDev->page = new wxNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
    wxBoxSizer *nb_sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(nb_sizer);
    nb_sizer->Add(indiDev->page, 1,  wxEXPAND | wxALL);
    parent_notebook->AddPage(panel, devname);
    indiDev->dp = dp;
    devlist[devname] = indiDev;
    panel->Fit();
    sizer->Layout();
    Fit();
}

void IndiGui::OnNewPropertyFromThread(wxThreadEvent& event)
{
    INDI::Property *property = (INDI::Property *) event.GetExtraLong();
    //printf("newproperty from thread %s %s %s\n",property->getDeviceName(),property->getGroupName(),property->getName());
    wxString devname =  wxString::FromAscii(property->getDeviceName());
    wxString groupname =  wxString::FromAscii(property->getGroupName());
    wxString propname =  wxString::FromAscii(property->getName());
    
    IndiProp *indiProp = new IndiProp();
    wxPanel *page;
    wxGridBagSizer *gbs;
    int next_free_row;
    IndiDev *indiDev = (IndiDev *)devlist[devname];
    if (! indiDev) return;
    indiProp->idev = indiDev;
    
    page = (wxPanel *)indiDev->groups[groupname];
    if (! page) {
	page = new wxPanel(indiDev->page);
	indiDev->page->AddPage(page, groupname);
	page->SetSizer(new wxGridBagSizer(0, 20));
	indiDev->groups[groupname] = page;
    }
    
    gbs = (wxGridBagSizer *)page->GetSizer();
    gbs->Layout();
    next_free_row = gbs->GetRows();
    BuildPropWidget(property, page, indiProp);
    
    gbs->Add(indiProp->state, POS(next_free_row, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    gbs->Add(indiProp->name, POS(next_free_row, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    gbs->Add(indiProp->panel,POS(next_free_row, 2), SPAN(1, 1), wxALIGN_LEFT | wxEXPAND | wxALL);
    gbs->Layout();
    page->Fit();
    panel->Fit();
    page->Show();
    indiDev->properties[propname] = indiProp;
    indiDev->page->Fit();
    indiDev->page->Layout();
    indiDev->page->Show();
    sizer->Layout();
    Fit();
}

void IndiGui::BuildPropWidget(INDI::Property *property, wxPanel *parent, IndiProp *indiProp)
{
   wxString propname =  wxString::FromAscii(property->getName());
   wxString proplbl =  wxString::FromAscii(property->getLabel());
   if (! proplbl) proplbl = propname;
   #ifdef INDI_PRE_1_1_0
     INDI_TYPE proptype = property->getType();
   #else
     INDI_PROPERTY_TYPE proptype = property->getType();
   #endif 
   
   
   indiProp->page = parent;
   indiProp->panel = new wxPanel(parent);
   indiProp->gbs  = new wxGridBagSizer(0, 20);
   indiProp->panel->SetSizer(indiProp->gbs);
   
   indiProp->state = new IndiStatus(parent, wxID_ANY, property->getState());
   indiProp->name  = new wxStaticText(parent, wxID_ANY,proplbl);
   indiProp->PropName = propname;
   indiProp->property = property;
   
   switch (proptype) {
      case INDI_TEXT:
	 CreateTextWidget(property, indiProp);
	 break;
      case INDI_SWITCH:
	 CreateSwitchWidget(property, indiProp);
	 break;
      case INDI_NUMBER:
	 CreateNumberWidget(property, indiProp);
	 break;
      case INDI_LIGHT:
	 CreateLightWidget(property, indiProp);
	 break;
      case INDI_BLOB:
	 CreateBlobWidget(property, indiProp);
	 break;
      case INDI_UNKNOWN:
	 CreateUnknowWidget(property, indiProp);
	 break;
   }
   indiProp->gbs->Layout();
}

int IndiGui::GetSwitchType(ISwitchVectorProperty *svp)
{
   int num_props = svp->nsp;
   
   if (svp->r == ISR_NOFMANY)
      return SWITCH_CHECKBOX;
   
   if (num_props <= 4)
      return SWITCH_BUTTON;
   
   return SWITCH_COMBOBOX;
}

void IndiGui::CreateSwitchWidget(INDI::Property *property, IndiProp *indiProp)
{
   //printf("CreateSwitchWidget\n");
   int guitype = GetSwitchType(property->getSwitch());
   
   switch (guitype) {
      case SWITCH_COMBOBOX: CreateSwitchCombobox(property->getSwitch(), indiProp); break;
      case SWITCH_CHECKBOX: CreateSwitchCheckbox(property->getSwitch(), indiProp); break;
      case SWITCH_BUTTON:   CreateSwitchButton(property->getSwitch(), indiProp);   break;
   }
}

void IndiGui::CreateSwitchCombobox(ISwitchVectorProperty *svp, IndiProp *indiProp)
{
   wxChoice *combo;
   wxPanel *p;
   wxGridBagSizer *gbs;
   wxString *choices = new wxString[svp->nsp];
   int i = 0;
   int idx = 0;
   
   p = indiProp->panel;
   gbs = indiProp->gbs;
   for (i = 0; i < svp->nsp; i++){
      if(svp->sp[i].s == ISS_ON)
	 idx = i;
      indiProp->ctrl[wxString::FromAscii(svp->sp[i].name)] = (void *) (intptr_t) i;
      wxString swlbl = wxString::FromAscii(svp->sp[i].label);
      if (! swlbl) swlbl = wxString::FromAscii(svp->sp[i].name);
      choices[i] = swlbl;
   }
   combo = new wxChoice(p, wxID_ANY, wxDefaultPosition, wxDefaultSize, svp->nsp, choices);
   combo->SetSelection(idx);
   combo->SetClientData(indiProp);
   Connect(combo->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
	   wxCommandEventHandler(IndiGui::SetComboboxEvent));
   gbs->Add(combo, POS(0, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
   indiProp->ctrl[wxString::FromAscii(svp->name)] = (void *) combo;
   delete [] choices;
}

void IndiGui::CreateSwitchCheckbox(ISwitchVectorProperty *svp, IndiProp *indiProp)
{
   wxPanel *p;
   wxGridBagSizer *gbs;
   int pos = 0;
   
   p = indiProp->panel;
   gbs = indiProp->gbs;
   for (pos = 0; pos < svp->nsp; pos++){
      wxString swlbl = wxString::FromAscii(svp->sp[pos].label);
      if (! swlbl) swlbl = wxString::FromAscii(svp->sp[pos].name);
      wxCheckBox *button = new wxCheckBox(p, wxID_ANY, swlbl);
      indiProp->ctrl[wxString::FromAscii(svp->sp[pos].name)] = button;
      if (svp->sp[pos].s == ISS_ON)
	 button->SetValue(true);
      button->SetClientData(indiProp);
      Connect(button->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
	      wxCommandEventHandler(IndiGui::SetCheckboxEvent));
      gbs->Add(button, POS(pos / 4, pos % 4), SPAN(1, 1), wxALIGN_LEFT | wxALL);
   }
}

void IndiGui::CreateSwitchButton(ISwitchVectorProperty *svp, IndiProp *indiProp)
{
   wxPanel *p;
   wxGridBagSizer *gbs;
   int pos = 0;
   
   p = indiProp->panel;
   gbs = indiProp->gbs;
   for (pos = 0; pos < svp->nsp; pos++){
      wxString swlbl = wxString::FromAscii(svp->sp[pos].label);
      if (! swlbl) swlbl = wxString::FromAscii(svp->sp[pos].name);
      wxToggleButton *button = new wxToggleButton(p, wxID_ANY, swlbl);
      indiProp->ctrl[wxString::FromAscii(svp->sp[pos].name)] = button;
      if (svp->sp[pos].s == ISS_ON)
         button->SetValue(true);
      button->SetClientData(indiProp);
      Connect(button->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
      wxCommandEventHandler(IndiGui::SetToggleButtonEvent));
      if (!allow_connect_disconnect && strcmp(svp->name,"CONNECTION")==0) {
        button->Enable(false);
      }
      gbs->Add(button, POS(0, pos), SPAN(1, 1), wxALIGN_LEFT | wxALL);
   }
}

void IndiGui::CreateTextWidget(INDI::Property *property, IndiProp *indiProp)
{
   //printf("CreateTextWidget\n");
   int pos = 0;
   wxStaticText *value;
   wxTextCtrl *entry;
   wxButton *button;
   wxPanel *p;
   wxGridBagSizer *gbs;
    
   ITextVectorProperty *tvp = property->getText();
   p = indiProp->panel;
   gbs = indiProp->gbs;
   for (pos = 0; pos < tvp->ntp; pos++) {
    	gbs->Add(new wxStaticText(p, wxID_ANY, wxString::FromAscii(tvp->tp[pos].label)),
    	         POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
     
    	value = new wxStaticText(p, wxID_ANY, wxString::FromAscii(tvp->tp[pos].text));
	indiProp->ctrl[wxString::FromAscii(tvp->tp[pos].name)] = value;
    	gbs->Add(value, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    	if (tvp->p != IP_RO) {
    		entry = new wxTextCtrl(p, wxID_ANY);
    		indiProp->entry[wxString::FromAscii(tvp->tp[pos].name)] = entry;
    		gbs->Add(entry, POS(pos, 2), SPAN(1, 1), wxALIGN_LEFT | wxEXPAND | wxALL);
}
}
if (tvp->p != IP_RO) {
   button = new wxButton(p, wxID_ANY, _T("Set"));
   button->SetClientData(indiProp);
   Connect(button->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(IndiGui::SetButtonEvent));
   gbs->Add(button, POS(0, 3), SPAN(pos, 1), wxALIGN_LEFT | wxALL);
} 
}

void IndiGui::CreateNumberWidget(INDI::Property *property, IndiProp *indiProp)
{
   //printf("CreateNumberWidget\n");
   int pos = 0;
   wxStaticText *value;
   wxTextCtrl *entry;
   wxButton *button;
   wxPanel *p;
   wxGridBagSizer *gbs;
   
   INumberVectorProperty *nvp = property->getNumber();
   p = indiProp->panel;
   gbs = indiProp->gbs;
   
   for (pos = 0; pos < nvp->nnp; pos++) {
      gbs->Add(new wxStaticText(p, wxID_ANY, wxString::FromAscii(nvp->np[pos].label)),
	       POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
      
      value = new wxStaticText(p, wxID_ANY, wxString::Format(_T("%f"), nvp->np[pos].value));
      indiProp->ctrl[wxString::FromAscii(nvp->np[pos].name)] = value;
      gbs->Add(value, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL);
      if (nvp->p != IP_RO) {
	 entry = new wxTextCtrl(p, wxID_ANY);
	 indiProp->entry[wxString::FromAscii(nvp->np[pos].name)] = entry;
	 gbs->Add(entry, POS(pos, 2), SPAN(1, 1), wxALIGN_LEFT | wxEXPAND | wxALL);
      }
   }
   if (nvp->p != IP_RO) {
      button = new wxButton(p, wxID_ANY, _T("Set"));
      button->SetClientData(indiProp);
      Connect(button->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(IndiGui::SetButtonEvent));
      gbs->Add(button, POS(0, 3), SPAN(pos, 1), wxALIGN_LEFT | wxALL);
   }
}

void IndiGui::CreateLightWidget(INDI::Property *property, IndiProp *indiProp)
{
   //printf("IndiGui: Unimplemented CreateLightWidget\n");
   
}

void IndiGui::CreateBlobWidget(INDI::Property *property, IndiProp *indiProp)
{
   //printf("IndiGui: Unimplemented CreateBlobWidget\n");
   
}

void IndiGui::CreateUnknowWidget(INDI::Property *property, IndiProp *indiProp)
{
   //printf("IndiGui: Unimplemented CreateUnknowWidget\n");
   
}

void IndiGui::OnNewNumberFromThread(wxThreadEvent& event)
{
    void *st;
    int i;
    INumberVectorProperty *nvp = (INumberVectorProperty *) event.GetExtraLong();
    //printf("newnumber from thread %s \n",nvp->name);
    wxString devname =  wxString::FromAscii(nvp->device);
    wxString propname =  wxString::FromAscii(nvp->name);
    IndiDev *indiDev = (IndiDev *) devlist[devname];
    IndiProp *indiProp = (IndiProp *) indiDev->properties[propname];
    for (i = 0; i < nvp->nnp; i++) {
	st = indiProp->ctrl[wxString::FromAscii(nvp->np[i].name)];
	wxStaticText *ctrl = (wxStaticText *)st;
	ctrl->SetLabel(wxString::Format(wxT("%f"), nvp->np[i].value));
    }
    indiProp->state->SetState(nvp->s);
}

void IndiGui::OnNewTextFromThread(wxThreadEvent& event)
{
    void *st;
    int i;
    ITextVectorProperty *tvp = (ITextVectorProperty *) event.GetExtraLong();
    //printf("newtext from thread %s \n",tvp->name);
    wxString devname =  wxString::FromAscii(tvp->device);
    wxString propname =  wxString::FromAscii(tvp->name);
    IndiDev *indiDev = (IndiDev *) devlist[devname];
    IndiProp *indiProp = (IndiProp *) indiDev->properties[propname];
    for (i = 0; i < tvp->ntp; i++) {
	st = indiProp->ctrl[wxString::FromAscii(tvp->tp[i].name)];
	wxStaticText *ctrl = (wxStaticText *)st;
	ctrl->SetLabel(wxString::Format(wxT("%s"), tvp->tp[i].text));
    }
    indiProp->state->SetState(tvp->s);
}

void IndiGui::OnNewSwitchFromThread(wxThreadEvent& event)
{
    void *st;
    int i,idx;
    ISwitchVectorProperty *svp = (ISwitchVectorProperty *) event.GetExtraLong();
    //printf("newswitch from thread %s \n",svp->name);
    wxString devname =  wxString::FromAscii(svp->device);
    wxString propname =  wxString::FromAscii(svp->name);
    int swtype = GetSwitchType(svp);
    IndiDev *indiDev = (IndiDev *) devlist[devname];
    IndiProp *indiProp = (IndiProp *) indiDev->properties[propname];
    switch (swtype) {
	case SWITCH_COMBOBOX: {
	    idx=0;
	    for (i = 0; i < svp->nsp; i++){
		if(svp->sp[i].s == ISS_ON)
		    idx = i;
	    }
	    st = indiProp->ctrl[wxString::FromAscii(svp->name)];
	    wxChoice *combo = (wxChoice *)st;
	    combo->SetSelection(idx);
	    break;
	}
	case SWITCH_CHECKBOX:{
	    for (i = 0; i < svp->nsp; i++){
		st = indiProp->ctrl[wxString::FromAscii(svp->sp[i].name)];
		wxCheckBox *button = (wxCheckBox *) st;
		button->SetValue(svp->sp[i].s ? true : false);
	    }	 
	    break;
	}
	case SWITCH_BUTTON:{
	    for (i = 0; i < svp->nsp; i++){
		st = indiProp->ctrl[wxString::FromAscii(svp->sp[i].name)];
		wxToggleButton *button = (wxToggleButton *) st;
		button->SetValue(svp->sp[i].s ? true : false);
	    }
	    break;
	}
    }
    
}

void IndiGui::OnNewMessageFromThread(wxThreadEvent& event)
{
   char *message = (char *) event.GetExtraLong();
   
   if (message && strlen(message) > 0) {
      textbuffer->SetInsertionPoint(0);
      textbuffer->WriteText(wxString::FromAscii(message));
      textbuffer->WriteText(_T("\n"));
   }
#ifdef INDI_PRE_1_0_0
   delete message; //http://sourceforge.net/p/indi/code/1803/
#endif   
}

void IndiGui::SetButtonEvent(wxCommandEvent & event)
{
   wxTextCtrl *entry;
   int i;
   wxButton *button = (wxButton *)event.GetEventObject();
   if (! button) return;
   IndiProp *indiProp = (IndiProp *) button->GetClientData();
   if (! indiProp) return;
   
   switch (indiProp->property->getType()) {
       case INDI_TEXT:{ 
	   ITextVectorProperty *tvp = indiProp->property->getText();
	   for (i = 0; i < tvp->ntp; i++) {
	       if (tvp->p != IP_RO) {
		   entry = (wxTextCtrl *)(indiProp->entry[wxString::FromAscii(tvp->tp[i].name)]);
		   sprintf(tvp->tp[i].text, "%s", entry->GetLineText(0).mb_str().data());
	       }
	   }    
	   sendNewText(tvp);
	   break;
       }
       case INDI_NUMBER:{
	   INumberVectorProperty *nvp = indiProp->property->getNumber();
	   for (i = 0; i < nvp->nnp; i++) {
	       if (nvp->p != IP_RO) {
		   entry = (wxTextCtrl *)(indiProp->entry[wxString::FromAscii(nvp->np[i].name)]);
		   entry->GetLineText(0).ToDouble(&nvp->np[i].value);
	       }
	   }    
	   sendNewNumber(nvp);
	   break;
       }
       default:{
       }
   }
}

void IndiGui::SetToggleButtonEvent(wxCommandEvent & event)
{
   wxString b_name;
   int i;
   wxToggleButton *button = (wxToggleButton *)event.GetEventObject();
   if (! button) return;
   IndiProp *indiProp = (IndiProp *) button->GetClientData();
   if (! indiProp) return;
   ISwitchVectorProperty *svp = indiProp->property->getSwitch();

   if (!allow_connect_disconnect && strcmp(svp->name,"CONNECTION")==0) {
      // Prevent device disconnection from this window.
      // Use Gear manager instead.
      return;
   }
      
   ptrHash::iterator it;
   for( it = indiProp->ctrl.begin(); it !=indiProp->ctrl.end(); ++it )
   {
      wxString key = it->first;
      wxToggleButton *value = (wxToggleButton *) it->second;
      if (value == button) {
	 b_name = key;
	 break;
      }
   }
   if (svp->r == ISR_1OFMANY){
      for (i=0; i<svp->nsp; i++){
	 if (svp->sp[i].name == b_name) {
	    svp->sp[i].s = ISS_ON;
	 }
	 else {
	    svp->sp[i].s = ISS_OFF;
	 }
      }
   }
   else {
      for (i=0; i<svp->nsp; i++){
	 if (svp->sp[i].name == b_name) {
	    svp->sp[i].s = button->GetValue() ? ISS_ON : ISS_OFF;
	    break;
	 }
      }
   }
   sendNewSwitch(svp);
}


void IndiGui::SetComboboxEvent(wxCommandEvent & event)
{
    int i;
    wxChoice *combo = (wxChoice *)event.GetEventObject();
    if (! combo) return;
    IndiProp *indiProp = (IndiProp *) combo->GetClientData();
    if (! indiProp) return;
    ISwitchVectorProperty *svp = indiProp->property->getSwitch();
    int choice = combo->GetSelection();
    for (i=0; i<svp->nsp; i++){
       if (i == choice) {
	  svp->sp[i].s = ISS_ON;
       }
       else {
	  svp->sp[i].s = ISS_OFF;
       }
    }
    sendNewSwitch(svp);
}

void IndiGui::SetCheckboxEvent(wxCommandEvent & event)
{
    wxString b_name;
    int i;
    wxCheckBox *button = (wxCheckBox *)event.GetEventObject();
    if (! button) return;
    IndiProp *indiProp = (IndiProp *) button->GetClientData();
    if (! indiProp) return;
    ISwitchVectorProperty *svp = indiProp->property->getSwitch();
    ptrHash::iterator it;
    for( it = indiProp->ctrl.begin(); it !=indiProp->ctrl.end(); ++it )
    {
       wxString key = it->first;
       wxCheckBox *value = (wxCheckBox *) it->second;
       if (value == button) {
	  b_name = key;
	  break;
       }
    }
    for (i=0; i<svp->nsp; i++){
       if (svp->sp[i].name == b_name) {
	  svp->sp[i].s = button->GetValue() ? ISS_ON : ISS_OFF;
	  break;
       }
    }
    sendNewSwitch(svp);
}

void IndiGui::OnRemovePropertyFromThread(wxThreadEvent& event)
{
    IndiProp *indiProp = (IndiProp *)event.GetExtraLong();
    if (! indiProp) return;
    IndiDev *indiDev = (IndiDev *) indiProp->idev;
    if (! indiDev) return;
    wxString propname = indiProp->PropName;
    
    for (int y = 0; y < indiProp->gbs->GetRows(); y++) {
	for (int x = 0; x < indiProp->gbs->GetCols(); x++) {
	    wxGBSizerItem *item = indiProp->gbs->FindItemAtPosition(POS(y, x));
	    if (item){
	        indiProp->gbs->Remove(item->GetId());
		item->GetWindow()->Destroy();
	    }
	}
    } 
    indiProp->gbs->Layout();
    if (indiProp->name)
	indiProp->name->Destroy();
    if (indiProp->state)
	indiProp->state->Destroy();
    if (indiProp->panel)
	indiProp->panel->Destroy();
    if (indiProp->page->GetChildren().GetCount() == 0) {
	for (unsigned int i = 0; i < indiDev->page->GetPageCount(); i++) {
	    if (indiProp->page == indiDev->page->GetPage(i)) {
		indiDev->groups.erase(indiDev->page->GetPageText(i));
		indiDev->page->DeletePage(i);
		break;
	    }
	}
    }
    delete indiProp;
    indiDev->properties.erase(propname);
    indiDev->page->Layout();
    indiDev->page->Fit();
    sizer->Layout();
    Fit();
}

IndiGui::IndiGui() : wxDialog((wxDialog *)wxTheApp->GetTopWindow(), wxID_ANY,
                             _("INDI Options"),
			      wxDefaultPosition, wxSize(640, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DOUBLE | wxTAB_TRAVERSAL);
   sizer = new wxBoxSizer(wxVERTICAL);
   panel->SetSizer(sizer);
   parent_notebook = new wxNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
   sizer->Add(parent_notebook, 0, wxEXPAND | wxALL);
   textbuffer = new wxTextCtrl(panel, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
   sizer->Add(textbuffer, 1, wxFIXED_MINSIZE | wxEXPAND | wxALL);
}

void IndiGui::OnQuit(wxCloseEvent& WXUNUSED(event))
{
   if (child_window) {
	Show(false);
   } else {
	Destroy();
   }
}

IndiGui::~IndiGui()
{
    ready = false;   
    disconnectServer();
    ptrHash::iterator itdev;
    if (!devlist.empty()) {
    for( itdev = devlist.begin(); itdev !=devlist.end(); ++itdev )
    {
	IndiDev *indiDev = (IndiDev *) itdev->second;
	if (indiDev) {
	   ptrHash::iterator itprop;
	   for( itprop =indiDev->properties.begin(); itprop !=indiDev->properties.end(); ++itprop )
	    {
	       IndiProp *indiProp = (IndiProp *) itprop->second;
	       indiProp->ctrl.clear();
	       indiProp->entry.clear();
	       delete indiProp;
	    }
	    indiDev->properties.clear();
	    indiDev->groups.clear();
	    delete indiDev;
	}
    }
    } 
}     


