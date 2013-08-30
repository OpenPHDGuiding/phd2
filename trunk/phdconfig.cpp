/*
 *  config.cpp
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

#include "phd.h"

wxString PhdConfig::DefaultProfileName = _("My Equipment");

ConfigSection::ConfigSection(void)
    : m_pConfig(NULL)
{
}

ConfigSection::~ConfigSection(void)
{
}

void ConfigSection::SelectProfile(int profileId)
{
    m_prefix = wxString::Format("/profile/%d", profileId);
}

bool ConfigSection::GetBoolean(const char *pName, bool defaultValue=false)
{
    bool bReturn = defaultValue;
    wxString name = m_prefix + pName;

    if (m_pConfig)
    {
        m_pConfig->Read(name, &bReturn, defaultValue);
    }

    Debug.AddLine(wxString::Format("GetBoolean(\"%s\", %d) returns %d", name, defaultValue, bReturn));

    return bReturn;
}

wxString ConfigSection::GetString(const char *pName, wxString defaultValue)
{
    wxString sReturn = defaultValue;
    wxString name = m_prefix + pName;

    if (m_pConfig)
    {
        m_pConfig->Read(name, &sReturn, defaultValue);
    }

    Debug.AddLine(wxString::Format("GetString(\"%s\", \"%s\") returns \"%s\"", name, defaultValue, sReturn));

    return sReturn;
}

double ConfigSection::GetDouble(const char *pName, double defaultValue)
{
    double dReturn = defaultValue;
    wxString name = m_prefix + pName;

    if (m_pConfig)
    {
        m_pConfig->Read(name, &dReturn, defaultValue);
    }

    Debug.AddLine(wxString::Format("GetDouble(\"%s\", %lf) returns %lf", name, defaultValue, dReturn));

    return dReturn;
}

long ConfigSection::GetLong(const char *pName, long defaultValue)
{
    long lReturn = defaultValue;
    wxString name = m_prefix + pName;

    if (m_pConfig)
    {
        m_pConfig->Read(name, &lReturn, defaultValue);
    }

    Debug.AddLine(wxString::Format("GetLong(\"%s\", %ld) returns %ld", name, defaultValue, lReturn));

    return lReturn;
}

int ConfigSection::GetInt(const char *pName, int defaultValue)
{
    long lReturn = defaultValue;
    wxString name = m_prefix + pName;

    if (m_pConfig)
    {
        m_pConfig->Read(name, &lReturn, defaultValue);
    }

    Debug.AddLine(wxString::Format("GetInt(\"%s\", %d) returns %d", name, defaultValue, (int)lReturn));

    return (int)lReturn;
}

void ConfigSection::SetBoolean(const char *pName, bool value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + pName, value);
    }
}

void ConfigSection::SetString(const char *pName, wxString value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + pName, value);
    }
}

void ConfigSection::SetDouble(const char *pName, double value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + pName, value);
    }
}

void ConfigSection::SetLong(const char *pName, long value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + pName, value);
    }
}

void ConfigSection::SetInt(const char *pName, int value)
{
    SetLong(pName, value);
}

bool ConfigSection::HasEntry(const wxString& name) const
{
    return m_pConfig && m_pConfig->HasEntry(m_prefix + name);
}

PhdConfig::PhdConfig(void)
{
}

PhdConfig::PhdConfig(const wxString& baseConfigName, int instance)
{
    Initialize(baseConfigName, instance);
}

PhdConfig::~PhdConfig(void)
{
    delete Global.m_pConfig;
}

// I thought wxConfigPathChanger would do this, but it didn't quite
struct AutoConfigPath
{
    wxConfigBase *m_cfg;
    wxString m_savePath;

    AutoConfigPath(wxConfigBase *cfg, const wxString& path)
        : m_cfg(cfg)
    {
        m_savePath = cfg->GetPath();
        cfg->SetPath(path);
    }
    ~AutoConfigPath()
    {
        m_cfg->SetPath(m_savePath);
    }
};

int PhdConfig::FirstProfile(void)
{
    AutoConfigPath changer(Profile.m_pConfig, "/profile");

    long id = 0;

    wxString str;
    long cookie;

    bool found = Profile.m_pConfig->GetFirstGroup(str, cookie);
    while (found)
    {
        if (str.ToLong(&id))
            break;

        found = Profile.m_pConfig->GetNextGroup(str, cookie);
    }

    return (int) id;
}

void PhdConfig::Initialize(const wxString& baseConfigName, int instance)
{
    wxString configName = baseConfigName;

    if (instance > 1)
    {
        configName += wxString::Format("-instance%d", instance);
    }

    wxConfig *config = new wxConfig(configName);
    Global.m_pConfig = Profile.m_pConfig = config;

    m_configVersion = Global.GetLong("ConfigVersion", 0);

    if (m_configVersion == 0)
    {
        Debug.AddLine(wxString::Format("Initializing a new config, m_pConfig=%p", Global.m_pConfig));

        Global.SetLong("ConfigVersion", CURRENT_CONFIG_VERSION);
    }

    InitializeProfile();
}

void PhdConfig::InitializeProfile(void)
{
    // select initial profile
    int currentProfile = Global.GetInt("/currentProfile", 0);
    if (currentProfile <= 0)
        currentProfile = FirstProfile();
    if (currentProfile <= 0)
    {
        CreateProfile(DefaultProfileName);
        currentProfile = GetProfileId(DefaultProfileName);
    }
    m_currentProfileId = currentProfile;
    Profile.SelectProfile(currentProfile);
    Global.SetInt("/currentProfile", currentProfile); // in case we just created it
}

void PhdConfig::DeleteAll(void)
{
    if (Global.m_pConfig)
    {
        Debug.AddLine(wxString::Format("Deleting all configuration data"));
        Global.m_pConfig->DeleteAll();
        InitializeProfile();
    }
}

wxString PhdConfig::GetCurrentProfile(void)
{
    return GetProfileName(m_currentProfileId);
}

bool PhdConfig::SetCurrentProfile(const wxString& name)
{
    if (GetProfileName(m_currentProfileId).CmpNoCase(name) == 0)
        return false;

    int id = GetProfileId(name);
    if (id <= 0)
    {
        CreateProfile(name);
        id = GetProfileId(name);
        if (id <= 0)
        {
            Debug.AddLine(wxString::Format("failed to create profile [%s]!", name));
            return true;
        }
    }

    m_currentProfileId = id;
    Profile.SelectProfile(id);
    Global.SetInt("/currentProfile", id);

    return false;
}

int PhdConfig::GetProfileId(const wxString& name)
{
    AutoConfigPath changer(Profile.m_pConfig, "/profile");

    int ret = 0;

    wxString str;
    long cookie;

    bool found = Profile.m_pConfig->GetFirstGroup(str, cookie);
    while (found)
    {
        long id;
        if (str.ToLong(&id))
        {
            if (GetProfileName(id).CmpNoCase(name) == 0)
            {
                ret = (int) id;
                break;
            }
        }

        found = Profile.m_pConfig->GetNextGroup(str, cookie);
    }

    return ret;
}

wxString PhdConfig::GetProfileName(int profileId)
{
    wxString name = Global.GetString(wxString::Format("/profile/%d/name", profileId), wxEmptyString);
    if (name.IsEmpty())
        name = wxString::Format("Profile %d", profileId);
    return name;
}

bool PhdConfig::CreateProfile(const wxString& name)
{
    // does the profile already exist?
    int id = GetProfileId(name);
    if (id > 0)
    {
        return true;
    }

    AutoConfigPath changer(Profile.m_pConfig, "/profile");

    // find the first available id
    for (id = 1; Profile.m_pConfig->HasGroup(wxString::Format("%d", id)); id++)
        ;

    Profile.m_pConfig->Write(wxString::Format("/profile/%d/name", id), name);

    return false;
}

static void CopyVal(wxConfigBase *cfg, const wxString& src, const wxString& dst)
{
    wxConfigBase::EntryType type = cfg->GetEntryType(src);
    switch (type) {
    case wxConfigBase::Type_String: {
        wxString val;
        cfg->Read(src, &val);
        cfg->Write(dst, val);
        break;
    }
    case wxConfigBase::Type_Boolean: {
        bool val;
        cfg->Read(src, &val);
        cfg->Write(dst, val);
        break;
    }
    case wxConfigBase::Type_Integer: {
        long val;
        cfg->Read(src, &val);
        cfg->Write(dst, val);
        break;
    }
    case wxConfigBase::Type_Float: {
        double val;
        cfg->Read(src, &val);
        cfg->Write(dst, val);
        break;
    }
    }
}

static void CopyGroup(wxConfigBase *cfg, const wxString& src, const wxString& dst)
{
    wxString str;
    long cookie;

    AutoConfigPath changer(cfg, src);

    bool more = cfg->GetFirstGroup(str, cookie);
    while (more)
    {
        CopyGroup(cfg, src + "/" + str, dst + "/" + str);
        more = cfg->GetNextGroup(str, cookie);
    }

    more = cfg->GetFirstEntry(str, cookie);
    while (more)
    {
        CopyVal(cfg, src + "/" + str, dst + "/" + str);
        more = cfg->GetNextEntry(str, cookie);
    }
}

bool PhdConfig::CloneProfile(const wxString& dest, const wxString& source)
{
    int srcId = GetProfileId(source);
    if (srcId <= 0)
    {
        Debug.AddLine(wxString::Format("Clone profile could not clone %s: profile not found", source));
        return true;
    }

    int dstId = GetProfileId(dest);
    if (dstId > 0)
    {
        Debug.AddLine(wxString::Format("Clone profile could not clone %s: destination profile %s already exists", source, dest));
        return true;
    }

    if (CreateProfile(dest))
    {
        return true;
    }

    dstId = GetProfileId(dest);
    if (dstId <= 0)
    {
        return true; // ??? should never happen
    }
    CopyGroup(Global.m_pConfig, wxString::Format("/profile/%d", srcId), wxString::Format("/profile/%d", dstId));
    // name was overwritten by copy
    Global.SetString(wxString::Format("/profile/%d/name", dstId), dest);

    return false;
}

void PhdConfig::DeleteProfile(const wxString& name)
{
    Debug.AddLine(wxString::Format("Delete profile %s", name));

    int id = GetProfileId(name);
    if (id <= 0)
        return;

    Global.m_pConfig->DeleteGroup(wxString::Format("/profile/%d", id));

    if (NumProfiles() == 0)
    {
        Debug.AddLine("Last profile deleted... create a new one");
        CreateProfile(DefaultProfileName);
    }
    if (id == m_currentProfileId)
    {
        m_currentProfileId = FirstProfile();
        Profile.SelectProfile(m_currentProfileId);
        Global.SetInt("/currentProfile", m_currentProfileId);
    }
}

bool PhdConfig::RenameProfile(const wxString& oldname, const wxString& newname)
{
    if (GetProfileId(newname) > 0)
    {
        Debug.AddLine(wxString::Format("error renaming profile %s to %s: new name already exists", oldname, newname));
        return true;
    }
    int id = GetProfileId(oldname);
    if (id <= 0)
    {
        Debug.AddLine(wxString::Format("error renaming profile %s to %s: profile does not exist", oldname, newname));
        return true;
    }

    Profile.m_pConfig->Write(wxString::Format("/profile/%d/name", id), newname);
    return false;
}

wxArrayString PhdConfig::ProfileNames(void)
{
    AutoConfigPath changer(Profile.m_pConfig, "/profile");

    wxArrayString ary;

    wxString str;
    long cookie;

    bool found = Profile.m_pConfig->GetFirstGroup(str, cookie);
    while (found)
    {
        long id;
        if (str.ToLong(&id))
        {
            ary.Add(GetProfileName(id));
        }

        found = Profile.m_pConfig->GetNextGroup(str, cookie);
    }

    return ary;
}

unsigned int PhdConfig::NumProfiles(void)
{
    AutoConfigPath changer(Profile.m_pConfig, "/profile");

    unsigned int count = 0;

    wxString str;
    long cookie;

    bool found = Profile.m_pConfig->GetFirstGroup(str, cookie);
    while (found)
    {
        long id;
        if (str.ToLong(&id))
        {
            ++count;
        }

        found = Profile.m_pConfig->GetNextGroup(str, cookie);
    }

    return count;
}
