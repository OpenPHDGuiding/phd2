/*
 *  debuglog.cpp
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

#include "phd.h"

#define ALWAYS_FLUSH_DEBUGLOG

void DebugLog::InitVars(void)
{
    m_bEnabled = false;
    m_lastWriteTime = wxDateTime::UNow();
}

DebugLog::DebugLog(void)
{
    InitVars();
}

DebugLog::DebugLog(const char *pName, bool bEnabled = true)
{
    InitVars();

    Init(pName, bEnabled);
}

DebugLog::~DebugLog(void)
{
    wxFFile::Flush();
    wxFFile::Close();
}

bool DebugLog::Enable(bool bEnabled)
{
    bool prevState = m_bEnabled;

    m_bEnabled = bEnabled;

    return prevState;
}

bool DebugLog::Init(const char *pName, bool bEnable, bool bForceOpen)
{
    wxCriticalSectionLocker lock(m_criticalSection);

    if (m_bEnabled)
    {
        wxFFile::Flush();
        wxFFile::Close();

        m_bEnabled = false;
    }

    if (bEnable && (m_pPathName.IsEmpty() || bForceOpen))
    {
        wxDateTime now = wxDateTime::UNow();

        m_pPathName = GetLogDir() + PATHSEPSTR + "PHD2_DebugLog" + now.Format(_T("_%Y-%m-%d")) +  now.Format(_T("_%H%M%S"))+ ".txt";

        if (!wxFFile::Open(m_pPathName, "a"))
        {
            wxMessageBox(wxString::Format("unable to open file %s", m_pPathName));
        }
    }

    m_bEnabled = bEnable;

    return m_bEnabled;
}

bool DebugLog::ChangeDirLog(const wxString& newdir)
{
    bool bEnabled = IsEnabled();
    bool bOk = true;

    if (!SetLogDir(newdir))
    {
        wxMessageBox(wxString::Format("invalid folder name %s, debug log folder unchanged", newdir));
        bOk = false;
    }

    Init("debug", bEnabled, true);                // lots of side effects, but all good...
    return bOk;
}

wxString DebugLog::AddLine(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    wxString ret = Write(wxString::FormatV(format, args) + "\n");

    va_end(args);

    return ret;
}

wxString DebugLog::AddBytes(const wxString& str, const unsigned char *pBytes, unsigned int count)
{
    wxString Line = str + " - ";

    for (unsigned int i=0; i < count; i++)
    {
        unsigned char ch = pBytes[i];
        Line += wxString::Format("%2.2X (%c) ", ch, isprint(ch) ? ch : '?');
    }

    return Write(Line + "\n");
}

bool DebugLog::Flush(void)
{
    bool bReturn = true;

    if (m_bEnabled)
    {
        wxCriticalSectionLocker lock(m_criticalSection);

        bReturn = wxFFile::Flush();
    }

    return bReturn;
}

wxString DebugLog::Write(const wxString& str)
{
    if (m_bEnabled)
    {
        wxCriticalSectionLocker lock(m_criticalSection);

        wxDateTime now = wxDateTime::UNow();
        wxTimeSpan deltaTime = now - m_lastWriteTime;
        m_lastWriteTime = now;
        wxString outputLine = wxString::Format("%s %s %lu %s", now.Format("%H:%M:%S.%l"),
                                                              deltaTime.Format("%S.%l"),
                                                              (unsigned long) wxThread::GetCurrentId(),
                                                              str);

        wxFFile::Write(outputLine);
#if defined(ALWAYS_FLUSH_DEBUGLOG)
        wxFFile::Flush();
#endif
#if defined(__WINDOWS__) && defined(_DEBUG)
        OutputDebugString(outputLine.c_str());
#endif
    }

    return str;
}

DebugLog& operator<< (DebugLog& out, const wxString &str)
{
    out.Write(str);
    return out;
}

DebugLog& operator<< (DebugLog& out, const char *str)
{
    out.Write(str);
    return out;
}

DebugLog& operator<< (DebugLog& out, const int i)
{
    out.Write(wxString::Format(_T("%d"), i));
    return out;
}

DebugLog& operator<< (DebugLog& out, const double d)
{
    out.Write(wxString::Format(_T("%f"), d));
    return out;
}
