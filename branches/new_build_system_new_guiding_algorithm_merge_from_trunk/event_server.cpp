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
#include <sstream>

EventServer EvtServer;

BEGIN_EVENT_TABLE(EventServer, wxEvtHandler)
    EVT_SOCKET(EVENT_SERVER_ID, EventServer::OnEventServerEvent)
    EVT_SOCKET(EVENT_SERVER_CLIENT_ID, EventServer::OnEventServerClientEvent)
END_EVENT_TABLE()

enum
{
    MSG_PROTOCOL_VERSION = 1,
};

static const wxString literal_null("null");
static const wxString literal_true("true");
static const wxString literal_false("false");

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

static wxString json_escape(const wxString& s)
{
    wxString t(s);
    static const wxString BACKSLASH("\\");
    static const wxString BACKSLASHBACKSLASH("\\\\");
    static const wxString DQUOT("\"");
    static const wxString BACKSLASHDQUOT("\\\"");
    t.Replace(BACKSLASH, BACKSLASHBACKSLASH);
    t.Replace(DQUOT, BACKSLASHDQUOT);
    return t;
}

template<char LDELIM, char RDELIM>
struct JSeq
{
    wxString m_s;
    bool m_first;
    bool m_closed;
    JSeq() : m_first(true), m_closed(false) { m_s << LDELIM; }
    void close() { m_s << RDELIM; m_closed = true; }
    wxString str() { if (!m_closed) close(); return m_s; }
};

typedef JSeq<'[', ']'> JAry;
typedef JSeq<'{', '}'> JObj;

static JAry& operator<<(JAry& a, const wxString& str)
{
    if (a.m_first)
        a.m_first = false;
    else
        a.m_s << ',';
    a.m_s << json_escape(str);
    return a;
}

static JAry& operator<<(JAry& a, double d)
{
    return a << wxString::Format("%.2f", d);
}

static JAry& operator<<(JAry& a, int i)
{
    return a << wxString::Format("%d", i);
}

static wxString json_format(const json_value *j)
{
    if (!j)
        return literal_null;

    switch (j->type) {
    default:
    case JSON_NULL: return literal_null;
    case JSON_OBJECT: {
        wxString ret("{");
        bool first = true;
        json_for_each (jj, j)
        {
            if (first)
                first = false;
            else
                ret << ",";
            ret << '"' << jj->name << "\":" << json_format(jj);
        }
        ret << "}";
        return ret;
    }
    case JSON_ARRAY: {
        wxString ret("[");
        bool first = true;
        json_for_each (jj, j)
        {
            if (first)
                first = false;
            else
                ret << ",";
            ret << json_format(jj);
        }
        ret << "]";
        return ret;
    }
    case JSON_STRING: return '"' + json_escape(j->string_value) + '"';
    case JSON_INT:    return wxString::Format("%d", j->int_value);
    case JSON_FLOAT:  return wxString::Format("%g", (double) j->float_value);
    case JSON_BOOL:   return j->int_value ? literal_true : literal_false;
    }
}

struct NULL_TYPE { } NULL_VALUE;

// name-value pair
struct NV
{
    wxString n;
    wxString v;
    NV(const wxString& n_, const wxString& v_) : n(n_), v('"' + json_escape(v_) + '"') { }
    NV(const wxString& n_, const char *v_) : n(n_), v('"' + json_escape(v_) + '"') { }
    NV(const wxString& n_, const wchar_t *v_) : n(n_), v('"' + json_escape(v_) + '"') { }
    NV(const wxString& n_, int v_) : n(n_), v(wxString::Format("%d", v_)) { }
    NV(const wxString& n_, double v_) : n(n_), v(wxString::Format("%g", v_)) { }
    NV(const wxString& n_, double v_, int prec) : n(n_), v(wxString::Format("%.*f", prec, v_)) { }
    NV(const wxString& n_, bool v_) : n(n_), v(v_ ? literal_true : literal_false) { }
    template<typename T>
    NV(const wxString& n_, const std::vector<T>& vec);
    NV(const wxString& n_, JAry& ary) : n(n_), v(ary.str()) { }
    NV(const wxString& n_, JObj& obj) : n(n_), v(obj.str()) { }
    NV(const wxString& n_, const json_value *v_) : n(n_), v(json_format(v_)) { }
    NV(const wxString& n_, const PHD_Point& p) : n(n_) { JAry ary; ary << p.X << p.Y; v = ary.str(); }
    NV(const wxString& n_, const wxPoint& p) : n(n_) { JAry ary; ary << p.x << p.y; v = ary.str(); }
    NV(const wxString& n_, const NULL_TYPE& nul) : n(n_), v(literal_null) { }
};

template<typename T>
NV::NV(const wxString& n_, const std::vector<T>& vec)
    : n(n_)
{
    std::ostringstream os;
    os << '[';
    for (unsigned int i = 0; i < vec.size(); i++)
    {
        if (i != 0)
            os << ',';
        os << vec[i];
    }
    os << ']';
    v = os.str();
}

static JObj& operator<<(JObj& j, const NV& nv)
{
    if (j.m_first)
        j.m_first = false;
    else
        j.m_s << ',';
    j.m_s << '"' << nv.n << "\":" << nv.v;
    return j;
}

static NV NVMount(const Mount *mount)
{
    return NV("Mount", mount->Name());
}

static JObj& operator<<(JObj& j, const PHD_Point& pt)
{
    return j << NV("X", pt.X, 3) << NV("Y", pt.Y, 3);
}

static JAry& operator<<(JAry& a, JObj& j)
{
    return a << j.str();
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

    if (mount->IsStepGuider())
    {
        ev << NV("Limit", mount->GetAoMaxPos());
    }

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

static Ev ev_settling(double distance, double time, double settleTime)
{
    Ev ev("Settling");

    ev << NV("Distance", distance, 2)
       << NV("Time", time, 1)
       << NV("SettleTime", settleTime, 1);

    return ev;
}

static Ev ev_settle_done(const wxString& errorMsg)
{
    Ev ev("SettleDone");

    int status = errorMsg.IsEmpty() ? 0 : 1;

    ev << NV("Status", status);

    if (status != 0)
    {
        ev << NV("Error", errorMsg);
    }

    return ev;
}

static void send_buf(wxSocketClient *client, const wxCharBuffer& buf)
{
    client->Write(buf.data(), buf.length());
    client->Write("\r\n", 2);
}

static void do_notify1(wxSocketClient *client, const JAry& ary)
{
    send_buf(client, JAry(ary).str().ToUTF8());
}

static void do_notify1(wxSocketClient *client, const JObj& j)
{
    send_buf(client, JObj(j).str().ToUTF8());
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

struct ClientReadBuf
{
    enum { SIZE = 1024 };
    char buf[SIZE];
    char *dest;

    ClientReadBuf() { reset(); }
    size_t avail() const { return &buf[SIZE] - dest; }
    void reset() { dest = &buf[0]; }
};

inline static ClientReadBuf *client_rdbuf(wxSocketClient *cli)
{
    return (ClientReadBuf *) cli->GetClientData();
}

static void destroy_client(wxSocketClient *cli)
{
    ClientReadBuf *buf = client_rdbuf(cli);
    cli->Destroy();
    delete buf;
}

static void drain_input(wxSocketInputStream& sis)
{
    while (sis.CanRead())
    {
        char buf[1024];
        if (sis.Read(buf, sizeof(buf)).LastRead() == 0)
            break;
    }
}

static bool find_eol(char *p, size_t len)
{
    const char *end = p + len;
    for (; p < end; p++)
    {
        if (*p == '\r' || *p == '\n')
        {
            *p = 0;
            return true;
        }
    }
    return false;
}

enum {
    JSONRPC_PARSE_ERROR = -32700,
    JSONRPC_INVALID_REQUEST = -32600,
    JSONRPC_METHOD_NOT_FOUND = -32601,
    JSONRPC_INVALID_PARAMS = -32602,
    JSONRPC_INTERNAL_ERROR = -32603,
};

static NV jrpc_error(int code, const wxString& msg)
{
    JObj err;
    err << NV("code", code) << NV("message", msg);
    return NV("error", err);
}

template<typename T>
static NV jrpc_result(const T& t)
{
    return NV("result", t);
}

template<typename T>
static NV jrpc_result(T& t)
{
    return NV("result", t);
}

static NV jrpc_id(const json_value *id)
{
    return NV("id", id);
}

struct JRpcResponse : public JObj
{
    JRpcResponse() { *this << NV("jsonrpc", "2.0"); }
};

static wxString parser_error(const JsonParser& parser)
{
    return wxString::Format("invalid JSON request: %s on line %d at \"%.12s...\"",
        parser.ErrorDesc(), parser.ErrorLine(), parser.ErrorPos());
}

static void
parse_request(const json_value *req, const json_value **pmethod, const json_value **pparams,
              const json_value **pid)
{
    *pmethod = *pparams = *pid = 0;

    if (req)
    {
        json_for_each (t, req)
        {
            if (t->name)
            {
                if (t->type == JSON_STRING && strcmp(t->name, "method") == 0)
                    *pmethod = t;
                else if (strcmp(t->name, "params") == 0)
                    *pparams = t;
                else if (strcmp(t->name, "id") == 0)
                    *pid = t;
            }
        }
    }
}

static const json_value *at(const json_value *ary, unsigned int idx)
{
    unsigned int i = 0;
    json_for_each (j, ary)
    {
        if (i == idx)
            return j;
    }
    return 0;
}

static void deselect_star(JObj& response, const json_value *params)
{
    pFrame->pGuider->Reset(true);
    response << jrpc_result(0);
}

static void get_exposure(JObj& response, const json_value *params)
{
    response << jrpc_result(pFrame->RequestedExposureDuration());
}

static void get_exposure_durations(JObj& response, const json_value *params)
{
    std::vector<int> exposure_durations;
    pFrame->GetExposureDurations(&exposure_durations);
    response << jrpc_result(exposure_durations);
}

static void get_profiles(JObj& response, const json_value *params)
{
    JAry ary;
    wxArrayString names = pConfig->ProfileNames();
    for (unsigned int i = 0; i < names.size(); i++)
    {
        wxString name = names[i];
        int id = pConfig->GetProfileId(name);
        if (id)
        {
            JObj t;
            t << NV("id", id) << NV("name", name);
            if (id == pConfig->GetCurrentProfileId())
                t << NV("selected", true);
            ary << t;
        }
    }
    response << jrpc_result(ary);
}

static void set_exposure(JObj& response, const json_value *params)
{
    const json_value *exp;
    if (!params || (exp = at(params, 0)) == 0 || exp->type != JSON_INT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected exposure param");
        return;
    }

    bool ok = pFrame->SetExposureDuration(exp->int_value);
    if (ok)
    {
        response << jrpc_result(1);
    }
    else
    {
        response << jrpc_error(1, "could not set exposure duration");
    }
}

static void get_profile(JObj& response, const json_value *params)
{
    int id = pConfig->GetCurrentProfileId();
    wxString name = pConfig->GetCurrentProfile();
    JObj t;
    t << NV("id", id) << NV("name", name);
    response << jrpc_result(t);
}

static bool all_equipment_connected()
{
    return pCamera && pCamera->Connected &&
        (!pMount || pMount->IsConnected()) &&
        (!pSecondaryMount || pSecondaryMount->IsConnected());
}

static void set_profile(JObj& response, const json_value *params)
{
    const json_value *id;
    if (!params || (id = at(params, 0)) == 0 || id->type != JSON_INT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected profile id param");
        return;
    }

    if (!pFrame || !pFrame->pGearDialog) // paranoia
    {
        response << jrpc_error(1, "internal error");
        return;
    }

    wxString errMsg;
    bool error = pFrame->pGearDialog->SetProfile(id->int_value, &errMsg);

    if (error)
    {
        response << jrpc_error(1, errMsg);
    }
    else
    {
        response << jrpc_result(0);
    }
}

static void get_connected(JObj& response, const json_value *params)
{
    response << jrpc_result(all_equipment_connected());
}

static void set_connected(JObj& response, const json_value *params)
{
    const json_value *val;
    if (!params || (val = at(params, 0)) == 0 || val->type != JSON_BOOL)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected connected boolean param");
        return;
    }

    if (!pFrame || !pFrame->pGearDialog) // paranoia
    {
        response << jrpc_error(1, "internal error");
        return;
    }

    wxString errMsg;
    bool error = val->int_value ? pFrame->pGearDialog->ConnectAll(&errMsg) :
        pFrame->pGearDialog->DisconnectAll(&errMsg);

    if (error)
    {
        response << jrpc_error(1, errMsg);
    }
    else
    {
        response << jrpc_result(0);
    }
}

static void get_calibrated(JObj& response, const json_value *params)
{
    bool calibrated = pMount && pMount->IsCalibrated() && (!pSecondaryMount || pSecondaryMount->IsCalibrated());
    response << jrpc_result(calibrated);
}

static bool float_param(const json_value *v, double *p)
{
    if (v->type == JSON_INT)
    {
        *p = (double) v->int_value;
        return true;
    }
    else if (v->type == JSON_FLOAT)
    {
        *p = v->float_value;
        return true;
    }

    return false;
}

static bool float_param(const char *name, const json_value *v, double *p)
{
    if (strcmp(name, v->name) != 0)
        return false;

    return float_param(v, p);
}

static void get_paused(JObj& response, const json_value *params)
{
    response << jrpc_result(pFrame->pGuider->IsPaused());
}

static void set_paused(JObj& response, const json_value *params)
{
    const json_value *p;
    bool val;

    if (!params || (p = at(params, 0)) == 0 || p->type != JSON_BOOL)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected bool param at index 0");
        return;
    }
    val = p->int_value ? true : false;

    PauseType pause = PAUSE_NONE;

    if (val)
    {
        pause = PAUSE_GUIDING;

        p = at(params, 1);
        if (p)
        {
            if (p->type == JSON_STRING)
            {
                if (strcmp(p->string_value, "full") == 0)
                    pause = PAUSE_FULL;
            }
            else
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected string param at index 1");
                return;
            }
        }
    }

    pFrame->SetPaused(pause);

    response << jrpc_result(0);
}

static void loop(JObj& response, const json_value *params)
{
    bool error = pFrame->StartLooping();

    if (error)
        response << jrpc_error(1, "could not start looping");
    else
        response << jrpc_result(0);
}

static void stop_capture(JObj& response, const json_value *params)
{
    pFrame->StopCapturing();
    response << jrpc_result(0);
}

static void find_star(JObj& response, const json_value *params)
{
    bool error = pFrame->pGuider->AutoSelect();

    if (!error)
    {
        const PHD_Point& lockPos = pFrame->pGuider->LockPosition();
        if (lockPos.IsValid())
        {
            response << jrpc_result(lockPos);
            return;
        }
    }

    response << jrpc_error(1, "could not find star");
}

static void get_pixel_scale(JObj& response, const json_value *params)
{
    double scale = pFrame->GetCameraPixelScale();
    if (scale == 1.0)
        response << jrpc_result(NULL_VALUE); // scale unknown
    else
        response << jrpc_result(scale);
}

static void get_app_state(JObj& response, const json_value *params)
{
    EXPOSED_STATE st = Guider::GetExposedState();
    response << jrpc_result(state_name(st));
}

static void get_lock_position(JObj& response, const json_value *params)
{
    const PHD_Point& lockPos = pFrame->pGuider->LockPosition();
    if (lockPos.IsValid())
        response << jrpc_result(lockPos);
    else
        response << jrpc_result(NULL_VALUE);
}

// {"method": "set_lock_position", "params": [X, Y, true], "id": 1}
static void set_lock_position(JObj& response, const json_value *params)
{
    const json_value *p0, *p1;
    double x, y;

    if (!params || (p0 = at(params, 0)) == 0 || (p1 = at(params, 1)) == 0 ||
        !float_param(p0, &x) || !float_param(p1, &y))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected lock position x, y params");
        return;
    }

    bool exact = true;
    const json_value *p2 = at(params, 2);
    if (p2)
    {
        if (p2->type != JSON_BOOL)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected boolean param at index 2");
            return;
        }
        exact = p2->int_value ? true : false;
    }

    bool error;

    if (exact)
        error = pFrame->pGuider->SetLockPosition(PHD_Point(x, y));
    else
        error = pFrame->pGuider->SetLockPosToStarAtPosition(PHD_Point(x, y));

    if (error)
    {
        response << jrpc_error(JSONRPC_INVALID_REQUEST, "could not set lock position");
        return;
    }

    response << jrpc_result(0);
}

inline static const char *string_val(const json_value *j)
{
    return j->type == JSON_STRING ? j->string_value : "";
}

static void clear_calibration(JObj& response, const json_value *params)
{
    bool clear_mount;
    bool clear_ao;

    if (!params)
    {
        clear_mount = clear_ao = true;
    }
    else
    {
        clear_mount = clear_ao = false;

        json_for_each (val, params)
        {
            const char *which = string_val(val);
            if (strcmp(which, "mount") == 0)
                clear_mount = true;
            else if (strcmp(which, "ao") == 0)
                clear_ao = true;
            else if (strcmp(which, "both") == 0)
                clear_mount = clear_ao = true;
            else
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected param \"mount\", \"ao\", or \"both\"");
                return;
            }
        }
    }

    Mount *mount;
    Mount *ao;
    if (pMount && pMount->IsStepGuider())
        ao = pMount, mount = pSecondaryMount;
    else
        ao = 0, mount = pMount;

    if (mount && clear_mount)
        mount->ClearCalibration();

    if (ao && clear_ao)
        ao->ClearCalibration();

    response << jrpc_result(0);
}

static void flip_calibration(JObj& response, const json_value *params)
{
    bool error = pFrame->FlipRACal();

    if (error)
        response << jrpc_error(1, "could not flip calibration");
    else
        response << jrpc_result(0);
}

static void get_lock_shift_enabled(JObj& response, const json_value *params)
{
    bool enabled = pFrame->pGuider->GetLockPosShiftParams().shiftEnabled;
    response << jrpc_result(enabled);
}

static void set_lock_shift_enabled(JObj& response, const json_value *params)
{
    const json_value *val;
    if (!params || (val = at(params, 0)) == 0 || val->type != JSON_BOOL)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected enabled boolean param");
        return;
    }

    if (!pFrame || !pFrame->pGuider) // paranoia
    {
        response << jrpc_error(1, "internal error");
        return;
    }

    pFrame->pGuider->EnableLockPosShift(val->int_value ? true : false);

    response << jrpc_result(0);
}

static void get_lock_shift_params(JObj& response, const json_value *params)
{
    const LockPosShiftParams& lockShift = pFrame->pGuider->GetLockPosShiftParams();
    JObj rslt;
    rslt << NV("enabled", lockShift.shiftEnabled);
    if (lockShift.shiftRate.IsValid())
    {
        rslt << NV("rate", lockShift.shiftRate)
             << NV("units", lockShift.shiftUnits == UNIT_ARCSEC ? "arcsec/hr" : "pixels/hr")
             << NV("axes", lockShift.shiftIsMountCoords ? "RA/Dec" : "X/Y");
    }
    response << jrpc_result(rslt);
}

static bool get_double(double *d, const json_value *j)
{
    if (j->type == JSON_FLOAT)
    {
        *d = j->float_value;
        return true;
    }
    else if (j->type == JSON_INT)
    {
        *d = j->int_value;
        return true;
    }
    return false;
}

static bool parse_point(PHD_Point *pt, const json_value *j)
{
    if (j->type != JSON_ARRAY)
        return false;
    const json_value *jx = j->first_child;
    if (!jx)
        return false;
    const json_value *jy = jx->next_sibling;
    if (!jy || jy->next_sibling)
        return false;
    double x, y;
    if (!get_double(&x, jx) || !get_double(&y, jy))
        return false;
    pt->SetXY(x, y);
    return true;
}

static bool parse_lock_shift_params(LockPosShiftParams *shift, const json_value *params, wxString *error)
{
    // {"rate":[3.3,1.1],"units":"arcsec/hr","axes":"RA/Dec"}
    const json_value *p0;
    if (!params || (p0 = at(params, 0)) == 0 || p0->type != JSON_OBJECT)
    {
        *error = "expected lock shift object param";
        return false;
    }
    shift->shiftUnits = UNIT_ARCSEC;
    shift->shiftIsMountCoords = true;

    json_for_each (j, p0)
    {
        if (strcmp(j->name, "rate") == 0)
        {
            if (!parse_point(&shift->shiftRate, j))
            {
                *error = "expected rate value array";
                return false;
            }
        }
        else if (strcmp(j->name, "units") == 0)
        {
            const char *units = string_val(j);
            if (wxStricmp(units, "arcsec/hr") == 0 ||
                wxStricmp(units, "arc-sec/hr") == 0)
            {
                shift->shiftUnits = UNIT_ARCSEC;
            }
            else if (wxStricmp(units, "pixels/hr") == 0)
            {
                shift->shiftUnits = UNIT_PIXELS;
            }
            else
            {
                *error = "expected units 'arcsec/hr' or 'pixels/hr'";
                return false;
            }
        }
        else if (strcmp(j->name, "axes") == 0)
        {
            const char *axes = string_val(j);
            if (wxStricmp(axes, "RA/Dec") == 0)
            {
                shift->shiftIsMountCoords = true;
            }
            else if (wxStricmp(axes, "X/Y") == 0)
            {
                shift->shiftIsMountCoords = false;
            }
            else
            {
                *error = "expected axes 'RA/Dec' or 'X/Y'";
                return false;
            }
        }
        else
        {
            *error = "unknown lock shift attribute name";
            return false;
        }
    }

    return true;
}

static void set_lock_shift_params(JObj& response, const json_value *params)
{
    wxString err;
    LockPosShiftParams shift;
    if (!parse_lock_shift_params(&shift, params, &err))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, err);
        return;
    }

    if (!pFrame || !pFrame->pGuider)
    {
        response << jrpc_error(1, "internal error");
        return;
    }

    pFrame->pGuider->SetLockPosShiftRate(shift.shiftRate, shift.shiftUnits, shift.shiftIsMountCoords);

    response << jrpc_result(0);
}

static void save_image(JObj& response, const json_value *params)
{
    if (!pFrame || ! pFrame->pGuider)
    {
        response << jrpc_error(1, "internal error");
        return;
    }

    if (!pFrame->pGuider->CurrentImage()->ImageData)
    {
        response << jrpc_error(2, "no image available");
        return;
    }

    wxString fname = wxFileName::CreateTempFileName(MyFrame::GetDefaultFileDir() + PATHSEPSTR + "save_image_");

    if (pFrame->pGuider->SaveCurrentImage(fname))
    {
        ::wxRemove(fname);
        response << jrpc_error(3, "error saving image");
        return;
    }

    JObj rslt;
    rslt << NV("filename", fname);
    response << jrpc_result(rslt);
}

static bool parse_settle(SettleParams *settle, const json_value *j, wxString *error)
{
    bool found_pixels = false, found_time = false, found_timeout = false;

    json_for_each (t, j)
    {
        if (float_param("pixels", t, &settle->tolerancePx))
        {
            found_pixels = true;
            continue;
        }
        double d;
        if (float_param("time", t, &d))
        {
            settle->settleTimeSec = (int) floor(d);
            found_time = true;
            continue;
        }
        if (float_param("timeout", t, &d))
        {
            settle->timeoutSec = (int) floor(d);
            found_timeout = true;
            continue;
        }
    }

    bool ok = found_pixels && found_time && found_timeout;
    if (!ok)
        *error = "invalid settle params";

    return ok;
}

static void guide(JObj& response, const json_value *params)
{
    // params:
    //   settle [object]:
    //     pixels [float]
    //     arcsecs [float]
    //     frames [integer]
    //     time [integer]
    //     timeout [integer]
    //   recalibrate: boolean
    //
    // {"method": "guide", "params": [{"pixels": 0.5, "time": 6, "timeout": 30}, false], "id": 42}
    //
    // todo:
    //   accept tolerance in arcsec or pixels
    //   accept settle time in seconds or frames

    SettleParams settle;

    const json_value *p0;
    if (!params || (p0 = at(params, 0)) == 0 || p0->type != JSON_OBJECT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected settle object param");
        return;
    }
    wxString errMsg;
    if (!parse_settle(&settle, p0, &errMsg))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, errMsg);
        return;
    }

    bool recalibrate = false;
    const json_value *p1 = at(params, 1);
    if (p1 && (p1->type == JSON_BOOL || p1->type == JSON_INT))
    {
        recalibrate = p1->int_value ? true : false;
    }

    wxString err;
    if (PhdController::Guide(recalibrate, settle, &err))
        response << jrpc_result(0);
    else
        response << jrpc_error(1, err);
}

static void dither(JObj& response, const json_value *params)
{
    // params:
    //   amount [integer] - max pixels to move in each axis
    //   raOnly [bool] - when true, only dither ra
    //   settle [object]:
    //     pixels [float]
    //     arcsecs [float]
    //     frames [integer]
    //     time [integer]
    //     timeout [integer]
    //
    // {"method": "dither", "params": [10, false, {"pixels": 1.5, "time": 8, "timeout": 30}], "id": 42}

    const json_value *p;
    double ditherAmt;

    if (!params || (p = at(params, 0)) == 0 || !float_param(p, &ditherAmt))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected dither amount param");
        return;
    }

    if ((p = at(params, 1)) == 0 || p->type != JSON_BOOL)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected dither raOnly param");
        return;
    }
    bool raOnly = p->int_value ? true : false;

    SettleParams settle;

    if ((p = at(params, 2)) == 0 || p->type != JSON_OBJECT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected settle object param");
        return;
    }
    wxString errMsg;
    if (!parse_settle(&settle, p, &errMsg))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, errMsg);
        return;
    }

    wxString error;
    if (PhdController::Dither(fabs(ditherAmt), raOnly, settle, &error))
        response << jrpc_result(0);
    else
        response << jrpc_error(1, error);
}

static void dump_request(const wxSocketClient *cli, const json_value *req)
{
    Debug.AddLine(wxString::Format("evsrv: cli %p request: %s", cli, json_format(req)));
}

static void dump_response(const wxSocketClient *cli, const JRpcResponse& resp)
{
    Debug.AddLine(wxString::Format("evsrv: cli %p response: %s", cli, const_cast<JRpcResponse&>(resp).str()));
}

static bool handle_request(const wxSocketClient *cli, JObj& response, const json_value *req)
{
    const json_value *method;
    const json_value *params;
    const json_value *id;

    dump_request(cli, req);

    parse_request(req, &method, &params, &id);

    if (!method)
    {
        response << jrpc_error(JSONRPC_INVALID_REQUEST, "invalid request") << jrpc_id(0);
        return true;
    }

    static struct {
        const char *name;
        void (*fn)(JObj& response, const json_value *params);
    } methods[] = {
        { "clear_calibration", &clear_calibration, },
        { "deselect_star", &deselect_star, },
        { "get_exposure", &get_exposure, },
        { "set_exposure", &set_exposure, },
        { "get_exposure_durations", &get_exposure_durations, },
        { "get_profiles", &get_profiles, },
        { "get_profile", &get_profile, },
        { "set_profile", &set_profile, },
        { "get_connected", &get_connected, },
        { "set_connected", &set_connected, },
        { "get_calibrated", &get_calibrated, },
        { "get_paused", &get_paused, },
        { "set_paused", &set_paused, },
        { "get_lock_position", &get_lock_position, },
        { "set_lock_position", &set_lock_position, },
        { "loop", &loop, },
        { "stop_capture", &stop_capture, },
        { "guide", &guide, },
        { "dither", &dither, },
        { "find_star", &find_star, },
        { "get_pixel_scale", &get_pixel_scale, },
        { "get_app_state", &get_app_state, },
        { "flip_calibration", &flip_calibration, },
        { "get_lock_shift_enabled", &get_lock_shift_enabled, },
        { "set_lock_shift_enabled", &set_lock_shift_enabled, },
        { "get_lock_shift_params", &get_lock_shift_params, },
        { "set_lock_shift_params", &set_lock_shift_params, },
        { "save_image", &save_image, },
    };

    for (unsigned int i = 0; i < WXSIZEOF(methods); i++)
    {
        if (strcmp(method->string_value, methods[i].name) == 0)
        {
            (*methods[i].fn)(response, params);
            if (id)
            {
                response << jrpc_id(id);
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    if (id)
    {
        response << jrpc_error(JSONRPC_METHOD_NOT_FOUND, "method not found") << jrpc_id(id);
        return true;
    }
    else
    {
        return false;
    }
}

static void handle_cli_input_complete(wxSocketClient *cli, char *input, JsonParser& parser)
{
    if (!parser.Parse(input))
    {
        JRpcResponse response;
        response << jrpc_error(JSONRPC_PARSE_ERROR, parser_error(parser)) << jrpc_id(0);
        dump_response(cli, response);
        do_notify1(cli, response);
        return;
    }

    const json_value *root = parser.Root();

    if (root->type == JSON_ARRAY)
    {
        // a batch request

        JAry ary;

        bool found = false;
        json_for_each (req, root)
        {
            JRpcResponse response;
            if (handle_request(cli, response, req))
            {
                dump_response(cli, response);
                ary << response;
                found = true;
            }
        }

        if (found)
            do_notify1(cli, ary);
    }
    else
    {
        // a single request

        const json_value *const req = root;
        JRpcResponse response;
        if (handle_request(cli, response, req))
        {
            dump_response(cli, response);
            do_notify1(cli, response);
        }
    }
}

static void handle_cli_input(wxSocketClient *cli, JsonParser& parser)
{
    ClientReadBuf *rdbuf = client_rdbuf(cli);

    wxSocketInputStream sis(*cli);
    size_t avail = rdbuf->avail();

    while (sis.CanRead())
    {
        if (avail == 0)
        {
            drain_input(sis);

            JRpcResponse response;
            response << jrpc_error(JSONRPC_INTERNAL_ERROR, "too big") << jrpc_id(0);
            do_notify1(cli, response);

            rdbuf->reset();
            break;
        }
        size_t n = sis.Read(rdbuf->dest, avail).LastRead();
        if (n == 0)
            break;

        if (find_eol(rdbuf->dest, n))
        {
            drain_input(sis);
            handle_cli_input_complete(cli, &rdbuf->buf[0], parser);
            rdbuf->reset();
            break;
        }

        rdbuf->dest += n;
        avail -= n;
    }
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
        Debug.AddLine(wxString::Format("Event server failed to start - Could not listen at port %u", port));
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
        destroy_client(*it);
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

    Debug.AddLine("evsrv: cli %p connect", client);

    client->SetEventHandler(*this, EVENT_SERVER_CLIENT_ID);
    client->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
    client->SetFlags(wxSOCKET_NOWAIT);
    client->Notify(true);
    client->SetClientData(new ClientReadBuf());

    send_catchup_events(client);

    m_eventServerClients.insert(client);
}

void EventServer::OnEventServerClientEvent(wxSocketEvent& event)
{
    wxSocketClient *cli = static_cast<wxSocketClient *>(event.GetSocket());

    if (event.GetSocketEvent() == wxSOCKET_LOST)
    {
        Debug.AddLine("evsrv: cli %p disconnect", cli);

        unsigned int const n = m_eventServerClients.erase(cli);
        if (n != 1)
            Debug.AddLine("client disconnected but not present in client set!");

        destroy_client(cli);
    }
    else if (event.GetSocketEvent() == wxSOCKET_INPUT)
    {
        handle_cli_input(cli, m_parser);
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

void EventServer::NotifyStarLost(const FrameDroppedInfo& info)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("StarLost");

    ev << NV("Frame", info.frameNumber)
       << NV("Time", info.time, 3)
       << NV("StarMass", info.starMass, 0)
       << NV("SNR", info.starSNR, 2)
       << NV("AvgDist", info.avgDist, 2);

    if (info.starError)
        ev << NV("ErrorCode", info.starError);

    if (!info.status.IsEmpty())
        ev << NV("Status", info.status);

    do_notify(m_eventServerClients, ev);
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

void EventServer::NotifyGuideStep(const GuideStepInfo& step)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("GuideStep");

    ev << NV("Frame", step.frameNumber)
       << NV("Time", step.time, 3)
       << NVMount(step.mount)
       << NV("dx", step.cameraOffset->X, 3)
       << NV("dy", step.cameraOffset->Y, 3)
       << NV("RADistanceRaw", step.mountOffset->X, 3)
       << NV("DECDistanceRaw", step.mountOffset->Y, 3)
       << NV("RADistanceGuide", step.guideDistanceRA, 3)
       << NV("DECDistanceGuide", step.guideDistanceDec, 3);

    if (step.durationRA > 0)
    {
       ev << NV("RADuration", step.durationRA)
          << NV("RADirection", step.mount->DirectionStr((GUIDE_DIRECTION)step.directionRA));
    }

    if (step.durationDec > 0)
    {
        ev << NV("DECDuration", step.durationDec)
           << NV("DECDirection", step.mount->DirectionStr((GUIDE_DIRECTION)step.directionDec));
    }

    if (step.mount->IsStepGuider())
    {
        ev << NV("Pos", step.aoPos);
    }

    ev << NV("StarMass", step.starMass, 0)
       << NV("SNR", step.starSNR, 2)
       << NV("AvgDist", step.avgDist, 2);

    if (step.starError)
       ev << NV("ErrorCode", step.starError);

    if (step.raLimited)
        ev << NV("RALimited", true);

    if (step.decLimited)
        ev << NV("DecLimited", true);

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

void EventServer::NotifySettling(double distance, double time, double settleTime)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev(ev_settling(distance, time, settleTime));

    Debug.AddLine(wxString::Format("evsrv: %s", ev.str()));

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifySettleDone(const wxString& errorMsg)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev(ev_settle_done(errorMsg));

    Debug.AddLine(wxString::Format("evsrv: %s", ev.str()));

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyAlert(const wxString& msg, int type)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("Alert");
    ev << NV("Msg", msg);

    wxString s;
    switch (type)
    {
    case wxICON_NONE:
    case wxICON_INFORMATION:
    default:
        s = "info";
        break;
    case wxICON_QUESTION:
        s = "question";
        break;
    case wxICON_WARNING:
        s = "warning";
        break;
    case wxICON_ERROR:
        s = "error";
        break;
    }
    ev << NV("Type", s);

    do_notify(m_eventServerClients, ev);
}
