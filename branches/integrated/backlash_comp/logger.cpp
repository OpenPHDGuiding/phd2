/*
 *  logger.cpp
 *  PHD Guiding
 *
 *  Created by Bruce Waddington
 *  Copyright (c) 2013 Bruce Waddington
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
#include "logger.h"
#include "wx/dir.h"
#include "wx/filefn.h"

Logger::Logger(void)
{
    m_Initialized = false;
}

Logger::~Logger(void)
{
}

// Default, safety-net implementation behind derived logger classes
bool Logger::ChangeDirLog(const wxString& newdir)
{
    return false;
}

// Return a valid default directory location for log files.  On Windows, this will normally be "My Documents\PHD2"
static wxString DefaultDir(void)
{
    wxStandardPathsBase& stdpath = wxStandardPaths::Get();
    wxString rslt = stdpath.GetDocumentsDir() + PATHSEPSTR + "PHD2";

    if (!wxDirExists(rslt))
        if (!wxFileName::Mkdir(rslt, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
            rslt = stdpath.GetDocumentsDir();             // should never happen

    return rslt;
}

// Return the current logging directory.  Design invaraint: returned string must always be a valid directory
wxString Logger::GetLogDir(void)
{
    if (m_Initialized)
        return m_CurrentDir;

    // One-time initialization at start-up
    wxString rslt = "";

    if (pConfig)
    {
        rslt = pConfig->Global.GetString ("/frame/LogDir", "");
        if (rslt.length() == 0)
            rslt = DefaultDir();                // user has never even looked at it
        else
            if (!wxDirExists(rslt))        // user might have deleted our old directories
            {
                if (!wxFileName::Mkdir(rslt, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))        // will build entire hierarchy if needed
                    rslt = DefaultDir();
            }
    }
    else
        rslt = DefaultDir();                    // shouldn't ever happen

    m_CurrentDir = rslt;
    m_Initialized = true;

    return rslt;
}

// Change the current logging directory, creating a new directory if needed. File system errors will result in a 'false' return
// and the current directory will be left unchanged.
bool Logger::SetLogDir(const wxString& dir)
{
    wxString newdir(dir);
    bool bOk = true;

    if (newdir.EndsWith(PATHSEPSTR))        // Need a standard form - no trailing separators
    {
        wxString stemp = PATHSEPSTR;
        newdir = newdir.substr(0, newdir.length() - stemp.length());
    }

    if (newdir.length() == 0)                // Empty-string shorthand for "default location"
    {
        newdir = DefaultDir();
    }
    else
    {
        if (!wxDirExists(newdir))
        {
            bOk = wxFileName::Mkdir(newdir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);        // will build entire hierarchy; client handles errors
        }
    }

    if (bOk)
    {
        m_CurrentDir = newdir;
        pConfig->Global.SetString("/frame/logdir", newdir);
    }

    return bOk;
}
// Clean up old log files in the directory.  Client gives us the target string - like PHD2_DebugLog*.txt - and the retention
// period.  Files older than that are removed.
void Logger::RemoveOldFiles(const wxString& FileTarget, int DaysOld)
{
    wxString dirName = GetLogDir();
    wxArrayString results;
    int numFiles;
    int hitCount = 0;
    wxDateTime oldestDate = wxDateTime::UNow() + wxDateSpan::Days(-DaysOld);
    wxString lastFile = "<None>";

    try
    {
        numFiles = wxDir::GetAllFiles(dirName, &results, FileTarget, wxDIR_FILES);      // No sub-directories, just files

        for (int inx = 0; inx < numFiles; inx++)
        {
            wxDateTime stamp = wxFileModificationTime(results[inx]);
            if (stamp < oldestDate)
            {
                hitCount++;
                lastFile = results[inx];            // For error logging
                wxRemoveFile(lastFile);
            }
    }
    }
    catch (wxString Msg)            // Eat the errors and press ahead, no place for UI here
    {
        Debug.Write(wxString::Format("Error cleaning up old log file %s: %s\n", lastFile, Msg));
    }

    if (hitCount > 0)
        Debug.Write(wxString::Format("Removing %d files of target: %s\n", hitCount, FileTarget));

}
