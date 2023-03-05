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
#include "event_server.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>

// NOTE: Not translated here, explicitly translated below. Using _("...")
// here in a static initializer crashes
wxString PhdConfig::DefaultProfileName = wxTRANSLATE("My Equipment");

#define PROFILE_STREAM_VERSION "1"

ConfigSection::ConfigSection()
    : m_pConfig(nullptr)
{
}

ConfigSection::~ConfigSection()
{
}

void ConfigSection::SelectProfile(int profileId)
{
    m_prefix = wxString::Format("/profile/%d", profileId);
}

bool ConfigSection::GetBoolean(const wxString& name, bool defaultValue)
{
    bool bReturn = defaultValue;
    wxString path = m_prefix + name;

    if (m_pConfig)
    {
        m_pConfig->Read(path, &bReturn, defaultValue);
    }

    Debug.Write(wxString::Format("GetBoolean(\"%s\", %d) returns %d\n", path, defaultValue, bReturn));

    return bReturn;
}

wxString ConfigSection::GetString(const wxString& name, const wxString& defaultValue)
{
    wxString sReturn = defaultValue;
    wxString path = m_prefix + name;

    if (m_pConfig)
    {
        m_pConfig->Read(path, &sReturn, defaultValue);
    }

    Debug.Write(wxString::Format("GetString(\"%s\", \"%s\") returns \"%s\"\n", path, defaultValue, sReturn));

    return sReturn;
}

double ConfigSection::GetDouble(const wxString& name, double defaultValue)
{
    double dReturn = defaultValue;
    wxString path = m_prefix + name;

    if (m_pConfig)
    {
        m_pConfig->Read(path, &dReturn, defaultValue);
    }

    Debug.Write(wxString::Format("GetDouble(\"%s\", %f) returns %f\n", path, defaultValue, dReturn));

    return dReturn;
}

long ConfigSection::GetLong(const wxString& name, long defaultValue)
{
    long lReturn = defaultValue;
    wxString path = m_prefix + name;

    if (m_pConfig)
    {
        m_pConfig->Read(path, &lReturn, defaultValue);
    }

    Debug.Write(wxString::Format("GetLong(\"%s\", %ld) returns %ld\n", path, defaultValue, lReturn));

    return lReturn;
}

int ConfigSection::GetInt(const wxString& name, int defaultValue)
{
    long lReturn = defaultValue;
    wxString path = m_prefix + name;

    if (m_pConfig)
    {
        m_pConfig->Read(path, &lReturn, defaultValue);
    }

    Debug.Write(wxString::Format("GetInt(\"%s\", %d) returns %d\n", path, defaultValue, (int)lReturn));

    return (int)lReturn;
}

void ConfigSection::SetBoolean(const wxString& name, bool value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + name, value);
        EvtServer.NotifyConfigurationChange();
    }
}

void ConfigSection::SetString(const wxString& name, const wxString& value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + name, value);
        EvtServer.NotifyConfigurationChange();
    }
}

void ConfigSection::SetDouble(const wxString& name, double value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + name, value);
        EvtServer.NotifyConfigurationChange();
    }
}

void ConfigSection::SetLong(const wxString& name, long value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(m_prefix + name, value);
        EvtServer.NotifyConfigurationChange();
    }
}

void ConfigSection::SetInt(const wxString& name, int value)
{
    SetLong(name, value);
}

bool ConfigSection::HasEntry(const wxString& name) const
{
    return m_pConfig && m_pConfig->HasEntry(m_prefix + name);
}

void ConfigSection::DeleteEntry(const wxString& name)
{
    m_pConfig->DeleteEntry(m_prefix + name);
    EvtServer.NotifyConfigurationChange();
}

void ConfigSection::DeleteGroup(const wxString& name)
{
    m_pConfig->DeleteGroup(m_prefix + name);
    EvtServer.NotifyConfigurationChange();
}

// Return a list of node names (group names) for the current profile, starting at the level specified by baseName
// e.g. baseName = "scope" would enumerate all the nodes in the profile whose parent is "scope"
std::vector<wxString> ConfigSection::GetGroupNames(const wxString& baseName)
{
    wxString oldPath = m_pConfig->GetPath();
    m_pConfig->SetPath(m_prefix + baseName);
    long lInx;
    wxString grpName;
    bool more;
    std::vector<wxString> entries;

    more = m_pConfig->GetFirstGroup(grpName, lInx);
    while (more)
    {
        entries.push_back(grpName);
        more = m_pConfig->GetNextGroup(grpName, lInx);
    }
    m_pConfig->SetPath(oldPath);
    return entries;
}

static wxString ConfigName(int instance)
{
    wxString configName = _T("PHDGuidingV2");
    if (instance > 1)
    {
        configName += wxString::Format("-instance%d", instance);
    }
    return configName;
}

PhdConfig::PhdConfig(int instance)
{
    wxConfig *config = new wxConfig(ConfigName(instance));
    Global.m_pConfig = Profile.m_pConfig = config;

    m_isNewInstance = false;

    m_configVersion = Global.GetLong("ConfigVersion", 0);
    if (m_configVersion == 0)
    {
        m_isNewInstance = true;

        Debug.Write(wxString::Format("Initializing a new config, m_pConfig=%p\n", Global.m_pConfig));

        Global.SetLong("ConfigVersion", CURRENT_CONFIG_VERSION);
        m_configVersion = CURRENT_CONFIG_VERSION;
    }
}

PhdConfig::~PhdConfig()
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

int PhdConfig::FirstProfile()
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

void PhdConfig::InitializeProfile()
{
    // select initial profile
    int currentProfile = Global.GetInt("/currentProfile", 0);
    if (currentProfile <= 0)
        currentProfile = FirstProfile();
    if (currentProfile <= 0)
    {
        CreateProfile(wxGetTranslation(DefaultProfileName));
        currentProfile = GetProfileId(wxGetTranslation(DefaultProfileName));
    }
    m_currentProfileId = currentProfile;
    Profile.SelectProfile(currentProfile);
    Global.SetInt("/currentProfile", currentProfile); // in case we just created it
}

void PhdConfig::DeleteAll()
{
    if (Global.m_pConfig)
    {
        Debug.Write(wxString::Format("Deleting all configuration data\n"));
        Global.m_pConfig->DeleteAll();
        InitializeProfile();
    }
    m_isNewInstance = true;
}

wxString PhdConfig::GetCurrentProfile()
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
            Debug.Write(wxString::Format("failed to create profile [%s]!\n", name));
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

bool PhdConfig::ProfileExists(int profileId)
{
    wxString name = Global.GetString(wxString::Format("/profile/%d/name", profileId), wxEmptyString);
    return !name.IsEmpty();
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

    EvtServer.NotifyConfigurationChange();

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
    case wxConfigBase::Type_Unknown:
        break;
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
        Debug.Write(wxString::Format("Clone profile could not clone %s: profile not found\n", source));
        return true;
    }

    int dstId = GetProfileId(dest);
    if (dstId > 0)
    {
        Debug.Write(wxString::Format("Clone profile could not clone %s: destination profile %s already exists\n", source, dest));
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
    Debug.Write(wxString::Format("Delete profile %s\n", name));

    int id = GetProfileId(name);
    if (id <= 0)
        return;

    Global.m_pConfig->DeleteGroup(wxString::Format("/profile/%d", id));

    if (NumProfiles() == 0)
    {
        Debug.Write("Last profile deleted... create a new one\n");
        CreateProfile(wxGetTranslation(DefaultProfileName));
    }
    if (id == m_currentProfileId)
    {
        m_currentProfileId = FirstProfile();
        Profile.SelectProfile(m_currentProfileId);
        Global.SetInt("/currentProfile", m_currentProfileId);
    }

    EvtServer.NotifyConfigurationChange();
}

bool PhdConfig::RenameProfile(const wxString& oldname, const wxString& newname)
{
    if (GetProfileId(newname) > 0)
    {
        Debug.Write(wxString::Format("error renaming profile %s to %s: new name already exists\n", oldname, newname));
        return true;
    }
    int id = GetProfileId(oldname);
    if (id <= 0)
    {
        Debug.Write(wxString::Format("error renaming profile %s to %s: profile does not exist\n", oldname, newname));
        return true;
    }

    Profile.m_pConfig->Write(wxString::Format("/profile/%d/name", id), newname);

    EvtServer.NotifyConfigurationChange();

    return false;
}

static wxString escape_string(const wxString& s)
{
    wxString t(s);
    static const wxString BACKSLASH("\\");
    static const wxString BACKSLASH_BACKSLASH("\\\\");
    static const wxString TAB("\t");
    static const wxString BACKSLASH_T("\\t");
    static const wxString CR("\r");
    static const wxString BACKSLASH_R("\\r");
    static const wxString LF("\n");
    static const wxString BACKSLASH_N("\\n");
    t.Replace(BACKSLASH, BACKSLASH_BACKSLASH);
    t.Replace(TAB, BACKSLASH_T);
    t.Replace(CR, BACKSLASH_R);
    t.Replace(LF, BACKSLASH_N);
    return t;
}

static wxString unescape_string(const wxString& s)
{
    size_t const len = s.length();
    wxString d;
    for (size_t i = 0; i < len; i++)
    {
        auto ch = s[i];
        if (ch.GetValue() == '\\' && i < len - 1)
        {
            switch (s[i + 1].GetValue())
            {
            case '\\': d.Append('\\'); ++i; break;
            case 't':  d.Append('\t'); ++i; break;
            case 'r':  d.Append('\r'); ++i; break;
            case 'n':  d.Append('\n'); ++i; break;
            default:   d.Append(ch);        break;
            }
        }
        else
            d.Append(ch);
    }
    return d;
}

static bool ParseLine(const wxString& s, wxString *name, wxString *typestr, wxString *val)
{
    if (s.IsEmpty())
        return false;
    wxStringTokenizer tokenizer(s, "\t\r\n");
    *name = tokenizer.GetNextToken();
    *typestr = tokenizer.GetNextToken();
    *val = tokenizer.GetString();
    val->Trim();
    return true;
}

static void LoadVal(ConfigSection& section, const wxString& s, const wxString& name,
    const wxString& typestr, const wxString& val)
{
    long type;
    if (!typestr.ToLong(&type))
    {
        Debug.Write(wxString::Format("bad type '%s' in file; line = %s\n", typestr, s));
        return;
    }
    switch ((wxConfigBase::EntryType) type)
    {
    case wxConfigBase::Type_String:
        section.SetString(name, unescape_string(val));
        break;
    case wxConfigBase::Type_Boolean: {
        long lval;
        if (!val.ToLong(&lval))
        {
            Debug.Write(wxString::Format("bad bool val '%s' in file; line = %s\n", val, s));
        }
        else
        {
            section.SetBoolean(name, lval ? true : false);
        }
        break;
    }
    case wxConfigBase::Type_Integer: {
        long lval;
        if (!val.ToLong(&lval))
        {
            Debug.Write(wxString::Format("bad int val '%s' in file; line = %s\n", val, s));
        }
        else
        {
            section.SetLong(name, lval);
        }
        break;
    }
    case wxConfigBase::Type_Float: {
        double dval;
        if (!val.ToDouble(&dval))
        {
            Debug.Write(wxString::Format("bad float val '%s' in file; line = %s\n", val, s));
        }
        else
        {
            section.SetDouble(name, dval);
        }
        break;
    }
    default:
        Debug.Write(wxString::Format("bad type '%s' in file; line = %s\n", typestr, s));
        break;
    }
}

bool PhdConfig::ReadProfile(const wxString& filename)
{
    wxFileInputStream is(filename);
    if (!is.IsOk())
    {
        Debug.Write(wxString::Format("Cannot open file '%s'\n", filename));
        return true;
    }
    wxTextInputStream tis(is, wxS("\t"), wxMBConvUTF8());

    wxString s = tis.ReadLine();
    if (s != "PHD Profile " PROFILE_STREAM_VERSION)
    {
        Debug.Write(wxString::Format("invalid profile file '%s'\n", filename));
        return true;
    }

    // use the filename as the profile name
    wxFileName fname(filename);
    wxString profileName = fname.GetName();

    // if a profile exists with this name, delete it

    int id = GetProfileId(profileName);
    if (id > 0)
    {
        Global.m_pConfig->DeleteGroup(wxString::Format("/profile/%d", id));
    }

    CreateProfile(profileName);
    SetCurrentProfile(profileName);

    while (!is.Eof())
    {
        wxString s = tis.ReadLine();
        wxString name, typestr, val;
        if (!ParseLine(s, &name, &typestr, &val))
            continue;
        // skip the stored name as we are using the file name for the profile name
        if (name == "/name")
            continue;
        LoadVal(Profile, s, name, typestr, val);
    }

    return false;
}

static void WriteVal(wxTextOutputStream& os, wxConfigBase *cfg, const wxString& key, const wxString& prefix)
{
    wxString sval;
    wxConfigBase::EntryType type = cfg->GetEntryType(key);
    switch (type) {
    case wxConfigBase::Type_String: {
        wxString val;
        cfg->Read(key, &val);
        sval = escape_string(val);
        break;
    }
    case wxConfigBase::Type_Boolean: {
        bool val;
        cfg->Read(key, &val);
        sval = wxString(val ? "1" : "0");
        break;
    }
    case wxConfigBase::Type_Integer: {
        long val;
        cfg->Read(key, &val);
        sval = wxString::Format("%lu", val);
        break;
    }
    case wxConfigBase::Type_Float: {
        double val;
        cfg->Read(key, &val);
        sval = wxString::Format("%g", val);
        break;
    }
    case wxConfigBase::Type_Unknown:
        break;
    }

    os.WriteString(wxString::Format("%s\t%d\t%s\n", key.substr(prefix.length()), (int) type, sval));
}

static void WriteGroup(wxTextOutputStream& os, wxConfigBase *cfg, const wxString& group, const wxString& prefix)
{
    wxString str;
    long cookie;

    AutoConfigPath changer(cfg, group);

    bool more = cfg->GetFirstGroup(str, cookie);
    while (more)
    {
        WriteGroup(os, cfg, group + "/" + str, prefix);
        more = cfg->GetNextGroup(str, cookie);
    }

    more = cfg->GetFirstEntry(str, cookie);
    while (more)
    {
        WriteVal(os, cfg, group + "/" + str, prefix);
        more = cfg->GetNextEntry(str, cookie);
    }
}

bool PhdConfig::WriteProfile(const wxString& filename)
{
    wxFileOutputStream os(filename);
    if (!os.IsOk())
    {
        return true;
    }
    wxTextOutputStream tos(os, wxEOL_NATIVE, wxMBConvUTF8());

    tos.WriteString("PHD Profile " PROFILE_STREAM_VERSION "\n");
    wxString profile = wxString::Format("/profile/%d", m_currentProfileId);
    WriteGroup(tos, Profile.m_pConfig, profile, profile);

    return false;
}

bool PhdConfig::SaveAll(const wxString& filename)
{
    wxFileOutputStream os(filename);
    if (!os.IsOk())
    {
        return true;
    }
    wxTextOutputStream tos(os, wxEOL_NATIVE, wxMBConvUTF8());

    tos.WriteString("PHD Config " PROFILE_STREAM_VERSION "\n");
    WriteGroup(tos, Global.m_pConfig, wxEmptyString, wxEmptyString);

    return false;
}

bool PhdConfig::RestoreAll(const wxString& filename)
{
    wxFileInputStream is(filename);
    if (!is.IsOk())
    {
        Debug.Write(wxString::Format("Cannot open file '%s'\n", filename));
        return true;
    }
    wxTextInputStream tis(is, wxS("\t"), wxMBConvUTF8());

    wxString s = tis.ReadLine();
    if (s != "PHD Config " PROFILE_STREAM_VERSION)
    {
        Debug.Write(wxString::Format("invalid config file '%s'\n", filename));
        return true;
    }

    Global.m_pConfig->DeleteAll();

    while (!is.Eof())
    {
        wxString s = tis.ReadLine();
        wxString name, typestr, val;
        if (!ParseLine(s, &name, &typestr, &val))
            continue;
        LoadVal(Global, s, name, typestr, val);
    }

    EvtServer.NotifyConfigurationChange();

    return false;
}

bool PhdConfig::Flush()
{
    Debug.Write("PhdConfig flush\n");

    // On Linux and Mac, this will write the config file if it is dirty
    // (no-op if it is not dirty).  Always a no-op on Windows.
    bool ok = Global.m_pConfig->Flush();
    return ok;
}

wxArrayString PhdConfig::ProfileNames()
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

unsigned int PhdConfig::NumProfiles()
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

AutoTempProfile::AutoTempProfile(bool init)
{
    if (init)
    {
        Init();
    }
}

AutoTempProfile::~AutoTempProfile()
{
    if (m_prev.empty())
    {
        // commited or never inited
        return;
    }

    pConfig->SetCurrentProfile(m_prev);
    pConfig->DeleteProfile(m_name);
}

void AutoTempProfile::Init()
{
    if (m_prev.empty())
        m_prev = pConfig->GetCurrentProfile();

    wxString const tempName(".!temp!profile!name!~");
    pConfig->DeleteProfile(tempName); // stale temp profile from a crash?
    pConfig->SetCurrentProfile(tempName); // creates it and make it the current profile
    m_name = tempName;
}

bool AutoTempProfile::Commit(const wxString& name)
{
    // caller should have validated name is not empty
    assert(!name.empty());

    bool err = pConfig->RenameProfile(m_name, name);
    if (err)
        return false; // named profile already exists

    m_prev.clear(); // signifies committed
    return true;
}
