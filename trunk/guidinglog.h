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

struct GuideStepInfo
{
    Mount *mount;
    int frameNumber;
    double time;
    const PHD_Point *cameraOffset;
    const PHD_Point *mountOffset;
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
    double avgDist;
    int starError;
};

struct FrameDroppedInfo
{
    int frameNumber;
    double time;
    double starMass;
    double starSNR;
    double avgDist;
    int starError;
    wxString status;
};

class GuidingLog : public Logger
{
    bool m_enabled;
    wxFFile m_file;
    wxString m_fileName;
    bool m_keepFile;
    bool m_isGuiding;

protected:
    void GuidingHeader(void);

public:
    GuidingLog(void);
    ~GuidingLog(void);

    bool EnableLogging(void);
    bool EnableLogging(bool enabled);
    void DisableLogging(void);
    bool IsEnabled(void) const;
    bool Flush(void);
    void Close(void);

    void StartCalibration(Mount *pCalibrationMount);
    void CalibrationFailed(Mount *pCalibrationMount, const wxString& msg);
    void CalibrationStep(Mount *pCalibrationMount, const wxString& direction, int steps, double dx, double dy, const PHD_Point &xy, double dist);
    void CalibrationDirectComplete(Mount *pCalibrationMount, const wxString& direction, double angle, double rate);
    void CalibrationComplete(Mount *pCalibrationMount);

    void StartGuiding();
    void StopGuiding();
    void GuideStep(const GuideStepInfo& info);
    void FrameDropped(const FrameDroppedInfo& info);

    void ServerCommand(Guider *guider, const wxString& cmd);
    void NotifyGuidingDithered(Guider *guider, double dx, double dy);
    void NotifySetLockPosition(Guider *guider);
    void NotifyLockShiftParams(const LockPosShiftParams& shiftParams, const PHD_Point& cameraRate);
    void NotifySettlingStateChange(const wxString& msg);

    void SetGuidingParam(const wxString& name, double val);
    void SetGuidingParam(const wxString& name, int val);
    void SetGuidingParam(const wxString& name, const wxString& val);

    bool ChangeDirLog(const wxString& newdir);
};

inline bool GuidingLog::IsEnabled(void) const
{
    return m_enabled;
}

extern GuidingLog GuideLog;

#endif
