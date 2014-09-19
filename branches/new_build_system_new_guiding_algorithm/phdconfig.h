/*
 *  phdconfig.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  Refactored by Bret McKee
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
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
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

#ifndef PHDCONFIG_H_INCLUDED
#define PHDCONFIG_H_INCLUDED

/*
 * The way configuration varialbes are handled has been
 * fundamentally changed from the way PHD 1.X handled them
 * because they are no longer all stored in global variables.
 *
 * The wxConfig routines allow for hierarchical configuration data, and
 * we now use it because it much more closesly matches the movement of
 * the data that was done to remove global variables.
 *
 * The hiearchy looks like:
 *
 * / program globals - logging, debug
 *   guider - guider globals if there are any
 *     onestar
 *     algorithms
 *     hysteresis
 *   mount - mount globals if there are any
 *     scope - default choice
 *       ascomlate
 *   camera - default choice
 *     ascom
 *
 * There is no separete "load" or "save" steps.  Constructors request
 * the configuration values for thier classes, and dialogs that modify them
 * write the values immediately.
 *
 */

class PhdConfig;

class ConfigSection
{
    wxConfig *m_pConfig;
    wxString m_prefix;

    friend class PhdConfig;

public:
    ConfigSection();
    ~ConfigSection();

    void SelectProfile(int profileId);

    bool     GetBoolean(const char *pName, bool defaultValue);
    wxString GetString(const char *pName, const wxString& defaultValue);
    double   GetDouble(const char *pName, double defaultValue);
    long     GetLong(const char *pName, long defaultValue);
    int      GetInt(const char *pName, int defaultValue);

    void SetBoolean(const char *pName, bool value);
    void SetString(const char *pName, const wxString& value);
    void SetDouble(const char *pName, double value);
    void SetLong(const char *pName, long value);
    void SetInt(const char *pName, int value);

    bool HasEntry(const wxString& name) const;

    void DeleteEntry(const wxString& name);
    void DeleteGroup(const wxString& name);
};

class PhdConfig
{
    static const long CURRENT_CONFIG_VERSION=2001;
    long m_configVersion;
    int m_currentProfileId;

public:
    PhdConfig(void);
    PhdConfig(const wxString& baseConfigName, int instance);
    ~PhdConfig(void);

    static wxString DefaultProfileName;

    void Initialize(const wxString& baseConfigName, int instance);
    void DeleteAll(void);

    void InitializeProfile(void);
    wxString GetCurrentProfile(void);
    int GetCurrentProfileId(void) { return m_currentProfileId; }
    bool SetCurrentProfile(const wxString& name);

    int GetProfileId(const wxString& name);
    int FirstProfile(void);
    wxString GetProfileName(int profileId);
    bool ProfileExists(int profileId);
    bool CreateProfile(const wxString& name);
    bool CloneProfile(const wxString& dest, const wxString& source);
    void DeleteProfile(const wxString& name);
    bool RenameProfile(const wxString& oldname, const wxString& newname);
    bool WriteProfile(const wxString& filename);
    bool ReadProfile(const wxString& filename);
    wxArrayString ProfileNames(void);
    unsigned int NumProfiles(void);

    ConfigSection Global;
    ConfigSection Profile;
};

#endif /* PHDCONFIG_H_INCLUDED */
