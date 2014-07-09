/*
 *  scope_onboard_st4.cpp
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
#include "scope_onboard_st4.h"

ScopeOnboardST4::ScopeOnboardST4(void)
{
    m_pOnboardHost = NULL;
}

ScopeOnboardST4::~ScopeOnboardST4(void)
{
    if (IsConnected())
    {
        Disconnect();
    }
    m_pOnboardHost = NULL;
}

bool ScopeOnboardST4::Connect(OnboardST4 *pOnboardHost)
{
    bool bError = false;

    try
    {
        if (!pOnboardHost)
        {
            throw ERROR_INFO("Attempt to Connect OnboardST4 mount with pOnboardHost == NULL");
        }

        m_pOnboardHost = pOnboardHost;

        if (IsConnected())
        {
            Disconnect();
        }

        if (!m_pOnboardHost->ST4HostConnected())
        {
            throw ERROR_INFO("Attempt to Connect Onboard ST4 mount when host is not connected");
        }

        if (!m_pOnboardHost->ST4HasGuideOutput())
        {
            throw ERROR_INFO("Attempt to Connect On Camera mount when camera does not have guide output");
        }

        Scope::Connect();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool ScopeOnboardST4::Disconnect(void)
{
    bool bError = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Attempt to Disconnect On Camera mount when not connected");
        }

        assert(m_pOnboardHost);

        m_pOnboardHost = NULL;

        bError = Scope::Disconnect();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

Mount::MOVE_RESULT ScopeOnboardST4::Guide(GUIDE_DIRECTION direction, int duration)
{
    MOVE_RESULT result = MOVE_OK;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Attempt to Guide On Camera mount when not connected");
        }

        if (!m_pOnboardHost)
        {
            throw ERROR_INFO("Attempt to Guide OnboardST4 mount when m_pOnboardHost == NULL");
        }

        if (!m_pOnboardHost->ST4HostConnected())
        {
            throw ERROR_INFO("Attempt to Guide On Camera mount when camera is not connected");
        }

        if (m_pOnboardHost->ST4PulseGuideScope(direction,duration))
        {
            result = MOVE_ERROR;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        result = MOVE_ERROR;
    }

    return result;
}

bool ScopeOnboardST4::HasNonGuiMove(void)
{
    bool bReturn = false;

    try
    {
        if (!IsConnected())
        {
            throw ERROR_INFO("Attempt to HasNonGuiMove On Camera mount when not connected");
        }

        if (!m_pOnboardHost->ST4HostConnected())
        {
            throw ERROR_INFO("Attempt to HasNonGuiMove On Camera mount when camera is not connected");
        }

        if (!m_pOnboardHost)
        {
            throw ERROR_INFO("Attempt HasNonGuiMove OnboardST4 mount when m_pOnboardHost == NULL");
        }

        bReturn = m_pOnboardHost->ST4HasNonGuiMove();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    return bReturn;
}
