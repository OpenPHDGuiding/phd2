/*
 *  scope_INDI.cpp
 *  PHD Guiding
 *
 *  Ported by Hans Lambermont in 2014 from tele_INDI.cpp which has Copyright (c) 2009 Geoffrey Hausheer.
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

#ifdef GUIDE_INDI

#include "config_INDI.h"

ScopeINDI::ScopeINDI() 
{
    m_Name = wxString("INDI Mount");
    coord_set_prop = NULL;
    abort_prop = NULL;
    moveNS = NULL;
    moveEW = NULL;
    pulseGuideNS = NULL;
    pulseGuideEW = NULL;
    scope_device = NULL;
    ready = false;
    INDIhost = pConfig->Profile.GetString("/indi/INDIhost", _T("localhost"));
    INDIport = pConfig->Profile.GetLong("/indi/INDIport", 7624);
    INDIMountName = pConfig->Profile.GetString("/indi/INDImount", _T("INDI Mount"));
    INDIMountPort = pConfig->Profile.GetString("/indi/INDImount_port",_T(""));
}

void ScopeINDI::CheckState() 
{
//printf("entering ScopeINDI::CheckState\n");
    if(IsConnected() && (
	(MotionRate && moveNS && moveEW) ||
        (pulseGuideNS && pulseGuideEW)))
    {
        if (! ready) {
            printf("Telescope is ready\n");
            ready = true;
            if (modal) {
                modal = false;
            }
        }
    }
}

bool ScopeINDI::HasSetupDialog(void) const
{
    return true;
}

void ScopeINDI::SetupDialog() 
{
    INDIConfig *indiDlg = new INDIConfig(wxGetActiveWindow());
    indiDlg->DevName = _T("Mount");
    indiDlg->INDIhost = INDIhost;
    indiDlg->INDIport = INDIport;
    indiDlg->INDIDevName = INDIMountName;
    indiDlg->INDIDevPort = INDIMountPort;
    indiDlg->SetSettings();
    indiDlg->Connect();
    if (indiDlg->ShowModal() == wxID_OK) {
	indiDlg->SaveSettings();
	INDIhost = indiDlg->INDIhost;
	INDIport = indiDlg->INDIport;
	INDIMountName = indiDlg->INDIDevName;
	INDIMountPort = indiDlg->INDIDevPort;
	pConfig->Profile.SetString("/indi/INDIhost", INDIhost);
	pConfig->Profile.SetLong("/indi/INDIport", INDIport);
	pConfig->Profile.SetString("/indi/INDImount", INDIMountName);
	pConfig->Profile.SetString("/indi/INDImount_port",INDIMountPort);
    }
    indiDlg->Disconnect();
    indiDlg->Destroy();
    delete indiDlg;
}

bool ScopeINDI::Connect() 
{
    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
    watchDevice(INDIMountName.mb_str(wxConvUTF8));
    if (connectServer()) {
	setBLOBMode(B_NEVER, INDIMountName.mb_str(wxConvUTF8), NULL);
	modal = true;
	wxLongLong msec = wxGetUTCTimeMillis();
	while (modal && wxGetUTCTimeMillis() - msec < 10 * 1000) {
	    ::wxSafeYield();
	}
	modal = false;
	if(! ready) {
	    Disconnect();
	    return true;
	}
	return false;
    }
    else {
	return true;
    }    
}

bool ScopeINDI::Disconnect() 
{
    if (disconnectServer()){
	Scope::Disconnect();
	ready = false;
	return false;
    }
    else return true;
}

void ScopeINDI::serverConnected()
{
    Scope::Connect();
}

void ScopeINDI::serverDisconnected(int exit_code)
{
    Scope::Disconnect();
}

void ScopeINDI::newDevice(INDI::BaseDevice *dp)
{
    scope_device = dp;
}

void ScopeINDI::newSwitch(ISwitchVectorProperty *svp)
{
    printf("Mount Receving Switch: %s = %i\n", svp->name, svp->sp->s);
}

void ScopeINDI::newMessage(INDI::BaseDevice *dp, int messageID)
{
    printf("Mount Receving message: %s\n", dp->messageQueue(messageID));
}

void ScopeINDI::newNumber(INumberVectorProperty *nvp)
{
    printf("Mount Receving Number: %s = %g\n", nvp->name, nvp->np[0].value);
}

void ScopeINDI::newText(ITextVectorProperty *tvp)
{
    printf("Mount Receving Text: %s = %s\n", tvp->name, tvp->tp[0].text);
}

void ScopeINDI::newProperty(INDI::Property *property) 
{
    const char* PropName = property->getName();
    INDI_TYPE Proptype = property->getType();
    printf("Mount Property: %s\n",PropName);
    
    if (strcmp(PropName, "EQUATORIAL_EOD_COORD_REQUEST") == 0) {
	coord_set_prop = property;
    }
    else if (strcmp(PropName, "EQUATORIAL_EOD_COORD") == 0) {
        
    }
    else if (strcmp(PropName, "ABORT") == 0) {
	abort_prop = property;
    }
    else if (strcmp(PropName, "TELESCOPE_MOTION_RATE") == 0) {
	MotionRate = property;
    }
    else if (strcmp(PropName, "TELESCOPE_MOTION_NS") == 0) {
	moveNS = property;
    }
    else if (strcmp(PropName, "TELESCOPE_MOTION_WE") == 0) {
	moveEW = property;
    }
    else if (strcmp(PropName, "TELESCOPE_TIMED_GUIDE_NS") == 0) {
	pulseGuideNS = property;
    }
    else if (strcmp(PropName, "TELESCOPE_TIMED_GUIDE_WE") == 0) {
	pulseGuideEW = property;
    }
    else if (strcmp(PropName, "DEVICE_PORT") == 0 && Proptype == INDI_TEXT && INDIMountPort.Length()) {    
	char* porttext = (const_cast<char*>((const char*)INDIMountPort.mb_str()));
	printf("Set port for mount %s\n", porttext);
	property->getText()->tp->text = porttext;
	sendNewText(property->getText());
    }
    else if (strcmp(PropName, "CONNECTION") == 0 && Proptype == INDI_SWITCH) {
	printf("Found CONNECTION for mount %s\n", PropName);
	property->getSwitch()->sp->s = ISS_ON;
	sendNewSwitch(property->getSwitch());
    }
    CheckState();
}

Mount::MOVE_RESULT ScopeINDI::Guide(GUIDE_DIRECTION direction, int duration_msec) 
{
  if (pulseGuideNS && pulseGuideEW) {
    // despite what is sayed in INDI standard properties description, every telescope driver expect the guided time in msec.  
    switch (direction) {
        case EAST:
	    pulseGuideEW->getNumber()->np[0].value=0;
	    pulseGuideEW->getNumber()->np[1].value=duration_msec;
	    sendNewNumber(pulseGuideEW->getNumber());
            break;
        case WEST:
	    pulseGuideEW->getNumber()->np[0].value=duration_msec;
	    pulseGuideEW->getNumber()->np[1].value=0;
	    sendNewNumber(pulseGuideEW->getNumber());
            break;
        case NORTH:
	    pulseGuideNS->getNumber()->np[0].value=duration_msec;
	    pulseGuideNS->getNumber()->np[1].value=0;
	    sendNewNumber(pulseGuideNS->getNumber());
            break;
        case SOUTH:
	    pulseGuideNS->getNumber()->np[0].value=0;
	    pulseGuideNS->getNumber()->np[1].value=duration_msec;
	    sendNewNumber(pulseGuideNS->getNumber());
            break;
        case NONE:
			printf("error ScopeINDI::Guide NONE\n");
            break;
    }
    wxMilliSleep(duration_msec);
    return MOVE_OK;
  }
  else if (MotionRate && moveNS && moveEW) {
      MotionRate->getNumber()->np->value=0.3 * 15;
      sendNewNumber(MotionRate->getNumber());
      switch (direction) {
	  case EAST:
	      moveEW->getSwitch()->sp[0].s=ISS_OFF;
	      moveEW->getSwitch()->sp[1].s=ISS_ON;
	      sendNewSwitch(moveEW->getSwitch());
	      wxMilliSleep(duration_msec);
	      moveEW->getSwitch()->sp[0].s=ISS_OFF;
	      moveEW->getSwitch()->sp[1].s=ISS_OFF;
	      sendNewSwitch(moveEW->getSwitch());
	      break;
	  case WEST:
	      moveEW->getSwitch()->sp[0].s=ISS_ON;
	      moveEW->getSwitch()->sp[1].s=ISS_OFF;
	      sendNewSwitch(moveEW->getSwitch());
	      wxMilliSleep(duration_msec);
	      moveEW->getSwitch()->sp[0].s=ISS_OFF;
	      moveEW->getSwitch()->sp[1].s=ISS_OFF;
	      sendNewSwitch(moveEW->getSwitch());
	      break;
	  case NORTH:
	      moveNS->getSwitch()->sp[0].s=ISS_ON;
	      moveNS->getSwitch()->sp[1].s=ISS_OFF;
	      sendNewSwitch(moveNS->getSwitch());
	      wxMilliSleep(duration_msec);
	      moveNS->getSwitch()->sp[0].s=ISS_OFF;
	      moveNS->getSwitch()->sp[1].s=ISS_OFF;
	      sendNewSwitch(moveNS->getSwitch());
	      break;
	  case SOUTH:
	      moveNS->getSwitch()->sp[0].s=ISS_OFF;
	      moveNS->getSwitch()->sp[1].s=ISS_ON;
	      sendNewSwitch(moveNS->getSwitch());
	      wxMilliSleep(duration_msec);
	      moveNS->getSwitch()->sp[0].s=ISS_OFF;
	      moveNS->getSwitch()->sp[1].s=ISS_OFF;
	      sendNewSwitch(moveNS->getSwitch());
	      break;
	  case NONE:
	      printf("error ScopeINDI::Guide NONE\n");
	      break;
      }
      return MOVE_OK;
  }    
  else return MOVE_ERROR;
}

#endif /* GUIDE_INDI */
