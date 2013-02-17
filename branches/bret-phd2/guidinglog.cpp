/*
 *  guidinglog.cpp
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

#include "phd.h"

GuidingLog::GuidingLog(bool active)
{
    if (active)
    {
        EnableLogging();
    }
}

GuidingLog::~GuidingLog(void)
{
}

bool GuidingLog::EnableLogging(void)
{
    bool bError = false;

    try
    {
        if (!m_file.IsOpened())
        {
            wxStandardPathsBase& stdpath = wxStandardPaths::Get();
            wxDateTime now = wxDateTime::Now();

            wxString fileName = stdpath.GetDocumentsDir() + PATHSEPSTR + "PHD_GuideLog" + now.Format(_T("_%Y-%m-%d")) +  now.Format(_T("_%H%M%S"))+ ".txt";

            if (!m_file.Open(fileName, "wb"))
            {
                throw ERROR_INFO("unable to open file");
            }
        }

        assert(m_file.IsOpened());

        m_enabled = true;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::DisableLogging(void)
{
    bool bError = false;

    try
    {
        m_enabled = false;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::Flush(void)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());

            if (!m_file.Flush())
            {
                throw ERROR_INFO("unable to flush file");
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::StartCalibration(Mount *pCalibrationMount)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            wxDateTime now = wxDateTime::Now();

            m_file.Write("\n");
            m_file.Write("Calibration Begins at " + now.Format(_T("%Y-%m-%d %H:%M:%S")) + "\n");

            assert(pCalibrationMount && pCalibrationMount->IsConnected());

            m_file.Write("Mount is " + pCalibrationMount->Name() + "\n");

            m_file.Write(wxString::Format("Lockposition = (%.2lf, %.2lf)\n",
                        pFrame->pGuider->LockPosition().X,
                        pFrame->pGuider->LockPosition().Y));
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::StartGuiding(void)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            wxDateTime now = wxDateTime::Now();

            m_file.Write("\n");
            m_file.Write("Guiding Begins at " + now.Format(_T("%Y-%m-%d %H:%M:%S")) + "\n");

            if (pCamera)
            {
                m_file.Write("Camera is " + pCamera->Name + "\n");
            }

            if (pMount)
            {
                m_file.Write("Mount is " + pMount->Name() + "\n");
            }

            if (pSecondaryMount)
            {
                m_file.Write("Secondary Mount is " + pSecondaryMount->Name() + "\n");
            }

            m_file.Write(wxString::Format("Lockposition = (%.2lf, %.2lf)\n",
                        pFrame->pGuider->LockPosition().X,
                        pFrame->pGuider->LockPosition().Y));
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuidingLog::StartEntry(void)
{
    bool bError = false;

    try
    {
        if (m_enabled)
        {
            assert(m_file.IsOpened());
            m_file.Write("\n");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}
