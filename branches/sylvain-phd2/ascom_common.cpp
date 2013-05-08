/*
 *  ascom_common.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
 *  All rights reserved.
 *
 *  Based upon work by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

#include "ascom_common.h"
#include "phd.h"

/* provides common ASCOM functionality */

// Lifted from the ASCOM sample Utilities.cpp
// -------------
// uni_to_ansi() - Convert unicode to ANSI, return pointer to new[]'ed string
// -------------
//
#ifdef __WINDOWS__
char *ASCOM_COMMON::uni_to_ansi(OLECHAR *os)
{
    char *cp;

    // Is this the right way??? (it works)
    int len = WideCharToMultiByte(CP_ACP,
                                0,
                                os,
                                -1,
                                NULL,
                                0,
                                NULL,
                                NULL);
    cp = new char[len + 5];
    if(cp == NULL)
        return NULL;

    if (0 == WideCharToMultiByte(CP_ACP,
                                    0,
                                    os,
                                    -1,
                                    cp,
                                    len,
                                    NULL,
                                    NULL))
    {
        delete [] cp;
        return NULL;
    }

    cp[len] = '\0';
    return(cp);
}

bool ASCOM_COMMON::GetDispatchID(IDispatch *pDriver, wchar_t *pName, DISPID *pId)
{
    bool bError = false;

    if(FAILED(pDriver->GetIDsOfNames(IID_NULL, &pName, 1, LOCALE_USER_DEFAULT, pId)))
    {
        Debug.AddLine(wxString::Format("for pDriver %p, get dispatch id for %s failed", pDriver, pName));
        bError = true;
    }
    else
    {
        Debug.AddLine(wxString::Format("for pDriver %p, dispatch id for %s = %d", pDriver, pName, *pId));
    }

    return bError;
}
#endif


