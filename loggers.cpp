/*
 *  loggers.cpp
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
#include "loggers.h"


Loggers::Loggers(void)
{
    m_Initialized = false;
}

Loggers::~Loggers(void)
{
}

// Default, safety-net implementation behind derived logger classes
bool Loggers::ChangeDirLog (wxString newdir)
{
    return (false);
}

// Return a valid default directory location for log files.  On Windows, this will normally be "My Documents\PHD2"
wxString DefaultDir (void)
{
    wxStandardPathsBase& stdpath = wxStandardPaths::Get();
    wxString srslt = stdpath.GetDocumentsDir () + PATHSEPSTR + "PHD2";

    if (!wxDirExists (srslt))
        if (!wxFileName::Mkdir (srslt, 511, true))
            srslt = stdpath.GetDocumentsDir ();             // should never happen
    return (srslt);

}
// Return the current logging directory.  Design invaraint: returned string must always be a valid directory
wxString Loggers::GetLogDir (void)
{
    if (m_Initialized)
        return (m_CurrentDir);
    else
    {                                // One-time initialization at start-up
        wxString srslt = "";

        if (pConfig)
        {
            srslt = pConfig->Global.GetString ("/frame/LogDir", "");
            if (srslt.length() == 0)
                srslt = DefaultDir();                // user has never even looked at it
            else
                if (!wxDirExists (srslt))        // user might have deleted our old directories
                {
                    if (!wxFileName::Mkdir (srslt, 511, true))        // will build entire hierarchy if needed
                        srslt = DefaultDir ();
                }

        }
        else
            srslt = DefaultDir ();                    // shouldn't ever happen

        m_CurrentDir = srslt;
        m_Initialized = true;
        return (srslt);
    }
}

// Change the current logging directory, creating a new directory if needed. File system errors will result in a 'false' return
// and the current directory will be left unchanged.
bool Loggers::SetLogDir (wxString newdir)
{
    bool bOk = true;

    if (newdir.EndsWith (PATHSEPSTR))        // Need a standard form - no trailing separators
    {
        wxString stemp = PATHSEPSTR;
        newdir = newdir.substr (0, newdir.length()-stemp.length());
    }

    if (newdir.length() == 0)                // Empty-string shorthand for "default location"
    {
        newdir = DefaultDir ();
    }
    else
        if (!wxDirExists (newdir))
        {
            bOk = wxFileName::Mkdir (newdir, 511, true);        // will build entire hierarchy; client handles errors
        }

    if (bOk)
    {
        m_CurrentDir = newdir;
        pConfig->Global.SetString ("/frame/logdir", newdir);
    }

    return (bOk);


}


