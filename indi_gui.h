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

#include "phdindiclient.h"
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>

#include "wxled.h"

#include <wx/wx.h>
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

WX_DECLARE_STRING_HASH_MAP(void *, PtrHash);

class IndiProp;

/*
 *  INDI gui windows
 */
class IndiGui : public wxDialog , public PhdIndiClient
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
    void SetButtonEvent(wxCommandEvent& event);
    void SetComboboxEvent(wxCommandEvent& event);
    void SetToggleButtonEvent(wxCommandEvent& event);
    void SetCheckboxEvent(wxCommandEvent& event);

    void OnQuit(wxCloseEvent& event);

    void ConnectServer(const wxString& INDIhost, long INDIport);
    bool allow_connect_disconnect;

    wxPanel *panel;
    wxBoxSizer *sizer;
    wxNotebook *parent_notebook;
    wxTextCtrl *textbuffer;
    wxLongLong m_lastUpdate;

    PtrHash devlist;

    bool m_deleted;
    IndiGui **m_holder;

    DECLARE_EVENT_TABLE()

    IndiGui();

protected:
    //////////////////////////////////////////////////////////////////////
    // Functions running in the INDI client thread
    //////////////////////////////////////////////////////////////////////
    void newDevice(INDI::BaseDevice *dp) override;
    void removeDevice(INDI::BaseDevice *dp) override {};
    void newProperty(INDI::Property *property) override;
    void removeProperty(INDI::Property *property) override;
    void newBLOB(IBLOB *bp) override {}
    void newSwitch(ISwitchVectorProperty *svp) override;
    void newNumber(INumberVectorProperty *nvp) override;
    void newMessage(INDI::BaseDevice *dp, int messageID) override;
    void newText(ITextVectorProperty *tvp) override;
    void newLight(ILightVectorProperty *lvp) override {}
    void serverConnected() override;
    void IndiServerDisconnected(int exit_code) override;

public:
    ~IndiGui();

    static void ShowIndiGui(IndiGui **ret, const wxString& host, long port, bool allow_connect_disconnect, bool modal);
    static void DestroyIndiGui(IndiGui **holder);
};

#endif //_INDIGUI_H_
