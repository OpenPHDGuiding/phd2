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

#ifdef LIBNOVA
  #include <libnova/sidereal_time.h>
  #include <libnova/julian_day.h>
#endif  

ScopeINDI::ScopeINDI() 
{
    ClearStatus();
    // load the values from the current profile
    INDIhost = pConfig->Profile.GetString("/indi/INDIhost", _T("localhost"));
    INDIport = pConfig->Profile.GetLong("/indi/INDIport", 7624);
    INDIMountName = pConfig->Profile.GetString("/indi/INDImount", _T("INDI Mount"));
    INDIMountPort = pConfig->Profile.GetString("/indi/INDImount_port",_T(""));
    m_Name = INDIMountName;
}

ScopeINDI::~ScopeINDI() 
{
   ready = false;
   Disconnect();
}

void ScopeINDI::ClearStatus()
{
    // reset properties pointer
    coord_prop = NULL;
    abort_prop = NULL;
    MotionRate_prop = NULL;
    moveNS_prop = NULL;
    moveEW_prop = NULL;
    GuideRate_prop = NULL;
    pulseGuideNS_prop = NULL;
    pulseGuideEW_prop = NULL;
    oncoordset_prop = NULL;
    GeographicCoord_prop = NULL;
    SiderealTime_prop = NULL;
    scope_device = NULL;
    scope_port = NULL;
    // reset connection status
    ready = false;
    eod_coord = false;
}

void ScopeINDI::CheckState() 
{
    // Check if the device has all the required properties for our usage.
    if(IsConnected() && (
	(MotionRate_prop && moveNS_prop && moveEW_prop) ||
	(pulseGuideNS_prop && pulseGuideEW_prop)))
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
	m_Name = INDIMountName;
    }
    indiDlg->Disconnect();
    indiDlg->Destroy();
    delete indiDlg;
}

bool ScopeINDI::Connect() 
{
   // If not configured open the setup dialog
   if (strcmp(INDIMountName,"INDI Mount")==0) SetupDialog();
    // define server to connect to.
    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
    // Receive messages only for our mount.
    watchDevice(INDIMountName.mb_str(wxConvUTF8));
    // Connect to server.
   if (connectServer()) {
      return !ready;
   }
   else {
      // last chance to fix the setup
      SetupDialog();
      setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
      watchDevice(INDIMountName.mb_str(wxConvUTF8));
      if (connectServer()) {
	 return !ready; 
      }
      else {
	 return true;
      }
   }
}

bool ScopeINDI::Disconnect() 
{
    // Disconnect from server
    if (disconnectServer()){
       if (ready) {
	  ready = false;
	  Scope::Disconnect();
       }
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
    if( ready) {
       Scope::Connect();
    }
    else {
       Disconnect();
    }
}

void ScopeINDI::serverDisconnected(int exit_code)
{
    // in case the connection lost we must reset the client socket
   Disconnect();
   if (ready) {
       ready = false;
       Scope::Disconnect();
    }
    // after disconnection we reset the connection status and the properties pointers
    ClearStatus();
}

void ScopeINDI::newDevice(INDI::BaseDevice *dp)
{
    if (strcmp(dp->getDeviceName(), INDIMountName.mb_str(wxConvUTF8)) == 0) {
	// The mount object
	scope_device = dp;
    }
}

void ScopeINDI::newSwitch(ISwitchVectorProperty *svp)
{
    // we go here every time a Switch state change
    //printf("Mount Receving Switch: %s = %i\n", svp->name, svp->sp->s);
    if (strcmp(svp->name, "CONNECTION") == 0) {
	ISwitch *connectswitch = IUFindSwitch(svp,"CONNECT");
	if (connectswitch->s == ISS_ON) {
            Scope::Connect();
        }
        else {
            if (ready) ScopeINDI::Disconnect();
        }
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
    #ifdef INDI_PRE_1_1_0
      INDI_TYPE Proptype = property->getType();
    #else
      INDI_PROPERTY_TYPE Proptype = property->getType();
    #endif 
    
    //printf("Mount Property: %s\n",PropName);
    
    if ((strcmp(PropName, "EQUATORIAL_EOD_COORD") == 0) && Proptype == INDI_NUMBER){
	// Epoch of date
	coord_prop = property->getNumber();
	eod_coord = true;
    }
    else if ((strcmp(PropName, "EQUATORIAL_COORD") == 0) && (!coord_prop) && Proptype == INDI_NUMBER){
	// Epoch J2000, used only if epoch of date is not available
	coord_prop = property->getNumber();
	eod_coord = false;
    }
    else if ((strcmp(PropName, "ON_COORD_SET") == 0) && Proptype == INDI_SWITCH){
	oncoordset_prop = property->getSwitch();
	setslew_prop = IUFindSwitch(oncoordset_prop,"SLEW");
	settrack_prop = IUFindSwitch(oncoordset_prop,"TRACK");
	setsync_prop = IUFindSwitch(oncoordset_prop,"SYNC");
    }
    else if ((strcmp(PropName, "ABORT") == 0) && Proptype == INDI_SWITCH){
       abort_prop = property->getSwitch();
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_RATE") == 0) && Proptype == INDI_NUMBER){
	MotionRate_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_NS") == 0) && Proptype == INDI_SWITCH){
	moveNS_prop = property->getSwitch();
	moveN_prop = IUFindSwitch(moveNS_prop,"MOTION_NORTH");
	moveS_prop = IUFindSwitch(moveNS_prop,"MOTION_SOUTH");
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_WE") == 0) && Proptype == INDI_SWITCH){
	moveEW_prop = property->getSwitch();
	moveE_prop = IUFindSwitch(moveEW_prop,"MOTION_EAST");
	moveW_prop = IUFindSwitch(moveEW_prop,"MOTION_WEST");
    }
    else if ((strcmp(PropName, "GUIDE_RATE") == 0) && Proptype == INDI_NUMBER){
	GuideRate_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_NS") == 0) && Proptype == INDI_NUMBER){
	pulseGuideNS_prop = property->getNumber();
	pulseN_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_N");
	pulseS_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_S");
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_WE") == 0) && Proptype == INDI_NUMBER){
	pulseGuideEW_prop = property->getNumber();
       pulseW_prop = IUFindNumber(pulseGuideEW_prop,"TIMED_GUIDE_W");
       pulseE_prop = IUFindNumber(pulseGuideEW_prop,"TIMED_GUIDE_E");
    }
    else if (strcmp(PropName, "DEVICE_PORT") == 0 && Proptype == INDI_TEXT) {    
	scope_port = property->getText();
    }
    else if (strcmp(PropName, "CONNECTION") == 0 && Proptype == INDI_SWITCH) {
	// Check the value here in case the device is already connected
	ISwitch *connectswitch = IUFindSwitch(property->getSwitch(),"CONNECT");
	if (connectswitch->s == ISS_ON) Scope::Connect();
    }
    else if ((strcmp(PropName, "GEOGRAPHIC_COORD") == 0) && Proptype == INDI_NUMBER){
	GeographicCoord_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "TIME_LST") == 0) && Proptype == INDI_NUMBER){
	SiderealTime_prop = property->getNumber();
    }
    CheckState();
}

Mount::MOVE_RESULT ScopeINDI::Guide(GUIDE_DIRECTION direction, int duration) 
{
  // guide using timed pulse guide 
    if (pulseGuideNS_prop && pulseGuideEW_prop) {
    // despite what is sayed in INDI standard properties description, every telescope driver expect the guided time in msec.  
    switch (direction) {
        case EAST:
	    pulseE_prop->value = duration;
	    pulseW_prop->value = 0;
	    sendNewNumber(pulseGuideEW_prop);
            break;
        case WEST:
	    pulseE_prop->value = 0;
	    pulseW_prop->value = duration;
	    sendNewNumber(pulseGuideEW_prop);
            break;
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
        case NONE:
	    printf("error ScopeINDI::Guide NONE\n");
            break;
    }
    wxMilliSleep(duration);
    return MOVE_OK;
  }
  // guide using motion rate and telescope motion
  // !!! untested as no driver implement TELESCOPE_MOTION_RATE at the moment (INDI 0.9.9) !!!
  else if (MotionRate_prop && moveNS_prop && moveEW_prop) {
      MotionRate_prop->np->value = 0.3 * 15 / 60;  // set 0.3 sidereal in arcmin/sec
      sendNewNumber(MotionRate_prop);
      switch (direction) {
	  case EAST:
	      moveW_prop->s = ISS_OFF;
	      moveE_prop->s = ISS_ON;
	      sendNewSwitch(moveEW_prop);
	      wxMilliSleep(duration);
	      moveW_prop->s = ISS_OFF;
	      moveE_prop->s = ISS_OFF;
	      sendNewSwitch(moveEW_prop);
	      break;
	  case WEST:
	      moveW_prop->s = ISS_ON;
	      moveE_prop->s = ISS_OFF;
	      sendNewSwitch(moveEW_prop);
	      wxMilliSleep(duration);
	      moveW_prop->s = ISS_OFF;
	      moveE_prop->s = ISS_OFF;
	      sendNewSwitch(moveEW_prop);
	      break;
	  case NORTH:
	      moveN_prop->s = ISS_ON;
	      moveS_prop->s = ISS_OFF;
	      sendNewSwitch(moveNS_prop);
	      wxMilliSleep(duration);
	      moveN_prop->s = ISS_OFF;
	      moveS_prop->s = ISS_OFF;
	      sendNewSwitch(moveNS_prop);
	      break;
	  case SOUTH:
	      moveN_prop->s = ISS_OFF;
	      moveS_prop->s = ISS_ON;
	      sendNewSwitch(moveNS_prop);
	      wxMilliSleep(duration);
	      moveN_prop->s = ISS_OFF;
	      moveS_prop->s = ISS_OFF;
	      sendNewSwitch(moveNS_prop);
	      break;
	  case NONE:
	      printf("error ScopeINDI::Guide NONE\n");
	      break;
      }
      return MOVE_OK;
  }    
  else return MOVE_ERROR;
}

double ScopeINDI::GetGuidingDeclination(void)
{
    double dec;
    dec = 0;
    if (coord_prop) {
	INumber *decprop = IUFindNumber(coord_prop,"DEC");
	if (decprop) {
	    dec = decprop->value;     // Degrees
	    if (dec>89) dec = 89;     // avoid crash when dividing by cos(dec) 
	    if (dec<-89) dec = -89; 
	    dec = dec * M_PI / 180;  // Radians
	}
    }
    return dec;
}

bool   ScopeINDI::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    const double dSiderealSecondPerSec = 0.9973;
    bool ok;
    double gra,gdec;
    ok = true;
    if (GuideRate_prop) {
	INumber *ratera = IUFindNumber(GuideRate_prop,"GUIDE_RATE_WE");
	INumber *ratedec = IUFindNumber(GuideRate_prop,"GUIDE_RATE_NS");
	if (ratera && ratedec) {
	    gra =  ratera->value;  // sidereal rate
	    gdec = ratedec->value;
	    gra = gra * (15.0 * dSiderealSecondPerSec)/3600;    // ASCOM compatible
	    gdec = gdec * (15.0 * dSiderealSecondPerSec)/3600;  // Degrees/sec
	    *pRAGuideRate =  gra;
	    *pDecGuideRate = gdec;
	    ok = false;
	}
    }
    return ok;
}

bool   ScopeINDI::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    bool ok;
    ok = true;
    if (coord_prop) {
	INumber *raprop = IUFindNumber(coord_prop,"RA");
	INumber *decprop = IUFindNumber(coord_prop,"DEC");
	if (raprop && decprop) {
	    *ra = raprop->value;   // hours
	    *dec = decprop->value; // degrees
	    ok = false;
	}
	if (SiderealTime_prop) {   // LX200 only
	    INumber *stprop = IUFindNumber(coord_prop,"LST"); 
	    if (stprop){
		*siderealTime = stprop->value;
	    }
	}
	else {
	   #ifdef LIBNOVA
	   double lat,lon;
	   double jd = ln_get_julian_from_sys();
	   *siderealTime = ln_get_apparent_sidereal_time (jd);
	   if (!GetSiteLatLong(&lat,&lon)) 
	      *siderealTime = *siderealTime + (lon/15);
	   #else
	   *siderealTime = 0;
	   #endif
	}
    }
    return ok;
}

bool   ScopeINDI::GetSiteLatLong(double *latitude, double *longitude)
{
    bool ok;
    ok = true;
    if (GeographicCoord_prop) {
       INumber *latprop = IUFindNumber(GeographicCoord_prop,"LAT");
       INumber *lonprop = IUFindNumber(GeographicCoord_prop,"LONG");
	if (latprop && lonprop) {
	    *latitude = latprop->value;
	    *longitude = lonprop->value;
	    ok = false;
	}
    }
    return ok;	
}

bool   ScopeINDI::SlewToCoordinates(double ra, double dec)
{
    bool ok;
    ok = true;
    if (coord_prop && oncoordset_prop) {
	setslew_prop->s = ISS_ON;
	settrack_prop->s = ISS_OFF;
	setsync_prop->s = ISS_OFF;
	sendNewSwitch(oncoordset_prop);
	INumber *raprop = IUFindNumber(coord_prop,"RA");
	INumber *decprop = IUFindNumber(coord_prop,"DEC");
	raprop->value = ra;
	decprop->value = dec;
	sendNewNumber(coord_prop);
	ok = false;
    }
    return ok;
}

bool   ScopeINDI::Slewing(void)
{
    bool ok;
    ok = true;
    if (coord_prop) {
	ok = !(coord_prop->s == IPS_BUSY);
    }
    return ok;
}

#endif /* GUIDE_INDI */
