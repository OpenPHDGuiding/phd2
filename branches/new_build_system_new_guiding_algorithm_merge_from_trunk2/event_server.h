/*
 *  event_server.h
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2013 Andy Galasso
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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

#ifndef EVENT_SERVER_INCLUDED
#define EVENT_SERVER_INCLUDED

#include <set>
#include "json_parser.h"

class EventServer : public wxEvtHandler
{
public:
    typedef std::set<wxSocketClient *> CliSockSet;

private:
    JsonParser m_parser;
    wxSocketServer *m_serverSocket;
    CliSockSet m_eventServerClients;

public:
    EventServer();
    ~EventServer(void);

    bool EventServerStart(unsigned int instanceId);
    void EventServerStop();

    void NotifyStartCalibration(Mount *pCalibrationMount);
    void NotifyCalibrationFailed(Mount *pCalibrationMount, const wxString& msg);
    void NotifyCalibrationComplete(Mount *pCalibrationMount);
    void NotifyCalibrationDataFlipped(Mount *mount);
    void NotifyLooping(unsigned int exposure);
    void NotifyLoopingStopped();
    void NotifyStarSelected(const PHD_Point& pos);
    void NotifyStarLost(const FrameDroppedInfo& info);
    void NotifyStartGuiding();
    void NotifyGuidingStopped();
    void NotifyPaused();
    void NotifyResumed();
    void NotifyGuideStep(const GuideStepInfo& info);
    void NotifyGuidingDithered(double dx, double dy);
    void NotifySetLockPosition(const PHD_Point& xy);
    void NotifyLockPositionLost();
    void NotifyAppState();
    void NotifySettling(double distance, double time, double settleTime);
    void NotifySettleDone(const wxString& errorMsg);
    void NotifyAlert(const wxString& msg, int type);

private:
    void OnEventServerEvent(wxSocketEvent& evt);
    void OnEventServerClientEvent(wxSocketEvent& evt);

    wxDECLARE_EVENT_TABLE();
};

extern EventServer EvtServer;

#endif
