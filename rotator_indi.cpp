/*
*  rotator_indi.cpp
*  PHD Guiding
*
*  Created by Philipp Weber
*  Copyright (c) 2023 Philipp Weber
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

#ifdef ROTATOR_INDI

#include "rotator_indi.h"
#include "rotator.h"
#include "config_indi.h"
#include "indi_gui.h"
#include "phdindiclient.h"

#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>


class RotatorINDI : public Rotator, public PhdIndiClient {
	public:
		RotatorINDI();
		~RotatorINDI();
        bool Connect() override;
        bool Disconnect() override;
		wxString Name() const override;
		float Position() const override;

	private:
        static const int MaxDeviceInitWaitMilliSeconds = 2000;
        static const int MaxDevicePropertiesWaitMilliSeconds = 5000;
        long          INDIport;
        wxString      INDIhost;
        wxString      INDIRotatorDeviceName;
        volatile bool modal;
        bool          m_ready = false;
		float         m_angle = POSITION_UNKNOWN;

		INumberVectorProperty *angle_prop;
        ISwitchVectorProperty *connection_prop;

    	IndiGui  *m_gui;

    	wxString INDIRotatorName;
		wxString m_Name;

		bool ConnectToDriver(RunInBg *ctx);
        void ClearStatus();
        void CheckState();
		void RotatorDialog();
		void RotatorSetup();
		void updateAngle();

	protected:
        void IndiServerConnected() override;
        void IndiServerDisconnected(int exit_code) override;
        void newDevice(INDI::BaseDevice *dp) override;
        void removeDevice(INDI::BaseDevice *dp) override;
        void newProperty(INDI::Property *property) override;
        void newSwitch(ISwitchVectorProperty *svp) override;
        void newNumber(INumberVectorProperty *nvp) override;
        void newMessage(INDI::BaseDevice *dp, int messageID) override;

        void removeProperty(INDI::Property *property) override {};
        void newBLOB(IBLOB *bp) override {};
        void newText(ITextVectorProperty *tvp) override {};
        void newLight(ILightVectorProperty *lvp) override {};

	private:
		void ShowPropertyDialog() override;
};

void RotatorINDI::ClearStatus() {
	angle_prop = nullptr;
	connection_prop = nullptr;
	m_ready = false;
	m_angle = POSITION_UNKNOWN;
}

void RotatorINDI::CheckState() {
	if ( ! IsConnected() ) {
		return;
	}
	if ( m_ready ) {
		return;
	}
	if ( ! angle_prop ) {
		return;
	}
	
	Debug.Write(wxString(_("INDI Rotator is ready\n")));
	m_ready = true;
	if ( modal ) {
		modal = false;
	}
}

bool RotatorINDI::Connect() {
	if ( INDIRotatorName == wxT("INDI Rotator") ) {
		RotatorSetup();
	}
	Debug.Write(wxString::Format("INDI Rotator connecting to device [%s]\n", INDIRotatorName));
	setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
	watchDevice(INDIRotatorName.mb_str(wxConvUTF8));
	if ( connectServer() ) {
		Debug.Write(wxString::Format("INDI Rotator: connectServer done ready = %d\n", m_ready));
		return !m_ready;
	}
	RotatorSetup();
	setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
	watchDevice(INDIRotatorName.mb_str(wxConvUTF8));
	if ( connectServer() ) {
		Debug.Write(wxString::Format("INDI Rotator: connectServer [2] done ready = %d\n", m_ready));
	}
	return true;
}

RotatorINDI::RotatorINDI() : m_gui(nullptr) {
	ClearStatus();
    INDIhost = pConfig->Profile.GetString("/indi/INDIhost", _T("localhost"));
    INDIport = pConfig->Profile.GetLong("/indi/INDIport", 7624);
    INDIRotatorName = pConfig->Profile.GetString("/indi/INDIrotator", _T("INDI Rotator"));
	m_Name = wxString::Format("INDI Rotator [%s]", INDIRotatorName);
}

RotatorINDI::~RotatorINDI() {
	if ( m_gui ) {
		IndiGui::DestroyIndiGui(&m_gui);
	}
	DisconnectIndiServer();
}

void RotatorINDI::IndiServerConnected() {
	struct ConnectInBg : public ConnectRotatorInBg {
		RotatorINDI *rotator;
		ConnectInBg(RotatorINDI *rotator_) : rotator(rotator_) { }
		bool Entry() {
			return ! rotator->ConnectToDriver(this);
		}
	};
	ConnectInBg bg(this);

	if ( bg.Run() ) {
		Debug.Write(wxString::Format("INDI Rotator bg connection failed canceled=%d\n", bg.IsCanceled()));
		pFrame->Alert(wxString::Format(_("Cannot connect to rotator %s: %s"), INDIRotatorName, bg.GetErrorMsg()));
		Disconnect();
	} else {
		Debug.Write("INDI Rotator bg connection succeeded\n");
		Rotator::Connect();
	}
}

void RotatorINDI::IndiServerDisconnected(int exit_code) {
	ClearStatus();
	if ( exit_code == -1 ) {
		pFrame->Alert(wxString(_("INDI server disconnected")));
		Disconnect();
	}
}

void RotatorINDI::removeDevice(INDI::BaseDevice *dp) {
	ClearStatus();
	Disconnect();
}

void RotatorINDI::ShowPropertyDialog() {
	if ( IsConnected() ) {
		RotatorDialog();
	} else {
		RotatorSetup();
	}
}

void RotatorINDI::RotatorDialog() {
	if ( m_gui ) {
		m_gui->Show();
	} else {
		IndiGui::ShowIndiGui(&m_gui, INDIhost, INDIport, false, false);
	}
}

void RotatorINDI::RotatorSetup() {
	INDIConfig indiDlg(wxGetApp().GetTopWindow(), _("INDI Rotator Selection"), INDI_TYPE_ROTATOR);
	indiDlg.INDIhost = INDIhost;
	indiDlg.INDIport = INDIport;
	indiDlg.INDIDevName = INDIRotatorName;
	indiDlg.SetSettings();
	indiDlg.Connect();
	if ( indiDlg.ShowModal() == wxID_OK ) {
		indiDlg.SaveSettings();
		INDIhost = indiDlg.INDIhost;
        INDIport = indiDlg.INDIport;
		INDIRotatorName = indiDlg.INDIDevName;
		pConfig->Profile.SetString("/indi/INDIhost", INDIhost);
        pConfig->Profile.SetLong("/indi/INDIport", INDIport);
        pConfig->Profile.SetString("/indi/INDIrotator", INDIRotatorName);
		m_Name = INDIRotatorName;
	}
	indiDlg.Disconnect();
}

wxString RotatorINDI::Name() const {
	return m_Name;
}

float RotatorINDI::Position() const {
	return m_angle;
}

void RotatorINDI::updateAngle() {
	if ( ! IsConnected() ) {
		m_angle = POSITION_UNKNOWN;
	}
	m_angle = IUFindNumber(angle_prop, "ANGLE")->value;
}

bool RotatorINDI::ConnectToDriver(RunInBg *r) {
	modal = true;
	wxLongLong msec = wxGetUTCTimeMillis();

	while ( ! connection_prop && wxGetUTCTimeMillis() - msec < 15 * 1000 ) {
		if ( r->IsCanceled() ) {
			modal = false;
			return false;
		}
		wxMilliSleep(20);
	}
	if ( ! connection_prop ) {
		r->SetErrorMsg(_("connection timed-out"));
		modal = false;
		return false;
	}
	connectDevice(INDIRotatorName.mb_str(wxConvUTF8));

	// If the ABS_ROTATOR_ANGLE property comes already when connecting to the
	// server (like an already connected simulator) we're ready early. If we 
	// don't bail out here the 30 seconds below will just pass for no apparent
	// reason
	if ( m_ready ) {
		modal = false;
		updateAngle();
		return m_ready;
	}

	msec = wxGetUTCTimeMillis();
	while ( modal && wxGetUTCTimeMillis() - msec < 30 * 1000 ) {
		if ( r->IsCanceled() ) {
			modal = false;
			return false;
		}
		wxMilliSleep(20);
	}
	if ( ! m_ready ) {
		r->SetErrorMsg(_("connection timed-out"));
	}
	modal = false;
    updateAngle();
	return m_ready;
}

bool RotatorINDI::Disconnect() {
	DisconnectIndiServer();
	ClearStatus();
	Rotator::Disconnect();
	return false;
}

void RotatorINDI::newProperty(INDI::Property *property) {
	wxString PropName(property->getName());
	INDI_PROPERTY_TYPE Proptype = property->getType();

	Debug.Write(wxString::Format("INDI Rotator: Received property: %s\n", PropName));

	if ( Proptype == INDI_NUMBER && PropName == "ABS_ROTATOR_ANGLE" ) {
		if ( INDIConfig::Verbose() ) {
			Debug.Write(wxString::Format(_("INDI Rotator found ABS_ROTATOR_ANGLE for %s %s\n"),
							property->getDeviceName(), PropName));
		}
		angle_prop = property->getNumber();
	}
	if ( Proptype == INDI_SWITCH && PropName == "CONNECTION" ) {
		if ( INDIConfig::Verbose() ) {
			Debug.Write(wxString::Format(_("INDI Rotator found CONNECTION for %s %s\n"),
						property->getDeviceName(), PropName));
		}
		connection_prop = property->getSwitch();
		ISwitch *connectswitch = IUFindSwitch(connection_prop, "CONNECT");
		if ( connectswitch->s == ISS_ON ) {
			Rotator::Connect();
		}
	}
	CheckState();
}

void RotatorINDI::newMessage(INDI::BaseDevice *dp, int messageID) {
	if ( INDIConfig::Verbose() ) {
		Debug.Write(wxString::Format(_("INDI Rotator received message: %s\n"), dp->messageQueue(messageID)));
	}
}

void RotatorINDI::newNumber(INumberVectorProperty *nvp) {
	if ( INDIConfig::Verbose() ) {
		Debug.Write(wxString::Format(_("INDI Rotator: New number: %s\n"), nvp->name));
	}
	if ( strcmp(nvp->name, "ABS_ROTATOR_ANGLE") == 0 ) {
		updateAngle();
	}
}

void RotatorINDI::newDevice(INDI::BaseDevice *dp) {
	if ( INDIConfig::Verbose() ) {
    	Debug.Write(wxString::Format("INDI Rotator new device %s\n", dp->getDeviceName()));
	}
}

void RotatorINDI::newSwitch(ISwitchVectorProperty *svp) {
	if ( INDIConfig::Verbose() ) {
		Debug.Write(wxString::Format("INDI Rotator: Receiving Switch: %s = %i\n",
					svp->name, svp->sp->s));
	}
	if ( strcmp(svp->name, "CONNECTION") == 0 ) {
		ISwitch *connectswitch = IUFindSwitch(svp, "CONNECT");
		if ( connectswitch->s == ISS_ON ) {
			Rotator::Connect();
		} else {
			if ( m_ready ) {
				ClearStatus();
				PhdApp::ExecInMainThread(
						[this]() {
						pFrame->Alert(_("INDI rotator was disconnected"));
						Disconnect();
						});
			}
		}
	}
}

Rotator *INDIRotatorFactory::MakeINDIRotator() {
	return new RotatorINDI();
}

#endif
