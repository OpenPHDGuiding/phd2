/*
 *  phdlog.h
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

#include <wx/stdpaths.h>
#include <wx/ffile.h>

class LOG:wxFFile
{
private:
    bool m_bEnabled;

    void InitVars(void)
    {
        m_bEnabled = false;
    }

public:
    LOG()
    {
        InitVars();
    }

    LOG(char *pName, bool bEnabled = true)
    {
        InitVars();

        Init(pName, bEnabled);
    }

    ~LOG()
    {
        wxFFile::Flush();
        wxFFile::Close();
    }

    bool Init(char *pName, bool bEnable=true)
    {
        if (m_bEnabled)
        {
            wxFFile::Flush();
            wxFFile::Close();

            m_bEnabled = false;
        }

        if (bEnable)
        {
            wxStandardPathsBase& stdpath = wxStandardPaths::Get();
            wxString strFileName = stdpath.GetDocumentsDir() + PATHSEPSTR + "PHD_" + pName + ".log";
            
            m_bEnabled = wxFFile::Open(strFileName, "a");
        }

        return m_bEnabled;
    }

    bool Write(const wxString& str)
    {
        bool bReturn = true;

        if (m_bEnabled)
        {
            bReturn = wxFFile::Write(str);
        }

        return bReturn;
    }

    bool AddLine(const wxString& str)
    {
        return Write(str + "\n");
    }

    bool Flush(void)
    {
        bool bReturn = true;

        if (m_bEnabled)
        {
            bReturn = wxFFile::Flush();
        }

        return bReturn;
    }
};

extern LOG& operator<< (LOG& out, const wxString &str);
extern LOG& operator<< (LOG& out, const char *str);
extern LOG& operator<< (LOG& out, const int i);
extern LOG& operator<< (LOG& out, const double d);
