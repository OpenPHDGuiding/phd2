/*
 *  debuglog.h
 *  PHD Guiding
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

#ifndef DEBUGLOG_INCLUDED
#define DEBUGLOG_INCLUDED

#include "logger.h"

class DebugLog : public wxFFile, public Logger
{
private:
    bool m_bEnabled;
    wxCriticalSection m_criticalSection;
    wxDateTime m_lastWriteTime;
    wxString m_pPathName;

    void InitVars(void);

public:
    DebugLog(void);
    DebugLog(const char *pName, bool bEnabled);
    ~DebugLog(void);

    bool Enable(bool bEnabled);
    bool IsEnabled(void);
    bool Init(const char *pName, bool bEnable, bool bForceOpen = false);
    wxString AddLine(const char *format, ...); // adds a newline
    wxString AddBytes(const wxString& str, const unsigned char *pBytes, unsigned count);
    wxString Write(const wxString& str);
    bool Flush(void);

    bool ChangeDirLog(const wxString& newdir);
};

extern DebugLog& operator<< (DebugLog& out, const wxString &str);
extern DebugLog& operator<< (DebugLog& out, const char *str);
extern DebugLog& operator<< (DebugLog& out, const int i);
extern DebugLog& operator<< (DebugLog& out, const double d);

inline bool DebugLog::IsEnabled(void)
{
    return m_bEnabled;
}

extern DebugLog Debug;

#endif
