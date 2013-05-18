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

PhdConfig::PhdConfig(void)
{
    m_pConfig = NULL;
}

PhdConfig::PhdConfig(const wxString& baseConfigName)
{
    m_pConfig = NULL;
    Initialize(baseConfigName);
}

PhdConfig::~PhdConfig(void)
{
    delete m_pConfig;
}

void PhdConfig::Initialize(const wxString& baseConfigName)
{
    m_pConfig = new wxConfig(baseConfigName);

    m_configVersion = GetLong("ConfigVersion", 0);

    if (m_configVersion == 0)
    {
        SetLong("ConfigVersion", CURRENT_CONFIG_VERSION);
    }
}

bool PhdConfig::GetBoolean(const char *pName, bool defaultValue=false)
{
    bool bReturn=defaultValue;

    if (m_pConfig)
    {
        m_pConfig->Read(pName, &bReturn, defaultValue);
    }

    return bReturn;
}

wxString PhdConfig::GetString(const char *pName, wxString defaultValue)
{
    wxString sReturn=defaultValue;

    if (m_pConfig)
    {
        m_pConfig->Read(pName, &sReturn, defaultValue);
    }

    return sReturn;
}

double PhdConfig::GetDouble(const char *pName, double defaultValue)
{
    double dReturn = defaultValue;

    if (m_pConfig)
    {
        m_pConfig->Read(pName, &dReturn, defaultValue);
    }

    return dReturn;
}

long PhdConfig::GetLong(const char *pName, long defaultValue)
{
    long lReturn = defaultValue;

    if (m_pConfig)
    {
        m_pConfig->Read(pName, &lReturn, defaultValue);
    }

    return lReturn;
}

int PhdConfig::GetInt(const char *pName, int defaultValue)
{
    long lReturn = GetLong(pName, defaultValue);

    return (int)lReturn;
}


void PhdConfig::SetBoolean(const char *pName, bool value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(pName, value);
    }
}

void PhdConfig::SetString(const char *pName, wxString value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(pName, value);
    }
}

void PhdConfig::SetDouble(const char *pName, double value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(pName, value);
    }
}

void PhdConfig::SetLong(const char *pName, long value)
{
    if (m_pConfig)
    {
        m_pConfig->Write(pName, value);
    }
}

void PhdConfig::SetInt(const char *pName, int value)
{
    SetLong(pName, value);
}
