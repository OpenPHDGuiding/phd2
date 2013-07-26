/*
 *  event_server.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2013 Andy Galasso.
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

#include <wx/sstream.h>
#include <wx/sckstrm.h>

EventServer EvtServer;

BEGIN_EVENT_TABLE(EventServer, wxEvtHandler)
    EVT_SOCKET(EVENT_SERVER_ID, EventServer::OnEventServerEvent)
    EVT_SOCKET(EVENT_SERVER_CLIENT_ID, EventServer::OnEventServerClientEvent)
END_EVENT_TABLE()

enum
{
    MSG_PROTOCOL_VERSION = 1,
};

static wxString state_name(EXPOSED_STATE st)
{
    switch (st)
    {
        case EXPOSED_STATE_NONE:             return "Stopped";
        case EXPOSED_STATE_SELECTED:         return "Selected";
        case EXPOSED_STATE_CALIBRATING:      return "Calibrating";
        case EXPOSED_STATE_GUIDING_LOCKED:   return "Guiding";
        case EXPOSED_STATE_GUIDING_LOST:     return "LostLock";
        case EXPOSED_STATE_PAUSED:           return "Paused";
        case EXPOSED_STATE_LOOPING:          return "Looping";
        default:                             return "Unknown";
    }
}

// name-value pair
struct NV
{
    wxString n;
    wxString v;
    NV(const wxString& n_, const wxString& v_) : n(n_), v('"'+v_+'"') { }
    NV(const wxString& n_, int v_) : n(n_), v(wxString::Format("%d", v_)) { }
    NV(const wxString& n_, double v_) : n(n_), v(wxString::Format("%g", v_)) { }
    NV(const wxString& n_, double v_, int prec) : n(n_), v(wxString::Format("%.*f", prec, v_)) { }
};

NV NVMount(const Mount *mount)
{
    return NV("Mount", mount->Name());
}

struct JObj
{
    wxString m_s;
    bool m_first;
    bool m_closed;
    JObj() : m_first(true), m_closed(false) { m_s << "{"; }
    void close() { m_s << "}"; m_closed = true; }
    wxString str() { if (!m_closed) close(); return m_s; }
};

JObj& operator<<(JObj& j, const NV& nv)
{
    if (j.m_first)
        j.m_first = false;
    else
        j.m_s << ',';
    j.m_s << '"' << nv.n << "\":" << nv.v;
    return j;
}

JObj& operator<<(JObj& j, const PHD_Point& pt)
{
    j << NV("X", pt.X, 3) << NV("Y", pt.Y, 3);
    return j;
}

struct Ev : public JObj
{
    Ev(const wxString& event)
    {
        double const now = ::wxGetUTCTimeMillis().ToDouble() / 1000.0;
        *this << NV("Event", event)
            << NV("Timestamp", now, 3)
            << NV("Host", wxGetHostName())
            << NV("Inst", pFrame->GetInstanceNumber());
    }
};

static Ev ev_message_version()
{
    Ev ev("Version");
    ev << NV("PHDVersion", PHDVERSION)
        << NV("PHDSubver", PHDSUBVER)
        << NV("MsgVersion", MSG_PROTOCOL_VERSION);
    return ev;
}

static Ev ev_set_lock_position(const PHD_Point& xy)
{
    Ev ev("LockPositionSet");
    ev << xy;
    return ev;
}

static Ev ev_calibration_complete(Mount *mount)
{
    Ev ev("CalibrationComplete");
    ev << NVMount(mount);
    return ev;
}

static Ev ev_star_selected(const PHD_Point& pos)
{
    Ev ev("StarSelected");
    ev << pos;
    return ev;
}

static Ev ev_start_guiding()
{
    return Ev("StartGuiding");
}

static Ev ev_paused()
{
    return Ev("Paused");
}

static Ev ev_start_calibration(Mount *mount)
{
    Ev ev("StartCalibration");
    ev << NVMount(mount);
    return ev;
}

static Ev ev_app_state(EXPOSED_STATE st = Guider::GetExposedState())
{
    Ev ev("AppState");
    ev << NV("State", state_name(st));
    return ev;
}

static void send_buf(wxSocketClient *client, const wxCharBuffer& buf)
{
    client->Write(buf.data(), buf.length());
    client->Write("\r\n", 2);
}

static void do_notify1(wxSocketClient *client, const JObj& jj)
{
    send_buf(client, JObj(jj).str().ToUTF8());
}

static void do_notify(const EventServer::CliSockSet& cli, const JObj& jj)
{
    wxCharBuffer buf = JObj(jj).str().ToUTF8();

    for (EventServer::CliSockSet::const_iterator it = cli.begin();
        it != cli.end(); ++it)
    {
        send_buf(*it, buf);
    }
}

inline static void simple_notify(const EventServer::CliSockSet& cli, const wxString& ev)
{
    if (!cli.empty())
        do_notify(cli, Ev(ev));
}

inline static void simple_notify_ev(const EventServer::CliSockSet& cli, const Ev& ev)
{
    if (!cli.empty())
        do_notify(cli, ev);
}

#define SIMPLE_NOTIFY(s) simple_notify(m_eventServerClients, s)
#define SIMPLE_NOTIFY_EV(ev) simple_notify_ev(m_eventServerClients, ev)

static void send_catchup_events(wxSocketClient *cli)
{
    EXPOSED_STATE st = Guider::GetExposedState();

    do_notify1(cli, ev_message_version());

    if (pFrame->pGuider)
    {
        if (pFrame->pGuider->LockPosition().IsValid())
            do_notify1(cli, ev_set_lock_position(pFrame->pGuider->LockPosition()));

        if (pFrame->pGuider->CurrentPosition().IsValid())
            do_notify1(cli, ev_star_selected(pFrame->pGuider->CurrentPosition()));
    }

    if (pMount && pMount->IsCalibrated())
        do_notify1(cli, ev_calibration_complete(pMount));

    if (pSecondaryMount && pSecondaryMount->IsCalibrated())
        do_notify1(cli, ev_calibration_complete(pSecondaryMount));

    if (st == EXPOSED_STATE_GUIDING_LOCKED)
    {
        do_notify1(cli, ev_start_guiding());
    }
    else if (st == EXPOSED_STATE_CALIBRATING)
    {
        Mount *mount = pMount;
        if (pFrame->pGuider->GetState() == STATE_CALIBRATING_SECONDARY)
            mount = pSecondaryMount;
        do_notify1(cli, ev_start_calibration(mount));
    }
    else if (st == EXPOSED_STATE_PAUSED) {
        do_notify1(cli, ev_paused());
    }

    do_notify1(cli, ev_app_state());
}

EventServer::EventServer()
{
}

EventServer::~EventServer()
{
}

bool EventServer::EventServerStart(unsigned int instanceId)
{
    if (m_serverSocket)
    {
        Debug.AddLine("attempt to start event server when it is already started?");
        return false;
    }

    unsigned int port = 4400 + instanceId - 1;
    wxIPV4address eventServerAddr;
    eventServerAddr.Service(port);
    m_serverSocket = new wxSocketServer(eventServerAddr);

    if (!m_serverSocket->Ok())
    {
        wxLogStatus(wxString::Format("Event server failed to start - Could not listen at port %u", port));
        delete m_serverSocket;
        m_serverSocket = NULL;
        return true;
    }

    m_serverSocket->SetEventHandler(*this, EVENT_SERVER_ID);
    m_serverSocket->SetNotify(wxSOCKET_CONNECTION_FLAG);
    m_serverSocket->Notify(true);

    Debug.AddLine(wxString::Format("event server started, listening on port %u", port));

    return false;
}

void EventServer::EventServerStop()
{
    if (!m_serverSocket)
        return;

    for (CliSockSet::const_iterator it = m_eventServerClients.begin();
         it != m_eventServerClients.end(); ++it)
    {
        (*it)->Destroy();
    }
    m_eventServerClients.clear();

    delete m_serverSocket;
    m_serverSocket = NULL;

    Debug.AddLine("event server stopped");
}

void EventServer::OnEventServerEvent(wxSocketEvent& event)
{
    wxSocketServer *server = static_cast<wxSocketServer *>(event.GetSocket());

    if (event.GetSocketEvent() != wxSOCKET_CONNECTION)
        return;

    wxSocketClient *client = static_cast<wxSocketClient *>(server->Accept(false));

    if (!client)
        return;

    Debug.AddLine("event server client connected");

    client->SetEventHandler(*this, EVENT_SERVER_CLIENT_ID);
    client->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
    client->SetFlags(wxSOCKET_NOWAIT);
    client->Notify(true);

    send_catchup_events(client);

    m_eventServerClients.insert(client);
}

void EventServer::OnEventServerClientEvent(wxSocketEvent& event)
{
    wxSocketClient *cli = static_cast<wxSocketClient *>(event.GetSocket());

    if (event.GetSocketEvent() == wxSOCKET_LOST)
    {
        Debug.AddLine("event server client disconnected");

        unsigned int const n = m_eventServerClients.erase(cli);
        if (n != 1)
            Debug.AddLine("client disconnected but not present in client set!");

        cli->Destroy();
    }
    else if (event.GetSocketEvent() == wxSOCKET_INPUT)
    {
        // consume the input
        // TODO: parse input message
        wxSocketInputStream sis(*cli);
        while (sis.CanRead())
        {
            char buf[4096];
            sis.Read(buf, sizeof(buf));
            if (sis.LastRead() == 0)
                break;
        }

        do_notify1(cli, ev_app_state());
    }
    else
    {
        Debug.AddLine("unexpected client socket event %d", event.GetSocketEvent());
    }
}

void EventServer::NotifyStartCalibration(Mount *mount)
{
    SIMPLE_NOTIFY_EV(ev_start_calibration(mount));
}

void EventServer::NotifyCalibrationFailed(Mount *mount, const wxString& msg)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("CalibrationFailed");
    ev << NVMount(mount) << NV("Reason", msg);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyCalibrationComplete(Mount *mount)
{
    if (m_eventServerClients.empty())
        return;

    do_notify(m_eventServerClients, ev_calibration_complete(mount));
}

void EventServer::NotifyCalibrationDataFlipped(Mount *mount)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("CalibrationDataFlipped");
    ev << NVMount(mount);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyLooping(unsigned int exposure)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("LoopingExposures");
    ev << NV("Frame", (int) exposure);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyLoopingStopped()
{
    SIMPLE_NOTIFY("LoopingExposuresStopped");
}

void EventServer::NotifyStarSelected(const PHD_Point& pt)
{
    SIMPLE_NOTIFY_EV(ev_star_selected(pt));
}

void EventServer::NotifyStarLost()
{
    SIMPLE_NOTIFY("StarLost");
}

void EventServer::NotifyStartGuiding()
{
    SIMPLE_NOTIFY_EV(ev_start_guiding());
}

void EventServer::NotifyGuidingStopped()
{
    SIMPLE_NOTIFY("GuidingStopped");
}

void EventServer::NotifyPaused()
{
    SIMPLE_NOTIFY_EV(ev_paused());
}

void EventServer::NotifyResumed()
{
    SIMPLE_NOTIFY("Resumed");
}

void EventServer::NotifyGuideStep(Mount *pGuideMount, const PHD_Point& vectorEndpoint, double RADuration, double RADistance,
    double DECDuration, double DECDistance, int errorCode)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("GuideStep");

    ev << NV("Frame", (int) pFrame->m_frameCounter)
       << NV("Time", (wxDateTime::UNow() - pFrame->m_guidingStarted).GetMilliseconds().ToDouble() / 1000.0, 3)
       << NV("mount", pGuideMount->Name())
       << NV("dx", vectorEndpoint.X, 3)
       << NV("dy", vectorEndpoint.Y, 3)
       << NV("Theta", vectorEndpoint.Angle(PHD_Point(0., 0.)), 3)
       << NV("RADuration", RADuration, 3)
       << NV("RADistance", RADistance, 3)
       << NV("RADirection", RADistance >= 0.0 ? "E" : "W")
       << NV("DECDuration", DECDuration, 3)
       << NV("DECDistance", DECDistance, 3)
       << NV("DECDirection", DECDistance >= 0.0 ? "S" : "N")
       << NV("StarMass", pFrame->pGuider->StarMass(), 0)
       << NV("SNR", pFrame->pGuider->SNR(), 2);

    if (errorCode)
       ev << NV("ErrorCode", errorCode);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyGuidingDithered(double dx, double dy)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("GuidingDithered");
    ev << NV("dx", dx, 3) << NV("dy", dy, 3);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifySetLockPosition(const PHD_Point& xy)
{
    if (m_eventServerClients.empty())
        return;

    do_notify(m_eventServerClients, ev_set_lock_position(xy));
}

void EventServer::NotifyLockPositionLost()
{
    SIMPLE_NOTIFY("LockPositionLost");
}

void EventServer::NotifyAppState()
{
    if (m_eventServerClients.empty())
        return;

    do_notify(m_eventServerClients, ev_app_state());
}
