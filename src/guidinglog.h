/*
 *  guidinglog.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012-2013 Bret McKee
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

#ifndef GUIDINGLOG_INCLUDED
#define GUIDINGLOG_INCLUDED

#include "logger.h"

class Mount;
class Guider;
struct LockPosShiftParams;

struct CalibrationStepInfo
{
    Mount *mount;
    wxString direction;
    int stepNumber;
    double dx;
    double dy;
    PHD_Point pos;
    double dist;
    wxString msg;

    CalibrationStepInfo(Mount *mount_, const wxString& dir_, int stepNumber_, double dx_,
        double dy_, const PHD_Point& pos_, double dist_, const wxString& msg_ = wxEmptyString)
        : mount(mount_), direction(dir_), stepNumber(stepNumber_), dx(dx_), dy(dy_), pos(pos_),
        dist(dist_), msg(msg_) { }
};

struct GuideStepInfo
{
    Mount *mount;
    unsigned int moveOptions;
    int frameNumber;
    double time;
    PHD_Point cameraOffset;
    PHD_Point mountOffset;
    double guideDistanceRA;
    double guideDistanceDec;
    int durationRA;
    int durationDec;
    bool raLimited;
    bool decLimited;
    // TODO: the following two members are GUIDE_DIRECTION, but we have circular
    // dependencies in our header files so cannot use GUIDE_DIRECTION here
    int directionRA;
    int directionDec;
    wxPoint aoPos;
    double starMass;
    double starSNR;
    double starHFD;
    double avgDist;
    int starError;
};

struct FrameDroppedInfo
{
    int frameNumber;
    double time;
    double starMass;
    double starSNR;
    double starHFD;
    double avgDist;
    int starError;
    wxString status;
};

struct GuideLogSummaryInfo
{
    bool valid;
    unsigned int cal_cnt;
    unsigned int guide_cnt;
    double guide_dur;
    unsigned int ga_cnt;

    void Clear()
    {
        valid = false;
        cal_cnt = 0;
        guide_cnt = 0;
        guide_dur = 0.;
        ga_cnt = 0;
    }
    GuideLogSummaryInfo() { Clear(); }
    void LoadSummaryInfo(wxFFile& guidelog);
};

class GuidingLog : public Logger
{
    bool m_enabled;
    wxFFile m_file;
    wxString m_fileName;
    bool m_keepFile;
    bool m_isGuiding;
    GuideLogSummaryInfo m_summary;

    void EnableLogging();
    void DisableLogging();

public:
    GuidingLog();
    ~GuidingLog();

    void EnableLogging(bool enabled);
    bool IsEnabled() const;
    bool Flush();
    void CloseGuideLog();

    wxFFile& File();

    void StartCalibration(const Mount *pCalibrationMount);
    void CalibrationFailed(const Mount *pCalibrationMount, const wxString& msg);
    void CalibrationStep(const CalibrationStepInfo& info);
    void CalibrationDirectComplete(const Mount *pCalibrationMount, const wxString& direction,
        double angle, double rate, int parity);
    void CalibrationComplete(const Mount *pCalibrationMount);

    void GuidingStarted();
    void GuidingStopped();
    void GuideStep(const GuideStepInfo& info);
    void FrameDropped(const FrameDroppedInfo& info);
    void CalibrationFrameDropped(const FrameDroppedInfo& info);

    void ServerCommand(Guider *guider, const wxString& cmd);
    void NotifyGuidingDithered(Guider *guider, double dx, double dy);
    void NotifySetLockPosition(Guider *guider);
    void NotifyLockShiftParams(const LockPosShiftParams& shiftParams, const PHD_Point& cameraRate);
    void NotifySettlingStateChange(const wxString& msg);
    void NotifyGACompleted();
    void NotifyGAResult(const wxString& msg);
    void NotifyManualGuide(const Mount *whichMount, int direction, int duration);

    void SetGuidingParam(const wxString& name, double val);
    void SetGuidingParam(const wxString& name, int val);
    void SetGuidingParam(const wxString& name, bool val);
    void SetGuidingParam(const wxString& name, const wxString& val);
    void SetGuidingParam(const wxString& name, const wxString& val, bool AlwaysLog);

    bool ChangeDirLog(const wxString& newdir);
    void RemoveOldFiles();
};

inline bool GuidingLog::IsEnabled() const
{
    return m_enabled;
}

inline wxFFile& GuidingLog::File()
{
    return m_file;
}

extern GuidingLog GuideLog;

#endif
