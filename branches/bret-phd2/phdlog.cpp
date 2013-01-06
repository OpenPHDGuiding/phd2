/*
 *  phdlog.cpp
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

void LOG::InitVars(void)
{
    m_bEnabled = false;
    m_lastWriteTime = wxDateTime::Now();
    m_pPathName = NULL;
}

LOG::LOG(void)
{
    InitVars();
}

LOG::LOG(char *pName, bool bEnabled = true)
{
    InitVars();

    Init(pName, bEnabled);
}

LOG::~LOG(void)
{
    wxFFile::Flush();
    wxFFile::Close();

    delete m_pPathName;
}

bool LOG::SetState(bool bEnabled)
{
    bool prevState = m_bEnabled;

    m_bEnabled = bEnabled;

    return prevState;
}

bool LOG::Init(char *pName, bool bEnable=true)
{
    wxCriticalSectionLocker lock(m_criticalSection);

    if (m_pPathName == NULL)
    {
        wxStandardPathsBase& stdpath = wxStandardPaths::Get();
        wxString strFileName = stdpath.GetDocumentsDir() + PATHSEPSTR + "PHD_" + (pName?pName:"debug") + ".log";

        m_pPathName = new wxString(strFileName);
    }

    if (m_bEnabled)
    {
        wxFFile::Flush();
        wxFFile::Close();

        m_bEnabled = false;
    }
        
    if (bEnable && m_pPathName)
    {
        m_bEnabled = wxFFile::Open(*m_pPathName, "a");
    }

    return m_bEnabled;
}

wxString LOG::AddLine(const wxString& str)
{
    return Write(str + "\n");
}

bool LOG::Flush(void)
{
    bool bReturn = true;

    if (m_bEnabled)
    {
        wxCriticalSectionLocker lock(m_criticalSection);

        bReturn = wxFFile::Flush();
    }

    return bReturn;
}

wxString LOG::Write(const wxString& str)
{
    if (m_bEnabled)
    {
        wxCriticalSectionLocker lock(m_criticalSection);

        wxDateTime now = wxDateTime::Now();
        wxTimeSpan deltaTime = now - m_lastWriteTime;
        wxLongLong milliSeconds = deltaTime.GetMilliseconds();

        m_lastWriteTime = now;

        wxFFile::Write(now.Format("%H:%M:%S.%l") + wxString::Format(" %d.%.6d ", milliSeconds/1000, milliSeconds%1000) + str);
    }

    return str;
}

LOG& operator<< (LOG& out, const wxString &str)
{
    out.Write(str);
    return out;
}

LOG& operator<< (LOG& out, const char *str)
{
    out.Write(str);
    return out;
}

LOG& operator<< (LOG& out, const int i)
{
    out.Write(wxString::Format(_T("%d"), i));
    return out;
}

LOG& operator<< (LOG& out, const double d)
{
    out.Write(wxString::Format(_T("%f"), d));
    return out;
}
