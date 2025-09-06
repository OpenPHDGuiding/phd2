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

#include <wx/dir.h>

#define ALWAYS_FLUSH_DEBUGLOG
const int RetentionPeriod = 30;

DebugLog::DebugLog() : m_enabled(false), m_lastWriteTime(wxDateTime::UNow()) { }

DebugLog::~DebugLog()
{
    wxFFile::Flush();
    wxFFile::Close();
}

static bool ParseLogTimestamp(wxDateTime *p, const wxString& s)
{
    wxDateTime dt;
    wxString::const_iterator iter;
    if (dt.ParseFormat(s, "%Y-%m-%d_%H%M%S", wxDateTime(), &iter) && iter == s.end())
    {
        *p = dt;
        return true;
    }
    return false;
}

wxDateTime DebugLog::GetLogFileTime()
{
    // find the latest log file time
    wxDir dir(Debug.GetLogDir());
    wxString filename;
    // PHD2_DebugLog_YYYY-mm-dd_HHMMSS.txt
    bool cont = dir.GetFirst(&filename, "PHD2_DebugLog_*.txt", wxDIR_FILES);
    wxDateTime latest;
    while (cont)
    {
        wxDateTime dt;
        if (filename.length() == 35 && ParseLogTimestamp(&dt, filename.substr(14, 17)) &&
            (!latest.IsValid() || dt.IsLaterThan(latest)))
        {
            latest = dt;
        }
        cont = dir.GetNext(&filename);
    }

    wxDateTime res = wxDateTime::Now();
    if (latest.IsValid() && PhdApp::IsSameImagingDay(latest, res))
        res = latest;

    return res;
}

bool DebugLog::Enable(bool enable)
{
    bool prevState = m_enabled;

    m_enabled = enable;

    return prevState;
}

void DebugLog::InitDebugLog(bool enable, bool forceOpen)
{
    const wxDateTime& logFileTime = wxGetApp().GetLogFileTime();

    wxCriticalSectionLocker lock(m_criticalSection);

    if (m_enabled)
    {
        wxFFile::Flush();
        wxFFile::Close();

        m_enabled = false;
    }

    if (enable && (m_path.IsEmpty() || forceOpen))
    {
        // Keep log files separated for multiple PHD2 instances
        wxString qualifier = wxGetApp().GetInstanceNumber() > 1 ? std::to_string(wxGetApp().GetInstanceNumber()) + "_" : "";
        m_path = GetLogDir() + PATHSEPSTR + _("PHD2_DebugLog_") + qualifier + logFileTime.Format(_T("%Y-%m-%d_%H%M%S.txt"));

        if (!wxFFile::Open(m_path, "a"))
        {
            wxMessageBox(wxString::Format(_("unable to open file %s"), m_path));
        }
    }

    m_enabled = enable;
}

bool DebugLog::ChangeDirLog(const wxString& newdir)
{
    bool enabled = IsEnabled();
    bool ok = true;

    if (!SetLogDir(newdir))
    {
        Debug.Write(wxString::Format("Error: unable to set new log dir %s\n", newdir));
        wxMessageBox(wxString::Format("invalid folder name %s, debug log folder unchanged", newdir));
        ok = false;
    }

    Debug.Write(wxString::Format("Changed log dir to %s\n", newdir));
    InitDebugLog(enabled, true);

    return ok;
}

void DebugLog::RemoveOldFiles()
{
    Logger::RemoveMatchingFiles("PHD2_DebugLog*.txt", RetentionPeriod);
}

wxString DebugLog::AddLine(const wxString& str)
{
    return Write(str + "\n");
}

wxString DebugLog::AddBytes(const wxString& str, const unsigned char *pBytes, unsigned int count)
{
    wxString Line = str + " - ";

    for (unsigned int i = 0; i < count; i++)
    {
        unsigned char ch = pBytes[i];
        Line += wxString::Format("%2.2X (%c) ", ch, isprint(ch) ? ch : '?');
    }

    return Write(Line + "\n");
}

bool DebugLog::Flush()
{
    bool ret = true;

    if (m_enabled)
    {
        wxCriticalSectionLocker lock(m_criticalSection);

        ret = wxFFile::Flush();
    }

    return ret;
}

wxString DebugLog::Write(const wxString& str)
{
    if (m_enabled)
    {
        wxCriticalSectionLocker lock(m_criticalSection);

        wxDateTime now = wxDateTime::UNow();
        wxTimeSpan deltaTime = now - m_lastWriteTime;
        m_lastWriteTime = now;
        wxString outputLine = wxString::Format("%s %s %lu %s", now.Format("%H:%M:%S.%l"), deltaTime.Format("%S.%l"),
                                               (unsigned long) wxThread::GetCurrentId(), str);

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

DebugLog& operator<<(DebugLog& out, const wxString& str)
{
    out.Write(str);
    return out;
}

DebugLog& operator<<(DebugLog& out, const char *str)
{
    out.Write(str);
    return out;
}

DebugLog& operator<<(DebugLog& out, const int i)
{
    out.Write(wxString::Format(_T("%d"), i));
    return out;
}

DebugLog& operator<<(DebugLog& out, const double d)
{
    out.Write(wxString::Format(_T("%f"), d));
    return out;
}
