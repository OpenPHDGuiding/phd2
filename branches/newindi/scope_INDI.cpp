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
    ClearStatus();
    // load the values from the current profile
    INDIhost = pConfig->Profile.GetString("/indi/INDIhost", _T("localhost"));
    INDIport = pConfig->Profile.GetLong("/indi/INDIport", 7624);
    INDIMountName = pConfig->Profile.GetString("/indi/INDImount", _T("INDI Mount"));
    INDIMountPort = pConfig->Profile.GetString("/indi/INDImount_port",_T(""));
}

ScopeINDI::~ScopeINDI() 
{
    disconnectServer();
}

void ScopeINDI::ClearStatus()
{
    // reset properties pointer
    coord_set_prop = NULL;
    abort_prop = NULL;
    moveNS = NULL;
    moveEW = NULL;
    pulseGuideNS = NULL;
    pulseGuideEW = NULL;
    scope_device = NULL;
    scope_port = NULL;
    // reset connection status
    ready = false;
}

void ScopeINDI::CheckState() 
{
    // Check if the device has all the required properties for our usage.
    if(IsConnected() && (
	(MotionRate && moveNS && moveEW) ||
        (pulseGuideNS && pulseGuideEW)))
    {
        if (! ready) {
            //printf("Telescope is ready\n");
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
    // contrary to camera the telescope setup dialog is called only when not connected
    // show the server and device configuration
    INDIConfig *indiDlg = new INDIConfig(wxGetActiveWindow(),TYPE_MOUNT);
    indiDlg->INDIhost = INDIhost;
    indiDlg->INDIport = INDIport;
    indiDlg->INDIDevName = INDIMountName;
    indiDlg->INDIDevPort = INDIMountPort;
    // initialize with actual values
    indiDlg->SetSettings();
    // try to connect to server
    indiDlg->Connect();
    if (indiDlg->ShowModal() == wxID_OK) {
	// if OK save the values to the current profile
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
    // define server to connect to.
    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
    // Receive messages only for our mount.
    watchDevice(INDIMountName.mb_str(wxConvUTF8));
    // Connect to server.
    if (connectServer()) {
	return false;
    }
    else {
	return true;
    }
}

bool ScopeINDI::Disconnect() 
{
    // Disconnect from server
    if (disconnectServer()){
	return false;
    }
    else return true;
}

void ScopeINDI::serverConnected()
{
    // After connection to the server
    // set option to receive only the messages, no blob
    setBLOBMode(B_NEVER, INDIMountName.mb_str(wxConvUTF8), NULL);
    modal = true; 
    // wait for the device port property
    wxLongLong msec;
    msec = wxGetUTCTimeMillis();
    while ((!scope_port) && wxGetUTCTimeMillis() - msec < 2 * 1000) {
	::wxSafeYield();
    }
    // Set the port, this must be done before to try to connect the device
    if (scope_port && INDIMountPort.Length()) {  // the mount port is not mandatory
	char* porttext = (const_cast<char*>((const char*)INDIMountPort.mb_str()));
	scope_port->tp->text = porttext;
	sendNewText(scope_port); 
    }
    // Connect the mount device
    connectDevice(INDIMountName.mb_str(wxConvUTF8));
    
    msec = wxGetUTCTimeMillis();
    while (modal && wxGetUTCTimeMillis() - msec < 5 * 1000) {
	::wxSafeYield();
    }
    modal = false;
    // In case we not get all the required properties or connection to the device failed
    if(! ready) {
	Disconnect();
    }
    Scope::Connect();
}

void ScopeINDI::serverDisconnected(int exit_code)
{
    // after disconnection we reset the connection status and the properties pointers
    Scope::Disconnect();
    ClearStatus();
}

void ScopeINDI::newDevice(INDI::BaseDevice *dp)
{
    if (strcmp(dp->getDeviceName(), INDIMountName.mb_str(wxConvUTF8)) == 0) {
	// The mount object, maybe this can be useful in the future
	scope_device = dp;
    }
}

void ScopeINDI::newSwitch(ISwitchVectorProperty *svp)
{
    // we go here every time a Switch state change
    //printf("Mount Receving Switch: %s = %i\n", svp->name, svp->sp->s);
    if (strcmp(svp->name, "CONNECTION") == 0) {
	ISwitch *connectswitch = IUFindSwitch(svp,"CONNECT");
	if (connectswitch->s == ISS_ON) Scope::Connect();
    }
    
}

void ScopeINDI::newMessage(INDI::BaseDevice *dp, int messageID)
{
    // we go here every time the mount driver send a message
    //printf("Mount Receving message: %s\n", dp->messageQueue(messageID));
}

void ScopeINDI::newNumber(INumberVectorProperty *nvp)
{
    // we go here every time a Number value change
    //printf("Mount Receving Number: %s = %g\n", nvp->name, nvp->np->value);
}

void ScopeINDI::newText(ITextVectorProperty *tvp)
{
    // we go here every time a Text value change
    //printf("Mount Receving Text: %s = %s\n", tvp->name, tvp->tp->text);
}

void ScopeINDI::newProperty(INDI::Property *property) 
{
    // Here we receive a list of all the properties after the connection
    // Updated values are not received here but in the newTYPE() functions above.
    // We keep the vector for each interesting property to send some data later.
    const char* PropName = property->getName();
    INDI_TYPE Proptype = property->getType();
    //printf("Mount Property: %s\n",PropName);
    
    if ((strcmp(PropName, "EQUATORIAL_EOD_COORD") == 0) && Proptype == INDI_NUMBER){
       coord_set_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "ABORT") == 0) && Proptype == INDI_SWITCH){
       abort_prop = property->getSwitch();
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_RATE") == 0) && Proptype == INDI_NUMBER){
       MotionRate = property->getNumber();
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_NS") == 0) && Proptype == INDI_SWITCH){
       moveNS = property->getSwitch();
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_WE") == 0) && Proptype == INDI_SWITCH){
       moveEW = property->getSwitch();
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_NS") == 0) && Proptype == INDI_NUMBER){
       pulseGuideNS = property->getNumber();
       pulseN = IUFindNumber(pulseGuideNS,"TIMED_GUIDE_N");
       pulseS = IUFindNumber(pulseGuideNS,"TIMED_GUIDE_S");
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_WE") == 0) && Proptype == INDI_NUMBER){
       pulseGuideEW = property->getNumber();
       pulseW = IUFindNumber(pulseGuideEW,"TIMED_GUIDE_W");
       pulseE = IUFindNumber(pulseGuideEW,"TIMED_GUIDE_E");
    }
    else if (strcmp(PropName, "DEVICE_PORT") == 0 && Proptype == INDI_TEXT) {    
	scope_port = property->getText();
    }
    else if (strcmp(PropName, "CONNECTION") == 0 && Proptype == INDI_SWITCH) {
	// Check the value here in case the device is already connected
	ISwitch *connectswitch = IUFindSwitch(property->getSwitch(),"CONNECT");
	if (connectswitch->s == ISS_ON) Scope::Connect();
    }
    CheckState();
}

Mount::MOVE_RESULT ScopeINDI::Guide(GUIDE_DIRECTION direction, int duration) 
{
  // guide using timed pulse guide 
  if (pulseGuideNS && pulseGuideEW) {
    // despite what is sayed in INDI standard properties description, every telescope driver expect the guided time in msec.  
    switch (direction) {
        case EAST:
	    pulseE->value = duration;
	    pulseW->value = 0;
	    sendNewNumber(pulseGuideEW);
            break;
        case WEST:
	    pulseE->value = 0;
	    pulseW->value = duration;
	    sendNewNumber(pulseGuideEW);
            break;
        case NORTH:
	    pulseN->value = duration;
	    pulseS->value = 0;
	    sendNewNumber(pulseGuideNS);
            break;
        case SOUTH:
	    pulseN->value = 0;
	    pulseS->value = duration;
	    sendNewNumber(pulseGuideNS);
            break;
        case NONE:
	    printf("error ScopeINDI::Guide NONE\n");
            break;
    }
    wxMilliSleep(duration);
    return MOVE_OK;
  }
  // guide using motion rate and telescope motion
  // !!! untested as no driver implement TELESCOPE_MOTION_RATE at the moment (INDI 0.9.9) !!!
  else if (MotionRate && moveNS && moveEW) {
      MotionRate->np->value = 0.3 * 15 / 60;  // set 0.3 sidereal in arcmin/sec
      sendNewNumber(MotionRate);
      switch (direction) {
	  case EAST:
	      moveW->s = ISS_OFF;
	      moveE->s = ISS_ON;
	      sendNewSwitch(moveEW);
	      wxMilliSleep(duration);
	      moveW->s = ISS_OFF;
	      moveE->s = ISS_OFF;
	      sendNewSwitch(moveEW);
	      break;
	  case WEST:
	      moveW->s = ISS_ON;
	      moveE->s = ISS_OFF;
	      sendNewSwitch(moveEW);
	      wxMilliSleep(duration);
	      moveW->s = ISS_OFF;
	      moveE->s = ISS_OFF;
	      sendNewSwitch(moveEW);
	      break;
	  case NORTH:
	      moveN->s = ISS_ON;
	      moveS->s = ISS_OFF;
	      sendNewSwitch(moveNS);
	      wxMilliSleep(duration);
	      moveN->s = ISS_OFF;
	      moveS->s = ISS_OFF;
	      sendNewSwitch(moveNS);
	      break;
	  case SOUTH:
	      moveN->s = ISS_OFF;
	      moveS->s = ISS_ON;
	      sendNewSwitch(moveNS);
	      wxMilliSleep(duration);
	      moveN->s = ISS_OFF;
	      moveS->s = ISS_OFF;
	      sendNewSwitch(moveNS);
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
