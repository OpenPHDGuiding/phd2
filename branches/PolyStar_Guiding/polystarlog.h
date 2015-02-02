/*
 *  polystarlog.h
 *  PHD Guiding
 *
 *  Adapted by KOR from debuglog.h by Brek McKee
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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

#ifndef POLYSTARLOG_INCLUDED
#define POLYSTARLOG_INCLUDED

#include "logger.h"
#include "polystar.h"
//#include "debuglog.h"

class PolyStarLog : public wxFFile, public Logger
//class PolyStarLog : public DebugLog
{
private:
	bool m_bEnabled;
	wxCriticalSection	m_criticalSection;
	wxDateTime			m_lastWriteTime;
	wxString			m_pPathName;

	wxString			m_line;

public:
    PolyStarLog(void);
//    PolyStarLog(const char *pName, bool bEnabled);
    ~PolyStarLog(void);

	bool Enable(const bool bEnabled);
	bool IsEnabled(void);
    bool Init(const bool bEnable = true, const bool bForceOpen = false);

	void AddHeaderLine(PolyStar& polystar);	
	void AddStar(Star& star);
	void AddPoint(double X, double Y);

	void ClearLine(void);
	void LogLine(void);
};

inline bool PolyStarLog::IsEnabled(void)
{
    return m_bEnabled;
}

extern PolyStarLog PolystarLog;

#endif
