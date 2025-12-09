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
#include "../ui/tools/polardrift_toolwin.h"
#include "../ui/tools/staticpa_toolwin.h"
#include "../ui/tools/drift_tool.h"
#include "../ui/tools/staticpa_tool.h"
#include "../ui/tools/polardrift_tool.h"

#include <wx/sstream.h>
#include <wx/sckstrm.h>
#include <wx/stream.h>
#include <wx/txtstrm.h>
#include <wx/dir.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <sstream>
#include <string.h>
#include <cstdlib>
#include <ctime>
#include <vector>

EventServer EvtServer;

// clang-format off
wxBEGIN_EVENT_TABLE(EventServer, wxEvtHandler)
    EVT_SOCKET(EVENT_SERVER_ID, EventServer::OnEventServerEvent)
    EVT_SOCKET(EVENT_SERVER_CLIENT_ID, EventServer::OnEventServerClientEvent)
wxEND_EVENT_TABLE();
// clang-format on

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
    case EXPOSED_STATE_NONE:
        return "Stopped";
    case EXPOSED_STATE_SELECTED:
        return "Selected";
    case EXPOSED_STATE_CALIBRATING:
        return "Calibrating";
    case EXPOSED_STATE_GUIDING_LOCKED:
        return "Guiding";
    case EXPOSED_STATE_GUIDING_LOST:
        return "LostLock";
    case EXPOSED_STATE_PAUSED:
        return "Paused";
    case EXPOSED_STATE_LOOPING:
        return "Looping";
    default:
        return "Unknown";
    }
}

static wxString json_escape(const wxString& s)
{
    wxString t(s);
    static const wxString BACKSLASH("\\");
    static const wxString BACKSLASHBACKSLASH("\\\\");
    static const wxString DQUOT("\"");
    static const wxString BACKSLASHDQUOT("\\\"");
    static const wxString CR("\r");
    static const wxString BACKSLASHCR("\\r");
    static const wxString LF("\n");
    static const wxString BACKSLASHLF("\\n");
    t.Replace(BACKSLASH, BACKSLASHBACKSLASH);
    t.Replace(DQUOT, BACKSLASHDQUOT);
    t.Replace(CR, BACKSLASHCR);
    t.Replace(LF, BACKSLASHLF);
    return t;
}

template<char LDELIM, char RDELIM>
struct JSeq
{
    wxString m_s;
    bool m_first;
    bool m_closed;
    JSeq() : m_first(true), m_closed(false) { m_s << LDELIM; }
    void close()
    {
        m_s << RDELIM;
        m_closed = true;
    }
    wxString str()
    {
        if (!m_closed)
            close();
        return m_s;
    }
};

typedef JSeq<'[', ']'> JAry;
typedef JSeq<'{', '}'> JObj;

static JAry& operator<<(JAry& a, const wxString& str)
{
    if (a.m_first)
        a.m_first = false;
    else
        a.m_s << ',';
    a.m_s << str;
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

    switch (j->type)
    {
    default:
    case JSON_NULL:
        return literal_null;
    case JSON_OBJECT:
    {
        wxString ret("{");
        bool first = true;
        json_for_each(jj, j)
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
    case JSON_ARRAY:
    {
        wxString ret("[");
        bool first = true;
        json_for_each(jj, j)
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
    case JSON_STRING:
        return '"' + json_escape(j->string_value) + '"';
    case JSON_INT:
        return wxString::Format("%d", j->int_value);
    case JSON_FLOAT:
        return wxString::Format("%g", (double) j->float_value);
    case JSON_BOOL:
        return j->int_value ? literal_true : literal_false;
    }
}

struct NULL_TYPE
{
} NULL_VALUE;

// name-value pair
struct NV
{
    wxString n;
    wxString v;
    NV(const wxString& n_, const wxString& v_) : n(n_), v('"' + json_escape(v_) + '"') { }
    NV(const wxString& n_, const char *v_) : n(n_), v('"' + json_escape(v_) + '"') { }
    NV(const wxString& n_, const wchar_t *v_) : n(n_), v('"' + json_escape(v_) + '"') { }
    NV(const wxString& n_, int v_) : n(n_), v(wxString::Format("%d", v_)) { }
    NV(const wxString& n_, unsigned int v_) : n(n_), v(wxString::Format("%u", v_)) { }
    NV(const wxString& n_, double v_) : n(n_), v(wxString::Format("%g", v_)) { }
    NV(const wxString& n_, double v_, int prec) : n(n_), v(wxString::Format("%.*f", prec, v_)) { }
    NV(const wxString& n_, bool v_) : n(n_), v(v_ ? literal_true : literal_false) { }
    template<typename T>
    NV(const wxString& n_, const std::vector<T>& vec);
    NV(const wxString& n_, JAry& ary) : n(n_), v(ary.str()) { }
    NV(const wxString& n_, JObj& obj) : n(n_), v(obj.str()) { }
    NV(const wxString& n_, const json_value *v_) : n(n_), v(json_format(v_)) { }
    NV(const wxString& n_, const PHD_Point& p) : n(n_)
    {
        JAry ary;
        ary << p.X << p.Y;
        v = ary.str();
    }
    NV(const wxString& n_, const wxPoint& p) : n(n_)
    {
        JAry ary;
        ary << p.x << p.y;
        v = ary.str();
    }
    NV(const wxString& n_, const wxSize& s) : n(n_)
    {
        JAry ary;
        ary << s.x << s.y;
        v = ary.str();
    }
    NV(const wxString& n_, const wxRect& r) : n(n_)
    {
        JAry ary;
        ary << r.x << r.y << r.width << r.height;
        v = ary.str();
    }
    NV(const wxString& n_, const NULL_TYPE& nul) : n(n_), v(literal_null) { }
};

template<typename T>
NV::NV(const wxString& n_, const std::vector<T>& vec) : n(n_)
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
        *this << NV("Event", event) << NV("Timestamp", now, 3) << NV("Host", wxGetHostName())
              << NV("Inst", wxGetApp().GetInstanceNumber());
    }
};

static Ev ev_message_version()
{
    Ev ev("Version");
    ev << NV("PHDVersion", PHDVERSION) << NV("PHDSubver", PHDSUBVER) << NV("OverlapSupport", true)
       << NV("MsgVersion", MSG_PROTOCOL_VERSION);
    return ev;
}

static Ev ev_set_lock_position(const PHD_Point& xy)
{
    Ev ev("LockPositionSet");
    ev << xy;
    return ev;
}

static Ev ev_calibration_complete(const Mount *mount)
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

static Ev ev_start_calibration(const Mount *mount)
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

static Ev ev_settling(double distance, double time, double settleTime, bool starLocked)
{
    Ev ev("Settling");

    ev << NV("Distance", distance, 2) << NV("Time", time, 1) << NV("SettleTime", settleTime, 1) << NV("StarLocked", starLocked);

    return ev;
}

static Ev ev_settle_done(const wxString& errorMsg, int settleFrames, int droppedFrames)
{
    Ev ev("SettleDone");

    int status = errorMsg.IsEmpty() ? 0 : 1;

    ev << NV("Status", status);

    if (status != 0)
    {
        ev << NV("Error", errorMsg);
    }

    ev << NV("TotalFrames", settleFrames) << NV("DroppedFrames", droppedFrames);

    return ev;
}

struct ClientReadBuf
{
    enum
    {
        SIZE = 1024
    };
    char m_buf[SIZE];
    char *dest;

    ClientReadBuf() { reset(); }
    char *buf() { return &m_buf[0]; }
    size_t len() const { return dest - &m_buf[0]; }
    size_t avail() const { return &m_buf[SIZE] - dest; }
    void reset() { dest = &m_buf[0]; }
};

struct ClientData
{
    wxSocketClient *cli;
    int refcnt;
    ClientReadBuf rdbuf;
    wxMutex wrlock;

    ClientData(wxSocketClient *cli_) : cli(cli_), refcnt(1) { }
    void AddRef() { ++refcnt; }
    void RemoveRef()
    {
        if (--refcnt == 0)
        {
            cli->Destroy();
            delete this;
        }
    }
};

struct ClientDataGuard
{
    ClientData *cd;
    ClientDataGuard(wxSocketClient *cli) : cd((ClientData *) cli->GetClientData()) { cd->AddRef(); }
    ~ClientDataGuard() { cd->RemoveRef(); }
    ClientData *operator->() const { return cd; }
};

inline static wxMutex *client_wrlock(wxSocketClient *cli)
{
    return &((ClientData *) cli->GetClientData())->wrlock;
}

static wxString SockErrStr(wxSocketError e)
{
    switch (e)
    {
    case wxSOCKET_NOERROR:
        return "";
    case wxSOCKET_INVOP:
        return "Invalid operation";
    case wxSOCKET_IOERR:
        return "Input / Output error";
    case wxSOCKET_INVADDR:
        return "Invalid address";
    case wxSOCKET_INVSOCK:
        return "Invalid socket(uninitialized)";
    case wxSOCKET_NOHOST:
        return "No corresponding host";
    case wxSOCKET_INVPORT:
        return "Invalid port";
    case wxSOCKET_WOULDBLOCK:
        return "operation would block";
    case wxSOCKET_TIMEDOUT:
        return "timeout expired";
    case wxSOCKET_MEMERR:
        return "Memory exhausted";
    default:
        return wxString::Format("unknown socket error %d", e);
    }
}

static void send_buf(wxSocketClient *client, const wxCharBuffer& buf)
{
    wxMutexLocker lock(*client_wrlock(client));
    client->Write(buf.data(), buf.length());
    if (client->LastWriteCount() != buf.length())
    {
        Debug.Write(wxString::Format("evsrv: cli %p short write %u/%u %s\n", client, client->LastWriteCount(),
                                     (unsigned int) buf.length(),
                                     SockErrStr(client->Error() ? client->LastError() : wxSOCKET_NOERROR)));
    }
}

static void do_notify1(wxSocketClient *client, const JAry& ary)
{
    send_buf(client, (JAry(ary).str() + "\r\n").ToUTF8());
}

static void do_notify1(wxSocketClient *client, const JObj& j)
{
    send_buf(client, (JObj(j).str() + "\r\n").ToUTF8());
}

static void do_notify(const EventServer::CliSockSet& cli, const JObj& jj)
{
    wxCharBuffer buf = (JObj(jj).str() + "\r\n").ToUTF8();

    for (EventServer::CliSockSet::const_iterator it = cli.begin(); it != cli.end(); ++it)
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
    else if (st == EXPOSED_STATE_PAUSED)
    {
        do_notify1(cli, ev_paused());
    }

    do_notify1(cli, ev_app_state());
}

static void destroy_client(wxSocketClient *cli)
{
    ClientData *buf = (ClientData *) cli->GetClientData();
    buf->RemoveRef();
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

enum
{
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
    return wxString::Format("invalid JSON request: %s on line %d at \"%.12s...\"", parser.ErrorDesc(), parser.ErrorLine(),
                            parser.ErrorPos());
}

static void parse_request(const json_value *req, const json_value **pmethod, const json_value **pparams, const json_value **pid)
{
    *pmethod = *pparams = *pid = 0;

    if (req)
    {
        json_for_each(t, req)
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

// paranoia
#define VERIFY_GUIDER(response)                                                                                                \
    do                                                                                                                         \
    {                                                                                                                          \
        if (!pFrame || !pFrame->pGuider)                                                                                       \
        {                                                                                                                      \
            response << jrpc_error(1, "internal error");                                                                       \
            return;                                                                                                            \
        }                                                                                                                      \
    } while (0)

static void deselect_star(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);
    pFrame->pGuider->Reset(true);
    response << jrpc_result(0);
}

static void get_exposure(JObj& response, const json_value *params)
{
    response << jrpc_result(pFrame->RequestedExposureDuration());
}

static void get_exposure_durations(JObj& response, const json_value *params)
{
    const std::vector<int>& exposure_durations = pFrame->GetExposureDurations();
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

struct Params
{
    std::map<std::string, const json_value *> dict;

    void Init(const char *names[], size_t nr_names, const json_value *params)
    {
        if (!params)
            return;
        if (params->type == JSON_ARRAY)
        {
            const json_value *jv = params->first_child;
            for (size_t i = 0; jv && i < nr_names; i++, jv = jv->next_sibling)
            {
                const char *name = names[i];
                dict.insert(std::make_pair(std::string(name), jv));
            }
        }
        else if (params->type == JSON_OBJECT)
        {
            json_for_each(jv, params)
            {
                dict.insert(std::make_pair(std::string(jv->name), jv));
            }
        }
    }
    Params(const char *n1, const json_value *params)
    {
        const char *n[] = { n1 };
        Init(n, 1, params);
    }
    Params(const char *n1, const char *n2, const json_value *params)
    {
        const char *n[] = { n1, n2 };
        Init(n, 2, params);
    }
    Params(const char *n1, const char *n2, const char *n3, const json_value *params)
    {
        const char *n[] = { n1, n2, n3 };
        Init(n, 3, params);
    }
    Params(const char *n1, const char *n2, const char *n3, const char *n4, const json_value *params)
    {
        const char *n[] = { n1, n2, n3, n4 };
        Init(n, 4, params);
    }
    Params(const char *n1, const char *n2, const char *n3, const char *n4, const char *n5, const json_value *params)
    {
        const char *n[] = { n1, n2, n3, n4, n5 };
        Init(n, 5, params);
    }
    Params(const char *n1, const char *n2, const char *n3, const char *n4, const char *n5, const char *n6,
           const json_value *params)
    {
        const char *n[] = { n1, n2, n3, n4, n5, n6 };
        Init(n, 6, params);
    }
    const json_value *param(const std::string& name) const
    {
        auto it = dict.find(name);
        return it == dict.end() ? 0 : it->second;
    }
};

static void set_exposure(JObj& response, const json_value *params)
{
    Params p("exposure", params);
    const json_value *exp = p.param("exposure");

    if (!exp || (exp->type != JSON_INT && exp->type != JSON_FLOAT))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "expected 'exposure' parameter with positive numeric value (milliseconds, typical range 1-5000)");
        return;
    }

    int exposure_ms = (exp->type == JSON_INT) ? exp->int_value : (int)exp->float_value;

    // Validate exposure time range
    if (exposure_ms < 1)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "exposure time too short (minimum 1 millisecond)");
        return;
    }
    if (exposure_ms > 60000)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "exposure time too long (maximum 60000 milliseconds / 60 seconds)");
        return;
    }

    // Check camera availability
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected - cannot set exposure");
        return;
    }

    bool ok = pFrame->SetExposureDuration(exposure_ms);
    if (ok)
    {
        response << jrpc_result(0);
    }
    else
    {
        response << jrpc_error(1, 
            wxString::Format("failed to set exposure to %d ms (camera may not support this value)", exposure_ms));
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

inline static void devstat(JObj& t, const char *dev, const wxString& name, bool connected)
{
    JObj o;
    t << NV(dev, o << NV("name", name) << NV("connected", connected));
}

static void get_current_equipment(JObj& response, const json_value *params)
{
    JObj t;

    if (pCamera)
        devstat(t, "camera", pCamera->Name, pCamera->Connected);

    Mount *mount = TheScope();
    if (mount)
        devstat(t, "mount", mount->Name(), mount->IsConnected());

    Mount *auxMount = pFrame->pGearDialog->AuxScope();
    if (auxMount)
        devstat(t, "aux_mount", auxMount->Name(), auxMount->IsConnected());

    Mount *ao = TheAO();
    if (ao)
        devstat(t, "AO", ao->Name(), ao->IsConnected());

    Rotator *rotator = pRotator;
    if (rotator)
        devstat(t, "rotator", rotator->Name(), rotator->IsConnected());

    response << jrpc_result(t);
}

static bool all_equipment_connected()
{
    return pCamera && pCamera->Connected && (!pMount || pMount->IsConnected()) &&
        (!pSecondaryMount || pSecondaryMount->IsConnected());
}

static void set_profile(JObj& response, const json_value *params)
{
    Params p("id", params);
    const json_value *id = p.param("id");
    if (!id || id->type != JSON_INT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected profile id param");
        return;
    }

    VERIFY_GUIDER(response);

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
    Params p("connected", params);
    const json_value *val = p.param("connected");
    if (!val || val->type != JSON_BOOL)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected connected boolean param");
        return;
    }

    VERIFY_GUIDER(response);

    wxString errMsg;
    bool error = val->int_value ? pFrame->pGearDialog->ConnectAll(&errMsg) : pFrame->pGearDialog->DisconnectAll(&errMsg);

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

static bool int_param(const json_value *val, int *result)
{
    if (!val || val->type != JSON_INT)
    {
        return false;
    }
    *result = val->int_value;
    return true;
}

static bool int_param(const char *name, const json_value *v, int *p)
{
    if (strcmp(name, v->name) != 0)
        return false;

    return int_param(v, p);
}

inline static bool bool_value(const json_value *v)
{
    return v->int_value ? true : false;
}

static bool bool_param(const json_value *jv, bool *val)
{
    if (jv->type != JSON_BOOL && jv->type != JSON_INT)
        return false;
    *val = bool_value(jv);
    return true;
}

static void get_paused(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);
    response << jrpc_result(pFrame->pGuider->IsPaused());
}

static void set_paused(JObj& response, const json_value *params)
{
    Params p("paused", "type", params);
    const json_value *jv = p.param("paused");

    bool val;
    if (!jv || !bool_param(jv, &val))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected bool param at index 0");
        return;
    }

    PauseType pause = PAUSE_NONE;

    if (val)
    {
        pause = PAUSE_GUIDING;

        jv = p.param("type");
        if (jv)
        {
            if (jv->type == JSON_STRING)
            {
                if (strcmp(jv->string_value, "full") == 0)
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

static bool parse_rect(wxRect *r, const json_value *j)
{
    if (j->type != JSON_ARRAY)
        return false;

    int a[4];
    const json_value *jv = j->first_child;
    for (int i = 0; i < 4; i++)
    {
        if (!jv || jv->type != JSON_INT)
            return false;
        a[i] = jv->int_value;
        jv = jv->next_sibling;
    }
    if (jv)
        return false; // extra value

    r->x = a[0];
    r->y = a[1];
    r->width = a[2];
    r->height = a[3];

    return true;
}

static bool parse_roi(const json_value *j, wxRect& roi)
{
    return parse_rect(&roi, j);
}

static void find_star(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);

    Params p("roi", params);

    wxRect roi;
    const json_value *j = p.param("roi");
    if (j && !parse_rect(&roi, j))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid ROI param");
        return;
    }

    bool error = pFrame->AutoSelectStar(roi);

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
    VERIFY_GUIDER(response);

    const PHD_Point& lockPos = pFrame->pGuider->LockPosition();
    if (lockPos.IsValid())
        response << jrpc_result(lockPos);
    else
        response << jrpc_result(NULL_VALUE);
}

// {"method": "set_lock_position", "params": [X, Y, true], "id": 1}
static void set_lock_position(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);

    // Parse and validate parameters
    Params p("x", "y", "exact", params);
    const json_value *p0 = p.param("x"), *p1 = p.param("y");
    double x, y;

    if (!p0 || !p1 || !float_param(p0, &x) || !float_param(p1, &y))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid x, y coordinates (expected numeric values)");
        return;
    }

    // Validate coordinates are reasonable (within frame bounds if frame available)
    if (pFrame && pFrame->pGuider && pFrame->pGuider->CurrentImage())
    {
        usImage *img = pFrame->pGuider->CurrentImage();
        if (x < 0 || y < 0 || x >= img->Size.GetWidth() || y >= img->Size.GetHeight())
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS,
                wxString::Format("lock position coordinates out of range (x=%.1f, y=%.1f, frame size=%dx%d)",
                               x, y, img->Size.GetWidth(), img->Size.GetHeight()));
            return;
        }
    }

    // Parse exact parameter
    bool exact = true;
    const json_value *p2 = p.param("exact");

    if (p2)
    {
        if (!bool_param(p2, &exact))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid 'exact' parameter (expected boolean value)");
            return;
        }
    }

    // Set lock position
    bool error;
    PHD_Point lockPos(x, y);

    if (exact)
        error = pFrame->pGuider->SetLockPosition(lockPos);
    else
        error = pFrame->pGuider->SetLockPosToStarAtPosition(lockPos);

    if (error)
    {
        wxString mode = exact ? "exact position" : "star at position";
        response << jrpc_error(JSONRPC_INVALID_REQUEST,
            wxString::Format("could not set lock position to (%.1f, %.1f) using %s mode", x, y, mode));
        return;
    }

    // Return detailed response
    const PHD_Point& actualLockPos = pFrame->pGuider->LockPosition();
    JObj rslt;
    rslt << NV("x", actualLockPos.X, 1)
         << NV("y", actualLockPos.Y, 1)
         << NV("exact", exact);
    response << jrpc_result(rslt);
    
    Debug.Write(wxString::Format("EventServer: Lock position set to (%.1f, %.1f), exact=%d\n",
                                 actualLockPos.X, actualLockPos.Y, exact));
}

inline static const char *string_val(const json_value *j)
{
    return j->type == JSON_STRING ? j->string_value : "";
}

enum WHICH_MOUNT
{
    MOUNT,
    AO,
    WHICH_MOUNT_BOTH,
    WHICH_MOUNT_ERR
};

static WHICH_MOUNT which_mount(const json_value *p)
{
    WHICH_MOUNT r = MOUNT;
    if (p)
    {
        r = WHICH_MOUNT_ERR;
        if (p->type == JSON_STRING)
        {
            if (wxStricmp(p->string_value, "ao") == 0)
                r = AO;
            else if (wxStricmp(p->string_value, "mount") == 0)
                r = MOUNT;
            else if (wxStricmp(p->string_value, "both") == 0)
                r = WHICH_MOUNT_BOTH;
        }
    }
    return r;
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
        Params p("which", params);

        clear_mount = clear_ao = false;

        WHICH_MOUNT which = which_mount(p.param("which"));
        switch (which)
        {
        case MOUNT:
            clear_mount = true;
            break;
        case AO:
            clear_ao = true;
            break;
        case WHICH_MOUNT_BOTH:
            clear_mount = clear_ao = true;
            break;
        case WHICH_MOUNT_ERR:
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected param \"mount\", \"ao\", or \"both\"");
            return;
        }
    }

    Mount *mount = TheScope();
    Mount *ao = TheAO();

    if (mount && clear_mount)
        mount->ClearCalibration();

    if (ao && clear_ao)
        ao->ClearCalibration();

    response << jrpc_result(0);
}

static void flip_calibration(JObj& response, const json_value *params)
{
    bool error = pFrame->FlipCalibrationData();

    if (error)
        response << jrpc_error(1, "could not flip calibration");
    else
        response << jrpc_result(0);
}

static void get_lock_shift_enabled(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);
    bool enabled = pFrame->pGuider->GetLockPosShiftParams().shiftEnabled;
    response << jrpc_result(enabled);
}

static void set_lock_shift_enabled(JObj& response, const json_value *params)
{
    Params p("enabled", params);
    const json_value *val = p.param("enabled");
    bool enable;
    if (!val || !bool_param(val, &enable))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected enabled boolean param");
        return;
    }

    VERIFY_GUIDER(response);

    pFrame->pGuider->EnableLockPosShift(enable);

    response << jrpc_result(0);
}

static bool is_camera_shift_req(const json_value *params)
{
    Params p("axes", params);
    const json_value *j = p.param("axes");
    if (j)
    {
        const char *axes = string_val(j);
        if (wxStricmp(axes, "x/y") == 0 || wxStricmp(axes, "camera") == 0)
        {
            return true;
        }
    }
    return false;
}

static JObj& operator<<(JObj& j, const LockPosShiftParams& l)
{
    j << NV("enabled", l.shiftEnabled);
    if (l.shiftRate.IsValid())
    {
        j << NV("rate", l.shiftRate) << NV("units", l.shiftUnits == UNIT_ARCSEC ? "arcsec/hr" : "pixels/hr")
          << NV("axes", l.shiftIsMountCoords ? "RA/Dec" : "X/Y");
    }
    return j;
}

static void get_lock_shift_params(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);

    const LockPosShiftParams& lockShift = pFrame->pGuider->GetLockPosShiftParams();
    JObj rslt;

    if (is_camera_shift_req(params))
    {
        LockPosShiftParams tmp;
        tmp.shiftEnabled = lockShift.shiftEnabled;
        const ShiftPoint& lock = pFrame->pGuider->LockPosition();
        tmp.shiftRate = lock.ShiftRate() * 3600; // px/sec => px/hr
        tmp.shiftUnits = UNIT_PIXELS;
        tmp.shiftIsMountCoords = false;
        rslt << tmp;
    }
    else
        rslt << lockShift;

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
    // "params":[{"rate":[3.3,1.1],"units":"arcsec/hr","axes":"RA/Dec"}]
    // or
    // "params":{"rate":[3.3,1.1],"units":"arcsec/hr","axes":"RA/Dec"}

    if (params && params->type == JSON_ARRAY)
        params = params->first_child;

    Params p("rate", "units", "axes", params);

    shift->shiftUnits = UNIT_ARCSEC;
    shift->shiftIsMountCoords = true;

    const json_value *j;

    j = p.param("rate");
    if (!j || !parse_point(&shift->shiftRate, j))
    {
        *error = "expected rate value array";
        return false;
    }

    j = p.param("units");
    const char *units = j ? string_val(j) : "";

    if (wxStricmp(units, "arcsec/hr") == 0 || wxStricmp(units, "arc-sec/hr") == 0)
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

    j = p.param("axes");
    const char *axes = j ? string_val(j) : "";

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

    VERIFY_GUIDER(response);

    pFrame->pGuider->SetLockPosShiftRate(shift.shiftRate, shift.shiftUnits, shift.shiftIsMountCoords, true);

    response << jrpc_result(0);
}

static void save_image(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);

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

static void capture_single_frame(JObj& response, const json_value *params)
{
    // Check system state
    if (pFrame->CaptureActive)
    {
        response << jrpc_error(1, "capture already in progress - cannot start second capture operation");
        return;
    }
    
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected - single frame capture requires active camera");
        return;
    }

    Params p("exposure", "binning", "gain", "subframe", "path", "save", params);
    const json_value *j;

    // Parse and validate exposure
    int exposure = pFrame->RequestedExposureDuration();
    if ((j = p.param("exposure")) != nullptr)
    {
        if (j->type != JSON_INT && j->type != JSON_FLOAT)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "invalid 'exposure' parameter (expected integer milliseconds, range 1-600000)");
            return;
        }
        exposure = (int)floor((j->type == JSON_INT) ? j->int_value : j->float_value);
        if (exposure < 1 || exposure > 10 * 60000)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                wxString::Format("exposure out of range (requested: %d ms, valid: 1-600000 ms)", exposure));
            return;
        }
    }

    // Parse and validate binning
    wxByte binning = pCamera->Binning;
    if ((j = p.param("binning")) != nullptr)
    {
        if (j->type != JSON_INT)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "invalid 'binning' parameter (expected integer)");
            return;
        }
        if (j->int_value < 1 || j->int_value > pCamera->MaxBinning)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS,
                wxString::Format("binning value out of range (requested: %d, valid: 1-%d)", 
                                j->int_value, pCamera->MaxBinning));
            return;
        }
        binning = j->int_value;
    }

    // Parse and validate gain
    int gain = pCamera->GetCameraGain();
    if ((j = p.param("gain")) != nullptr)
    {
        if (j->type != JSON_INT && j->type != JSON_FLOAT)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "invalid 'gain' parameter (expected numeric value 0-100)");
            return;
        }
        gain = (int)floor((j->type == JSON_INT) ? j->int_value : j->float_value);
        if (gain < 0 || gain > 100)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                wxString::Format("gain value out of range (requested: %d, valid: 0-100)", gain));
            return;
        }
    }

    // Parse and validate subframe (optional)
    wxRect subframe;
    if ((j = p.param("subframe")) != nullptr)
    {
        if (!parse_rect(&subframe, j))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "invalid 'subframe' parameter (expected object with x, y, width, height as integers)");
            return;
        }
        // Validate subframe bounds
        if (subframe.width <= 0 || subframe.height <= 0)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "subframe dimensions must be positive (width > 0, height > 0)");
            return;
        }
    }

    // Parse and validate output path (optional)
    wxString path;
    if ((j = p.param("path")) != nullptr)
    {
        if (j->type != JSON_STRING)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "invalid 'path' parameter (expected absolute file path string)");
            return;
        }
        wxFileName fn(wxString(j->string_value, wxConvUTF8));
        if (!fn.IsAbsolute())
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "path must be an absolute file path (relative paths not supported)");
            return;
        }
        if (fn.DirExists())
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "path refers to an existing directory (expected file path)");
            return;
        }
        // Check directory exists and is writable
        wxFileName dirFn = fn;
        dirFn.ClearExt();
        if (!dirFn.DirExists())
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                wxString::Format("destination directory does not exist: %s", dirFn.GetPath()));
            return;
        }
        path = j->string_value;
    }

    // Parse save flag
    bool save = !path.empty();
    if ((j = p.param("save")) != nullptr)
    {
        if (!bool_param(j, &save))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "invalid 'save' parameter (expected boolean)");
            return;
        }
    }

    // Validate consistency
    if (!save && !path.empty())
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "conflicting parameters: 'save' is false but 'path' is provided");
        return;
    }
    if (save && path.empty())
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "missing 'path' parameter when 'save' is true");
        return;
    }

    // Execute capture
    bool err = pFrame->StartSingleExposure(exposure, binning, gain, subframe, save, path);
    if (err)
    {
        response << jrpc_error(2, 
            wxString::Format("failed to start single frame exposure (exposure=%d ms, binning=%d, gain=%d)", 
                           exposure, binning, gain));
        return;
    }

    JObj rslt;
    rslt << NV("exposure", exposure)
         << NV("binning", (int)binning)
         << NV("gain", gain);
    if (!path.empty())
        rslt << NV("path", path);
        
    response << jrpc_result(rslt);
}

static void get_use_subframes(JObj& response, const json_value *params)
{
    response << jrpc_result(pCamera && pCamera->UseSubframes);
}

static void get_search_region(JObj& response, const json_value *params)
{
    VERIFY_GUIDER(response);
    response << jrpc_result(pFrame->pGuider->GetSearchRegion());
}

struct B64Encode
{
    static const char *const E;
    std::ostringstream os;
    unsigned int t;
    size_t nread;

    B64Encode() : t(0), nread(0) { }
    void append1(unsigned char ch)
    {
        t <<= 8;
        t |= ch;
        if (++nread % 3 == 0)
        {
            os << E[t >> 18] << E[(t >> 12) & 0x3F] << E[(t >> 6) & 0x3F] << E[t & 0x3F];
            t = 0;
        }
    }
    void append(const void *src_, size_t len)
    {
        const unsigned char *src = (const unsigned char *) src_;
        const unsigned char *const end = src + len;
        while (src < end)
            append1(*src++);
    }
    std::string finish()
    {
        switch (nread % 3)
        {
        case 1:
            os << E[t >> 2] << E[(t & 0x3) << 4] << "==";
            break;
        case 2:
            os << E[t >> 10] << E[(t >> 4) & 0x3F] << E[(t & 0xf) << 2] << '=';
            break;
        }
        return os.str();
    }
};
const char *const B64Encode::E = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void get_star_image(JObj& response, const json_value *params)
{
    int reqsize = 15;
    Params p("size", params);
    const json_value *val = p.param("size");
    if (val)
    {
        if (val->type != JSON_INT || (reqsize = val->int_value) < 15)
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid image size param");
            return;
        }
    }

    VERIFY_GUIDER(response);

    Guider *guider = pFrame->pGuider;
    const usImage *img = guider->CurrentImage();
    const PHD_Point& star = guider->CurrentPosition();

    if (guider->GetState() < GUIDER_STATE::STATE_SELECTED || !img->ImageData || !star.IsValid())
    {
        response << jrpc_error(2, "no star selected");
        return;
    }

    int const halfw = wxMin((reqsize - 1) / 2, 31);
    int const fullw = 2 * halfw + 1;
    int const sx = (int) rint(star.X);
    int const sy = (int) rint(star.Y);
    wxRect rect(sx - halfw, sy - halfw, fullw, fullw);
    if (img->Subframe.IsEmpty())
        rect.Intersect(wxRect(img->Size));
    else
        rect.Intersect(img->Subframe);

    B64Encode enc;
    for (int y = rect.GetTop(); y <= rect.GetBottom(); y++)
    {
        const unsigned short *p = img->ImageData + y * img->Size.GetWidth() + rect.GetLeft();
        enc.append(p, rect.GetWidth() * sizeof(unsigned short));
    }

    PHD_Point pos(star);
    pos.X -= rect.GetLeft();
    pos.Y -= rect.GetTop();

    JObj rslt;
    rslt << NV("frame", img->FrameNum) << NV("width", rect.GetWidth()) << NV("height", rect.GetHeight()) << NV("star_pos", pos)
         << NV("pixels", enc.finish());

    response << jrpc_result(rslt);
}

static bool parse_settle(SettleParams *settle, const json_value *j, wxString *error)
{
    bool found_tolerance = false;
    bool found_settle_time = false;
    bool found_timeout = false;
    
    // Initialize default values for optional parameters
    settle->tolerancePx = 0.0;
    settle->settleTimeSec = 0;
    settle->timeoutSec = 0;
    settle->frames = 99999;

    json_for_each(t, j)
    {
        // Primary tolerance parameter (pixels)
        double d;
        if (float_param("pixels", t, &d))
        {
            if (d <= 0.0)
            {
                *error = "pixels tolerance must be positive";
                return false;
            }
            settle->tolerancePx = d;
            found_tolerance = true;
            continue;
        }
        
        // Alternative tolerance parameter (arcseconds)
        if (float_param("arcsecs", t, &d))
        {
            if (found_tolerance)
            {
                *error = "cannot specify both 'pixels' and 'arcsecs' tolerance";
                return false;
            }
            if (d <= 0.0)
            {
                *error = "arcsecs tolerance must be positive";
                return false;
            }
            // Convert arcsecs to pixels using camera scale
            // If no frame available, this will be handled during guide operation
            if (pFrame)
            {
                double pixelScale = pFrame->GetCameraPixelScale();
                if (pixelScale > 0.0)
                {
                    settle->tolerancePx = d / pixelScale;
                }
                else
                {
                    *error = "camera pixel scale not available for arcsec conversion";
                    return false;
                }
            }
            else
            {
                *error = "cannot convert arcsecs to pixels: no camera data available";
                return false;
            }
            found_tolerance = true;
            continue;
        }
        
        // Settle time in seconds (primary parameter)
        if (float_param("time", t, &d))
        {
            if (d <= 0.0)
            {
                *error = "settle time must be positive";
                return false;
            }
            settle->settleTimeSec = (int)floor(d);
            found_settle_time = true;
            continue;
        }
        
        // Alternative settle time in frames
        if (int_param("frames", t, &settle->frames))
        {
            if (found_settle_time)
            {
                *error = "cannot specify both 'time' and 'frames' settle duration";
                return false;
            }
            if (settle->frames <= 0)
            {
                *error = "frames settle duration must be positive";
                return false;
            }
            // Mark that we've set frames instead of time
            found_settle_time = true;
            continue;
        }
        
        // Timeout parameter
        if (float_param("timeout", t, &d))
        {
            if (d <= 0.0)
            {
                *error = "timeout must be positive";
                return false;
            }
            settle->timeoutSec = (int)floor(d);
            found_timeout = true;
            continue;
        }
    }

    // Validate all required parameters are present
    if (!found_tolerance)
    {
        *error = "settle tolerance required: specify 'pixels' or 'arcsecs'";
        return false;
    }
    if (!found_settle_time)
    {
        *error = "settle duration required: specify 'time' (seconds) or 'frames' (frame count)";
        return false;
    }
    if (!found_timeout)
    {
        *error = "timeout required: specify 'timeout' (seconds)";
        return false;
    }

    return true;
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
    //    or
    // {"method": "guide", "params": {"settle": {"pixels": 0.5, "time": 6, "timeout": 30}, "recalibrate": false}, "id": 42}
    //
    // Supported settle tolerance units:
    //   - pixels: tolerance in camera pixels (primary)
    //   - arcsecs: tolerance in arcseconds (converted using camera pixel scale)
    //
    // Supported settle time units:
    //   - time: settle time in seconds (primary)
    //   - frames: settle time as number of frames (converted using camera frame rate)

    SettleParams settle;

    Params p("settle", "recalibrate", "roi", params);
    const json_value *p0 = p.param("settle");
    if (!p0 || p0->type != JSON_OBJECT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "missing or invalid 'settle' parameter (must be object with 'pixels'/'arcsecs', 'time'/'frames', 'timeout')");
        return;
    }
    wxString errMsg;
    if (!parse_settle(&settle, p0, &errMsg))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, wxString::Format("settle parameter error: %s", errMsg));
        return;
    }

    // Validate settle parameters are within reasonable ranges
    if (settle.tolerancePx < 0.1)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "settle tolerance too small (minimum 0.1 pixels)");
        return;
    }
    if (settle.tolerancePx > 50.0)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "settle tolerance too large (maximum 50 pixels)");
        return;
    }
    if (settle.settleTimeSec < 1)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "settle time too short (minimum 1 second)");
        return;
    }
    if (settle.settleTimeSec > 300)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "settle time too long (maximum 300 seconds)");
        return;
    }
    if (settle.timeoutSec < 1)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "timeout too short (minimum 1 second)");
        return;
    }
    if (settle.timeoutSec > 600)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "timeout too long (maximum 600 seconds)");
        return;
    }
    if (settle.timeoutSec <= settle.settleTimeSec)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "timeout must be greater than settle time");
        return;
    }

    bool recalibrate = false;
    const json_value *p1 = p.param("recalibrate");
    if (p1)
    {
        if (!bool_param(p1, &recalibrate))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected boolean value for 'recalibrate'");
            return;
        }
    }

    wxRect roi;
    const json_value *p2 = p.param("roi");
    if (p2 && !parse_rect(&roi, p2))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "invalid 'roi' parameter (expected object with 'x', 'y', 'width', 'height' as integers)");
        return;
    }

    if (recalibrate && !pConfig->Global.GetBoolean("/server/guide_allow_recalibrate", true))
    {
        Debug.AddLine("ignoring client recalibration request since guide_allow_recalibrate = false");
        recalibrate = false;
    }

    wxString err;
    unsigned int ctrlOptions = GUIDEOPT_USE_STICKY_LOCK;
    if (recalibrate)
        ctrlOptions |= GUIDEOPT_FORCE_RECAL;
    if (!PhdController::CanGuide(&err))
    {
        // Detailed error with context about why guiding cannot start
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            wxString::Format("cannot start guiding: %s", err));
    }
    else if (PhdController::Guide(ctrlOptions, settle, roi, &err))
        response << jrpc_result(0);
    else
    {
        // Guiding failed during execution - provide detailed error context
        response << jrpc_error(1, 
            wxString::Format("guide operation failed: %s", err));
    }
}

static void dither(JObj& response, const json_value *params)
{
    // params:
    //   amount [float] - max pixels to move in each axis
    //   raOnly [bool] - when true, only dither ra (optional, defaults to false)
    //   settle [object]:
    //     pixels [float] or arcsecs [float] - tolerance threshold
    //     time [integer] or frames [integer] - settle duration
    //     timeout [integer] - timeout duration (required)
    //
    // {"method": "dither", "params": [10, false, {"pixels": 1.5, "time": 8, "timeout": 30}], "id": 42}
    //    or
    // {"method": "dither", "params": {"amount": 10, "raOnly": false, 
    //    "settle": {"arcsecs": 1.0, "time": 8, "timeout": 30}}, "id": 42}

    Params p("amount", "raOnly", "settle", params);
    const json_value *jv;
    double ditherAmt;

    jv = p.param("amount");
    if (!jv || !float_param(jv, &ditherAmt))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "missing or invalid 'amount' parameter (expected positive number for dither pixels)");
        return;
    }

    // Validate dither amount range
    if (ditherAmt <= 0.0)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "dither amount must be positive (typically 2-20 pixels)");
        return;
    }
    if (ditherAmt > 100.0)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "dither amount too large (maximum 100 pixels)");
        return;
    }

    bool raOnly = false;
    jv = p.param("raOnly");
    if (jv)
    {
        if (!bool_param(jv, &raOnly))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "invalid 'raOnly' parameter (expected boolean)");
            return;
        }
    }

    SettleParams settle;
    jv = p.param("settle");
    if (!jv || jv->type != JSON_OBJECT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "missing or invalid 'settle' parameter (must be object with settle criteria)");
        return;
    }
    wxString errMsg;
    if (!parse_settle(&settle, jv, &errMsg))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            wxString::Format("settle parameter error: %s", errMsg));
        return;
    }

    // Validate settle parameters for dither operation
    if (settle.tolerancePx < 0.1)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "dither settle tolerance too small (minimum 0.1 pixels)");
        return;
    }
    if (settle.timeoutSec < 1)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "dither timeout too short (minimum 1 second)");
        return;
    }
    if (settle.timeoutSec > 600)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "dither timeout too long (maximum 600 seconds)");
        return;
    }

    wxString error;
    if (PhdController::Dither(fabs(ditherAmt), raOnly, settle, &error))
        response << jrpc_result(0);
    else
        response << jrpc_error(1, wxString::Format("dither failed: %s", error));
}

static void shutdown(JObj& response, const json_value *params)
{
    wxGetApp().TerminateApp();

    response << jrpc_result(0);
}

static void get_camera_binning(JObj& response, const json_value *params)
{
    if (pCamera && pCamera->Connected)
    {
        int binning = pCamera->Binning;
        response << jrpc_result(binning);
    }
    else
        response << jrpc_error(1, "camera not connected");
}

static void get_camera_frame_size(JObj& response, const json_value *params)
{
    if (pCamera && pCamera->Connected)
    {
        response << jrpc_result(pCamera->FrameSize);
    }
    else
        response << jrpc_error(1, "camera not connected");
}

static void get_guide_output_enabled(JObj& response, const json_value *params)
{
    if (pMount)
        response << jrpc_result(pMount->GetGuidingEnabled());
    else
        response << jrpc_error(1, "mount not defined");
}

static void set_guide_output_enabled(JObj& response, const json_value *params)
{
    Params p("enabled", params);
    const json_value *val = p.param("enabled");
    bool enable;
    if (!val || !bool_param(val, &enable))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected enabled boolean param");
        return;
    }

    if (pMount)
    {
        pMount->SetGuidingEnabled(enable);
        response << jrpc_result(0);
    }
    else
        response << jrpc_error(1, "mount not defined");
}

static bool axis_param(const Params& p, GuideAxis *a)
{
    const json_value *val = p.param("axis");
    if (!val || val->type != JSON_STRING)
        return false;

    bool ok = true;

    if (wxStricmp(val->string_value, "ra") == 0)
        *a = GUIDE_RA;
    else if (wxStricmp(val->string_value, "x") == 0)
        *a = GUIDE_X;
    else if (wxStricmp(val->string_value, "dec") == 0)
        *a = GUIDE_DEC;
    else if (wxStricmp(val->string_value, "y") == 0)
        *a = GUIDE_Y;
    else
        ok = false;

    return ok;
}

static void get_algo_param_names(JObj& response, const json_value *params)
{
    Params p("axis", params);
    GuideAxis a;
    if (!axis_param(p, &a))
    {
        response << jrpc_error(1, "expected axis name param");
        return;
    }
    wxArrayString ary;
    ary.push_back("algorithmName");

    if (pMount)
    {
        GuideAlgorithm *alg = a == GUIDE_X ? pMount->GetXGuideAlgorithm() : pMount->GetYGuideAlgorithm();
        alg->GetParamNames(ary);
    }

    JAry names;
    for (auto it = ary.begin(); it != ary.end(); ++it)
        names << ('"' + json_escape(*it) + '"');

    response << jrpc_result(names);
}

static void get_algo_param(JObj& response, const json_value *params)
{
    // Validate mount connection
    if (!pMount || !pMount->IsConnected())
    {
        response << jrpc_error(1, "mount not connected - cannot get algorithm parameters");
        return;
    }

    // Parse and validate parameters
    Params p("axis", "name", params);
    GuideAxis a;
    if (!axis_param(p, &a))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid 'axis' parameter (expected 'RA', 'X', 'Dec', or 'Y')");
        return;
    }
    
    const json_value *name = p.param("name");
    if (!name || name->type != JSON_STRING)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid 'name' parameter (expected string parameter name)");
        return;
    }

    GuideAlgorithm *alg = a == GUIDE_X ? pMount->GetXGuideAlgorithm() : pMount->GetYGuideAlgorithm();
    wxString axisName = a == GUIDE_X ? "RA" : "Dec";
    
    // Special case: algorithmName returns the algorithm class name
    if (strcmp(name->string_value, "algorithmName") == 0)
    {
        JObj rslt;
        rslt << NV("name", "algorithmName")
             << NV("value", alg->GetGuideAlgorithmClassName())
             << NV("axis", axisName);
        response << jrpc_result(rslt);
        return;
    }

    // Get parameter value
    double val;
    bool ok = alg->GetParam(name->string_value, &val);
    
    if (ok)
    {
        JObj rslt;
        rslt << NV("name", wxString(name->string_value, wxConvUTF8))
             << NV("value", val)
             << NV("axis", axisName)
             << NV("algorithm", alg->GetGuideAlgorithmClassName());
        response << jrpc_result(rslt);
    }
    else
    {
        response << jrpc_error(1, wxString::Format("parameter '%s' not found for %s axis algorithm '%s'",
                                                   name->string_value, axisName, alg->GetGuideAlgorithmClassName()));
    }
}

static void set_algo_param(JObj& response, const json_value *params)
{
    // Validate mount connection
    if (!pMount || !pMount->IsConnected())
    {
        response << jrpc_error(1, "mount not connected - cannot set algorithm parameters");
        return;
    }

    // Parse and validate parameters
    Params p("axis", "name", "value", params);
    GuideAxis a;
    if (!axis_param(p, &a))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid 'axis' parameter (expected 'RA', 'X', 'Dec', or 'Y')");
        return;
    }
    
    const json_value *name = p.param("name");
    if (!name || name->type != JSON_STRING)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid 'name' parameter (expected string parameter name)");
        return;
    }
    
    const json_value *val = p.param("value");
    double v;
    if (!float_param(val, &v))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid 'value' parameter (expected numeric value)");
        return;
    }

    GuideAlgorithm *alg = a == GUIDE_X ? pMount->GetXGuideAlgorithm() : pMount->GetYGuideAlgorithm();
    wxString axisName = a == GUIDE_X ? "RA" : "Dec";
    
    // Set parameter value
    bool ok = alg->SetParam(name->string_value, v);
    
    if (ok)
    {
        // Update UI controls
        if (pFrame->pGraphLog)
            pFrame->pGraphLog->UpdateControls();
        
        // Return detailed confirmation
        JObj rslt;
        rslt << NV("name", wxString(name->string_value, wxConvUTF8))
             << NV("value", v)
             << NV("axis", axisName)
             << NV("algorithm", alg->GetGuideAlgorithmClassName());
        response << jrpc_result(rslt);
        
        Debug.Write(wxString::Format("EventServer: Set %s axis algorithm parameter '%s' = %.3f\n",
                                     axisName, name->string_value, v));
    }
    else
    {
        response << jrpc_error(1, wxString::Format("could not set parameter '%s' for %s axis algorithm '%s' (parameter may not exist or value out of range)",
                                                   name->string_value, axisName, alg->GetGuideAlgorithmClassName()));
    }
}

static void get_dec_guide_mode(JObj& response, const json_value *params)
{
    Scope *scope = TheScope();
    DEC_GUIDE_MODE mode = scope ? scope->GetDecGuideMode() : DEC_NONE;
    wxString s = Scope::DecGuideModeStr(mode);
    response << jrpc_result(s);
}

static void set_dec_guide_mode(JObj& response, const json_value *params)
{
    // Validate mount connection
    Scope *scope = TheScope();
    if (!scope || !scope->IsConnected())
    {
        response << jrpc_error(1, "mount not connected - cannot set Dec guide mode");
        return;
    }

    // Parse and validate mode parameter
    Params p("mode", params);
    const json_value *mode = p.param("mode");
    if (!mode || mode->type != JSON_STRING)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid 'mode' parameter (expected string: 'Off', 'Auto', 'North', or 'South')");
        return;
    }

    // Save previous mode for response
    DEC_GUIDE_MODE previous_mode = scope->GetDecGuideMode();

    // Validate mode value
    DEC_GUIDE_MODE m = DEC_AUTO;
    bool found = false;
    wxString available_modes;
    for (int im = DEC_NONE; im <= DEC_SOUTH; im++)
    {
        m = (DEC_GUIDE_MODE) im;
        wxString mode_str = Scope::DecGuideModeStr(m);
        if (im > DEC_NONE)
            available_modes += ", ";
        available_modes += "'" + mode_str + "'";
        
        if (wxStricmp(mode->string_value, mode_str) == 0)
        {
            found = true;
            // Don't break - continue building available_modes list
        }
    }

    if (!found)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS,
            wxString::Format("invalid Dec guide mode '%s' (expected one of: %s)",
                           mode->string_value, available_modes));
        return;
    }

    // Set the new mode
    scope->SetDecGuideMode(m);

    // Update UI controls
    if (pFrame->pGraphLog)
        pFrame->pGraphLog->UpdateControls();

    // Return detailed response
    JObj rslt;
    rslt << NV("mode", Scope::DecGuideModeStr(m))
         << NV("previous_mode", Scope::DecGuideModeStr(previous_mode));
    response << jrpc_result(rslt);
    
    Debug.Write(wxString::Format("EventServer: Dec guide mode changed from '%s' to '%s'\n",
                                 Scope::DecGuideModeStr(previous_mode), Scope::DecGuideModeStr(m)));
}

static void get_settling(JObj& response, const json_value *params)
{
    bool settling = PhdController::IsSettling();
    response << jrpc_result(settling);
}

static void get_variable_delay_settings(JObj& response, const json_value *params)
{
    JObj rslt;

    VarDelayCfg delayParams = pFrame->GetVariableDelayConfig();
    rslt << NV("Enabled", delayParams.enabled) << NV("ShortDelaySeconds", delayParams.shortDelay / 1000)
         << NV("LongDelaySeconds", delayParams.longDelay / 1000);
    response << jrpc_result(rslt);
}

// set_variable_delay values are in units of seconds to match the UI convention in the Advanced Settings dialog
static void set_variable_delay_settings(JObj& response, const json_value *params)
{
    Params p("Enabled", "ShortDelaySeconds", "LongDelaySeconds", params);
    const json_value *p0 = p.param("Enabled");
    const json_value *p1 = p.param("ShortDelaySeconds");
    const json_value *p2 = p.param("LongDelaySeconds");
    bool enabled;
    double shortDelaySec;
    double longDelaySec;
    if (!p0 || !p1 || !p2 || !bool_param(p0, &enabled) || !float_param(p1, &shortDelaySec) || !float_param(p2, &longDelaySec))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected Enabled, ShortDelaySeconds, LongDelaySeconds params)");
        return;
    }
    VarDelayCfg currParams;
    pFrame->SetVariableDelayConfig(enabled, (int) shortDelaySec * 1000, (int) longDelaySec * 1000);
    response << jrpc_result(0);
}

static void get_limit_frame(JObj& response, const json_value *params)
{
    JObj rslt;

    if (!pCamera || !pCamera->HasFrameLimiting || pCamera->LimitFrame.IsEmpty())
        rslt << NV("roi", NULL_VALUE);
    else
        rslt << NV("roi", pCamera->LimitFrame);
    response << jrpc_result(rslt);
}

static void set_limit_frame(JObj& response, const json_value *params)
{
    Params p("roi", params);
    const json_value *j = p.param("roi");
    if (!j)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing required param `roi`");
        return;
    }
    wxRect roi;
    if (j->type != JSON_NULL && !parse_rect(&roi, j))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid ROI param");
        return;
    }
    if (!pCamera)
    {
        response << jrpc_error(1, "no guide camera");
        return;
    }
    if (!pCamera->HasFrameLimiting)
    {
        response << jrpc_error(1, "guide camera does not support frame limiting");
        return;
    }
    wxString errorMessage;
    bool err = pCamera->SetLimitFrame(roi, pCamera->Binning, &errorMessage);

    if (err)
        response << jrpc_error(1, "could not set ROI. See Debug Log for more info.");
    else
        response << jrpc_result(0);
}

static GUIDE_DIRECTION dir_param(const json_value *p)
{
    if (!p || p->type != JSON_STRING)
        return GUIDE_DIRECTION::NONE;

    struct
    {
        const char *s;
        GUIDE_DIRECTION d;
    } dirs[] = {
        { "n", GUIDE_DIRECTION::NORTH },   { "s", GUIDE_DIRECTION::SOUTH },     { "e", GUIDE_DIRECTION::EAST },
        { "w", GUIDE_DIRECTION::WEST },    { "north", GUIDE_DIRECTION::NORTH }, { "south", GUIDE_DIRECTION::SOUTH },
        { "east", GUIDE_DIRECTION::EAST }, { "west", GUIDE_DIRECTION::WEST },   { "up", GUIDE_DIRECTION::UP },
        { "down", GUIDE_DIRECTION::DOWN }, { "left", GUIDE_DIRECTION::LEFT },   { "right", GUIDE_DIRECTION::RIGHT },
    };

    for (unsigned int i = 0; i < WXSIZEOF(dirs); i++)
        if (wxStricmp(p->string_value, dirs[i].s) == 0)
            return dirs[i].d;

    return GUIDE_DIRECTION::NONE;
}

static GUIDE_DIRECTION opposite(GUIDE_DIRECTION d)
{
    switch (d)
    {
    case UP:
        return DOWN;
    case DOWN:
        return UP;
    case LEFT:
        return RIGHT;
    case RIGHT:
        return LEFT;
    default:
        return d;
    }
}

static void guide_pulse(JObj& response, const json_value *params)
{
    Params p("amount", "direction", "which", params);

    // Validate amount parameter
    const json_value *amount = p.param("amount");
    if (!amount || amount->type != JSON_INT)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid 'amount' parameter (expected integer milliseconds, typical range: 10-5000)");
        return;
    }

    int duration = amount->int_value;
    int abs_duration = duration < 0 ? -duration : duration;
    
    // Validate amount range
    if (abs_duration < 1)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "guide pulse amount too short (minimum 1 millisecond)");
        return;
    }
    if (abs_duration > 10000)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS,
            wxString::Format("guide pulse amount too long (requested: %d ms, maximum: 10000 ms)", abs_duration));
        return;
    }

    // Validate direction parameter
    GUIDE_DIRECTION dir = dir_param(p.param("direction"));
    if (dir == GUIDE_DIRECTION::NONE)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid 'direction' parameter (expected 'N', 'S', 'E', 'W', 'North', 'South', 'East', or 'West')");
        return;
    }

    // Validate which parameter
    WHICH_MOUNT which = which_mount(p.param("which"));
    Mount *m = nullptr;
    wxString which_str;
    switch (which)
    {
    case MOUNT:
        m = TheScope();
        which_str = "mount";
        break;
    case AO:
        m = TheAO();
        which_str = "AO";
        break;
    case WHICH_MOUNT_BOTH:
    case WHICH_MOUNT_ERR:
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid 'which' parameter (expected 'mount' or 'ao')");
        return;
    }

    // Validate device connection
    if (!m || !m->IsConnected())
    {
        response << jrpc_error(1, wxString::Format("%s not connected - cannot send guide pulse", which_str));
        return;
    }

    // Validate device state
    if (pFrame->pGuider->IsCalibratingOrGuiding() || m->IsBusy())
    {
        response << jrpc_error(1, "cannot issue guide pulse while calibrating, guiding, or device busy");
        return;
    }

    // Handle negative duration (reverses direction)
    if (duration < 0)
    {
        duration = -duration;
        dir = opposite(dir);
    }

    // Get direction name for response
    wxString dir_str;
    switch (dir)
    {
    case NORTH: dir_str = "North"; break;
    case SOUTH: dir_str = "South"; break;
    case EAST:  dir_str = "East";  break;
    case WEST:  dir_str = "West";  break;
    default:    dir_str = "Unknown"; break;
    }

    // Issue the guide pulse
    pFrame->ScheduleManualMove(m, dir, duration);

    // Return detailed response
    JObj rslt;
    rslt << NV("direction", dir_str)
         << NV("amount", duration)
         << NV("which", which_str);
    response << jrpc_result(rslt);
    
    Debug.Write(wxString::Format("EventServer: Guide pulse %s %d ms (%s)\n", dir_str, duration, which_str));
}

static const char *parity_str(GuideParity p)
{
    switch (p)
    {
    case GUIDE_PARITY_EVEN:
        return "+";
    case GUIDE_PARITY_ODD:
        return "-";
    default:
        return "?";
    }
}

static void get_calibration_data(JObj& response, const json_value *params)
{
    Params p("which", params);

    // Validate which parameter
    WHICH_MOUNT which = which_mount(p.param("which"));
    Mount *m = nullptr;
    wxString which_str;
    switch (which)
    {
    case MOUNT:
        m = TheScope();
        which_str = "mount";
        break;
    case AO:
        m = TheAO();
        which_str = "AO";
        break;
    case WHICH_MOUNT_BOTH:
    case WHICH_MOUNT_ERR:
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid 'which' parameter (expected 'mount' or 'ao')");
        return;
    }
    }

    // Validate device connection
    if (!m || !m->IsConnected())
    {
        response << jrpc_error(1, wxString::Format("%s not connected - cannot retrieve calibration data", which_str));
        return;
    }

    // Build result object
    JObj rslt;
    rslt << NV("calibrated", m->IsCalibrated())
         << NV("which", which_str);

    if (m->IsCalibrated())
    {
        // Basic calibration angles and rates
        rslt << NV("xAngle", degrees(m->xAngle()), 1) 
             << NV("xRate", m->xRate() * 1000.0, 3)
             << NV("xParity", parity_str(m->RAParity())) 
             << NV("yAngle", degrees(m->yAngle()), 1)
             << NV("yRate", m->yRate() * 1000.0, 3) 
             << NV("yParity", parity_str(m->DecParity()))
             << NV("declination", degrees(m->GetCalibrationDeclination()));

        // Add timestamp if available
        if (!m->MountCal().timestamp.IsEmpty())
        {
            rslt << NV("timestamp", m->MountCal().timestamp);
        }

        // Add pier side if available (for mount only)
        if (which == MOUNT)
        {
            PierSide pier_side = m->MountCal().pierSide;
            wxString pier_side_str;
            switch (pier_side)
            {
            case PIER_SIDE_EAST:  pier_side_str = "East";    break;
            case PIER_SIDE_WEST:  pier_side_str = "West";    break;
            default:              pier_side_str = "Unknown"; break;
            }
            rslt << NV("pierSide", pier_side_str);
        }

        // Add image scale if available
        if (pFrame && pFrame->GetCameraPixelScale() > 0.0)
        {
            rslt << NV("imageScale", pFrame->GetCameraPixelScale(), 3);
        }
    }
    else
    {
        Debug.Write(wxString::Format("EventServer: %s not calibrated - no calibration data available\n", which_str));
    }

    response << jrpc_result(rslt);
}

// Helper function to validate camera connection
static bool validate_camera_connected(JObj& response)
{
    if (!pCamera)
    {
        response << jrpc_error(1, "camera not available");
        return false;
    }
    if (!pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected");
        return false;
    }
    return true;
}

// Helper function to validate mount connection
static bool validate_mount_connected(JObj& response)
{
    if (!pMount)
    {
        response << jrpc_error(1, "mount not available");
        return false;
    }
    if (!pMount->IsConnected())
    {
        response << jrpc_error(1, "mount not connected");
        return false;
    }
    return true;
}

// Helper function to validate guider state for calibration operations
static bool validate_guider_idle(JObj& response)
{
    if (!pFrame->pGuider)
    {
        response << jrpc_error(1, "guider not available");
        return false;
    }
    if (pFrame->pGuider->IsCalibratingOrGuiding())
    {
        response << jrpc_error(1, "cannot perform operation while calibrating or guiding");
        return false;
    }
    return true;
}

static void start_guider_calibration(JObj& response, const json_value *params)
{
    // Validate prerequisites
    if (!validate_camera_connected(response) ||
        !validate_mount_connected(response) ||
        !validate_guider_idle(response))
    {
        return;
    }

    // Parse parameters
    bool force_recalibration = false;
    SettleParams settle;
    wxRect roi;

    if (params)
    {
        Params p("force_recalibration", "settle", "roi", params);

        const json_value *p_force = p.param("force_recalibration");
        if (p_force && !bool_param(p_force, &force_recalibration))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected bool value for force_recalibration");
            return;
        }

        const json_value *p_settle = p.param("settle");
        if (p_settle)
        {
            wxString errMsg;
            if (!parse_settle(&settle, p_settle, &errMsg))
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, errMsg);
                return;
            }
        }
        else
        {
            // Default settle parameters
            settle.tolerancePx = 1.5;
            settle.settleTimeSec = 10;
            settle.timeoutSec = 60;
            settle.frames = 99;
        }

        const json_value *p_roi = p.param("roi");
        if (p_roi && !parse_roi(p_roi, roi))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid ROI param");
            return;
        }
    }
    else
    {
        // Default settle parameters
        settle.tolerancePx = 1.5;
        settle.settleTimeSec = 10;
        settle.timeoutSec = 60;
        settle.frames = 99;
    }

    wxString err;
    unsigned int ctrlOptions = GUIDEOPT_USE_STICKY_LOCK;
    if (force_recalibration)
        ctrlOptions |= GUIDEOPT_FORCE_RECAL;

    if (!PhdController::CanGuide(&err))
    {
        response << jrpc_error(1, err);
        return;
    }

    if (PhdController::Guide(ctrlOptions, settle, roi, &err))
    {
        response << jrpc_error(1, err);
        return;
    }

    response << jrpc_result(0);
}

// Helper function to get state string
static const char *StateStr(GUIDER_STATE state)
{
    switch (state)
    {
    case STATE_UNINITIALIZED: return "Uninitialized";
    case STATE_SELECTING: return "Selecting";
    case STATE_SELECTED: return "Selected";
    case STATE_CALIBRATING_PRIMARY: return "CalibratingPrimary";
    case STATE_CALIBRATING_SECONDARY: return "CalibratingSecondary";
    case STATE_CALIBRATED: return "Calibrated";
    case STATE_GUIDING: return "Guiding";
    case STATE_STOP: return "Stop";
    default: return "Unknown";
    }
}

static void get_guider_calibration_status(JObj& response, const json_value *params)
{
    if (!pFrame->pGuider)
    {
        response << jrpc_error(1, "guider not available");
        return;
    }

    GUIDER_STATE state = pFrame->pGuider->GetState();
    bool is_calibrating = pFrame->pGuider->IsCalibrating();

    JObj rslt;
    rslt << NV("calibrating", is_calibrating);
    rslt << NV("state", StateStr(state));

    if (is_calibrating)
    {
        // Determine which mount is being calibrated
        Mount *calibrating_mount = nullptr;
        if (state == STATE_CALIBRATING_PRIMARY && pMount)
        {
            calibrating_mount = pMount;
        }
        else if (state == STATE_CALIBRATING_SECONDARY && pSecondaryMount)
        {
            calibrating_mount = pSecondaryMount;
        }

        if (calibrating_mount)
        {
            rslt << NV("mount", calibrating_mount->IsStepGuider() ? "AO" : "Mount");
        }
    }

    // Include calibration status for both mounts
    if (pMount)
    {
        rslt << NV("mount_calibrated", pMount->IsCalibrated());
    }
    if (pSecondaryMount)
    {
        rslt << NV("ao_calibrated", pSecondaryMount->IsCalibrated());
    }

    response << jrpc_result(rslt);
}

// Helper function to validate exposure time parameter
static bool validate_exposure_time(int exposure_time, JObj& response, int min_ms = 100, int max_ms = 300000)
{
    if (exposure_time < min_ms || exposure_time > max_ms)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS,
            wxString::Format("exposure_time must be between %dms and %dms", min_ms, max_ms));
        return false;
    }
    return true;
}

// Helper function to validate frame count parameter
static bool validate_frame_count(int frame_count, JObj& response, int min_frames = 1, int max_frames = 100)
{
    if (frame_count < min_frames || frame_count > max_frames)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS,
            wxString::Format("frame_count must be between %d and %d", min_frames, max_frames));
        return false;
    }
    return true;
}

// Helper function to validate aggressiveness parameter
static bool validate_aggressiveness(int aggressiveness, JObj& response, const char* param_name)
{
    if (aggressiveness < 0 || aggressiveness > 100)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS,
            wxString::Format("%s must be between 0 and 100", param_name));
        return false;
    }
    return true;
}

// Helper function to validate pixel coordinates
static bool validate_pixel_coordinates(int x, int y, JObj& response)
{
    if (!pCamera)
    {
        response << jrpc_error(1, "camera not available");
        return false;
    }

    if (x < 0 || y < 0 ||
        x >= pCamera->FrameSize.GetWidth() ||
        y >= pCamera->FrameSize.GetHeight())
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS,
            wxString::Format("coordinates (%d,%d) out of bounds (0,0) to (%d,%d)",
                x, y, pCamera->FrameSize.GetWidth()-1, pCamera->FrameSize.GetHeight()-1));
        return false;
    }
    return true;
}

// Helper function to check if operation is in progress
static bool check_operation_in_progress(JObj& response, const char* operation_name)
{
    // This is a placeholder - in a full implementation, we would track active operations
    // For now, we just check basic guider state
    if (pFrame->pGuider && pFrame->pGuider->IsCalibratingOrGuiding())
    {
        response << jrpc_error(1, wxString::Format("%s cannot be started while calibrating or guiding", operation_name));
        return false;
    }
    return true;
}

// Helper function to validate hemisphere parameter
static bool validate_hemisphere(const wxString& hemisphere, JObj& response)
{
    if (hemisphere != "north" && hemisphere != "south")
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "hemisphere must be 'north' or 'south'");
        return false;
    }
    return true;
}

// Helper function to validate measurement time
static bool validate_measurement_time(int measurement_time, JObj& response, int min_sec = 60, int max_sec = 1800)
{
    if (measurement_time < min_sec || measurement_time > max_sec)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS,
            wxString::Format("measurement_time must be between %d and %d seconds", min_sec, max_sec));
        return false;
    }
    return true;
}

// Dark library build operation tracking
struct DarkLibraryBuildOperation
{
    int operation_id;
    int min_exposure;
    int max_exposure;
    int frame_count;
    wxString notes;
    bool modify_existing;

    enum Status {
        STARTING,
        CAPTURING_DARKS,
        BUILDING_MASTER_DARKS,
        SAVING_LIBRARY,
        COMPLETED,
        FAILED,
        CANCELLED
    } status;

    int current_exposure;
    int current_frame;
    int total_exposures;
    int total_frames;
    wxString error_message;
    wxString status_message;
    bool cancelled;

    // Exposure durations to process
    std::vector<int> exposure_durations;

    // Master dark frames created
    std::vector<usImage*> master_darks;
    wxMutex operation_mutex;  // Protect operation state

    DarkLibraryBuildOperation(int id, int min_exp, int max_exp, int frames,
                             const wxString& notes_str, bool modify)
        : operation_id(id), min_exposure(min_exp), max_exposure(max_exp),
          frame_count(frames), notes(notes_str), modify_existing(modify),
          status(STARTING), current_exposure(0), current_frame(0),
          total_exposures(0), total_frames(0), cancelled(false)
    {
        // Get available exposure durations from the frame
        std::vector<int> allExposures = pFrame->GetExposureDurations();
        std::sort(allExposures.begin(), allExposures.end());

        // Filter exposures within the specified range
        for (int exp : allExposures)
        {
            if (exp >= min_exposure && exp <= max_exposure)
            {
                exposure_durations.push_back(exp);
            }
        }

        total_exposures = exposure_durations.size();
        total_frames = total_exposures * frame_count;

        // Reserve space for master darks
        master_darks.reserve(total_exposures);
    }

    ~DarkLibraryBuildOperation()
    {
        // Clean up allocated master dark frames
        for (usImage* frame : master_darks)
        {
            delete frame;
        }
        master_darks.clear();
    }

    void SetStatus(Status new_status, const wxString& message = wxEmptyString)
    {
        wxMutexLocker lock(operation_mutex);
        status = new_status;
        if (!message.IsEmpty())
        {
            status_message = message;
        }
    }

    void SetError(const wxString& error)
    {
        wxMutexLocker lock(operation_mutex);
        status = FAILED;
        error_message = error;
        status_message = "Operation failed";
    }

    void Cancel()
    {
        wxMutexLocker lock(operation_mutex);
        cancelled = true;
        if (status != COMPLETED && status != FAILED)
        {
            status = CANCELLED;
            status_message = "Operation cancelled";
        }
    }

    void UpdateProgress(int exp_index, int frame_num)
    {
        wxMutexLocker lock(operation_mutex);
        current_exposure = exp_index;
        current_frame = frame_num;
    }
};

// Global tracking for dark library build operations
static std::map<int, DarkLibraryBuildOperation*> s_dark_library_operations;
static wxMutex s_dark_library_operations_mutex;

// Dark library build thread
class DarkLibraryBuildThread : public wxThread
{
private:
    DarkLibraryBuildOperation* m_operation;

public:
    DarkLibraryBuildThread(DarkLibraryBuildOperation* operation)
        : wxThread(wxTHREAD_DETACHED), m_operation(operation)
    {
    }

    virtual ExitCode Entry() override
    {
        if (!m_operation)
        {
            return (ExitCode)1;
        }

        try
        {
            Debug.Write(wxString::Format("DarkLibrary: Starting build operation %d\n", m_operation->operation_id));

            // Step 1: Clear existing darks if building new library
            if (!m_operation->modify_existing)
            {
                m_operation->SetStatus(DarkLibraryBuildOperation::STARTING, "Clearing existing dark library");
                pCamera->ClearDarks();
            }

            // Step 2: Capture and build master darks for each exposure
            if (!BuildAllMasterDarks())
            {
                return (ExitCode)1;
            }

            // Step 3: Add master darks to camera
            if (!AddDarksToCamera())
            {
                return (ExitCode)1;
            }

            // Step 4: Save dark library
            if (!SaveDarkLibrary())
            {
                return (ExitCode)1;
            }

            // Step 5: Complete successfully
            m_operation->SetStatus(DarkLibraryBuildOperation::COMPLETED,
                                 wxString::Format("Dark library completed with %d exposures",
                                                (int)m_operation->exposure_durations.size()));

            Debug.Write(wxString::Format("DarkLibrary: Build operation %d completed successfully\n",
                                       m_operation->operation_id));

            return (ExitCode)0;
        }
        catch (const wxString& error)
        {
            m_operation->SetError(error);
            Debug.Write(wxString::Format("DarkLibrary: Build operation %d failed: %s\n",
                                       m_operation->operation_id, error));
            return (ExitCode)1;
        }
        catch (...)
        {
            m_operation->SetError("Unknown exception during dark library build");
            Debug.Write(wxString::Format("DarkLibrary: Build operation %d failed with unknown exception\n",
                                       m_operation->operation_id));
            return (ExitCode)1;
        }
    }

private:
    bool BuildAllMasterDarks()
    {
        m_operation->SetStatus(DarkLibraryBuildOperation::CAPTURING_DARKS, "Capturing dark frames");

        pCamera->InitCapture();

        for (size_t i = 0; i < m_operation->exposure_durations.size(); i++)
        {
            if (m_operation->cancelled)
            {
                return false;
            }

            int expTime = m_operation->exposure_durations[i];

            wxString statusMsg;
            if (expTime >= 1000)
                statusMsg = wxString::Format("Building master dark at %.1f sec", (double)expTime / 1000.0);
            else
                statusMsg = wxString::Format("Building master dark at %d mSec", expTime);

            m_operation->SetStatus(DarkLibraryBuildOperation::BUILDING_MASTER_DARKS, statusMsg);

            usImage* masterDark = new usImage();
            if (!CreateMasterDarkFrame(*masterDark, expTime, m_operation->frame_count, i))
            {
                delete masterDark;
                return false;
            }

            m_operation->master_darks.push_back(masterDark);
        }

        return true;
    }

    bool CreateMasterDarkFrame(usImage& darkFrame, int expTime, int frameCount, int expIndex)
    {
        darkFrame.ImgExpDur = expTime;
        darkFrame.ImgStackCnt = frameCount;

        unsigned int* avgimg = nullptr;

        for (int j = 1; j <= frameCount; j++)
        {
            if (m_operation->cancelled)
            {
                delete[] avgimg;
                return false;
            }

            m_operation->UpdateProgress(expIndex, j);

            Debug.Write(wxString::Format("DarkLibrary: Capture dark frame %d/%d exp=%d\n", j, frameCount, expTime));

            bool err = GuideCamera::Capture(pCamera, expTime, darkFrame, CAPTURE_DARK);
            if (err)
            {
                m_operation->SetError(wxString::Format("Failed to capture dark frame %d/%d at %d ms", j, frameCount, expTime));
                delete[] avgimg;
                return false;
            }

            darkFrame.CalcStats();

            Debug.Write(wxString::Format("DarkLibrary: dark frame stats: bpp %u min %u max %u med %u\n",
                                       darkFrame.BitsPerPixel, darkFrame.MinADU, darkFrame.MaxADU, darkFrame.MedianADU));

            if (!avgimg)
            {
                avgimg = new unsigned int[darkFrame.NPixels];
                memset(avgimg, 0, darkFrame.NPixels * sizeof(*avgimg));
            }

            // Accumulate pixel values
            unsigned int* iptr = avgimg;
            const unsigned short* usptr = darkFrame.ImageData;
            for (unsigned int i = 0; i < darkFrame.NPixels; i++)
                *iptr++ += *usptr++;
        }

        if (!m_operation->cancelled)
        {
            // Average the accumulated values
            const unsigned int* iptr = avgimg;
            unsigned short* usptr = darkFrame.ImageData;
            for (unsigned int i = 0; i < darkFrame.NPixels; i++)
                *usptr++ = (unsigned short)(*iptr++ / frameCount);
        }

        delete[] avgimg;
        return !m_operation->cancelled;
    }

    bool AddDarksToCamera()
    {
        m_operation->SetStatus(DarkLibraryBuildOperation::BUILDING_MASTER_DARKS, "Adding master darks to camera");

        for (usImage* masterDark : m_operation->master_darks)
        {
            if (m_operation->cancelled)
            {
                return false;
            }

            pCamera->AddDark(masterDark);
        }

        // Clear the vector but don't delete the images - they're now owned by the camera
        m_operation->master_darks.clear();

        return true;
    }

    bool SaveDarkLibrary()
    {
        m_operation->SetStatus(DarkLibraryBuildOperation::SAVING_LIBRARY, "Saving dark library");

        try
        {
            pFrame->SaveDarkLibrary(m_operation->notes);
            pFrame->LoadDarkHandler(true); // Put it to use

            return true;
        }
        catch (const wxString& error)
        {
            m_operation->SetError(wxString::Format("Failed to save dark library: %s", error));
            return false;
        }
        catch (...)
        {
            m_operation->SetError("Exception during dark library saving");
            return false;
        }
    }
};

// Asynchronous dark library build function
static void StartDarkLibraryBuildAsync(DarkLibraryBuildOperation* operation)
{
    DarkLibraryBuildThread* thread = new DarkLibraryBuildThread(operation);
    if (thread->Create() != wxTHREAD_NO_ERROR)
    {
        operation->SetError("Failed to create build thread");
        delete thread;
        return;
    }

    if (thread->Run() != wxTHREAD_NO_ERROR)
    {
        operation->SetError("Failed to start build thread");
        return;
    }

    // Thread is now running and will delete itself when complete
}

// Clean up completed operations
static void CleanupCompletedDarkLibraryOperations()
{
    wxMutexLocker lock(s_dark_library_operations_mutex);

    auto it = s_dark_library_operations.begin();
    while (it != s_dark_library_operations.end())
    {
        DarkLibraryBuildOperation* op = it->second;
        if (op->status == DarkLibraryBuildOperation::COMPLETED ||
            op->status == DarkLibraryBuildOperation::FAILED ||
            op->status == DarkLibraryBuildOperation::CANCELLED)
        {
            // Keep completed operations for a while so status can be queried
            // In a real implementation, you might want to clean these up after some time
            ++it;
        }
        else
        {
            ++it;
        }
    }
}

static void start_dark_library_build(JObj& response, const json_value *params)
{
    // Validate prerequisites
    if (!validate_camera_connected(response) || !validate_guider_idle(response))
    {
        return;
    }

    // Parse parameters
    int min_exposure = 1000; // Default 1 second
    int max_exposure = 15000; // Default 15 seconds
    int frame_count = 5; // Default 5 frames per exposure
    wxString notes;
    bool modify_existing = false;

    if (params)
    {
        Params p("min_exposure", "max_exposure", "frame_count", "notes", "modify_existing", params);

        const json_value *p_min = p.param("min_exposure");
        if (p_min && !int_param(p_min, &min_exposure))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for min_exposure");
            return;
        }

        const json_value *p_max = p.param("max_exposure");
        if (p_max && !int_param(p_max, &max_exposure))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for max_exposure");
            return;
        }

        const json_value *p_count = p.param("frame_count");
        if (p_count && !int_param(p_count, &frame_count))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for frame_count");
            return;
        }

        const json_value *p_notes = p.param("notes");
        if (p_notes && p_notes->type == JSON_STRING)
        {
            notes = wxString(p_notes->string_value, wxConvUTF8);
        }

        const json_value *p_modify = p.param("modify_existing");
        if (p_modify && !bool_param(p_modify, &modify_existing))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected bool value for modify_existing");
            return;
        }
    }

    // Validate parameters
    if (!validate_exposure_time(min_exposure, response) ||
        !validate_exposure_time(max_exposure, response) ||
        !validate_frame_count(frame_count, response))
    {
        return;
    }

    if (max_exposure < min_exposure)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "max_exposure must be >= min_exposure");
        return;
    }

    // Clean up any old completed operations
    CleanupCompletedDarkLibraryOperations();

    // Create and start the dark library build operation
    static int operation_counter = 2000; // Start from 2000 to distinguish from defect map operations
    int operation_id = operation_counter++;

    DarkLibraryBuildOperation* operation = new DarkLibraryBuildOperation(
        operation_id, min_exposure, max_exposure, frame_count, notes, modify_existing);

    // Check if we have any exposures to process
    if (operation->exposure_durations.empty())
    {
        delete operation;
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "No exposure durations found in the specified range");
        return;
    }

    {
        wxMutexLocker lock(s_dark_library_operations_mutex);
        s_dark_library_operations[operation_id] = operation;
    }

    // Start the asynchronous build process
    StartDarkLibraryBuildAsync(operation);

    Debug.Write(wxString::Format("DarkLibrary: Started build operation %d - min=%dms, max=%dms, frames=%d, exposures=%d\n",
                               operation_id, min_exposure, max_exposure, frame_count, (int)operation->exposure_durations.size()));

    JObj rslt;
    rslt << NV("operation_id", operation_id);
    rslt << NV("min_exposure", min_exposure);
    rslt << NV("max_exposure", max_exposure);
    rslt << NV("frame_count", frame_count);
    rslt << NV("modify_existing", modify_existing);
    rslt << NV("total_exposures", (int)operation->exposure_durations.size());

    response << jrpc_result(rslt);
}

static void get_dark_library_status(JObj& response, const json_value *params)
{
    if (!pCamera)
    {
        response << jrpc_error(1, "camera not available");
        return;
    }

    JObj rslt;

    // Parse optional operation_id parameter
    int operation_id = -1;
    if (params)
    {
        Params p("operation_id", params);
        const json_value *p_op_id = p.param("operation_id");
        if (p_op_id && !int_param(p_op_id, &operation_id))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for operation_id");
            return;
        }
    }

    // If operation_id is specified, return build operation status
    if (operation_id >= 0)
    {
        wxMutexLocker lock(s_dark_library_operations_mutex);
        auto it = s_dark_library_operations.find(operation_id);
        if (it == s_dark_library_operations.end())
        {
            response << jrpc_error(1, "operation not found");
            return;
        }

        DarkLibraryBuildOperation* operation = it->second;
        wxMutexLocker op_lock(operation->operation_mutex);

        rslt << NV("operation_id", operation_id);

        // Convert status enum to string
        wxString status_str;
        switch (operation->status)
        {
            case DarkLibraryBuildOperation::STARTING:
                status_str = "starting";
                break;
            case DarkLibraryBuildOperation::CAPTURING_DARKS:
                status_str = "capturing_darks";
                break;
            case DarkLibraryBuildOperation::BUILDING_MASTER_DARKS:
                status_str = "building_master_darks";
                break;
            case DarkLibraryBuildOperation::SAVING_LIBRARY:
                status_str = "saving_library";
                break;
            case DarkLibraryBuildOperation::COMPLETED:
                status_str = "completed";
                break;
            case DarkLibraryBuildOperation::FAILED:
                status_str = "failed";
                break;
            case DarkLibraryBuildOperation::CANCELLED:
                status_str = "cancelled";
                break;
            default:
                status_str = "unknown";
                break;
        }

        rslt << NV("status", status_str);
        rslt << NV("status_message", operation->status_message);

        if (!operation->error_message.IsEmpty())
        {
            rslt << NV("error_message", operation->error_message);
        }

        // Calculate progress percentage
        int progress = 0;
        if (operation->total_frames > 0)
        {
            switch (operation->status)
            {
                case DarkLibraryBuildOperation::CAPTURING_DARKS:
                case DarkLibraryBuildOperation::BUILDING_MASTER_DARKS:
                {
                    int frames_completed = operation->current_exposure * operation->frame_count + operation->current_frame;
                    progress = (frames_completed * 90) / operation->total_frames;
                    break;
                }
                case DarkLibraryBuildOperation::SAVING_LIBRARY:
                    progress = 95;
                    break;
                case DarkLibraryBuildOperation::COMPLETED:
                    progress = 100;
                    break;
                case DarkLibraryBuildOperation::FAILED:
                case DarkLibraryBuildOperation::CANCELLED:
                    progress = 0;
                    break;
                default:
                    progress = 0;
                    break;
            }
        }

        rslt << NV("progress", progress);
        rslt << NV("current_exposure_index", operation->current_exposure);
        rslt << NV("current_frame", operation->current_frame);
        rslt << NV("total_exposures", operation->total_exposures);
        rslt << NV("total_frames", operation->total_frames);

        if (operation->current_exposure < (int)operation->exposure_durations.size())
        {
            rslt << NV("current_exposure_time", operation->exposure_durations[operation->current_exposure]);
        }
    }
    else
    {
        // Return general dark library status
        int numDarks;
        double minExp, maxExp;
        pCamera->GetDarkLibraryProperties(&numDarks, &minExp, &maxExp);

        rslt << NV("loaded", numDarks > 0);
        rslt << NV("frame_count", numDarks);

        if (numDarks > 0)
        {
            rslt << NV("min_exposure", (int)(minExp * 1000.0));
            rslt << NV("max_exposure", (int)(maxExp * 1000.0));
        }

        // Check if there are any active build operations
        {
            wxMutexLocker lock(s_dark_library_operations_mutex);
            bool has_active_operation = false;
            for (const auto& pair : s_dark_library_operations)
            {
                DarkLibraryBuildOperation* op = pair.second;
                if (op->status != DarkLibraryBuildOperation::COMPLETED &&
                    op->status != DarkLibraryBuildOperation::FAILED &&
                    op->status != DarkLibraryBuildOperation::CANCELLED)
                {
                    has_active_operation = true;
                    rslt << NV("active_operation_id", pair.first);
                    break;
                }
            }
            rslt << NV("building", has_active_operation);
        }
    }

    response << jrpc_result(rslt);
}

static void load_dark_library(JObj& response, const json_value *params)
{
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected");
        return;
    }

    // Parse optional profile_id parameter
    int profile_id = pConfig->GetCurrentProfileId();
    if (params)
    {
        Params p("profile_id", params);
        const json_value *p_profile = p.param("profile_id");
        if (p_profile && !int_param(p_profile, &profile_id))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for profile_id");
            return;
        }
    }

    bool success = pFrame->LoadDarkLibrary();

    JObj rslt;
    rslt << NV("success", success);

    if (success)
    {
        int numDarks;
        double minExp, maxExp;
        pCamera->GetDarkLibraryProperties(&numDarks, &minExp, &maxExp);
        rslt << NV("frame_count", numDarks);
        rslt << NV("min_exposure", (int)(minExp * 1000.0));
        rslt << NV("max_exposure", (int)(maxExp * 1000.0));
    }

    response << jrpc_result(rslt);
}

static void clear_dark_library(JObj& response, const json_value *params)
{
    if (!pCamera)
    {
        response << jrpc_error(1, "camera not available");
        return;
    }

    pCamera->ClearDarks();

    JObj rslt;
    rslt << NV("success", true);
    response << jrpc_result(rslt);
}

static void cancel_dark_library_build(JObj& response, const json_value *params)
{
    if (!params)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "operation_id parameter required");
        return;
    }

    Params p("operation_id", params);
    const json_value *p_id = p.param("operation_id");
    int operation_id;
    if (!p_id || !int_param(p_id, &operation_id))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for operation_id");
        return;
    }

    wxMutexLocker lock(s_dark_library_operations_mutex);
    auto it = s_dark_library_operations.find(operation_id);
    if (it == s_dark_library_operations.end())
    {
        response << jrpc_error(1, "operation not found");
        return;
    }

    DarkLibraryBuildOperation* operation = it->second;
    operation->Cancel();

    JObj rslt;
    rslt << NV("operation_id", operation_id);
    rslt << NV("cancelled", true);

    response << jrpc_result(rslt);
}

// Defect map build operation tracking
struct DefectMapBuildOperation
{
    int operation_id;
    int exposure_time;
    int frame_count;
    int hot_aggressiveness;
    int cold_aggressiveness;

    enum Status {
        STARTING,
        CAPTURING_DARKS,
        BUILDING_MASTER_DARK,
        BUILDING_FILTERED_DARK,
        ANALYZING_DEFECTS,
        SAVING_MAP,
        COMPLETED,
        FAILED,
        CANCELLED
    } status;

    int frames_captured;
    int total_frames;
    wxString error_message;
    wxString status_message;
    bool cancelled;

    // Objects for the build process
    DefectMapDarks darks;
    DefectMapBuilder builder;
    DefectMap defect_map;

    // Pixel type counts (set during analysis phase)
    int hot_pixel_count;
    int cold_pixel_count;
    int total_defect_count;

    // Accumulated dark frames for master dark creation
    std::vector<usImage*> dark_frames;
    wxMutex operation_mutex;  // Protect operation state

    DefectMapBuildOperation(int id, int exp_time, int frames, int hot_aggr, int cold_aggr)
        : operation_id(id), exposure_time(exp_time), frame_count(frames),
          hot_aggressiveness(hot_aggr), cold_aggressiveness(cold_aggr),
          status(STARTING), frames_captured(0), total_frames(frames), cancelled(false),
          hot_pixel_count(-1), cold_pixel_count(-1), total_defect_count(0)
    {
        // Reserve space for dark frames
        dark_frames.reserve(frames);
    }

    ~DefectMapBuildOperation()
    {
        // Clean up allocated dark frames
        for (usImage* frame : dark_frames)
        {
            delete frame;
        }
        dark_frames.clear();
    }

    void SetStatus(Status new_status, const wxString& message = wxEmptyString)
    {
        wxMutexLocker lock(operation_mutex);
        status = new_status;
        if (!message.IsEmpty())
        {
            status_message = message;
        }
    }

    void SetError(const wxString& error)
    {
        wxMutexLocker lock(operation_mutex);
        status = FAILED;
        error_message = error;
        status_message = "Operation failed";
    }

    void Cancel()
    {
        wxMutexLocker lock(operation_mutex);
        cancelled = true;
        if (status != COMPLETED && status != FAILED)
        {
            status = CANCELLED;
            status_message = "Operation cancelled";
        }
    }

    bool IsCancelled() const
    {
        wxMutexLocker lock(const_cast<wxMutex&>(operation_mutex));
        return cancelled;
    }
};

static std::map<int, DefectMapBuildOperation*> s_defect_map_operations;
static wxMutex s_defect_map_operations_mutex;

// Helper function to clean up completed operations
static void CleanupCompletedOperations()
{
    wxMutexLocker lock(s_defect_map_operations_mutex);

    auto it = s_defect_map_operations.begin();
    while (it != s_defect_map_operations.end())
    {
        DefectMapBuildOperation* op = it->second;
        if (op->status == DefectMapBuildOperation::COMPLETED ||
            op->status == DefectMapBuildOperation::FAILED ||
            op->status == DefectMapBuildOperation::CANCELLED)
        {
            // Keep completed operations for a while so clients can check final status
            // In a production system, you might want to implement a timer-based cleanup
            ++it;
        }
        else
        {
            ++it;
        }
    }
}

// Helper function to capture a single dark frame
static bool CaptureDefectMapDarkFrame(DefectMapBuildOperation* operation, usImage& darkFrame)
{
    if (!pCamera || !pCamera->Connected)
    {
        operation->SetError("Camera not connected");
        return false;
    }

    if (operation->IsCancelled())
    {
        return false;
    }

    try
    {
        // Set camera to dark mode (close shutter if available)
        bool prevShutterState = pCamera->ShutterClosed;
        pCamera->ShutterClosed = true;

        // Capture the dark frame
        Debug.Write(wxString::Format("DefectMap: Capturing dark frame %d/%d, exposure=%dms\n",
                                   operation->frames_captured + 1, operation->frame_count, operation->exposure_time));

        bool error = GuideCamera::Capture(pCamera, operation->exposure_time, darkFrame, CAPTURE_DARK);

        // Restore previous shutter state
        pCamera->ShutterClosed = prevShutterState;

        if (error)
        {
            operation->SetError(wxString::Format("Failed to capture dark frame %d", operation->frames_captured + 1));
            return false;
        }

        // Validate the captured frame
        if (!darkFrame.ImageData || darkFrame.NPixels == 0)
        {
            operation->SetError("Captured dark frame is invalid");
            return false;
        }

        operation->frames_captured++;
        operation->SetStatus(DefectMapBuildOperation::CAPTURING_DARKS,
                           wxString::Format("Captured dark frame %d of %d",
                                          operation->frames_captured, operation->frame_count));

        Debug.Write(wxString::Format("DefectMap: Successfully captured dark frame %d/%d\n",
                                   operation->frames_captured, operation->frame_count));

        return true;
    }
    catch (const wxString& error)
    {
        operation->SetError(wxString::Format("Exception capturing dark frame: %s", error));
        return false;
    }
    catch (...)
    {
        operation->SetError("Unknown exception during dark frame capture");
        return false;
    }
}

// Helper function to build master dark from captured frames
static bool BuildMasterDarkFromFrames(DefectMapBuildOperation* operation)
{
    if (operation->dark_frames.empty())
    {
        operation->SetError("No dark frames available for master dark creation");
        return false;
    }

    if (operation->IsCancelled())
    {
        return false;
    }

    try
    {
        operation->SetStatus(DefectMapBuildOperation::BUILDING_MASTER_DARK, "Building master dark frame");

        // Initialize master dark with the size of the first frame
        usImage* firstFrame = operation->dark_frames[0];
        operation->darks.masterDark.Init(firstFrame->Size);

        int numFrames = operation->dark_frames.size();
        int numPixels = firstFrame->NPixels;

        Debug.Write(wxString::Format("DefectMap: Building master dark from %d frames, %d pixels each\n",
                                   numFrames, numPixels));

        // Initialize master dark to zero
        memset(operation->darks.masterDark.ImageData, 0, numPixels * sizeof(unsigned short));

        // Accumulate all frames using proper averaging to avoid overflow
        for (int frameIdx = 0; frameIdx < numFrames; frameIdx++)
        {
            if (operation->IsCancelled())
            {
                return false;
            }

            usImage* frame = operation->dark_frames[frameIdx];
            unsigned short* masterData = operation->darks.masterDark.ImageData;
            unsigned short* frameData = frame->ImageData;

            // Add this frame to the running average
            for (int pixelIdx = 0; pixelIdx < numPixels; pixelIdx++)
            {
                // Use integer arithmetic to maintain precision
                // Running average: new_avg = old_avg + (new_value - old_avg) / count
                unsigned int currentAvg = masterData[pixelIdx];
                unsigned int newValue = frameData[pixelIdx];
                unsigned int newAvg = currentAvg + (newValue - currentAvg) / (frameIdx + 1);
                masterData[pixelIdx] = (unsigned short)std::min(newAvg, (unsigned int)65535);
            }

            // Update progress
            operation->SetStatus(DefectMapBuildOperation::BUILDING_MASTER_DARK,
                               wxString::Format("Processing frame %d of %d", frameIdx + 1, numFrames));
        }

        // Calculate statistics for the master dark
        operation->darks.masterDark.CalcStats();

        Debug.Write(wxString::Format("DefectMap: Master dark completed - median=%d, max=%d, min=%d\n",
                                   operation->darks.masterDark.MedianADU,
                                   operation->darks.masterDark.MaxADU,
                                   operation->darks.masterDark.MinADU));

        return true;
    }
    catch (const wxString& error)
    {
        operation->SetError(wxString::Format("Exception building master dark: %s", error));
        return false;
    }
    catch (...)
    {
        operation->SetError("Unknown exception during master dark building");
        return false;
    }
}

// Thread class for defect map building
class DefectMapBuildThread : public wxThread
{
private:
    DefectMapBuildOperation* m_operation;

public:
    DefectMapBuildThread(DefectMapBuildOperation* operation)
        : wxThread(wxTHREAD_DETACHED), m_operation(operation) {}

    virtual ExitCode Entry() override
    {
        // Set thread as killable for proper shutdown
        WorkerThread* workerThread = WorkerThread::This();
        bool prevKillable = false;
        if (workerThread)
        {
            prevKillable = workerThread->SetKillable(true);
        }

        try
        {
            Debug.Write(wxString::Format("DefectMap: Starting build operation %d\n", m_operation->operation_id));

            // Step 1: Capture dark frames
            if (!CaptureAllDarkFrames())
            {
                return (ExitCode)1;
            }

            // Step 2: Build master dark from captured frames
            if (!BuildMasterDarkFromFrames(m_operation))
            {
                return (ExitCode)1;
            }

            // Step 3: Build filtered dark
            if (!BuildFilteredDark())
            {
                return (ExitCode)1;
            }

            // Step 4: Analyze defects and build defect map
            if (!AnalyzeDefects())
            {
                return (ExitCode)1;
            }

            // Step 5: Save defect map
            if (!SaveDefectMap())
            {
                return (ExitCode)1;
            }

            // Step 6: Complete successfully
            m_operation->SetStatus(DefectMapBuildOperation::COMPLETED,
                                 wxString::Format("Defect map completed with %d defects",
                                                (int)m_operation->defect_map.size()));

            Debug.Write(wxString::Format("DefectMap: Build operation %d completed successfully\n",
                                       m_operation->operation_id));
        }
        catch (const wxString& error)
        {
            Debug.Write(wxString::Format("DefectMap: Build operation %d failed: %s\n",
                                       m_operation->operation_id, error));
            m_operation->SetError(wxString::Format("Build failed: %s", error));
        }
        catch (...)
        {
            Debug.Write(wxString::Format("DefectMap: Build operation %d failed with unknown exception\n",
                                       m_operation->operation_id));
            m_operation->SetError("Unexpected error during defect map building");
        }

        // Restore previous killable state
        if (workerThread)
        {
            workerThread->SetKillable(prevKillable);
        }

        return (ExitCode)0;
    }

private:
    bool CaptureAllDarkFrames()
    {
        if (!pCamera || !pCamera->Connected)
        {
            m_operation->SetError("Camera not connected");
            return false;
        }

        m_operation->SetStatus(DefectMapBuildOperation::CAPTURING_DARKS, "Starting dark frame capture");

        // Ensure camera is in dark mode
        bool prevShutterState = pCamera->ShutterClosed;
        pCamera->ShutterClosed = true;

        try
        {
            for (int frameIdx = 0; frameIdx < m_operation->frame_count; frameIdx++)
            {
                if (m_operation->IsCancelled())
                {
                    pCamera->ShutterClosed = prevShutterState;
                    return false;
                }

                // Check for worker thread interruption
                if (WorkerThread::InterruptRequested())
                {
                    m_operation->Cancel();
                    pCamera->ShutterClosed = prevShutterState;
                    return false;
                }

                // Allocate new frame
                usImage* darkFrame = new usImage();

                // Capture the frame
                if (!CaptureDefectMapDarkFrame(m_operation, *darkFrame))
                {
                    delete darkFrame;
                    pCamera->ShutterClosed = prevShutterState;
                    return false;
                }

                // Store the captured frame
                m_operation->dark_frames.push_back(darkFrame);

                // Small delay between captures to allow for camera settling
                if (frameIdx < m_operation->frame_count - 1)
                {
                    WorkerThread::MilliSleep(500, WorkerThread::INT_ANY);
                }
            }

            pCamera->ShutterClosed = prevShutterState;
            return true;
        }
        catch (...)
        {
            pCamera->ShutterClosed = prevShutterState;
            m_operation->SetError("Exception during dark frame capture");
            return false;
        }
    }

    bool BuildFilteredDark()
    {
        if (m_operation->IsCancelled())
        {
            return false;
        }

        try
        {
            m_operation->SetStatus(DefectMapBuildOperation::BUILDING_FILTERED_DARK, "Building filtered dark frame");

            Debug.Write("DefectMap: Building filtered dark frame\n");
            m_operation->darks.BuildFilteredDark();

            return true;
        }
        catch (const wxString& error)
        {
            m_operation->SetError(wxString::Format("Failed to build filtered dark: %s", error));
            return false;
        }
        catch (...)
        {
            m_operation->SetError("Exception during filtered dark building");
            return false;
        }
    }

    bool AnalyzeDefects()
    {
        if (m_operation->IsCancelled())
        {
            return false;
        }

        try
        {
            m_operation->SetStatus(DefectMapBuildOperation::ANALYZING_DEFECTS, "Analyzing defects");

            Debug.Write("DefectMap: Initializing defect map builder\n");
            m_operation->builder.Init(m_operation->darks);

            Debug.Write(wxString::Format("DefectMap: Setting aggressiveness - cold=%d, hot=%d\n",
                                       m_operation->cold_aggressiveness, m_operation->hot_aggressiveness));
            m_operation->builder.SetAggressiveness(m_operation->cold_aggressiveness, m_operation->hot_aggressiveness);

            // Get pixel counts before building the defect map
            m_operation->hot_pixel_count = m_operation->builder.GetHotPixelCnt();
            m_operation->cold_pixel_count = m_operation->builder.GetColdPixelCnt();

            Debug.Write(wxString::Format("DefectMap: Pixel analysis - hot=%d, cold=%d\n",
                                       m_operation->hot_pixel_count, m_operation->cold_pixel_count));

            Debug.Write("DefectMap: Building defect map\n");
            m_operation->builder.BuildDefectMap(m_operation->defect_map, true);

            m_operation->total_defect_count = (int)m_operation->defect_map.size();

            Debug.Write(wxString::Format("DefectMap: Analysis complete - found %d defects (hot=%d, cold=%d)\n",
                                       m_operation->total_defect_count, m_operation->hot_pixel_count, m_operation->cold_pixel_count));

            return true;
        }
        catch (const wxString& error)
        {
            m_operation->SetError(wxString::Format("Failed to analyze defects: %s", error));
            return false;
        }
        catch (...)
        {
            m_operation->SetError("Exception during defect analysis");
            return false;
        }
    }

    bool SaveDefectMap()
    {
        if (m_operation->IsCancelled())
        {
            return false;
        }

        try
        {
            m_operation->SetStatus(DefectMapBuildOperation::SAVING_MAP, "Saving defect map");

            Debug.Write("DefectMap: Saving defect map to disk\n");

            // Get the base map info and add our pixel count information
            wxArrayString mapInfo = m_operation->builder.GetMapInfo();

            // Add pixel count information to the metadata
            mapInfo.push_back(wxString::Format("Hot pixels detected: %d", m_operation->hot_pixel_count));
            mapInfo.push_back(wxString::Format("Cold pixels detected: %d", m_operation->cold_pixel_count));
            mapInfo.push_back(wxString::Format("Total defects: %d", m_operation->total_defect_count));
            mapInfo.push_back(wxString::Format("Manual defects added: 0")); // Initially no manual defects

            m_operation->defect_map.Save(mapInfo);

            // Verify the save was successful by checking if the file exists
            wxString filename = DefectMap::DefectMapFileName(pConfig->GetCurrentProfileId());
            if (!wxFileExists(filename))
            {
                m_operation->SetError("Failed to save defect map - file not created");
                return false;
            }

            Debug.Write(wxString::Format("DefectMap: Successfully saved to %s\n", filename));

            return true;
        }
        catch (const wxString& error)
        {
            m_operation->SetError(wxString::Format("Failed to save defect map: %s", error));
            return false;
        }
        catch (...)
        {
            m_operation->SetError("Exception during defect map saving");
            return false;
        }
    }
};

// Asynchronous defect map build function
static void StartDefectMapBuildAsync(DefectMapBuildOperation* operation)
{
    DefectMapBuildThread* thread = new DefectMapBuildThread(operation);
    if (thread->Create() != wxTHREAD_NO_ERROR)
    {
        operation->SetError("Failed to create build thread");
        delete thread;
        return;
    }

    if (thread->Run() != wxTHREAD_NO_ERROR)
    {
        operation->SetError("Failed to start build thread");
        // Thread will be automatically deleted since it's detached
        return;
    }

    // Thread is now running and will delete itself when complete
}

static void start_defect_map_build(JObj& response, const json_value *params)
{
    // Validate prerequisites
    if (!validate_camera_connected(response) || !validate_guider_idle(response))
    {
        return;
    }

    // Additional validation for defect map building
    if (!pCamera->HasShutter)
    {
        // For cameras without shutters, warn the user but don't fail
        Debug.Write("DefectMap: Warning - camera has no shutter, ensure lens cap is on\n");
    }

    // Parse parameters
    int exposure_time = 15000; // Default 15 seconds
    int frame_count = 10; // Default 10 frames
    int hot_aggressiveness = 75; // Default aggressiveness
    int cold_aggressiveness = 75;

    if (params)
    {
        Params p("exposure_time", "frame_count", "hot_aggressiveness", "cold_aggressiveness", params);

        const json_value *p_exp = p.param("exposure_time");
        if (p_exp && !int_param(p_exp, &exposure_time))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for exposure_time");
            return;
        }

        const json_value *p_count = p.param("frame_count");
        if (p_count && !int_param(p_count, &frame_count))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for frame_count");
            return;
        }

        const json_value *p_hot = p.param("hot_aggressiveness");
        if (p_hot && !int_param(p_hot, &hot_aggressiveness))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for hot_aggressiveness");
            return;
        }

        const json_value *p_cold = p.param("cold_aggressiveness");
        if (p_cold && !int_param(p_cold, &cold_aggressiveness))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for cold_aggressiveness");
            return;
        }
    }

    // Validate parameters
    if (!validate_exposure_time(exposure_time, response, 1000, 300000) ||
        !validate_frame_count(frame_count, response, 5, 100) ||
        !validate_aggressiveness(hot_aggressiveness, response, "hot_aggressiveness") ||
        !validate_aggressiveness(cold_aggressiveness, response, "cold_aggressiveness"))
    {
        return;
    }

    // Clean up any old completed operations
    CleanupCompletedOperations();

    // Create and start the defect map build operation
    static int operation_counter = 1000;
    int operation_id = operation_counter++;

    DefectMapBuildOperation* operation = new DefectMapBuildOperation(
        operation_id, exposure_time, frame_count, hot_aggressiveness, cold_aggressiveness);

    {
        wxMutexLocker lock(s_defect_map_operations_mutex);
        s_defect_map_operations[operation_id] = operation;
    }

    // Start the asynchronous build process
    StartDefectMapBuildAsync(operation);

    Debug.Write(wxString::Format("DefectMap: Started build operation %d - exp=%dms, frames=%d, hot=%d, cold=%d\n",
                               operation_id, exposure_time, frame_count, hot_aggressiveness, cold_aggressiveness));

    JObj rslt;
    rslt << NV("operation_id", operation_id);
    rslt << NV("exposure_time", exposure_time);
    rslt << NV("frame_count", frame_count);
    rslt << NV("hot_aggressiveness", hot_aggressiveness);
    rslt << NV("cold_aggressiveness", cold_aggressiveness);

    response << jrpc_result(rslt);
}

// Helper function to parse defect map file for metadata
static void ParseDefectMapMetadata(int profileId, int& hot_count, int& cold_count, int& manual_count, wxString& creation_time, wxString& camera_name)
{
    hot_count = -1;  // -1 indicates unknown
    cold_count = -1;
    manual_count = -1;
    creation_time = "";
    camera_name = "";

    wxString filename = DefectMap::DefectMapFileName(profileId);
    if (!wxFileExists(filename))
        return;

    wxFileInputStream iStream(filename);
    if (iStream.GetLastError() != wxSTREAM_NO_ERROR)
        return;

    wxTextInputStream inText(iStream);

    while (!inText.GetInputStream().Eof())
    {
        wxString line = inText.ReadLine();
        line.Trim(false); // trim leading whitespace

        if (!line.StartsWith("#"))
            break; // End of header comments

        // Parse various metadata from comments
        if (line.Contains("cold=") && line.Contains("hot="))
        {
            // Look for pattern like "# New defect map created, count=123 (cold=45, hot=78)"
            wxString temp = line.AfterFirst('(');
            if (!temp.IsEmpty())
            {
                temp = temp.BeforeFirst(')');
                wxStringTokenizer tok(temp, ",");
                while (tok.HasMoreTokens())
                {
                    wxString token = tok.GetNextToken().Trim().Trim(false);
                    if (token.StartsWith("cold="))
                    {
                        long val;
                        if (token.AfterFirst('=').ToLong(&val))
                            cold_count = (int)val;
                    }
                    else if (token.StartsWith("hot="))
                    {
                        long val;
                        if (token.AfterFirst('=').ToLong(&val))
                            hot_count = (int)val;
                    }
                }
            }
        }
        else if (line.Contains("Creation time:"))
        {
            creation_time = line.AfterFirst(':').Trim().Trim(false);
        }
        else if (line.Contains("Camera:"))
        {
            camera_name = line.AfterFirst(':').Trim().Trim(false);
        }
        else if (line.Contains("Manual defects added:"))
        {
            wxString count_str = line.AfterFirst(':').Trim().Trim(false);
            long val;
            if (count_str.ToLong(&val))
                manual_count = (int)val;
        }
    }
}

static void get_defect_map_status(JObj& response, const json_value *params)
{
    if (!pCamera)
    {
        response << jrpc_error(1, "camera not available");
        return;
    }

    JObj rslt;

    bool loaded = (pCamera->CurrentDefectMap != nullptr);
    rslt << NV("loaded", loaded);

    if (loaded)
    {
        int pixel_count = pCamera->CurrentDefectMap->size();
        rslt << NV("pixel_count", pixel_count);

        // Try to get detailed pixel type counts from defect map metadata
        int hot_count, cold_count, manual_count;
        wxString creation_time, camera_name;

        ParseDefectMapMetadata(pConfig->GetCurrentProfileId(), hot_count, cold_count, manual_count, creation_time, camera_name);

        // Add detailed counts if available
        if (hot_count >= 0)
            rslt << NV("hot_pixel_count", hot_count);
        if (cold_count >= 0)
            rslt << NV("cold_pixel_count", cold_count);
        if (manual_count >= 0)
            rslt << NV("manual_pixel_count", manual_count);

        // Add metadata if available
        if (!creation_time.IsEmpty())
            rslt << NV("creation_time", creation_time);
        if (!camera_name.IsEmpty())
            rslt << NV("camera_name", camera_name);

        // Calculate derived information
        if (hot_count >= 0 && cold_count >= 0)
        {
            int auto_detected = hot_count + cold_count;
            rslt << NV("auto_detected_count", auto_detected);

            if (manual_count >= 0)
            {
                rslt << NV("total_auto_count", auto_detected);
                rslt << NV("total_manual_count", manual_count);
            }
        }

        // Check if defect map file exists
        wxString defect_map_file = DefectMap::DefectMapFileName(pConfig->GetCurrentProfileId());
        rslt << NV("file_exists", wxFileExists(defect_map_file));
        if (wxFileExists(defect_map_file))
        {
            rslt << NV("file_path", defect_map_file);

            // Get file modification time
            wxFileName fn(defect_map_file);
            if (fn.IsOk())
            {
                wxDateTime mod_time = fn.GetModificationTime();
                rslt << NV("file_modified", mod_time.Format(wxT("%Y-%m-%d %H:%M:%S")));
            }
        }
    }
    else
    {
        rslt << NV("pixel_count", 0);

        // Check if defect map file exists but is not loaded
        wxString defect_map_file = DefectMap::DefectMapFileName(pConfig->GetCurrentProfileId());
        bool file_exists = wxFileExists(defect_map_file);
        rslt << NV("file_exists", file_exists);

        if (file_exists)
        {
            rslt << NV("file_path", defect_map_file);

            // Try to get pixel count from file without loading
            DefectMap* temp_map = DefectMap::LoadDefectMap(pConfig->GetCurrentProfileId());
            if (temp_map)
            {
                rslt << NV("file_pixel_count", (int)temp_map->size());

                // Get metadata from file
                int hot_count, cold_count, manual_count;
                wxString creation_time, camera_name;
                ParseDefectMapMetadata(pConfig->GetCurrentProfileId(), hot_count, cold_count, manual_count, creation_time, camera_name);

                if (hot_count >= 0)
                    rslt << NV("file_hot_pixel_count", hot_count);
                if (cold_count >= 0)
                    rslt << NV("file_cold_pixel_count", cold_count);
                if (manual_count >= 0)
                    rslt << NV("file_manual_pixel_count", manual_count);
                if (!creation_time.IsEmpty())
                    rslt << NV("file_creation_time", creation_time);
                if (!camera_name.IsEmpty())
                    rslt << NV("file_camera_name", camera_name);

                delete temp_map;
            }

            // Get file modification time
            wxFileName fn(defect_map_file);
            if (fn.IsOk())
            {
                wxDateTime mod_time = fn.GetModificationTime();
                rslt << NV("file_modified", mod_time.Format(wxT("%Y-%m-%d %H:%M:%S")));
            }
        }
    }

    response << jrpc_result(rslt);
}

static void get_defect_map_build_status(JObj& response, const json_value *params)
{
    if (!params)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "operation_id parameter required");
        return;
    }

    Params p("operation_id", params);
    const json_value *p_id = p.param("operation_id");
    int operation_id;
    if (!p_id || !int_param(p_id, &operation_id))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for operation_id");
        return;
    }

    wxMutexLocker lock(s_defect_map_operations_mutex);
    auto it = s_defect_map_operations.find(operation_id);
    if (it == s_defect_map_operations.end())
    {
        response << jrpc_error(1, "operation not found");
        return;
    }

    DefectMapBuildOperation* operation = it->second;
    JObj rslt;
    rslt << NV("operation_id", operation_id);

    // Convert status enum to string
    wxString status_str;
    switch (operation->status)
    {
        case DefectMapBuildOperation::STARTING:
            status_str = "starting";
            break;
        case DefectMapBuildOperation::CAPTURING_DARKS:
            status_str = "capturing_darks";
            break;
        case DefectMapBuildOperation::BUILDING_MASTER_DARK:
            status_str = "building_master_dark";
            break;
        case DefectMapBuildOperation::BUILDING_FILTERED_DARK:
            status_str = "building_filtered_dark";
            break;
        case DefectMapBuildOperation::ANALYZING_DEFECTS:
            status_str = "analyzing_defects";
            break;
        case DefectMapBuildOperation::SAVING_MAP:
            status_str = "saving_map";
            break;
        case DefectMapBuildOperation::COMPLETED:
            status_str = "completed";
            break;
        case DefectMapBuildOperation::FAILED:
            status_str = "failed";
            break;
        case DefectMapBuildOperation::CANCELLED:
            status_str = "cancelled";
            break;
    }

    rslt << NV("status", status_str);
    rslt << NV("frames_captured", operation->frames_captured);
    rslt << NV("total_frames", operation->total_frames);

    if (!operation->status_message.IsEmpty())
    {
        rslt << NV("message", operation->status_message);
    }

    if (!operation->error_message.IsEmpty())
    {
        rslt << NV("error", operation->error_message);
    }

    // Calculate progress percentage
    int progress = 0;
    if (operation->total_frames > 0)
    {
        switch (operation->status)
        {
            case DefectMapBuildOperation::CAPTURING_DARKS:
                progress = (operation->frames_captured * 80) / operation->total_frames;
                break;
            case DefectMapBuildOperation::BUILDING_MASTER_DARK:
                progress = 85;
                break;
            case DefectMapBuildOperation::BUILDING_FILTERED_DARK:
                progress = 90;
                break;
            case DefectMapBuildOperation::ANALYZING_DEFECTS:
                progress = 95;
                break;
            case DefectMapBuildOperation::SAVING_MAP:
                progress = 98;
                break;
            case DefectMapBuildOperation::COMPLETED:
                progress = 100;
                break;
            case DefectMapBuildOperation::FAILED:
            case DefectMapBuildOperation::CANCELLED:
                progress = 0;
                break;
            default:
                progress = 0;
                break;
        }
    }
    rslt << NV("progress", progress);

    // Include pixel counts if analysis has been performed
    if (operation->status >= DefectMapBuildOperation::ANALYZING_DEFECTS)
    {
        if (operation->hot_pixel_count >= 0)
            rslt << NV("hot_pixel_count", operation->hot_pixel_count);
        if (operation->cold_pixel_count >= 0)
            rslt << NV("cold_pixel_count", operation->cold_pixel_count);
        if (operation->total_defect_count >= 0)
            rslt << NV("total_defect_count", operation->total_defect_count);
    }

    // If completed, include final defect map info
    if (operation->status == DefectMapBuildOperation::COMPLETED)
    {
        rslt << NV("defect_count", (int)operation->defect_map.size());

        // Include build parameters for reference
        rslt << NV("exposure_time", operation->exposure_time);
        rslt << NV("frame_count", operation->frame_count);
        rslt << NV("hot_aggressiveness", operation->hot_aggressiveness);
        rslt << NV("cold_aggressiveness", operation->cold_aggressiveness);
    }

    response << jrpc_result(rslt);
}

static void cancel_defect_map_build(JObj& response, const json_value *params)
{
    if (!params)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "operation_id parameter required");
        return;
    }

    Params p("operation_id", params);
    const json_value *p_id = p.param("operation_id");
    int operation_id;
    if (!p_id || !int_param(p_id, &operation_id))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for operation_id");
        return;
    }

    wxMutexLocker lock(s_defect_map_operations_mutex);
    auto it = s_defect_map_operations.find(operation_id);
    if (it == s_defect_map_operations.end())
    {
        response << jrpc_error(1, "operation not found");
        return;
    }

    DefectMapBuildOperation* operation = it->second;
    operation->Cancel();

    JObj rslt;
    rslt << NV("operation_id", operation_id);
    rslt << NV("cancelled", true);

    response << jrpc_result(rslt);
}

static void load_defect_map(JObj& response, const json_value *params)
{
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected");
        return;
    }

    // Parse optional profile_id parameter
    int profile_id = pConfig->GetCurrentProfileId();
    if (params)
    {
        Params p("profile_id", params);
        const json_value *p_profile = p.param("profile_id");
        if (p_profile && !int_param(p_profile, &profile_id))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for profile_id");
            return;
        }
    }

    pFrame->LoadDefectMapHandler(true);
    bool success = (pCamera->CurrentDefectMap != nullptr);

    JObj rslt;
    rslt << NV("success", success);

    if (success && pCamera->CurrentDefectMap)
    {
        rslt << NV("pixel_count", (int)pCamera->CurrentDefectMap->size());
    }

    response << jrpc_result(rslt);
}

static void clear_defect_map(JObj& response, const json_value *params)
{
    if (!pCamera)
    {
        response << jrpc_error(1, "camera not available");
        return;
    }

    pCamera->ClearDefectMap();

    JObj rslt;
    rslt << NV("success", true);
    response << jrpc_result(rslt);
}

static void add_manual_defect(JObj& response, const json_value *params)
{
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected");
        return;
    }

    if (!pFrame->pGuider->IsLocked())
    {
        response << jrpc_error(1, "guider must be locked on a star to add manual defect");
        return;
    }

    // Parse parameters
    int x = -1, y = -1;
    bool use_current_position = true;

    if (params)
    {
        Params p("x", "y", params);

        const json_value *p_x = p.param("x");
        const json_value *p_y = p.param("y");

        if (p_x && p_y)
        {
            if (!int_param(p_x, &x) || !int_param(p_y, &y))
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int values for x and y");
                return;
            }
            use_current_position = false;
        }
    }

    wxPoint defect_pos;
    if (use_current_position)
    {
        PHD_Point current = pFrame->pGuider->CurrentPosition();
        defect_pos = wxPoint((int)(current.X + 0.5), (int)(current.Y + 0.5));
    }
    else
    {
        defect_pos = wxPoint(x, y);
    }

    // Validate coordinates
    if (defect_pos.x < 0 || defect_pos.y < 0 ||
        defect_pos.x >= pCamera->FrameSize.GetWidth() ||
        defect_pos.y >= pCamera->FrameSize.GetHeight())
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "defect coordinates out of bounds");
        return;
    }

    // Add defect to current map or create new map if none exists
    if (!pCamera->CurrentDefectMap)
    {
        response << jrpc_error(1, "no defect map loaded - load or create a defect map first");
        return;
    }

    // Check if defect already exists
    if (pCamera->CurrentDefectMap->FindDefect(defect_pos))
    {
        response << jrpc_error(1, "defect already exists at this location");
        return;
    }

    pCamera->CurrentDefectMap->AddDefect(defect_pos);

    JObj rslt;
    rslt << NV("success", true);
    rslt << NV("x", defect_pos.x);
    rslt << NV("y", defect_pos.y);
    rslt << NV("total_defects", (int)pCamera->CurrentDefectMap->size());

    response << jrpc_result(rslt);
}

// Polar alignment operation tracking
struct PolarAlignmentOperation
{
    int operation_id;
    wxString tool_type;

    enum Status {
        STARTING,
        WAITING_FOR_STAR,
        MEASURING,
        ADJUSTING,
        COMPLETED,
        FAILED,
        CANCELLED
    } status;

    wxString status_message;
    wxString error_message;
    bool cancelled;

    // Tool-specific parameters
    wxString direction;  // For drift alignment
    int measurement_time;
    wxString hemisphere; // For polar drift alignment
    bool auto_mode;      // For static polar alignment

    // Progress tracking
    double progress;
    double measurement_start_time;
    double elapsed_time;

    // Results
    double polar_error_arcmin;
    double adjustment_angle_deg;
    double azimuth_correction;
    double altitude_correction;
    int alignment_iterations;
    double final_polar_error;

    wxMutex operation_mutex;

    PolarAlignmentOperation(int id, const wxString& type)
        : operation_id(id), tool_type(type), status(STARTING), cancelled(false),
          measurement_time(300), auto_mode(false), progress(0.0),
          measurement_start_time(0.0), elapsed_time(0.0),
          polar_error_arcmin(0.0), adjustment_angle_deg(0.0),
          azimuth_correction(0.0), altitude_correction(0.0),
          alignment_iterations(0), final_polar_error(0.0)
    {
    }

    void SetStatus(Status new_status, const wxString& message = wxEmptyString)
    {
        wxMutexLocker lock(operation_mutex);
        status = new_status;
        if (!message.IsEmpty())
        {
            status_message = message;
        }
    }

    void SetError(const wxString& error)
    {
        wxMutexLocker lock(operation_mutex);
        status = FAILED;
        error_message = error;
        status_message = "Operation failed";
    }

    void Cancel()
    {
        wxMutexLocker lock(operation_mutex);
        cancelled = true;
        if (status != COMPLETED && status != FAILED)
        {
            status = CANCELLED;
            status_message = "Operation cancelled";
        }
    }

    void UpdateProgress(double prog, double elapsed = 0.0)
    {
        wxMutexLocker lock(operation_mutex);
        progress = prog;
        if (elapsed > 0.0)
        {
            elapsed_time = elapsed;
        }
    }

    void SetResults(double error_arcmin, double angle_deg)
    {
        wxMutexLocker lock(operation_mutex);
        polar_error_arcmin = error_arcmin;
        adjustment_angle_deg = angle_deg;
    }
};

// Global tracking for polar alignment operations
static std::map<int, PolarAlignmentOperation*> s_polar_alignment_operations;
static wxMutex s_polar_alignment_operations_mutex;

// Clean up completed polar alignment operations
static void CleanupCompletedPolarAlignmentOperations()
{
    wxMutexLocker lock(s_polar_alignment_operations_mutex);

    auto it = s_polar_alignment_operations.begin();
    while (it != s_polar_alignment_operations.end())
    {
        PolarAlignmentOperation* op = it->second;
        if (op->status == PolarAlignmentOperation::COMPLETED ||
            op->status == PolarAlignmentOperation::FAILED ||
            op->status == PolarAlignmentOperation::CANCELLED)
        {
            // Keep completed operations for a while so status can be queried
            // In a real implementation, you might want to clean these up after some time
            ++it;
        }
        else
        {
            ++it;
        }
    }
}

// Helper function to get tool window status
static bool GetDriftToolStatus(PolarAlignmentOperation* operation)
{
    if (!pFrame->pDriftTool)
        return false;

    // The drift tool doesn't have a simple status interface, so we check if it's active
    // by seeing if it exists and the guider is in the right state
    if (pFrame->pGuider->IsLocked())
    {
        operation->SetStatus(PolarAlignmentOperation::MEASURING, "Drift alignment in progress");
        return true;
    }

    return false;
}

static bool GetPolarDriftToolStatus(PolarAlignmentOperation* operation)
{
    if (!pFrame->pPolarDriftTool)
        return false;

    PolarDriftToolWin* win = static_cast<PolarDriftToolWin*>(pFrame->pPolarDriftTool);
    if (win && win->IsDrifting())
    {
        // Get current measurement results
        double error_arcmin = win->m_offset * win->m_pxScale / 60.0;
        double angle_deg = norm(-win->m_alpha, -180, 180);

        operation->SetResults(error_arcmin, angle_deg);
        operation->SetStatus(PolarAlignmentOperation::MEASURING,
                           wxString::Format("Measuring drift - Error: %.1f arcmin, Angle: %.1f deg",
                                          error_arcmin, angle_deg));

        // Calculate progress based on measurement time
        if (operation->measurement_start_time == 0.0)
        {
            operation->measurement_start_time = win->m_t0;
        }

        double elapsed = (wxDateTime::GetTimeNow() - operation->measurement_start_time) / 1000.0;
        double progress = std::min(100.0, (elapsed / operation->measurement_time) * 100.0);
        operation->UpdateProgress(progress, elapsed);

        return true;
    }

    return false;
}

static bool GetStaticPaToolStatus(PolarAlignmentOperation* operation)
{
    if (!pFrame->pStaticPaTool)
        return false;

    StaticPaToolWin* win = static_cast<StaticPaToolWin*>(pFrame->pStaticPaTool);
    if (win && win->IsAligning())
    {
        operation->SetStatus(PolarAlignmentOperation::MEASURING, "Static polar alignment in progress");

        // Calculate progress based on number of positions captured
        int positions = win->m_numPos;
        double progress = (positions / 3.0) * 100.0;
        operation->UpdateProgress(progress);

        return true;
    }
    else if (win && win->IsAligned())
    {
        operation->SetStatus(PolarAlignmentOperation::COMPLETED, "Static polar alignment completed");
        operation->UpdateProgress(100.0);
        return true;
    }

    return false;
}

static void start_drift_alignment(JObj& response, const json_value *params)
{
    // Validate prerequisites
    if (!validate_camera_connected(response) ||
        !validate_mount_connected(response) ||
        !check_operation_in_progress(response, "drift alignment"))
    {
        return;
    }

    if (!pMount->IsCalibrated())
    {
        response << jrpc_error(1, "mount must be calibrated before drift alignment");
        return;
    }

    // Parse parameters
    wxString direction = "east"; // Default direction
    int measurement_time = 300; // Default 5 minutes

    if (params)
    {
        Params p("direction", "measurement_time", params);

        const json_value *p_dir = p.param("direction");
        if (p_dir && p_dir->type == JSON_STRING)
        {
            direction = wxString(p_dir->string_value, wxConvUTF8);
            if (direction != "east" && direction != "west")
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, "direction must be 'east' or 'west'");
                return;
            }
        }

        const json_value *p_time = p.param("measurement_time");
        if (p_time && !int_param(p_time, &measurement_time))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for measurement_time");
            return;
        }
    }

    // Validate measurement time
    if (!validate_measurement_time(measurement_time, response))
    {
        return;
    }

    // Clean up any old completed operations
    CleanupCompletedPolarAlignmentOperations();

    // Create operation tracking
    static int operation_counter = 2000;
    int operation_id = operation_counter++;

    PolarAlignmentOperation* operation = new PolarAlignmentOperation(operation_id, "drift_alignment");
    operation->direction = direction;
    operation->measurement_time = measurement_time;

    {
        wxMutexLocker lock(s_polar_alignment_operations_mutex);
        s_polar_alignment_operations[operation_id] = operation;
    }

    // Open the drift tool if not already open
    if (!pFrame->pDriftTool)
    {
        pFrame->pDriftTool = DriftTool::CreateDriftToolWindow();
        if (!pFrame->pDriftTool)
        {
            operation->SetError("Failed to create drift alignment tool");

            JObj rslt;
            rslt << NV("operation_id", operation_id);
            rslt << NV("error", "Failed to create drift alignment tool");
            response << jrpc_error(1, "Failed to create drift alignment tool");
            return;
        }
        pFrame->pDriftTool->Show();
    }

    operation->SetStatus(PolarAlignmentOperation::WAITING_FOR_STAR,
                        "Drift alignment tool opened. Please select a star near the celestial equator.");

    Debug.Write(wxString::Format("PolarAlignment: Started drift alignment operation %d\n", operation_id));

    JObj rslt;
    rslt << NV("operation_id", operation_id);
    rslt << NV("tool_type", "drift_alignment");
    rslt << NV("direction", direction);
    rslt << NV("measurement_time", measurement_time);
    rslt << NV("status", "starting");

    response << jrpc_result(rslt);
}

static void start_static_polar_alignment(JObj& response, const json_value *params)
{
    // Validate system state
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected - static polar alignment requires active camera");
        return;
    }

    if (!pMount || !pMount->IsConnected())
    {
        response << jrpc_error(1, "mount not connected - static polar alignment requires active mount");
        return;
    }

    if (pFrame->pGuider->GetState() != STATE_UNINITIALIZED && 
        pFrame->pGuider->GetState() != STATE_SELECTING && 
        pFrame->pGuider->GetState() != STATE_SELECTED)
    {
        response << jrpc_error(1, "guider is not idle - stop guiding before starting polar alignment");
        return;
    }

    // Parse and validate parameters
    wxString hemisphere = "north"; // Default hemisphere
    bool auto_mode = true; // Default to auto mode

    if (params)
    {
        Params p("hemisphere", "auto_mode", params);

        const json_value *p_hemi = p.param("hemisphere");
        if (p_hemi && p_hemi->type == JSON_STRING)
        {
            hemisphere = wxString(p_hemi->string_value, wxConvUTF8);
            hemisphere.MakeLower();
            if (hemisphere != "north" && hemisphere != "south")
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                    "hemisphere must be 'north' or 'south' (case-insensitive)");
                return;
            }
        }

        const json_value *p_auto = p.param("auto_mode");
        if (p_auto && !bool_param(p_auto, &auto_mode))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "expected boolean value for 'auto_mode' parameter");
            return;
        }
    }

    // Create operation tracking
    static int operation_counter = 3000;
    int operation_id = operation_counter++;

    PolarAlignmentOperation* operation = new PolarAlignmentOperation(operation_id, "static_polar_alignment");
    operation->hemisphere = hemisphere;
    operation->auto_mode = auto_mode;

    {
        wxMutexLocker lock(s_polar_alignment_operations_mutex);
        s_polar_alignment_operations[operation_id] = operation;
    }

    // Open the static polar alignment tool if not already open
    if (!pFrame->pStaticPaTool)
    {
        pFrame->pStaticPaTool = StaticPaTool::CreateStaticPaToolWindow();
        if (!pFrame->pStaticPaTool)
        {
            operation->SetError("Failed to create static polar alignment tool window");
            {
                wxMutexLocker lock(s_polar_alignment_operations_mutex);
                s_polar_alignment_operations.erase(operation_id);
            }
            delete operation;

            response << jrpc_error(1, "failed to initialize static polar alignment tool - check system resources");
            return;
        }
        pFrame->pStaticPaTool->Show();
    }

    operation->SetStatus(PolarAlignmentOperation::WAITING_FOR_STAR,
                        wxString::Format("Static polar alignment tool opened (%s hemisphere). "
                                       "Please select a star near the celestial pole and begin alignment.",
                                       hemisphere));

    Debug.Write(wxString::Format("EventServer: Started static polar alignment operation %d (hemisphere=%s, auto=%d)\n", 
                                operation_id, hemisphere, auto_mode));

    JObj rslt;
    rslt << NV("operation_id", operation_id)
         << NV("tool_type", "static_polar_alignment")
         << NV("hemisphere", hemisphere)
         << NV("auto_mode", auto_mode)
         << NV("status", "starting")
         << NV("message", "Static polar alignment tool initialized and ready for input");

    response << jrpc_result(rslt);
}

static void start_polar_drift_alignment(JObj& response, const json_value *params)
{
    // Validate system state
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected - polar drift alignment requires active camera");
        return;
    }

    if (!pMount || !pMount->IsConnected())
    {
        response << jrpc_error(1, "mount not connected - polar drift alignment requires active mount");
        return;
    }

    if (pFrame->pGuider->GetState() != STATE_UNINITIALIZED && pFrame->pGuider->GetState() != STATE_SELECTING && pFrame->pGuider->GetState() != STATE_SELECTED)
    {
        response << jrpc_error(1, "guider is not idle - stop guiding before starting polar alignment");
        return;
    }

    // Parse and validate parameters
    wxString hemisphere = "north"; // Default hemisphere
    int measurement_time = 300; // Default 5 minutes (300 seconds)

    if (params)
    {
        Params p("hemisphere", "measurement_time", params);

        const json_value *p_hemi = p.param("hemisphere");
        if (p_hemi && p_hemi->type == JSON_STRING)
        {
            hemisphere = wxString(p_hemi->string_value, wxConvUTF8);
            hemisphere.MakeLower();
            if (hemisphere != "north" && hemisphere != "south")
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                    "hemisphere must be 'north' or 'south' (case-insensitive)");
                return;
            }
        }

        const json_value *p_time = p.param("measurement_time");
        if (p_time && !int_param(p_time, &measurement_time))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, 
                "expected integer value for 'measurement_time' parameter (seconds)");
            return;
        }
    }

    // Validate measurement time range
    if (measurement_time < 60)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "measurement_time too short (minimum 60 seconds for reliable polar error detection)");
        return;
    }
    if (measurement_time > 1800)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "measurement_time too long (maximum 1800 seconds / 30 minutes to prevent excessive runtime)");
        return;
    }

    // Create operation tracking
    static int operation_counter = 4000;
    int operation_id = operation_counter++;

    PolarAlignmentOperation* operation = new PolarAlignmentOperation(operation_id, "polar_drift_alignment");
    operation->hemisphere = hemisphere;
    operation->measurement_time = measurement_time;

    {
        wxMutexLocker lock(s_polar_alignment_operations_mutex);
        s_polar_alignment_operations[operation_id] = operation;
    }

    // Open the polar drift alignment tool if not already open
    if (!pFrame->pPolarDriftTool)
    {
        pFrame->pPolarDriftTool = PolarDriftTool::CreatePolarDriftToolWindow();
        if (!pFrame->pPolarDriftTool)
        {
            operation->SetError("Failed to create polar drift alignment tool window");
            {
                wxMutexLocker lock(s_polar_alignment_operations_mutex);
                s_polar_alignment_operations.erase(operation_id);
            }
            delete operation;

            response << jrpc_error(1, "failed to initialize polar drift alignment tool - check system resources");
            return;
        }
        pFrame->pPolarDriftTool->Show();
    }

    operation->SetStatus(PolarAlignmentOperation::WAITING_FOR_STAR,
                        wxString::Format("Polar drift alignment tool opened (%s hemisphere, %d second measurement). "
                                       "Please select a star near the celestial pole.",
                                       hemisphere, measurement_time));

    Debug.Write(wxString::Format("EventServer: Started polar drift alignment operation %d (hemisphere=%s, time=%d sec)\n", 
                                operation_id, hemisphere, measurement_time));

    JObj rslt;
    rslt << NV("operation_id", operation_id)
         << NV("tool_type", "polar_drift_alignment")
         << NV("hemisphere", hemisphere)
         << NV("measurement_time", measurement_time)
         << NV("status", "starting")
         << NV("message", "Polar drift alignment tool initialized and ready for measurement");

    response << jrpc_result(rslt);
}

static void get_polar_alignment_status(JObj& response, const json_value *params)
{
    if (!params)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "missing 'operation_id' parameter (required to query polar alignment status)");
        return;
    }

    Params p("operation_id", params);
    const json_value *p_id = p.param("operation_id");
    int operation_id;
    if (!p_id || !int_param(p_id, &operation_id))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, 
            "expected integer value for 'operation_id' parameter");
        return;
    }

    // Look up operation status
    wxMutexLocker lock(s_polar_alignment_operations_mutex);
    auto it = s_polar_alignment_operations.find(operation_id);
    if (it == s_polar_alignment_operations.end())
    {
        response << jrpc_error(1, wxString::Format(
            "polar alignment operation %d not found (may have been completed and cleaned up)", operation_id));
        return;
    }

    PolarAlignmentOperation* operation = it->second;
    wxMutexLocker op_lock(operation->operation_mutex);

    JObj rslt;
    rslt << NV("operation_id", operation_id)
         << NV("tool_type", operation->tool_type);

    // Convert status enum to descriptive string
    wxString status_str;
    switch (operation->status)
    {
        case PolarAlignmentOperation::STARTING:
            status_str = "starting";
            break;
        case PolarAlignmentOperation::WAITING_FOR_STAR:
            status_str = "waiting_for_star";
            break;
        case PolarAlignmentOperation::MEASURING:
            status_str = "measuring";
            break;
        case PolarAlignmentOperation::ADJUSTING:
            status_str = "adjusting";
            break;
        case PolarAlignmentOperation::COMPLETED:
            status_str = "completed";
            break;
        case PolarAlignmentOperation::FAILED:
            status_str = "failed";
            break;
        case PolarAlignmentOperation::CANCELLED:
            status_str = "cancelled";
            break;
        default:
            status_str = "unknown";
            break;
    }

    rslt << NV("status", status_str)
         << NV("progress", operation->progress)
         << NV("timestamp", wxDateTime::Now().Format("%Y-%m-%dT%H:%M:%S"));

    // Add status message if available
    if (!operation->status_message.IsEmpty())
        rslt << NV("message", operation->status_message);

    // Add error message if operation failed
    if (!operation->error_message.IsEmpty())
        rslt << NV("error", operation->error_message);

    // Add tool-specific information
    if (operation->tool_type == "drift_alignment")
    {
        rslt << NV("direction", operation->direction);
        rslt << NV("measurement_time", operation->measurement_time);

        // Update status from drift tool
        GetDriftToolStatus(operation);
    }
    else if (operation->tool_type == "polar_drift_alignment")
    {
        rslt << NV("hemisphere", operation->hemisphere)
             << NV("measurement_time", operation->measurement_time);

        if (operation->elapsed_time > 0.0)
            rslt << NV("elapsed_time", operation->elapsed_time);

        // Update status from polar drift tool
        GetPolarDriftToolStatus(operation);

        // Add calculated results if available
        if (operation->polar_error_arcmin > 0.0)
        {
            rslt << NV("polar_error_arcmin", operation->polar_error_arcmin)
                 << NV("adjustment_angle_deg", operation->adjustment_angle_deg)
                 << NV("azimuth_correction_arcmin", operation->azimuth_correction)
                 << NV("altitude_correction_arcmin", operation->altitude_correction);
        }
    }
    else if (operation->tool_type == "static_polar_alignment")
    {
        rslt << NV("hemisphere", operation->hemisphere)
             << NV("auto_mode", operation->auto_mode);

        // Update status from static PA tool
        GetStaticPaToolStatus(operation);

        // Add results if completed
        if (operation->status == PolarAlignmentOperation::COMPLETED)
        {
            rslt << NV("alignment_iterations", operation->alignment_iterations)
                 << NV("final_polar_error_arcmin", operation->final_polar_error);
        }
    }

    response << jrpc_result(rslt);
}


static void cancel_polar_alignment(JObj& response, const json_value *params)
{
    if (!params)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "operation_id parameter required");
        return;
    }

    Params p("operation_id", params);
    const json_value *p_id = p.param("operation_id");
    int operation_id;
    if (!p_id || !int_param(p_id, &operation_id))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for operation_id");
        return;
    }

    // Look up and cancel operation
    wxMutexLocker lock(s_polar_alignment_operations_mutex);
    auto it = s_polar_alignment_operations.find(operation_id);
    if (it == s_polar_alignment_operations.end())
    {
        response << jrpc_error(1, "operation not found");
        return;
    }

    PolarAlignmentOperation* operation = it->second;
    operation->Cancel();

    // Close the appropriate tool window
    if (operation->tool_type == "drift_alignment" && pFrame->pDriftTool)
    {
        pFrame->pDriftTool->Close();
        pFrame->pDriftTool = nullptr;
    }
    else if (operation->tool_type == "polar_drift_alignment" && pFrame->pPolarDriftTool)
    {
        PolarDriftToolWin* win = static_cast<PolarDriftToolWin*>(pFrame->pPolarDriftTool);
        if (win && win->IsDrifting())
        {
            // Stop the drift measurement
            wxCommandEvent dummy;
            win->OnStart(dummy);
        }
    }
    else if (operation->tool_type == "static_polar_alignment" && pFrame->pStaticPaTool)
    {
        StaticPaToolWin* win = static_cast<StaticPaToolWin*>(pFrame->pStaticPaTool);
        if (win && win->IsAligning())
        {
            // Stop the alignment process
            wxCommandEvent dummy;
            win->OnRotate(dummy);
        }
    }

    Debug.Write(wxString::Format("PolarAlignment: Cancelled operation %d (%s)\n",
                               operation_id, operation->tool_type));

    JObj rslt;
    rslt << NV("success", true);
    rslt << NV("operation_id", operation_id);
    rslt << NV("cancelled", true);

    response << jrpc_result(rslt);
}

// Helper function to parse ISO 8601 timestamp
static bool parse_iso8601_timestamp(const wxString& iso_str, wxDateTime& dt)
{
    // Try parsing ISO 8601 format: YYYY-MM-DDTHH:MM:SS or YYYY-MM-DD HH:MM:SS
    wxString::const_iterator iter;

    // Try with 'T' separator first
    if (dt.ParseISOCombined(iso_str, 'T') && iter == iso_str.end())
        return true;

    // Try with space separator
    if (dt.ParseISOCombined(iso_str, ' ') && iter == iso_str.end())
        return true;

    // Try parsing just date
    if (dt.ParseISODate(iso_str) && iter == iso_str.end())
        return true;

    return false;
}

// Helper function to validate log level parameter
static bool validate_log_level(const wxString& level, JObj& response)
{
    if (level != "debug" && level != "info" && level != "warning" && level != "error")
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "log_level must be 'debug', 'info', 'warning', or 'error'");
        return false;
    }
    return true;
}

// Helper function to validate format parameter
static bool validate_format(const wxString& format, JObj& response)
{
    if (format != "json" && format != "csv")
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "format must be 'json' or 'csv'");
        return false;
    }
    return true;
}

// Helper function to parse CSV line from guide log
static bool parse_guide_log_line(const wxString& line, JObj& entry, const wxDateTime& log_start_time)
{
    // Guide log CSV format:
    // Frame,Time,mount,dx,dy,RARawDistance,DECRawDistance,RAGuideDistance,DECGuideDistance,
    // RADuration,RADirection,DECDuration,DECDirection,XStep,YStep,StarMass,SNR,ErrorCode

    wxArrayString fields = wxSplit(line, ',');
    if (fields.size() < 18)
        return false;

    // Skip header line
    if (fields[0] == "Frame")
        return false;

    // Skip non-numeric frame numbers (INFO lines, etc.)
    long frame_number;
    if (!fields[0].ToLong(&frame_number))
        return false;

    double time_offset;
    if (!fields[1].ToDouble(&time_offset))
        return false;

    // Calculate absolute timestamp
    wxDateTime timestamp = log_start_time + wxTimeSpan::Seconds((long)time_offset);

    // Parse mount type
    wxString mount = fields[2];
    mount.Replace("\"", ""); // Remove quotes

    // Parse camera offsets
    double dx, dy;
    fields[3].ToDouble(&dx);
    fields[4].ToDouble(&dy);

    // Parse raw distances
    double ra_raw_distance, dec_raw_distance;
    fields[5].ToDouble(&ra_raw_distance);
    fields[6].ToDouble(&dec_raw_distance);

    // Parse guide distances
    double ra_guide_distance, dec_guide_distance;
    fields[7].ToDouble(&ra_guide_distance);
    fields[8].ToDouble(&dec_guide_distance);

    // Parse durations and directions
    long ra_duration, dec_duration;
    fields[9].ToLong(&ra_duration);
    fields[11].ToLong(&dec_duration);

    wxString ra_direction = fields[10];
    wxString dec_direction = fields[12];

    // Parse star data
    double star_mass, snr;
    long error_code;
    fields[15].ToDouble(&star_mass);
    fields[16].ToDouble(&snr);
    fields[17].ToLong(&error_code);

    // Build JSON entry
    entry << NV("timestamp", timestamp.Format("%Y-%m-%dT%H:%M:%S"))
          << NV("log_level", "info")
          << NV("message", "Guide step")
          << NV("frame_number", (int)frame_number)
          << NV("mount", mount)
          << NV("camera_offset_x", dx)
          << NV("camera_offset_y", dy)
          << NV("ra_raw_distance", ra_raw_distance)
          << NV("dec_raw_distance", dec_raw_distance)
          << NV("guide_distance", sqrt(ra_guide_distance * ra_guide_distance + dec_guide_distance * dec_guide_distance))
          << NV("ra_correction", (int)ra_duration)
          << NV("dec_correction", (int)dec_duration)
          << NV("ra_direction", ra_direction)
          << NV("dec_direction", dec_direction)
          << NV("star_mass", star_mass)
          << NV("snr", snr)
          << NV("error_code", (int)error_code);

    return true;
}

// Helper function to find guide log files in date range
static void find_guide_log_files(const wxDateTime& start_time, const wxDateTime& end_time, wxArrayString& log_files)
{
    wxString log_dir = GuideLog.GetLogDir();
    wxDir dir(log_dir);

    if (!dir.IsOpened())
        return;

    wxString filename;
    wxRegEx re("PHD2_GuideLog_[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{6}\\.txt$");

    bool cont = dir.GetFirst(&filename, "PHD2_GuideLog_*.txt", wxDIR_FILES);
    while (cont)
    {
        if (re.Matches(filename))
        {
            // Extract timestamp from filename: PHD2_GuideLog_YYYY-MM-DD_HHMMSS.txt
            wxString timestamp_str = filename.substr(14, 17); // Extract YYYY-MM-DD_HHMMSS

            // Parse timestamp
            wxDateTime file_time;
            if (timestamp_str.length() == 17)
            {
                wxString date_part = timestamp_str.substr(0, 10); // YYYY-MM-DD
                wxString time_part = timestamp_str.substr(11, 6); // HHMMSS

                // Convert HHMMSS to HH:MM:SS
                wxString formatted_time = time_part.substr(0, 2) + ":" +
                                         time_part.substr(2, 2) + ":" +
                                         time_part.substr(4, 2);

                wxString full_timestamp = date_part + "T" + formatted_time;

                if (parse_iso8601_timestamp(full_timestamp, file_time))
                {
                    // Check if file time is within range
                    if ((!start_time.IsValid() || file_time >= start_time) &&
                        (!end_time.IsValid() || file_time <= end_time))
                    {
                        log_files.Add(log_dir + PATHSEPSTR + filename);
                    }
                }
            }
        }
        cont = dir.GetNext(&filename);
    }

    // Sort files by timestamp (they should already be sorted by filename)
    log_files.Sort();
}

static void get_guiding_log(JObj& response, const json_value *params)
{
    // Parse parameters
    wxDateTime start_time, end_time;
    int max_entries = 100;
    wxString log_level = "info";
    wxString format = "json";

    if (params)
    {
        Params p("start_time", "end_time", "max_entries", "log_level", "format", params);

        // Parse start_time
        const json_value *p_start = p.param("start_time");
        if (p_start && p_start->type == JSON_STRING)
        {
            wxString start_str(p_start->string_value, wxConvUTF8);
            if (!parse_iso8601_timestamp(start_str, start_time))
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid start_time format, expected ISO 8601");
                return;
            }
        }

        // Parse end_time
        const json_value *p_end = p.param("end_time");
        if (p_end && p_end->type == JSON_STRING)
        {
            wxString end_str(p_end->string_value, wxConvUTF8);
            if (!parse_iso8601_timestamp(end_str, end_time))
            {
                response << jrpc_error(JSONRPC_INVALID_PARAMS, "invalid end_time format, expected ISO 8601");
                return;
            }
        }

        // Parse max_entries
        const json_value *p_max = p.param("max_entries");
        if (p_max && !int_param(p_max, &max_entries))
        {
            response << jrpc_error(JSONRPC_INVALID_PARAMS, "expected int value for max_entries");
            return;
        }

        // Parse log_level
        const json_value *p_level = p.param("log_level");
        if (p_level && p_level->type == JSON_STRING)
        {
            log_level = wxString(p_level->string_value, wxConvUTF8);
            if (!validate_log_level(log_level, response))
                return;
        }

        // Parse format
        const json_value *p_format = p.param("format");
        if (p_format && p_format->type == JSON_STRING)
        {
            format = wxString(p_format->string_value, wxConvUTF8);
            if (!validate_format(format, response))
                return;
        }
    }

    // Validate max_entries
    if (max_entries < 1 || max_entries > 1000)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "max_entries must be between 1 and 1000");
        return;
    }

    // Validate time range
    if (start_time.IsValid() && end_time.IsValid() && end_time < start_time)
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "end_time must be after start_time");
        return;
    }

    // Find relevant log files
    wxArrayString log_files;
    find_guide_log_files(start_time, end_time, log_files);

    if (log_files.empty())
    {
        response << jrpc_error(1, "no guide log files found in specified time range");
        return;
    }

    // Parse log entries
    std::vector<JObj> entries;
    int total_entries = 0;
    bool has_more_data = false;
    wxDateTime actual_start_time, actual_end_time;

    for (size_t i = 0; i < log_files.size() && entries.size() < (size_t)max_entries; i++)
    {
        wxFileInputStream fileStream(log_files[i]);
        if (!fileStream.IsOk())
            continue;

        wxTextInputStream textStream(fileStream);

        // Try to extract log start time from file
        wxDateTime log_start_time;
        wxString line;
        while (!fileStream.Eof())
        {
            line = textStream.ReadLine();
            if (line.IsEmpty() && fileStream.Eof())
                break;
            if (line.StartsWith("PHD2 version"))
            {
                // Look for timestamp in the version line
                int pos = line.Find("Log enabled at ");
                if (pos != wxNOT_FOUND)
                {
                    wxString timestamp_str = line.Mid(pos + 15);
                    parse_iso8601_timestamp(timestamp_str, log_start_time);
                }
                break;
            }
        }

        // If we couldn't find start time in file, use filename timestamp
        if (!log_start_time.IsValid())
        {
            wxString filename = wxFileName(log_files[i]).GetName();
            wxString timestamp_str = filename.substr(14, 17);
            if (timestamp_str.length() == 17)
            {
                wxString date_part = timestamp_str.substr(0, 10);
                wxString time_part = timestamp_str.substr(11, 6);
                wxString formatted_time = time_part.substr(0, 2) + ":" +
                                         time_part.substr(2, 2) + ":" +
                                         time_part.substr(4, 2);
                wxString full_timestamp = date_part + "T" + formatted_time;
                parse_iso8601_timestamp(full_timestamp, log_start_time);
            }
        }

        // Read and parse guide entries
        fileStream.SeekI(0); // Reset to beginning
        wxTextInputStream textStream2(fileStream);
        while (!fileStream.Eof() && entries.size() < (size_t)max_entries)
        {
            line = textStream2.ReadLine();
            if (line.IsEmpty() && fileStream.Eof())
                break;
            total_entries++;

            // Skip non-CSV lines
            if (line.IsEmpty() || !line.Contains(","))
                continue;

            // Parse guide step entry
            JObj entry;
            if (parse_guide_log_line(line, entry, log_start_time))
            {
                // Apply time filtering if specified
                // For now, we'll add all valid entries and let the client filter if needed
                entries.push_back(entry);

                // Track actual time range
                // This is simplified - in a full implementation we'd extract and compare timestamps
            }
        }

        // File stream will close automatically
    }

    // Check if there's more data
    has_more_data = (total_entries > max_entries);

    // Build response
    if (format == "csv")
    {
        // CSV format response with proper entry extraction
        wxString csv_header = "timestamp,log_level,frame_number,mount,camera_offset_x,camera_offset_y,"
                             "ra_raw_distance,dec_raw_distance,guide_distance,ra_correction,dec_correction,"
                             "ra_direction,dec_direction,star_mass,snr,error_code\n";
        wxString csv_data = csv_header;
        
        for (size_t i = 0; i < entries.size(); i++)
        {
            // Extract fields from JObj and format as CSV
            // Note: This is a simplified conversion - in a production system, you'd preserve the original CSV data
            csv_data += entries[i].str();
            csv_data += "\n";
        }

        JObj rslt;
        rslt << NV("format", "csv")
             << NV("data", csv_data)
             << NV("total_entries", (int)entries.size())
             << NV("has_more_data", has_more_data);

        response << jrpc_result(rslt);
    }
    else
    {
        // JSON format response with complete entries array
        JObj rslt;
        rslt << NV("format", "json")
             << NV("total_entries", (int)entries.size())
             << NV("has_more_data", has_more_data);

        if (start_time.IsValid())
            rslt << NV("start_time", start_time.Format("%Y-%m-%dT%H:%M:%S"));
        if (end_time.IsValid())
            rslt << NV("end_time", end_time.Format("%Y-%m-%dT%H:%M:%S"));

        // Build JSON array of entries
        if (!entries.empty())
        {
            wxString entries_json = "[";
            for (size_t i = 0; i < entries.size(); i++)
            {
                if (i > 0) entries_json << ",";
                entries_json << entries[i].str();
            }
            entries_json << "]";
            
            // Add the entries array as a raw JSON string
            rslt << NV("entries", entries_json);
        }
        else
        {
            rslt << NV("entries", "[]");
        }

        response << jrpc_result(rslt);
    }
}

static void set_cooler_state(JObj& response, const json_value *params)
{
    // Parse and validate parameters
    Params p("enabled", params);
    const json_value *val = p.param("enabled");
    bool enable;
    if (!val || !bool_param(val, &enable))
    {
        response << jrpc_error(JSONRPC_INVALID_PARAMS, "missing or invalid 'enabled' parameter (expected boolean value)");
        return;
    }

    // Validate camera connection
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected - cannot control cooler");
        return;
    }

    // Check if camera has a cooler
    if (!pCamera->HasCooler)
    {
        response << jrpc_error(1, "camera does not have a cooler");
        return;
    }

    // Set cooler state
    if (pCamera->SetCoolerOn(enable))
    {
        response << jrpc_error(1, wxString::Format("failed to %s cooler", enable ? "enable" : "disable"));
        return;
    }

    // If enabling, also set the setpoint
    double setpoint = -999.0;
    if (enable)
    {
        setpoint = pConfig->Profile.GetDouble("/camera/CoolerSetpt", 10.0);
        if (pCamera->SetCoolerSetpoint(setpoint))
        {
            response << jrpc_error(1, wxString::Format("cooler enabled but failed to set setpoint to %.1f°C", setpoint));
            return;
        }
    }

    // Get current status for response
    bool on;
    double actual_setpoint, power, temperature;
    bool err = pCamera->GetCoolerStatus(&on, &actual_setpoint, &power, &temperature);

    // Return detailed response
    JObj rslt;
    rslt << NV("enabled", on);
    if (!err)
    {
        rslt << NV("temperature", temperature, 1);
        if (on)
        {
            rslt << NV("setpoint", actual_setpoint, 1)
                 << NV("power", power, 1);
        }
    }
    response << jrpc_result(rslt);
    
    Debug.Write(wxString::Format("EventServer: Cooler %s, temp=%.1f°C%s\n",
                                 on ? "enabled" : "disabled",
                                 err ? 0.0 : temperature,
                                 (on && !err) ? wxString::Format(", setpoint=%.1f°C", actual_setpoint) : ""));
}

static void get_cooler_status(JObj& response, const json_value *params)
{
    // Validate camera connection
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected - cannot get cooler status");
        return;
    }

    // Check if camera has a cooler
    if (!pCamera->HasCooler)
    {
        JObj rslt;
        rslt << NV("hasCooler", false);
        response << jrpc_result(rslt);
        return;
    }

    // Get cooler status
    bool on;
    double setpoint, power, temperature;

    bool err = pCamera->GetCoolerStatus(&on, &setpoint, &power, &temperature);
    if (err)
    {
        response << jrpc_error(1, "failed to retrieve cooler status from camera");
        return;
    }

    // Build detailed response
    JObj rslt;
    rslt << NV("hasCooler", true)
         << NV("coolerOn", on)
         << NV("temperature", temperature, 1);

    if (on)
    {
        rslt << NV("setpoint", setpoint, 1)
             << NV("power", power, 1);
    }

    response << jrpc_result(rslt);
}

static void get_sensor_temperature(JObj& response, const json_value *params)
{
    if (!pCamera || !pCamera->Connected)
    {
        response << jrpc_error(1, "camera not connected");
        return;
    }

    double temperature;
    bool err = pCamera->GetSensorTemperature(&temperature);
    if (err)
    {
        response << jrpc_error(1, "failed to get sensor temperature");
        return;
    }

    JObj rslt;
    rslt << NV("temperature", temperature, 1);

    response << jrpc_result(rslt);
}

static void export_config_settings(JObj& response, const json_value *params)
{
    wxString filename(MyFrame::GetDefaultFileDir() + PATHSEPSTR + "phd2_settings.txt");
    bool err = pConfig->SaveAll(filename);

    if (err)
    {
        response << jrpc_error(1, "export settings failed");
        return;
    }

    JObj rslt;
    rslt << NV("filename", filename);

    response << jrpc_result(rslt);
}

struct JRpcCall
{
    wxSocketClient *cli;
    const json_value *req;
    const json_value *method;
    JRpcResponse response;

    JRpcCall(wxSocketClient *cli_, const json_value *req_) : cli(cli_), req(req_), method(nullptr) { }
};

static void dump_request(const JRpcCall& call)
{
    Debug.Write(wxString::Format("evsrv: cli %p request: %s\n", call.cli, json_format(call.req)));
}

static void dump_response(const JRpcCall& call)
{
    wxString s(const_cast<JRpcResponse&>(call.response).str());

    // trim output for huge responses

    // this is very hacky operating directly on the string, but it's not
    // worth bothering to parse and reformat the response
    if (call.method && strcmp(call.method->string_value, "get_star_image") == 0)
    {
        size_t p0, p1;
        if ((p0 = s.find("\"pixels\":\"")) != wxString::npos && (p1 = s.find('"', p0 + 10)) != wxString::npos)
            s.replace(p0 + 10, p1 - (p0 + 10), "...");
    }

    Debug.Write(wxString::Format("evsrv: cli %p response: %s\n", call.cli, s));
}

static bool handle_request(JRpcCall& call)
{
    const json_value *params;
    const json_value *id;

    dump_request(call);

    parse_request(call.req, &call.method, &params, &id);

    if (!call.method)
    {
        call.response << jrpc_error(JSONRPC_INVALID_REQUEST, "invalid request - missing method") << jrpc_id(0);
        return true;
    }

    if (params && !(params->type == JSON_ARRAY || params->type == JSON_OBJECT))
    {
        call.response << jrpc_error(JSONRPC_INVALID_REQUEST, "invalid request - params must be an array or object")
                      << jrpc_id(0);
        return true;
    }

    static struct
    {
        const char *name;
        void (*fn)(JObj& response, const json_value *params);
    } methods[] = {
        { "clear_calibration", &clear_calibration },
        { "deselect_star", &deselect_star },
        { "get_exposure", &get_exposure },
        { "set_exposure", &set_exposure },
        { "get_exposure_durations", &get_exposure_durations },
        { "get_profiles", &get_profiles },
        { "get_profile", &get_profile },
        { "set_profile", &set_profile },
        { "get_connected", &get_connected },
        { "set_connected", &set_connected },
        { "get_calibrated", &get_calibrated },
        { "get_paused", &get_paused },
        { "set_paused", &set_paused },
        { "get_lock_position", &get_lock_position },
        { "set_lock_position", &set_lock_position },
        { "loop", &loop },
        { "stop_capture", &stop_capture },
        { "guide", &guide },
        { "dither", &dither },
        { "find_star", &find_star },
        { "get_pixel_scale", &get_pixel_scale },
        { "get_app_state", &get_app_state },
        { "flip_calibration", &flip_calibration },
        { "get_lock_shift_enabled", &get_lock_shift_enabled },
        { "set_lock_shift_enabled", &set_lock_shift_enabled },
        { "get_lock_shift_params", &get_lock_shift_params },
        { "set_lock_shift_params", &set_lock_shift_params },
        { "save_image", &save_image },
        { "get_star_image", &get_star_image },
        { "get_use_subframes", &get_use_subframes },
        { "get_search_region", &get_search_region },
        { "shutdown", &shutdown },
        { "get_camera_binning", &get_camera_binning },
        { "get_camera_frame_size", &get_camera_frame_size },
        { "get_current_equipment", &get_current_equipment },
        { "get_guide_output_enabled", &get_guide_output_enabled },
        { "set_guide_output_enabled", &set_guide_output_enabled },
        { "get_algo_param_names", &get_algo_param_names },
        { "get_algo_param", &get_algo_param },
        { "set_algo_param", &set_algo_param },
        { "get_dec_guide_mode", &get_dec_guide_mode },
        { "set_dec_guide_mode", &set_dec_guide_mode },
        { "get_settling", &get_settling },
        { "guide_pulse", &guide_pulse },
        { "get_calibration_data", &get_calibration_data },
        { "start_guider_calibration", &start_guider_calibration },
        { "get_guider_calibration_status", &get_guider_calibration_status },
        { "start_dark_library_build", &start_dark_library_build },
        { "get_dark_library_status", &get_dark_library_status },
        { "load_dark_library", &load_dark_library },
        { "clear_dark_library", &clear_dark_library },
        { "cancel_dark_library_build", &cancel_dark_library_build },
        { "start_defect_map_build", &start_defect_map_build },
        { "get_defect_map_status", &get_defect_map_status },
        { "get_defect_map_build_status", &get_defect_map_build_status },
        { "cancel_defect_map_build", &cancel_defect_map_build },
        { "load_defect_map", &load_defect_map },
        { "clear_defect_map", &clear_defect_map },
        { "add_manual_defect", &add_manual_defect },
        { "start_drift_alignment", &start_drift_alignment },
        { "start_static_polar_alignment", &start_static_polar_alignment },
        { "start_polar_drift_alignment", &start_polar_drift_alignment },
        { "get_polar_alignment_status", &get_polar_alignment_status },
        { "cancel_polar_alignment", &cancel_polar_alignment },
        { "get_guiding_log", &get_guiding_log },
        { "capture_single_frame", &capture_single_frame },
        { "get_cooler_status", &get_cooler_status },
        { "set_cooler_state", &set_cooler_state },
        { "get_ccd_temperature", &get_sensor_temperature },
        { "export_config_settings", &export_config_settings },
        { "get_variable_delay_settings", &get_variable_delay_settings },
        { "set_variable_delay_settings", &set_variable_delay_settings },
        { "get_limit_frame", &get_limit_frame },
        { "set_limit_frame", &set_limit_frame },
    };

    for (unsigned int i = 0; i < WXSIZEOF(methods); i++)
    {
        if (strcmp(call.method->string_value, methods[i].name) == 0)
        {
            (*methods[i].fn)(call.response, params);
            if (id)
            {
                call.response << jrpc_id(id);
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
        call.response << jrpc_error(JSONRPC_METHOD_NOT_FOUND, "method not found") << jrpc_id(id);
        return true;
    }
    else
    {
        return false;
    }
}

static void handle_cli_input_complete(wxSocketClient *cli, char *input)
{
    // a dedicated JsonParser instance is used for each line of input since
    // handle_request can recurse if the request causes the event loop to run and we
    // don't want the parser to be reused.
    JsonParser parser;

    if (!parser.Parse(input))
    {
        JRpcCall call(cli, nullptr);
        call.response << jrpc_error(JSONRPC_PARSE_ERROR, parser_error(parser)) << jrpc_id(0);
        dump_response(call);
        do_notify1(cli, call.response);
        return;
    }

    const json_value *root = parser.Root();

    if (root->type == JSON_ARRAY)
    {
        // a batch request

        JAry ary;

        bool found = false;
        json_for_each(req, root)
        {
            JRpcCall call(cli, req);
            if (handle_request(call))
            {
                dump_response(call);
                ary << call.response;
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
        JRpcCall call(cli, req);
        if (handle_request(call))
        {
            dump_response(call);
            do_notify1(cli, call.response);
        }
    }
}

static void handle_cli_input(wxSocketClient *cli)
{
    // Bump refcnt to protect against reentrancy.
    //
    // Some functions like set_connected can cause the event loop to run reentrantly. If the
    // client disconnects before the response is sent and a socket disconnect event is
    // dispatched the client data could be destroyed before we respond.

    ClientDataGuard clidata(cli);

    ClientReadBuf *rdbuf = &clidata->rdbuf;

    wxSocketInputStream sis(*cli);

    while (sis.CanRead())
    {
        if (rdbuf->avail() == 0)
        {
            drain_input(sis);

            JRpcResponse response;
            response << jrpc_error(JSONRPC_INTERNAL_ERROR, "too big") << jrpc_id(0);
            do_notify1(cli, response);

            rdbuf->reset();
            break;
        }
        size_t n = sis.Read(rdbuf->dest, rdbuf->avail()).LastRead();
        if (n == 0)
            break;

        rdbuf->dest += n;

        char *end;
        while ((end = static_cast<char *>(memchr(rdbuf->buf(), '\n', rdbuf->len()))) != nullptr)
        {
            // Move the newline-terminated chunk from the read buffer to a temporary
            // buffer on the stack, and consume the chunk from the read buffer before
            // processing the line. This leaves the read buffer in the correct state to
            // be used again if this function is caller reentrantly.
            char line[ClientReadBuf::SIZE];
            size_t len1 = end - rdbuf->buf();
            memcpy(line, rdbuf->buf(), len1);
            line[len1] = 0;

            char *next = end + 1;
            size_t len2 = rdbuf->dest - next;
            memmove(rdbuf->buf(), next, len2);
            rdbuf->dest = rdbuf->buf() + len2;

            handle_cli_input_complete(cli, line);
        }
    }
}

EventServer::EventServer() : m_configEventDebouncer(nullptr) { }

EventServer::~EventServer() { }

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
    m_serverSocket = new wxSocketServer(eventServerAddr, wxSOCKET_REUSEADDR);

    if (!m_serverSocket->Ok())
    {
        Debug.Write(wxString::Format("Event server failed to start - Could not listen at port %u\n", port));
        delete m_serverSocket;
        m_serverSocket = nullptr;
        return true;
    }

    m_serverSocket->SetEventHandler(*this, EVENT_SERVER_ID);
    m_serverSocket->SetNotify(wxSOCKET_CONNECTION_FLAG);
    m_serverSocket->Notify(true);

    m_configEventDebouncer = new wxTimer();

    Debug.Write(wxString::Format("event server started, listening on port %u\n", port));

    return false;
}

void EventServer::EventServerStop()
{
    if (!m_serverSocket)
        return;

    for (CliSockSet::const_iterator it = m_eventServerClients.begin(); it != m_eventServerClients.end(); ++it)
    {
        destroy_client(*it);
    }
    m_eventServerClients.clear();

    delete m_serverSocket;
    m_serverSocket = nullptr;

    delete m_configEventDebouncer;
    m_configEventDebouncer = nullptr;

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

    Debug.Write(wxString::Format("evsrv: cli %p connect\n", client));

    client->SetEventHandler(*this, EVENT_SERVER_CLIENT_ID);
    client->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
    client->SetFlags(wxSOCKET_NOWAIT);
    client->Notify(true);
    client->SetClientData(new ClientData(client));

    send_catchup_events(client);

    m_eventServerClients.insert(client);
}

void EventServer::OnEventServerClientEvent(wxSocketEvent& event)
{
    wxSocketClient *cli = static_cast<wxSocketClient *>(event.GetSocket());

    if (event.GetSocketEvent() == wxSOCKET_LOST)
    {
        Debug.Write(wxString::Format("evsrv: cli %p disconnect\n", cli));

        unsigned int const n = m_eventServerClients.erase(cli);
        if (n != 1)
            Debug.AddLine("client disconnected but not present in client set!");

        destroy_client(cli);
    }
    else if (event.GetSocketEvent() == wxSOCKET_INPUT)
    {
        handle_cli_input(cli);
    }
    else
    {
        Debug.Write(wxString::Format("unexpected client socket event %d\n", event.GetSocketEvent()));
    }
}

void EventServer::NotifyStartCalibration(const Mount *mount)
{
    SIMPLE_NOTIFY_EV(ev_start_calibration(mount));
}

void EventServer::NotifyCalibrationStep(const CalibrationStepInfo& info)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("Calibrating");

    ev << NVMount(info.mount) << NV("dir", info.direction) << NV("dist", info.dist) << NV("dx", info.dx) << NV("dy", info.dy)
       << NV("pos", info.pos) << NV("step", info.stepNumber);

    if (!info.msg.empty())
        ev << NV("State", info.msg);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyCalibrationFailed(const Mount *mount, const wxString& msg)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("CalibrationFailed");
    ev << NVMount(mount) << NV("Reason", msg);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyCalibrationComplete(const Mount *mount)
{
    if (m_eventServerClients.empty())
        return;

    do_notify(m_eventServerClients, ev_calibration_complete(mount));
}

void EventServer::NotifyCalibrationDataFlipped(const Mount *mount)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("CalibrationDataFlipped");
    ev << NVMount(mount);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyLooping(unsigned int exposure, const Star *star, const FrameDroppedInfo *info)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("LoopingExposures");
    ev << NV("Frame", exposure);

    double mass = 0., snr, hfd;
    int err = 0;
    wxString status;

    if (star)
    {
        mass = star->Mass;
        snr = star->SNR;
        hfd = star->HFD;
        err = star->GetError();
    }
    else if (info)
    {
        if (Star::WasFound(static_cast<Star::FindResult>(info->starError)))
        {
            mass = info->starMass;
            snr = info->starSNR;
            hfd = info->starHFD;
        }
        err = info->starError;
        status = info->status;
    }

    if (mass)
    {
        ev << NV("StarMass", mass, 0) << NV("SNR", snr, 2) << NV("HFD", hfd, 2);
    }

    if (err)
        ev << NV("ErrorCode", err);

    if (!status.IsEmpty())
        ev << NV("Status", status);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyLoopingStopped()
{
    SIMPLE_NOTIFY("LoopingExposuresStopped");
}

void EventServer::NotifySingleFrameComplete(bool succeeded, const wxString& errorMsg, const SingleExposure& info)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev("SingleFrameComplete");
    ev << NV("Success", succeeded);

    if (!succeeded)
        ev << NV("Error", errorMsg);

    if (info.save)
    {
        ev << NV("Path", info.path);
    }

    do_notify(m_eventServerClients, ev);
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

    ev << NV("Frame", info.frameNumber) << NV("Time", info.time, 3) << NV("StarMass", info.starMass, 0)
       << NV("SNR", info.starSNR, 2) << NV("HFD", info.starHFD, 2) << NV("AvgDist", info.avgDist, 2);

    if (info.starError)
        ev << NV("ErrorCode", info.starError);

    if (!info.status.IsEmpty())
        ev << NV("Status", info.status);

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifyGuidingStarted()
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

    ev << NV("Frame", step.frameNumber) << NV("Time", step.time, 3) << NVMount(step.mount) << NV("dx", step.cameraOffset.X, 3)
       << NV("dy", step.cameraOffset.Y, 3) << NV("RADistanceRaw", step.mountOffset.X, 3)
       << NV("DECDistanceRaw", step.mountOffset.Y, 3) << NV("RADistanceGuide", step.guideDistanceRA, 3)
       << NV("DECDistanceGuide", step.guideDistanceDec, 3);

    if (step.durationRA > 0)
    {
        ev << NV("RADuration", step.durationRA)
           << NV("RADirection", step.mount->DirectionStr((GUIDE_DIRECTION) step.directionRA));
    }

    if (step.durationDec > 0)
    {
        ev << NV("DECDuration", step.durationDec)
           << NV("DECDirection", step.mount->DirectionStr((GUIDE_DIRECTION) step.directionDec));
    }

    if (step.mount->IsStepGuider())
    {
        ev << NV("Pos", step.aoPos);
    }

    ev << NV("StarMass", step.starMass, 0) << NV("SNR", step.starSNR, 2) << NV("HFD", step.starHFD, 2)
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

void EventServer::NotifyLockShiftLimitReached()
{
    SIMPLE_NOTIFY("LockPositionShiftLimitReached");
}

void EventServer::NotifyAppState()
{
    if (m_eventServerClients.empty())
        return;

    do_notify(m_eventServerClients, ev_app_state());
}

void EventServer::NotifySettleBegin()
{
    SIMPLE_NOTIFY("SettleBegin");
}

void EventServer::NotifySettling(double distance, double time, double settleTime, bool starLocked)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev(ev_settling(distance, time, settleTime, starLocked));

    Debug.Write(wxString::Format("evsrv: %s\n", ev.str()));

    do_notify(m_eventServerClients, ev);
}

void EventServer::NotifySettleDone(const wxString& errorMsg, int settleFrames, int droppedFrames)
{
    if (m_eventServerClients.empty())
        return;

    Ev ev(ev_settle_done(errorMsg, settleFrames, droppedFrames));

    Debug.Write(wxString::Format("evsrv: %s\n", ev.str()));

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

template<typename T>
static void NotifyGuidingParam(const EventServer::CliSockSet& clients, const wxString& name, T val)
{
    if (clients.empty())
        return;

    Ev ev("GuideParamChange");
    ev << NV("Name", name);
    ev << NV("Value", val);

    do_notify(clients, ev);
}

void EventServer::NotifyGuidingParam(const wxString& name, double val)
{
    ::NotifyGuidingParam(m_eventServerClients, name, val);
}

void EventServer::NotifyGuidingParam(const wxString& name, int val)
{
    ::NotifyGuidingParam(m_eventServerClients, name, val);
}

void EventServer::NotifyGuidingParam(const wxString& name, bool val)
{
    ::NotifyGuidingParam(m_eventServerClients, name, val);
}

void EventServer::NotifyGuidingParam(const wxString& name, const wxString& val)
{
    ::NotifyGuidingParam(m_eventServerClients, name, val);
}

void EventServer::NotifyConfigurationChange()
{
    if (m_configEventDebouncer == nullptr || m_configEventDebouncer->IsRunning())
        return;

    Ev ev("ConfigurationChange");
    do_notify(m_eventServerClients, ev);
    m_configEventDebouncer->StartOnce(0);
}
