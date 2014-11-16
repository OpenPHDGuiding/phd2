/*
 *    indi_gui.h
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

#ifndef _INDIGUI_H_
#define _INDIGUI_H_

#include <libindi/baseclient.h>
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>

#include "wxled.h"

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/string.h>
#include <wx/hashmap.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/tglbtn.h>
#include <wx/choice.h>
#include <wx/menu.h>

WX_DECLARE_STRING_HASH_MAP( void *, ptrHash );

wxDECLARE_EVENT(INDIGUI_THREAD_NEWDEVICE_EVENT, wxThreadEvent);
wxDECLARE_EVENT(INDIGUI_THREAD_NEWPROPERTY_EVENT, wxThreadEvent);
wxDECLARE_EVENT(INDIGUI_THREAD_NEWNUMBER_EVENT, wxThreadEvent);
wxDECLARE_EVENT(INDIGUI_THREAD_NEWTEXT_EVENT, wxThreadEvent);
wxDECLARE_EVENT(INDIGUI_THREAD_NEWSWITCH_EVENT, wxThreadEvent);
wxDECLARE_EVENT(INDIGUI_THREAD_NEWMESSAGE_EVENT, wxThreadEvent);
wxDECLARE_EVENT(INDIGUI_THREAD_REMOVEPROPERTY_EVENT, wxThreadEvent);

/*
 *  Status LED
 */
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
      static const char indi_state[4][6] = {
	 "Idle",
	 "Ok",
	 "Busy",
	 "Alert",
      };
      switch(state) {
	 case IPS_IDLE:  SetColor("808080"); break;
	 case IPS_OK:    SetColor("008000"); break;
	 case IPS_BUSY:  SetColor("FFFF00"); break;
	 case IPS_ALERT: SetColor("FF0000"); break;
      }
      SetToolTip(wxString::FromAscii(indi_state[state]));
   }
};


enum {
   SWITCH_CHECKBOX,
   SWITCH_BUTTON,
   SWITCH_COMBOBOX,
};

enum {
   ID_Save = 1,
};

/*
 *  A device page and related properties
 */
class IndiDev
{
public:
   wxNotebook		*page;
   INDI::BaseDevice	*dp;
   ptrHash		groups;
   ptrHash		properties;
};

/*
 *  Property Information
 */
class IndiProp
{
public:
   ptrHash		ctrl;
   ptrHash		entry;
   IndiStatus		*state;
   wxStaticText		*name;
   wxPanel		*page;
   wxGridBagSizer	*gbs;
   INDI::Property	*property;
   IndiDev		*idev;
};

/*
 *  INDI gui windows
 */
class IndiGui : public wxFrame , public INDI::BaseClient
{

private:
   // Main thread events called from INDI thread
   void OnNewDeviceFromThread(wxThreadEvent& event);
   void OnNewPropertyFromThread(wxThreadEvent& event);
   void OnNewNumberFromThread(wxThreadEvent& event);
   void OnNewTextFromThread(wxThreadEvent& event);
   void OnNewSwitchFromThread(wxThreadEvent& event);
   void OnNewMessageFromThread(wxThreadEvent& event);
   void OnRemovePropertyFromThread(wxThreadEvent& event);
   
   // Widget creation 
   void BuildPropWidget(INDI::Property *property, wxPanel *parent, IndiProp *indiProp);
   void CreateTextWidget(INDI::Property *property, IndiProp *indiProp);
   void CreateSwitchWidget(INDI::Property *property, IndiProp *indiProp);
   void CreateNumberWidget(INDI::Property *property, IndiProp *indiProp);
   void CreateLightWidget(INDI::Property *property, IndiProp *indiProp);
   void CreateBlobWidget(INDI::Property *property, IndiProp *indiProp);
   void CreateUnknowWidget(INDI::Property *property, IndiProp *indiProp);
   // More switch stuff
   int GetSwitchType(ISwitchVectorProperty *svp);
   void CreateSwitchCombobox(ISwitchVectorProperty *svp, IndiProp *indiProp);
   void CreateSwitchCheckbox(ISwitchVectorProperty *svp, IndiProp *indiProp);
   void CreateSwitchButton(ISwitchVectorProperty *svp, IndiProp *indiProp);
   
   // Button events
   void SetButtonEvent(wxCommandEvent & event);
   void SetComboboxEvent(wxCommandEvent & event);
   void SetToggleButtonEvent(wxCommandEvent & event);
   void SetCheckboxEvent(wxCommandEvent & event);
   
   void OnQuit(wxCloseEvent& WXUNUSED(event));
   
   wxPanel *panel;
   wxBoxSizer *sizer;
   wxNotebook *parent_notebook;
   wxTextCtrl *textbuffer;
   
   ptrHash	devlist;
   bool		ready;
   
   DECLARE_EVENT_TABLE()
   
protected:
   //////////////////////////////////////////////////////////////////////
   // Functions running in the INDI client thread
   //////////////////////////////////////////////////////////////////////
   virtual void newDevice(INDI::BaseDevice *dp);
   virtual void newProperty(INDI::Property *property);
   virtual void removeProperty(INDI::Property *property);
   virtual void newBLOB(IBLOB *bp){}
   virtual void newSwitch(ISwitchVectorProperty *svp);
   virtual void newNumber(INumberVectorProperty *nvp);
   virtual void newMessage(INDI::BaseDevice *dp, int messageID);
   virtual void newText(ITextVectorProperty *tvp);
   virtual void newLight(ILightVectorProperty *lvp) {}
   virtual void serverConnected();
   virtual void serverDisconnected(int exit_code);
   
public:
   IndiGui();
   ~IndiGui();
   
   void ConnectServer(wxString INDIhost, long INDIport);
   void ShowMessage(const char *message);
   void DeleteProp(INDI::Property *property);
   bool child_window; 
   bool allow_connect_disconnect;
};

#endif //_INDIGUI_H_
