/*
 *  config_VIDEODEVICE.cpp
 *  PHD Guiding
 *
 *  Created by Steffen Elste
 *  Copyright (c) 2010 Steffen Elste.
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

#ifndef CONFIG_VIDEODEVICE_H_INCLUDED
#define CONFIG_VIDEODEVICE_H_INCLUDED

#include "v4lcontrol.h"

#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/control.h>
#include <wx/dialog.h>


class V4LPropertiesDialog: public wxDialog {
public:
	V4LPropertiesDialog(V4LControlMap &map);
	~V4LPropertiesDialog() {};

protected:
	void onUpdate(wxCommandEvent& event);
	void onReset(wxCommandEvent& event);

private:
	V4LControlMap controlMap;
};

class BooleanControl : public wxCheckBox {
public:
	BooleanControl(wxWindow* parent, wxWindowID id, const wxString& label);

	void onUpdate(wxCommandEvent& event);
	void onReset(wxCommandEvent& event);
};

class IntegerControl : public wxSpinCtrl {
public:
	IntegerControl(wxWindow* parent, wxWindowID id);

	void onUpdate(wxCommandEvent& event);
	void onReset(wxCommandEvent& event);
};

class MenueControl : public wxChoice {
public:
	MenueControl(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, const wxArrayString& choices);

	void onUpdate(wxCommandEvent& event);
	void onReset(wxCommandEvent& event);
};

#endif // CONFIG_VIDEODEVICE_H_INCLUDED
