/*
 *  runinbg.h
 *  PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2014 Andy Galasso
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

#ifndef RUNINBG_INCLUDED
#define RUNINBG_INCLUDED

struct RunInBgImpl;

class RunInBg
{
    RunInBgImpl *m_impl;

    RunInBg(const RunInBg&); // not implemented
    RunInBg& operator= (const RunInBg&); // not implemented

public:

    RunInBg(wxWindow *parent, const wxString& title, const wxString& message);
    virtual ~RunInBg(void);
    bool Run();

    // sub-classes implement the background activity in Entry()
    virtual bool Entry(void) = 0;

    // sub-classes should check IsCanceled() frequently in Entry() to see if the user clicked "Cancel"
    bool IsCanceled(void);

    void SetErrorMsg(const wxString& msg);
    wxString GetErrorMsg(void);

    // If cancel is requested and the background thread does not exit after a short grace period,
    // the backdound thread will be killed. Sub-classes can override OnKill() to do something right
    // before the background thread is killed
    virtual void OnKill();
};

inline static wxWindow *GetConnectGearParentWindow(void)
{
    if (pFrame->pGearDialog->IsActive())
        return pFrame->pGearDialog;
    else
        return pFrame;
}

class ConnectMountInBg : public RunInBg
{
public:
    ConnectMountInBg(wxWindow *parent = GetConnectGearParentWindow()) : RunInBg(parent, _("Connect"), _("Connecting to Mount...")) { }
};

class ConnectAoInBg : public RunInBg
{
public:
    ConnectAoInBg(wxWindow *parent = GetConnectGearParentWindow()) : RunInBg(parent, _("Connect"), _("Connecting to AO...")) { }
};

class ConnectCameraInBg : public RunInBg
{
public:
    ConnectCameraInBg(wxWindow *parent = GetConnectGearParentWindow()) : RunInBg(parent, _("Connect"), _("Connecting to Camera...")) { }
};

class ConnectRotatorInBg : public RunInBg
{
public:
    ConnectRotatorInBg(wxWindow *parent = GetConnectGearParentWindow()) : RunInBg(parent, _("Connect"), _("Connecting to Rotator...")) { }
};

#endif // RUNINBG_INLCUDED
