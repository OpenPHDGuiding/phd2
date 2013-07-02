/*
 *  parallelport_win32.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2013 Bret McKee
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

#ifdef __WINDOWS__

/* ----Prototypes of Inp and Out32--- */
short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);

wxArrayString ParallelPortWin32::GetParallelPortList(void)
{
    wxArrayString ret;

    try
    {
        ret.Add("LPT1 - 0x3BC");
        ret.Add("LPT2 - 0x378");
        ret.Add("LPT3 - 0x278");
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    return ret;
}

ParallelPortWin32::ParallelPortWin32(void)
{
    m_portAddr = 0;
}

ParallelPortWin32::~ParallelPortWin32(void)
{
    m_portAddr = 0;
}

bool ParallelPortWin32::Connect(wxString portName)
{
    bool bError = false;

    try
    {
        wxString address = portName.AfterLast(' ');
        long addr;

        if (!address.ToLong(&addr, 16))
        {
            throw ERROR_INFO("uable to convert [" + address + "] to a port number");
        }

        m_portAddr = addr;

        Debug.AddLine(wxString::Format("com port %s assigned address 0x%x", portName, m_portAddr));
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool ParallelPortWin32::Disconnect(void)
{
    bool bError = false;

    try
    {
        m_portAddr = 0;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool ParallelPortWin32::ReadByte(BYTE *pData)
{
    bool bError = false;

    try
    {
        if (m_portAddr == 0)
        {
            throw ERROR_INFO("mPortAddr == 0");
        }
        *pData = Inp32(m_portAddr);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool ParallelPortWin32::WriteByte(UINT8 data)
{
    bool bError = false;

    try
    {
        if (m_portAddr == 0)
        {
            throw ERROR_INFO("mPortAddr == 0");
        }

        Out32(m_portAddr, data);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

#endif // __WINDOWS__
