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


class IndiGui : public wxFrame , INDI::BaseClient
{

private:
   void CreateTextWidget(INDI::Property *property, int num_props);
   void CreateSwitchWidget(INDI::Property *property, int num_props);
   void CreateNumberWidget(INDI::Property *property, int num_props);
   void CreateLightWidget(INDI::Property *property, int num_props);
   void CreateBlobWidget(INDI::Property *property, int num_props);
   
   //More switch stuff
   int GetSwitchType(INDI::Property *property);
   void CreateSwitchCombobox(INDI::Property *property, int num_props);
   void CreateSwitchCheckbox(INDI::Property *property, int num_props);
   void CreateSwitchButton(INDI::Property *property, int num_props);
   
   void BuildPropWidget(INDI::Property *property, wxPanel *parent);
   
   void SetButtonEvent(wxCommandEvent & event);
   void SetComboboxEvent(wxCommandEvent & event);
   void SetToggleButtonEvent(wxCommandEvent & event);
   void SetCheckboxEvent(wxCommandEvent & event);
   void OnQuit(wxCloseEvent& WXUNUSED(event));
   void SaveDialog(wxCommandEvent& WXUNUSED(event));
   
   wxPanel *panel;
   wxBoxSizer *sizer;
   wxNotebook *parent_notebook;
   wxTextCtrl *textbuffer;
   
   struct indi_t *indi;
   DECLARE_EVENT_TABLE()
   
protected:
   virtual void newDevice(INDI::BaseDevice *dp){}
   virtual void newProperty(INDI::Property *property){}
   virtual void removeProperty(INDI::Property *property) {}
   virtual void newBLOB(IBLOB *bp){}
   virtual void newSwitch(ISwitchVectorProperty *svp){}
   virtual void newNumber(INumberVectorProperty *nvp){}
   virtual void newMessage(INDI::BaseDevice *dp, int messageID){}
   virtual void newText(ITextVectorProperty *tvp){}
   virtual void newLight(ILightVectorProperty *lvp) {}
   virtual void serverConnected(){}
   virtual void serverDisconnected(int exit_code){}
   
public:
   IndiGui();
   //~IndiGui();
   
   void MakeDevicePage(INDI::BaseDevice *dp);
   void UpdateWidget(INDI::Property *property);
   void ShowMessage(const char *message);
   void AddProp(INDI::BaseDevice *dp, const wxString groupname, INDI::Property *property);
   void DeleteProp(INDI::Property *property);
   bool child_window;   
};

#endif //_INDIGUI_H_
