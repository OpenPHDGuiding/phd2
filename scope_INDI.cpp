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
# include <libnova/sidereal_time.h>
# include <libnova/julian_day.h>
#endif

#ifdef INDI_PRE_1_1_0
typedef INDI_TYPE INDI_PROPERTY_TYPE;
#endif

static bool s_verbose = false;

ScopeINDI::ScopeINDI()
    :
    sync_cond(sync_lock)
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
    disconnectServer();
}

void ScopeINDI::ClearStatus()
{
    // reset properties pointers
    connection_prop = nullptr;
    coord_prop = nullptr;
    AbortMotion_prop = nullptr;
    MotionRate_prop = nullptr;
    moveNS_prop = nullptr;
    moveEW_prop = nullptr;
    GuideRate_prop = nullptr;
    pulseGuideNS_prop = nullptr;
    pulseGuideEW_prop = nullptr;
    oncoordset_prop = nullptr;
    GeographicCoord_prop = nullptr;
    SiderealTime_prop = nullptr;
    scope_port = nullptr;
    pierside_prop = nullptr;
    // reset connection status
    ready = false;
    eod_coord = false;
    guide_active = false;
    sync_cond.Broadcast(); // just in case worker thread was blocked waiting for guide pulse to complete
}

void ScopeINDI::CheckState()
{
    // Check if the device has all the required properties for our usage.
    if (IsConnected() &&
        ((MotionRate_prop && moveNS_prop && moveEW_prop) ||
         (pulseGuideNS_prop && pulseGuideEW_prop)))
    {
        if (!ready)
        {
            Debug.Write(wxString::Format("INDI Telescope is ready "
                                         "MotionRate=%d moveNS=%d moveEW=%d guideNS=%d guideEW=%d\n",
                                         MotionRate_prop ? 1 : 0, moveNS_prop ? 1 : 0, moveEW_prop ? 1 : 0,
                                         pulseGuideNS_prop ? 1 : 0, pulseGuideEW_prop ? 1 : 0));

            ready = true;

            if (modal)
                modal = false;
        }
    }
}

bool ScopeINDI::HasSetupDialog() const
{
    return true;
}

void ScopeINDI::SetupDialog()
{
    wxString title;
    bool isAuxMount = pFrame->pGearDialog->AuxScope() == this;
    if (isAuxMount)
        title = _("INDI Aux Mount Selection");
    else
        title = _("INDI Mount Selection");

    INDIConfig indiDlg(wxGetActiveWindow(), title, TYPE_MOUNT);

    indiDlg.INDIhost = INDIhost;
    indiDlg.INDIport = INDIport;
    indiDlg.INDIDevName = INDIMountName;
    indiDlg.INDIDevPort = INDIMountPort;
    // initialize with actual values
    indiDlg.SetSettings();
    // try to connect to server
    indiDlg.Connect();

    if (indiDlg.ShowModal() == wxID_OK)
    {
        // if OK save the values to the current profile
        indiDlg.SaveSettings();
        INDIhost = indiDlg.INDIhost;
        INDIport = indiDlg.INDIport;
        INDIMountName = indiDlg.INDIDevName;
        INDIMountPort = indiDlg.INDIDevPort;
        pConfig->Profile.SetString("/indi/INDIhost", INDIhost);
        pConfig->Profile.SetLong("/indi/INDIport", INDIport);
        pConfig->Profile.SetString("/indi/INDImount", INDIMountName);
        pConfig->Profile.SetString("/indi/INDImount_port",INDIMountPort);
        m_Name = INDIMountName;
    }

    indiDlg.Disconnect();
}

bool ScopeINDI::Connect()
{
    s_verbose = pConfig->Profile.GetBoolean("/indi/VerboseLogging", false);

    // If not configured open the setup dialog
    if (INDIMountName == wxT("INDI Mount"))
    {
        SetupDialog();
    }

    // define server to connect to.
    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);

    // Receive messages only for our mount.
    watchDevice(INDIMountName.mb_str(wxConvUTF8));

    // Connect to server.
    if (connectServer())
    {
        return !ready;
    }

    // last chance to fix the setup
    SetupDialog();

    setServer(INDIhost.mb_str(wxConvUTF8), INDIport);
    watchDevice(INDIMountName.mb_str(wxConvUTF8));

    if (connectServer())
    {
        return !ready;
    }

    return true;
}

bool ScopeINDI::Disconnect()
{
    if (!ready)
        return true;

    // Disconnect from server
    if (disconnectServer())
    {
        ClearStatus();
        Scope::Disconnect();
        return false;
    }

    return true;
}

void ScopeINDI::serverConnected()
{
    // After connection to the server
    // set option to receive only the messages, no blob
    setBLOBMode(B_NEVER, INDIMountName.mb_str(wxConvUTF8), nullptr);
    modal = true;

    // wait for the device port property
    wxLongLong msec;

    msec = wxGetUTCTimeMillis();
    if (INDIMountPort.Length())    // the mount port is not mandatory
    {
        // wait for Port property
        while ((!scope_port) && wxGetUTCTimeMillis() - msec < 15 * 1000)
        {
            ::wxSafeYield();
        }
        // Set the port, this must be done before to try to connect the device
        if (scope_port)
        {
            char *porttext = const_cast<char*>((const char*)INDIMountPort.mb_str());
            scope_port->tp->text = porttext;
            sendNewText(scope_port);
        }
    }
    // Connect the mount device
    while (!connection_prop && wxGetUTCTimeMillis() - msec < 15 * 1000)
    {
        ::wxSafeYield();
    }
    connectDevice(INDIMountName.mb_str(wxConvUTF8));

    msec = wxGetUTCTimeMillis();
    while (modal && wxGetUTCTimeMillis() - msec < 30 * 1000)
    {
        ::wxSafeYield();
    }
    modal = false;

    // In case we not get all the required properties or connection to the device failed
    if (ready)
    {
        Scope::Connect();
    }
    else
    {
        Disconnect();
    }
}

void ScopeINDI::serverDisconnected(int exit_code)
{
    // after disconnection we reset the connection status and the properties pointers
    ClearStatus();

    // in case the connection lost we must reset the client socket
    if (exit_code == -1)
    {
        Disconnect();
        Scope::Disconnect();
    }
}

#ifndef INDI_PRE_1_0_0
void ScopeINDI::removeDevice(INDI::BaseDevice *dp)
{
    ClearStatus();
    Disconnect();
    Scope::Disconnect();
}
#endif

void ScopeINDI::newDevice(INDI::BaseDevice *dp)
{
    Debug.Write(wxString::Format("INDI Mount: new device %s\n", dp->getDeviceName()));
}

void ScopeINDI::newSwitch(ISwitchVectorProperty *svp)
{
    // we go here every time a Switch state change
    if (s_verbose)
        Debug.Write(wxString::Format("INDI Mount: Receiving Switch: %s = %i\n", svp->name, svp->sp->s));

    if (strcmp(svp->name, "CONNECTION") == 0)
    {
        ISwitch *connectswitch = IUFindSwitch(svp, "CONNECT");

        if (connectswitch->s == ISS_ON)
        {
            Scope::Connect();
        }
        else
        {
            if (ready)
            {
                ClearStatus();
                Scope::Disconnect();
            }
        }
    }
}

void ScopeINDI::newMessage(INDI::BaseDevice *dp, int messageID)
{
    // we go here every time the mount driver send a message
    if (s_verbose)
        Debug.Write(wxString::Format("INDI Mount: Receiving message: %s\n", dp->messageQueue(messageID)));
}

inline static const char *StateStr(IPState st)
{
    switch (st) {
    default: case IPS_IDLE: return "Idle";
    case IPS_OK: return "Ok";
    case IPS_BUSY: return "Busy";
    case IPS_ALERT: return "Alert";
    }
}

void ScopeINDI::newNumber(INumberVectorProperty *nvp)
{
    if (s_verbose)
    {
        if (strcmp(nvp->name, "EQUATORIAL_EOD_COORD") != 0) // too noisy
            Debug.Write(wxString::Format("INDI Mount: Receiving Number: %s = %g  state = %s\n", nvp->name, nvp->np->value, StateStr(nvp->s)));
    }

    if (nvp == pulseGuideEW_prop || nvp == pulseGuideNS_prop)
    {
        bool notify = false;
        {
            wxMutexLocker lck(sync_lock);
            if (guide_active && nvp->s != IPS_BUSY &&
                ((guide_active_axis == GUIDE_RA && nvp == pulseGuideEW_prop) || (guide_active_axis == GUIDE_DEC && nvp == pulseGuideNS_prop)))
            {
                guide_active = false;
                notify = true;
            }
            else if (!guide_active && nvp->s == IPS_BUSY)
            {
                guide_active = true;
                guide_active_axis = nvp == pulseGuideEW_prop ? GUIDE_RA : GUIDE_DEC;
            }
        }
        if (notify)
            sync_cond.Broadcast();
    }
}

void ScopeINDI::newText(ITextVectorProperty *tvp)
{
    // we go here every time a Text value change
    if (s_verbose)
        Debug.Write(wxString::Format("INDI Mount: Receiving Text: %s = %s\n", tvp->name, tvp->tp->text));
}

void ScopeINDI::newProperty(INDI::Property *property)
{
    // Here we receive a list of all the properties after the connection
    // Updated values are not received here but in the newTYPE() functions above.
    // We keep the vector for each interesting property to send some data later.
    const char *PropName = property->getName();

    INDI_PROPERTY_TYPE Proptype = property->getType();

    Debug.Write(wxString::Format("INDI Mount: Received property: %s\n", PropName));

    if ((strcmp(PropName, "EQUATORIAL_EOD_COORD") == 0) && Proptype == INDI_NUMBER)
    {
        // Epoch of date
        coord_prop = property->getNumber();
        eod_coord = true;
    }
    else if ((strcmp(PropName, "EQUATORIAL_COORD") == 0) && (!coord_prop) && Proptype == INDI_NUMBER)
    {
        // Epoch J2000, used only if epoch of date is not available
        coord_prop = property->getNumber();
        eod_coord = false;
    }
    else if ((strcmp(PropName, "ON_COORD_SET") == 0) && Proptype == INDI_SWITCH)
    {
        oncoordset_prop = property->getSwitch();
        setslew_prop = IUFindSwitch(oncoordset_prop,"SLEW");
        settrack_prop = IUFindSwitch(oncoordset_prop,"TRACK");
        setsync_prop = IUFindSwitch(oncoordset_prop,"SYNC");
    }
    else if ((strcmp(PropName, "TELESCOPE_ABORT_MOTION") == 0) && Proptype == INDI_SWITCH)
    {
        AbortMotion_prop = property->getSwitch();
        Abort_prop = IUFindSwitch(AbortMotion_prop,"ABORT");
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_RATE") == 0) && Proptype == INDI_NUMBER)
    {
        MotionRate_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_NS") == 0) && Proptype == INDI_SWITCH)
    {
        moveNS_prop = property->getSwitch();
        moveN_prop = IUFindSwitch(moveNS_prop,"MOTION_NORTH");
        moveS_prop = IUFindSwitch(moveNS_prop,"MOTION_SOUTH");
    }
    else if ((strcmp(PropName, "TELESCOPE_MOTION_WE") == 0) && Proptype == INDI_SWITCH)
    {
        moveEW_prop = property->getSwitch();
        moveE_prop = IUFindSwitch(moveEW_prop,"MOTION_EAST");
        moveW_prop = IUFindSwitch(moveEW_prop,"MOTION_WEST");
    }
    else if ((strcmp(PropName, "GUIDE_RATE") == 0) && Proptype == INDI_NUMBER)
    {
        GuideRate_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_NS") == 0) && Proptype == INDI_NUMBER)
    {
        pulseGuideNS_prop = property->getNumber();
        pulseN_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_N");
        pulseS_prop = IUFindNumber(pulseGuideNS_prop,"TIMED_GUIDE_S");
    }
    else if ((strcmp(PropName, "TELESCOPE_TIMED_GUIDE_WE") == 0) && Proptype == INDI_NUMBER)
    {
        pulseGuideEW_prop = property->getNumber();
        pulseW_prop = IUFindNumber(pulseGuideEW_prop,"TIMED_GUIDE_W");
        pulseE_prop = IUFindNumber(pulseGuideEW_prop,"TIMED_GUIDE_E");
    }
    else if ((strcmp(PropName, "TELESCOPE_PIER_SIDE") == 0) && Proptype == INDI_SWITCH)
    {
        pierside_prop = property->getSwitch();
        piersideEast_prop = IUFindSwitch(pierside_prop,"PIER_EAST");
        piersideWest_prop = IUFindSwitch(pierside_prop,"PIER_WEST");
    }
    else if (strcmp(PropName, "DEVICE_PORT") == 0 && Proptype == INDI_TEXT)
    {
        scope_port = property->getText();
    }
    else if (strcmp(PropName, "CONNECTION") == 0 && Proptype == INDI_SWITCH)
    {
        // Check the value here in case the device is already connected
        connection_prop = property->getSwitch();
        ISwitch *connectswitch = IUFindSwitch(connection_prop,"CONNECT");
        if (connectswitch->s == ISS_ON)
            Scope::Connect();
    }
    else if ((strcmp(PropName, "GEOGRAPHIC_COORD") == 0) && Proptype == INDI_NUMBER)
    {
        GeographicCoord_prop = property->getNumber();
    }
    else if ((strcmp(PropName, "TIME_LST") == 0) && Proptype == INDI_NUMBER)
    {
        SiderealTime_prop = property->getNumber();
    }

    CheckState();
}

Mount::MOVE_RESULT ScopeINDI::Guide(GUIDE_DIRECTION direction, int duration)
{
    if (pulseGuideNS_prop && pulseGuideEW_prop)
    {
        if (s_verbose)
            Debug.Write(wxString::Format("INDI Mount: timed pulse dir %d dur %d ms\n", direction, duration));

        switch (direction) {
        case EAST:
        case WEST:
        case NORTH:
        case SOUTH:
            break;
        default:
            Debug.Write("INDI Camera error CameraINDI::Guide NONE\n");
            return MOVE_ERROR;
        }

        // set guide active before initiating the pulse

        {
            wxMutexLocker lck(sync_lock);

            if (guide_active)
            {
                // todo: try to abort it?
                Debug.Write("Cannot guide with guide pulse in progress!\n");
                return MOVE_ERROR;
            }

            guide_active = true;
            guide_active_axis = direction == EAST || direction == WEST ? GUIDE_RA : GUIDE_DEC;

        } // lock scope

        // despite what is said in INDI standard properties description, every telescope driver expect the guided time in msec.
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
        }

        if (s_verbose)
            Debug.Write("INDI Mount: wait for move complete\n");

        { // lock scope
            wxMutexLocker lck(sync_lock);
            while (guide_active)
            {
                sync_cond.WaitTimeout(100);
                if (WorkerThread::InterruptRequested())
                {
                    Debug.Write("interrupt requested\n");
                    return MOVE_ERROR;
                }
            }
        } // lock scope

        if (s_verbose)
            Debug.Write("INDI Mount: move completed\n");

        return MOVE_OK;
    }
    // guide using motion rate and telescope motion
    // !!! untested as no driver implement TELESCOPE_MOTION_RATE at the moment (INDI 0.9.9) !!!
    else if (MotionRate_prop && moveNS_prop && moveEW_prop)
    {
        if (s_verbose)
            Debug.Write(wxString::Format("INDI Mount: motion rate guide dir %d dur %d ms\n", direction, duration));

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
            Debug.Write("INDI Mount: error ScopeINDI::Guide NONE\n");
            break;
        }

        return MOVE_OK;
    }
    else
    {
        Debug.Write(wxString::Format("INDI Mount: pulse guide properties unavailable!\n"));
        return MOVE_ERROR;
    }
}

double ScopeINDI::GetDeclination()
{
    if (coord_prop)
    {
        INumber *decprop = IUFindNumber(coord_prop, "DEC");
        if (decprop)
        {
            double dec = decprop->value;     // Degrees
            if (dec > 89.0) dec = 89.0;     // avoid crash when dividing by cos(dec)
            if (dec < -89.0) dec = -89.0;
            return radians(dec);
        }
    }

    return UNKNOWN_DECLINATION;
}

bool ScopeINDI::GetGuideRates(double *pRAGuideRate, double *pDecGuideRate)
{
    bool err = true;

    if (GuideRate_prop)
    {
        INumber *ratera = IUFindNumber(GuideRate_prop, "GUIDE_RATE_WE");
        INumber *ratedec = IUFindNumber(GuideRate_prop, "GUIDE_RATE_NS");
        if (ratera && ratedec)
        {
            const double dSiderealSecondPerSec = 0.9973;
            double gra = ratera->value;  // sidereal rate
            double gdec = ratedec->value;
            gra *= (15.0 * dSiderealSecondPerSec) / 3600.;   // ASCOM compatible
            gdec *= (15.0 * dSiderealSecondPerSec) / 3600.;  // Degrees/sec
            *pRAGuideRate =  gra;
            *pDecGuideRate = gdec;
            err = false;
        }
    }

    return err;
}

bool ScopeINDI::GetCoordinates(double *ra, double *dec, double *siderealTime)
{
    bool err = true;

    if (coord_prop)
    {
        INumber *raprop = IUFindNumber(coord_prop, "RA");
        INumber *decprop = IUFindNumber(coord_prop, "DEC");
        if (raprop && decprop)
        {
            *ra = raprop->value;   // hours
            *dec = decprop->value; // degrees
            err = false;
        }

        if (SiderealTime_prop)   // LX200 only
        {
            INumber *stprop = IUFindNumber(SiderealTime_prop, "LST");
            if (stprop)
                *siderealTime = stprop->value;
        }
        else
        {
#ifdef LIBNOVA
            double jd = ln_get_julian_from_sys();
            *siderealTime = ln_get_apparent_sidereal_time(jd);
            double lat,lon;
            if (!GetSiteLatLong(&lat,&lon))
                *siderealTime += (lon / 15.);
#else
            *siderealTime = 0;
#endif
        }
    }

    return err;
}

bool ScopeINDI::GetSiteLatLong(double *latitude, double *longitude)
{
    bool err = true;

    if (GeographicCoord_prop)
    {
        INumber *latprop = IUFindNumber(GeographicCoord_prop,"LAT");
        INumber *lonprop = IUFindNumber(GeographicCoord_prop,"LONG");
        if (latprop && lonprop)
        {
            *latitude = latprop->value;
            *longitude = lonprop->value;
            err = false;
        }
    }

    return err;
}

bool ScopeINDI::CanSlewAsync()
{
    // INDI slew is always async
    return true;
}

bool ScopeINDI::SlewToCoordinates(double ra, double dec)
{
    bool err = true;

    if (coord_prop && oncoordset_prop)
    {
        err = SlewToCoordinatesAsync(ra, dec);
        wxLongLong msec = wxGetUTCTimeMillis();
        while (coord_prop->s == IPS_BUSY && wxGetUTCTimeMillis() - msec < 90 * 1000)
        {
            ::wxSafeYield();
        }
        if (coord_prop->s != IPS_BUSY)
            err = false;
    }

    return err;
}

bool ScopeINDI::SlewToCoordinatesAsync(double ra, double dec)
{
    bool err = true;

    if (coord_prop && oncoordset_prop)
    {
        setslew_prop->s = ISS_ON;
        settrack_prop->s = ISS_OFF;
        setsync_prop->s = ISS_OFF;
        sendNewSwitch(oncoordset_prop);
        INumber *raprop = IUFindNumber(coord_prop, "RA");
        INumber *decprop = IUFindNumber(coord_prop, "DEC");
        raprop->value = ra;
        decprop->value = dec;
        sendNewNumber(coord_prop);
        err = false;
    }

    return err;
}

void ScopeINDI::AbortSlew()
{
    if (AbortMotion_prop && Abort_prop)
    {
        Abort_prop->s = ISS_ON;
        sendNewSwitch(AbortMotion_prop);
    }
}

bool ScopeINDI::Slewing()
{
    return coord_prop && coord_prop->s == IPS_BUSY;
}

PierSide ScopeINDI::SideOfPier()
{
    PierSide pierSide = PIER_SIDE_UNKNOWN;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("INDI Scope: cannot get side of pier when not connected");
        }

        if (!pierside_prop)
        {
            throw THROW_INFO("INDI Scope: not capable of getting side of pier");
        }

        if (piersideEast_prop->s == ISS_ON)
        {
            pierSide = PIER_SIDE_EAST;
        }
        else if (piersideWest_prop->s == ISS_ON)
        {
            pierSide = PIER_SIDE_WEST;
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    Debug.Write(wxString::Format("INDI Mount: SideOfPier returns %d\n", pierSide));

    return pierSide;
}

bool ScopeINDI::HasNonGuiMove()
{
    return true;
}

#endif /* GUIDE_INDI */
