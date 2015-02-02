/*
*  polystarlog.h
*  PHD Guiding
*
*  Adapted by KOR from debuglog.cpp by Brek McKee
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

//******************************************************************************
PolyStarLog::PolyStarLog(void)
{
    m_bEnabled = false;
    m_lastWriteTime = wxDateTime::UNow();
}

//******************************************************************************
PolyStarLog::~PolyStarLog(void)
{
	wxFFile::Flush();
	wxFFile::Close();
}

//******************************************************************************
bool PolyStarLog::Enable(const bool bEnabled)
{
	bool prevState = m_bEnabled;

	m_bEnabled = bEnabled;

	return prevState;
}

//******************************************************************************
bool PolyStarLog::Init(const bool bEnable, const bool bForceOpen)
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

		m_pPathName = GetLogDir() + PATHSEPSTR + "PHD2_PolyStarLog" + now.Format(_T("_%Y-%m-%d")) +  now.Format(_T("_%H%M%S"))+ ".txt";

		if (!wxFFile::Open(m_pPathName, "a"))
		{
			wxMessageBox(wxString::Format("unable to open file %s", m_pPathName));
		}
	}

	m_bEnabled = bEnable;
	m_line = wxString();

	return m_bEnabled;
}

//******************************************************************************
void PolyStarLog::ClearLine(void)
{
	m_line = wxString();
}

//******************************************************************************
void PolyStarLog::AddHeaderLine(PolyStar& polystar)
{
	if (!m_bEnabled)
		return;

	wxString line = wxString("time,time delta");
	for (size_t ndx = 0; ndx < polystar.len(); ndx++)
	{
		wxString star_string = wxString::Format(",Star %02d X,Star %2d Y, Star %2d SNR, Star %d Mass", ndx, ndx, ndx, ndx);
		line += star_string;
	}
	line += wxString(",Centroid X, Centroid Y");
	line += wxString(",Lock Pos X, Lock Pos Y");
	line += wxString(",Correction X, Correction Y");
	line += wxString("\n");
	
	wxCriticalSectionLocker lock(m_criticalSection);

	wxFFile::Write(line);
	wxFFile::Flush();

	return;
}

//******************************************************************************
void PolyStarLog::AddStar(Star& star)
{
	if (!m_bEnabled)
		return;

	m_line += wxString::Format(",%8.4f,%8.4f,%5.1f,%7.1f", star.X, star.Y, star.SNR, star.Mass);
	return;
}

//******************************************************************************
void PolyStarLog::AddPoint(double X, double Y)
{
	if (!m_bEnabled)
		return;

	m_line += wxString::Format(",%8.4f,%8.4f", X, Y);

	return;
}


//******************************************************************************
void PolyStarLog::LogLine(void)
{
	if (!m_bEnabled)
		return;

	wxCriticalSectionLocker lock(m_criticalSection);

	wxDateTime now = wxDateTime::UNow();
	wxTimeSpan deltaTime = now - m_lastWriteTime;
	m_lastWriteTime = now;
	wxString outputLine = wxString::Format("%s,%s%s\n", now.Format("%H:%M:%S.%l"), deltaTime.Format("%S.%l"), m_line);

	wxFFile::Write(outputLine);
	wxFFile::Flush();

	return;
}
