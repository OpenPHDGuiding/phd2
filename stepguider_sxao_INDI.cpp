/*
 *  stepguider_sxao_INDI.cpp
 *  PHD Guiding
 *
 *  Created by Hans Lambermont
 *  Copyright (c) 2016 Hans Lambermont
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
 *    Neither the name Craig Stark, Stark Labs nor the names of its
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

#ifdef STEPGUIDER_SXAO_INDI

StepGuiderSxAoINDI::StepGuiderSxAoINDI(void)
{
    ClearStatus();

    // load the values from the current profile
    INDIhost   = pConfig->Profile.GetString("/indi/INDIhost", _T("localhost"));
    INDIport   = pConfig->Profile.GetLong("/indi/INDIport", 7624);
    INDIaoDeviceName = pConfig->Profile.GetString("/indi/INDIao", _T("INDI SVX-AO-LF"));
    INDIaoDevicePort = pConfig->Profile.GetString("/indi/INDIao_port",_T("/dev/sx-ao-lf"));

    m_Name = INDIaoDeviceName;
    m_maxSteps = pConfig->Profile.GetInt("/stepguider/sxao/MaxSteps", DefaultMaxSteps);
}

StepGuiderSxAoINDI::~StepGuiderSxAoINDI(void)
{
	ready = false;
	Disconnect();
}

void StepGuiderSxAoINDI::ClearStatus(void)
{
	// reset properties
    pulseGuideNS_prop = NULL;
    pulseN_prop = NULL;
    pulseS_prop = NULL;
    pulseGuideWE_prop = NULL;
    pulseW_prop = NULL;
    pulseE_prop = NULL;
    aoNS_prop = NULL;
    aoN_prop = NULL;
    aoS_prop = NULL;
    aoWE_prop = NULL;
    aoW_prop = NULL;
    aoE_prop = NULL;
    aoCenterUnjam_prop = NULL;
    aoCenter_prop = NULL;
    aoUnjam_prop = NULL;
    ao_device = NULL;
    ao_port = NULL;
    SxAoVersion = -1;
	// reset connection status
    ready = false;
}

void StepGuiderSxAoINDI::CheckState(void)
{
    // Check if the device has all the required properties for our usage.
    if (IsConnected() && (aoN_prop && aoS_prop && aoW_prop && aoE_prop && aoCenter_prop)) {
        if (! ready) {
        	Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::CheckState is ready"));
            ready = true;
            if (modal) {
                modal = false;
            }
        }
    }
}

void StepGuiderSxAoINDI::newDevice(INDI::BaseDevice *dp)
{
    if (strcmp(dp->getDeviceName(), INDIaoDeviceName.mb_str(wxConvUTF8)) == 0) {
    	ao_device = dp;
    }
}
void StepGuiderSxAoINDI::removeDevice(INDI::BaseDevice *dp) {}

void StepGuiderSxAoINDI::newProperty(INDI::Property *property)
{
    const char* PropName = property->getName();
    #ifdef INDI_PRE_1_1_0
      INDI_TYPE Proptype = property->getType();
    #else
      INDI_PROPERTY_TYPE Proptype = property->getType();
    #endif

    /*
    printf("SXAO PropName: %s Proptype: %d\n", PropName, Proptype);
    SXAO PropName: CONNECTION Proptype: 1
    SXAO PropName: DRIVER_INFO Proptype: 2
    SXAO PropName: DEBUG Proptype: 1
    SXAO PropName: SIMULATION Proptype: 1
    SXAO PropName: DEVICE_PORT Proptype: 2
    SXAO PropName: CONFIG_PROCESS Proptype: 1
    SXAO PropName: TELESCOPE_TIMED_GUIDE_NS Proptype: 0
    SXAO PropName: TELESCOPE_TIMED_GUIDE_WE Proptype: 0
    SXAO PropName: AO_NS Proptype: 0
    SXAO PropName: AO_WE Proptype: 0
    SXAO PropName: AO_CENTER Proptype: 1
    */

    if (strcmp(PropName, "CONNECTION") == 0 && Proptype == INDI_SWITCH) {
    	ISwitch *connectswitch = IUFindSwitch(property->getSwitch(),"CONNECT");
    	if (connectswitch->s == ISS_ON) {
    		StepGuider::Connect();
    	}
    } else if (strcmp(PropName, "DEVICE_PORT") == 0 && Proptype == INDI_TEXT) {
    	ao_port = property->getText();
    } else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_NS") == 0) && Proptype == INDI_NUMBER){
    	pulseGuideNS_prop = property->getNumber();
    	pulseN_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_N");
    	pulseS_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_S");
    } else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_WE") == 0) && Proptype == INDI_NUMBER){
    	pulseGuideWE_prop = property->getNumber();
    	pulseW_prop = IUFindNumber(pulseGuideWE_prop,"TIMED_GUIDE_W");
    	pulseE_prop = IUFindNumber(pulseGuideWE_prop,"TIMED_GUIDE_E");
    } else if ((strcmp(PropName, "AO_NS") == 0) && Proptype == INDI_NUMBER){
    	aoNS_prop = property->getNumber();
    	aoN_prop = IUFindNumber(aoNS_prop,"AO_N");
    	aoS_prop = IUFindNumber(aoNS_prop,"AO_S");
    } else if ((strcmp(PropName, "AO_WE") == 0) && Proptype == INDI_NUMBER){
    	aoWE_prop = property->getNumber();
    	aoW_prop = IUFindNumber(aoWE_prop,"AO_W");
    	aoE_prop = IUFindNumber(aoWE_prop,"AO_E");
    } else if (strcmp(PropName, "AO_CENTER") == 0 && Proptype == INDI_SWITCH) {
    	aoCenterUnjam_prop = property->getSwitch();
    	aoCenter_prop = IUFindSwitch(aoCenterUnjam_prop,"CENTER");
    	aoUnjam_prop  = IUFindSwitch(aoCenterUnjam_prop,"UNJAM");
    }

    CheckState();
}

void StepGuiderSxAoINDI::newNumber(INumberVectorProperty *nvp) {}
void StepGuiderSxAoINDI::newMessage(INDI::BaseDevice *dp, int messageID) {}


bool StepGuiderSxAoINDI::Connect(void)
{
	if (INDIaoDeviceName == wxT("INDI SVX-AO-LF")) {
		SetupDialog(); // If not configured open the setup dialog
	}
	setServer(INDIhost.mb_str(wxConvUTF8), INDIport); // define server to connect to.
	watchDevice(INDIaoDeviceName.mb_str(wxConvUTF8)); // Receive messages only for our device.
	Debug.AddLine(wxString::Format("Connecting to INDI server %s on port %d, device %s", INDIhost.mb_str(wxConvUTF8), INDIport, INDIaoDeviceName.mb_str(wxConvUTF8)));
	if (connectServer() ) { // Connect to INDI server.
		return false; // and wait for serverConnected event
	}
	return true;
}

bool StepGuiderSxAoINDI::Disconnect(void)
{
    if (disconnectServer() ) { // Disconnect from INDI server
        if (ready) {
            Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::Disconnect"));
    	    ready = false;
    	    StepGuider::Disconnect();
        }
        return false;
    } else return true;
}

bool StepGuiderSxAoINDI::HasSetupDialog(void) const
{
    return true;
}

void StepGuiderSxAoINDI::SetupDialog()
{
    // show the server and device configuration
    INDIConfig *indiDlg = new INDIConfig(wxGetActiveWindow(),TYPE_AO);
    indiDlg->INDIhost = INDIhost;
    indiDlg->INDIport = INDIport;
    indiDlg->INDIDevName = INDIaoDeviceName;
    indiDlg->INDIDevPort = INDIaoDevicePort;
    // initialize with actual values
    indiDlg->SetSettings();
    // try to connect to server
    indiDlg->Connect();
    if (indiDlg->ShowModal() == wxID_OK) {
        // if OK save the values to the current profile
        indiDlg->SaveSettings();
        INDIhost = indiDlg->INDIhost;
        INDIport = indiDlg->INDIport;
        INDIaoDeviceName = indiDlg->INDIDevName;
        INDIaoDevicePort = indiDlg->INDIDevPort;
        pConfig->Profile.SetString("/indi/INDIhost", INDIhost);
        pConfig->Profile.SetLong("/indi/INDIport", INDIport);
        pConfig->Profile.SetString("/indi/INDIao", INDIaoDeviceName);
        pConfig->Profile.SetString("/indi/INDIao_port",INDIaoDevicePort);
        m_Name = INDIaoDeviceName;
    }
    indiDlg->Disconnect();
    indiDlg->Destroy();
    delete indiDlg;
}

void StepGuiderSxAoINDI::ShowPropertyDialog(void)
{
	SetupDialog();
}

void StepGuiderSxAoINDI::serverConnected(void)
{
	// wait for the DEVICE_PORT property
	modal = true;
	wxLongLong msec;
	msec = wxGetUTCTimeMillis();
	while ( (! ao_port) && wxGetUTCTimeMillis() - msec < MaxDeviceInitWaitMilliSeconds) {
		::wxSafeYield();
	}
	// connect to the device, first set its port
    if (ao_port && INDIaoDevicePort.Length()) {
        char* porttext = (const_cast<char*>((const char*)INDIaoDevicePort.mb_str()));
        ao_port->tp->text = porttext;
        sendNewText(ao_port);
    }
    connectDevice(INDIaoDeviceName.mb_str(wxConvUTF8));

    // wait for all defined properties in CheckState
    msec = wxGetUTCTimeMillis();
    while (modal && wxGetUTCTimeMillis() - msec < MaxDevicePropertiesWaitMilliSeconds) {
        ::wxSafeYield();
    }
    modal = false; // even if CheckState still says no

    if (ready) {
		try {
			Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::serverConnected connecting StepGuider"));
			StepGuider::Connect();
		} catch (wxString Msg) {
			POSSIBLY_UNUSED(Msg);
		}
    } else {
    	Disconnect();
    }
}

void StepGuiderSxAoINDI::serverDisconnected(int exit_code)
{
    // in case the connection is lost we must reset the client socket
    Disconnect();
    if (ready) {
        Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::serverDisconnected disconnecting StepGuider"));
        ready = false;
        StepGuider::Disconnect();
    }
    // after disconnection we reset the connection status and the properties pointers
    ClearStatus();
}

bool StepGuiderSxAoINDI::Step(GUIDE_DIRECTION direction, int steps)
{
    bool bError = true;
    if (aoNS_prop && aoWE_prop) {
        bError = false;
		try {
			unsigned char parameter;
			switch (direction) {
			case NORTH:
				aoN_prop->value = steps;
				aoS_prop->value = 0;
				sendNewNumber(aoNS_prop);
				break;
			case SOUTH:
				aoN_prop->value = 0;
				aoS_prop->value = steps;
				sendNewNumber(aoNS_prop);
				break;
			case EAST:
				aoW_prop->value = 0;
				aoE_prop->value = steps;
				sendNewNumber(aoWE_prop);
				break;
			case WEST:
				aoW_prop->value = steps;
				aoE_prop->value = 0;
				sendNewNumber(aoWE_prop);
				break;
			default:
				throw ERROR_INFO("StepGuiderSxAO::step: invalid direction");
				break;
			}
		} catch (wxString Msg) {
			POSSIBLY_UNUSED(Msg);
			bError = true;
		}
    }

    return bError;
}

int StepGuiderSxAoINDI::MaxPosition(GUIDE_DIRECTION direction) const
{
	Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::MaxPosition %d", m_maxSteps));
    return m_maxSteps;
}

bool StepGuiderSxAoINDI::IsAtLimit(GUIDE_DIRECTION direction, bool *isAtLimit)
{
	Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::IsAtLimit TODO"));
	// TODO https://sourceforge.net/p/indi/feature-requests/7/
	return false;
}

bool StepGuiderSxAoINDI::FirmwareVersion(unsigned int *version)
{
	Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::FirmwareVersion TODO"));
	// TODO https://sourceforge.net/p/indi/feature-requests/6/
	return false;
}

bool StepGuiderSxAoINDI::Unjam(void)
{
	Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::Unjam"));
    if (aoCenterUnjam_prop) {
	    aoUnjam_prop->s = ISS_ON;
	    sendNewSwitch(aoCenterUnjam_prop);
	    return false;
    }
	return true;
}

bool StepGuiderSxAoINDI::Center(void)
{
	Debug.AddLine(wxString::Format("StepGuiderSxAoINDI::Center"));
    if (aoCenterUnjam_prop) {
	    aoCenter_prop->s = ISS_ON;
	    sendNewSwitch(aoCenterUnjam_prop);
	    return false;
    }
	return true;
}

bool StepGuiderSxAoINDI::Center(unsigned char cmd)
{
	return Center();
}

bool StepGuiderSxAoINDI::ST4HasGuideOutput(void)
{
	return true;
}

bool StepGuiderSxAoINDI::ST4HostConnected(void)
{
	return IsConnected();
}

bool StepGuiderSxAoINDI::ST4HasNonGuiMove(void)
{
	return true;
}

bool StepGuiderSxAoINDI::ST4PulseGuideScope(int direction, int duration)
{
    bool bError = true;
    if (pulseGuideNS_prop && pulseGuideWE_prop) {
        bError = false;
		try {
			unsigned char parameter;
			switch (direction) {
			case NORTH:
				pulseN_prop->value = duration;
				pulseS_prop->value = 0;
				sendNewNumber(pulseGuideNS_prop);
				break;
			case SOUTH:
				pulseN_prop->value = 0;
				pulseS_prop->value = duration;
				sendNewNumber(pulseGuideNS_prop);
				break;
			case EAST:
				pulseW_prop->value = 0;
				pulseE_prop->value = duration;
				sendNewNumber(pulseGuideWE_prop);
				break;
			case WEST:
				pulseW_prop->value = duration;
				pulseE_prop->value = 0;
				sendNewNumber(pulseGuideWE_prop);
				break;
			default:
				throw ERROR_INFO("StepGuiderSxAO::ST4PulseGuideScope: invalid direction");
				break;
			}
		} catch (wxString Msg) {
			POSSIBLY_UNUSED(Msg);
			bError = true;
		}
    }

    return bError;
}

#endif // #ifdef STEPGUIDER_SXAO_INDI
