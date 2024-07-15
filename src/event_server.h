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
    wxTimer *m_configEventDebouncer;

public:
    EventServer();
    ~EventServer(void);

    bool EventServerStart(unsigned int instanceId);
    void EventServerStop();

    void NotifyStartCalibration(const Mount *mount);
    void NotifyCalibrationStep(const CalibrationStepInfo& info);
    void NotifyCalibrationFailed(const Mount *mount, const wxString& msg);
    void NotifyCalibrationComplete(const Mount *mount);
    void NotifyCalibrationDataFlipped(const Mount *mount);
    void NotifyLooping(unsigned int exposure, const Star *star, const FrameDroppedInfo *info);
    void NotifyLoopingStopped();
    void NotifyStarSelected(const PHD_Point& pos);
    void NotifyStarLost(const FrameDroppedInfo& info);
    void NotifyGuidingStarted();
    void NotifyGuidingStopped();
    void NotifyPaused();
    void NotifyResumed();
    void NotifyGuideStep(const GuideStepInfo& info);
    void NotifyGuidingDithered(double dx, double dy);
    void NotifySetLockPosition(const PHD_Point& xy);
    void NotifyLockPositionLost();
    void NotifyLockShiftLimitReached();
    void NotifyAppState();
    void NotifySettleBegin();
    void NotifySettling(double distance, double time, double settleTime, bool starLocked);
    void NotifySettleDone(const wxString& errorMsg, int settleFrames, int droppedFrames);
    void NotifyAlert(const wxString& msg, int type);
    void NotifyGuidingParam(const wxString& name, double val);
    void NotifyGuidingParam(const wxString& name, int val);
    void NotifyGuidingParam(const wxString& name, bool val);
    void NotifyGuidingParam(const wxString& name, const wxString& val);
    void NotifyConfigurationChange();

private:
    void OnEventServerEvent(wxSocketEvent& evt);
    void OnEventServerClientEvent(wxSocketEvent& evt);

    wxDECLARE_EVENT_TABLE();
};

extern EventServer EvtServer;

#endif
