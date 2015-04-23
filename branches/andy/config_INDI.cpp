/*
 *  config_INDI.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark
 *  Copyright (c) 2006, 2007, 2008, 2009 Craig Stark.
 *  All rights reserved.
 *
 *  Redraw for libindi/baseclient by Patrick Chevalley
 *  Copyright (c) 2014 Patrick Chevalley
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
#include "camera.h"
#include "scope.h"


#if defined (INDI_CAMERA) || defined (GUIDE_INDI)

#include "config_INDI.h"

#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/combobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

#define MCONNECT 101
#define MINDIGUI 102

#define POS(r, c)        wxGBPosition(r,c)
#define SPAN(r, c)       wxGBSpan(r,c)

INDIConfig::INDIConfig(wxWindow *parent, int devtype) : wxDialog(parent, wxID_ANY, (const wxString)_T("INDI Configuration"))
{
    dev_type = devtype;
    gui = NULL;
    
    int pos;
    wxGridBagSizer *gbs = new wxGridBagSizer(0, 20);
    wxBoxSizer *sizer;

    pos = 0;
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Hostname:")),
	     POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxALIGN_CENTER_VERTICAL);
    host = new wxTextCtrl(this, wxID_ANY);
    gbs->Add(host, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    pos ++;
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Port:")),
	     POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxALIGN_CENTER_VERTICAL);
    port = new wxTextCtrl(this, wxID_ANY);
    gbs->Add(port, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);
    
    pos ++;
    connect_status = new wxStaticText(this, wxID_ANY, _T("Disconnected"));
    gbs->Add(connect_status,POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxALIGN_CENTER_VERTICAL);
    gbs->Add(new wxButton(this, MCONNECT, _T("Connect")), POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);
    
    pos ++;
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("========")),
	     POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    devlabel = new wxStaticText(this, wxID_ANY, _T("Device"));
    gbs->Add(devlabel,POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL);
    if (devtype == TYPE_CAMERA) {
	devlabel->SetLabel(_T("Camera"));
    }
    else if (devtype == TYPE_MOUNT) {
	devlabel->SetLabel(_T("Mount"));
    }
    
    pos ++;
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Driver:")),
	     POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxALIGN_CENTER_VERTICAL);
    dev =  new wxComboBox(this, wxID_ANY, _T(""));
    gbs->Add(dev, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    if (devtype == TYPE_CAMERA) {
	pos ++;
	gbs->Add(new wxStaticText(this, wxID_ANY, _T("CCD:")),
		 POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxALIGN_CENTER_VERTICAL);
	ccd =  new wxComboBox(this, wxID_ANY, _T(""));
	gbs->Add(ccd, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);
	ccd->Append(_T("Main imager"));
	ccd->Append(_T("Guider"));
    }
    
    pos ++;
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Port:")),
	     POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxALIGN_CENTER_VERTICAL);
    devport = new wxTextCtrl(this, wxID_ANY);
    gbs->Add(devport, POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    pos ++;
    gbs->Add(new wxStaticText(this, wxID_ANY, _T("Other options")),
	     POS(pos, 0), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxALIGN_CENTER_VERTICAL);
    gbs->Add(new wxButton(this, MINDIGUI, _T("INDI")), POS(pos, 1), SPAN(1, 1), wxALIGN_LEFT | wxALL | wxEXPAND);

    sizer = new wxBoxSizer(wxVERTICAL) ;
    sizer->Add(gbs);
    sizer->Add(CreateButtonSizer(wxOK | wxCANCEL));
    SetSizer(sizer);
    sizer->SetSizeHints(this) ;
    sizer->Fit(this) ;
}
BEGIN_EVENT_TABLE(INDIConfig, wxDialog)
EVT_BUTTON(MCONNECT, INDIConfig::OnConnectButton)
EVT_BUTTON(MINDIGUI, INDIConfig::OnIndiGui)
END_EVENT_TABLE()

INDIConfig::~INDIConfig()
{
    disconnectServer();    
}

void INDIConfig::OnIndiGui(wxCommandEvent& WXUNUSED(event)) 
{
    if (gui) {
	gui->ShowModal();
    }
    else {
	gui = new IndiGui();
	gui->child_window = true;
	gui->allow_connect_disconnect = true;
	gui->ConnectServer(INDIhost, INDIport);
	gui->ShowModal();
    }    
}

void INDIConfig::OnConnectButton(wxCommandEvent& WXUNUSED(event)) 
{
    Connect();
}

void INDIConfig::Connect()
{
    dev->Clear();
    Disconnect();
    INDIhost = host->GetLineText(0);
    port->GetLineText(0).ToLong(&INDIport);
    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
    if (connectServer()) {
	connect_status->SetLabel(_T("Connected"));
    }
}

void INDIConfig::Disconnect() 
{
   disconnectServer();
   connect_status->SetLabel(_T("Disconnected"));
   gui = NULL;
}

void INDIConfig::newDevice(INDI::BaseDevice *dp)
{
   const char * d_name = dp->getDeviceName();
   dev->Append(d_name);
   if (strcmp(d_name, INDIDevName) == 0){
      dev->SetValue(INDIDevName);
   }
}

void INDIConfig::SetSettings()
{
    char str[80];
    sprintf(str, "%d", (int)INDIport);
    port->WriteText(str);
    host->WriteText(INDIhost);
    dev->SetValue(INDIDevName);
    devport->SetValue(INDIDevPort);
    if (dev_type == TYPE_CAMERA) {
	ccd->SetSelection(INDIDevCCD);
    }
}

void INDIConfig::SaveSettings()
{
    INDIhost = host->GetLineText(0);
    port->GetLineText(0).ToLong(&INDIport);
    INDIDevName = dev->GetValue();
    INDIDevPort = devport->GetValue();
    if (dev_type == TYPE_CAMERA) {
	INDIDevCCD = ccd->GetSelection();
    }
}

#endif
