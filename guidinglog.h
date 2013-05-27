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

enum LOGGED_IMAGE_FORMAT {
    LIF_LOW_Q_JPEG,
    LIF_HI_Q_JPEG,
    LIF_RAW_FITS
};

class GuidingLog
{
    bool m_enabled;
    wxFFile m_file;
    bool m_image_logging_enabled;
    LOGGED_IMAGE_FORMAT m_logged_image_format;
    int m_frame;
    wxDateTime m_guidingStarted;

public:
    GuidingLog(bool active=false);
    ~GuidingLog(void);

    bool EnableLogging(void);
    bool EnableLogging(bool enabled);
    bool DisableLogging(void);
    bool IsEnabled(void);
    bool Flush(void);
    void Close(void);

    bool EnableImageLogging(LOGGED_IMAGE_FORMAT fmt);
    bool DisableImageLogging(void);
    bool IsImageLoggingEnabled(void);
    LOGGED_IMAGE_FORMAT LoggedImageFormat(void);

    bool StartCalibration(Mount *pCalibrationMount);
    bool CalibrationFailed(Mount *pCalibrationMount, wxString msg);
    bool CalibrationStep(Mount *pCalibrationMount, wxString direction, int steps, double dx, double dy, const PHD_Point &xy, double dist);
    bool CalibrationDirectComplete(Mount *pCalibrationMount, wxString direction, double angle, double rate);
    bool CalibrationComplete(Mount *pCalibrationMount);

    bool StartGuiding();
    bool GuideStep(Mount *pGuideMount, const PHD_Point& vectorEndpoint,
        double RADuration, double RADistance,
        double DECDuration, double DECDistance, int errorCode);

    bool ServerCommand(Guider* guider,  wxString cmd);
    bool ServerGuidingDithered(Guider* guider, double dx, double dy);
    bool ServerSetLockPosition(Guider* guider, const PHD_Point &xy);

    bool StartEntry(void);
};
