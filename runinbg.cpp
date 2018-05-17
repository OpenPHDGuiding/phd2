/*
 *  runinbg.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2014-2017 Andy Galasso
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

#include <wx/progdlg.h>

struct ProgressWindow : public wxProgressDialog
{
    ProgressWindow(wxWindow *parent, const wxString& title, const wxString& message)
        : wxProgressDialog(title, message, 100, parent, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT)
    {
    }
};

struct RunInBgImpl : public wxTimer, public wxThreadHelper
{
    RunInBg *m_bg;
    wxWindow *m_parent;
    wxString m_title;
    wxString m_message;
    wxCriticalSection m_updateMsgCS;
    wxString *m_updateMsg;
    ProgressWindow *m_win;
    bool m_shown;
    volatile bool m_done;
    volatile bool m_canceled;
    wxDateTime m_showTime;
    wxString m_errorMsg;
    unsigned int m_popupDelayMillis;

    RunInBgImpl(RunInBg *bg, wxWindow *parent, const wxString& title, const wxString& message)
        : m_bg(bg),
        m_parent(parent),
        m_title(title),
        m_message(message),
        m_updateMsg(nullptr),
        m_win(0),
        m_shown(false),
        m_done(false),
        m_canceled(false),
        m_popupDelayMillis(2500)
    {
    }

    bool Run()
    {
        bool err = false;

        wxBusyCursor busy;
        if (m_parent)
            m_parent->SetCursor(wxCURSOR_WAIT); // need to do this too!
#ifndef __APPLE__
        // this makes the progress window inaccessible on OSX
        wxWindowDisabler wd;
#endif // __APPLE__

        CreateThread();
        wxThread *thread = GetThread();
        thread->Run();
        m_showTime = wxDateTime::UNow() + wxTimeSpan(0, 0, m_popupDelayMillis / 1000, m_popupDelayMillis % 1000);
        Start(250); // start timer
        while (!m_done && !m_canceled)
        {
            wxYield();
            wxMilliSleep(20);
        }
        Stop(); // stop timer
        if (m_canceled && !m_done)
        {
            // give it a bit of time to respond to cancel before killing it
            for (int i = 0; i < 50 && !m_done; i++)
            {
                wxYield();
                wxMilliSleep(20);
            }
            if (!m_done)
            {
                Debug.AddLine("Background thread did not respond to cancel... kill it");
                m_bg->OnKill();
                thread->Kill();
                thread = 0;
                err = true;
            }
        }
        if (m_canceled && m_errorMsg.empty())
            m_errorMsg = _("The operation was canceled");
        if (m_win)
        {
            delete m_win;
            m_win = 0;
        }
        if (thread)
            err = thread->Wait() ? true : false;

        if (m_parent)
            m_parent->SetCursor(wxCURSOR_ARROW);

        return err;
    }

    void SetMessage(const wxString& message)
    {
        if (m_win)
        {
            wxCriticalSectionLocker _lck(m_updateMsgCS);
            m_updateMsg = new wxString(message);
        }
        else
            m_message = message;
    }

    void Notify() // timer notification
    {
        if (!m_shown && wxDateTime::UNow() >= m_showTime)
        {
            m_win = new ProgressWindow(m_parent, m_title, m_message);
            m_shown = true;
        }
        if (m_win)
        {
            wxString *updateMsg;
            { // lock scope
                wxCriticalSectionLocker _lck(m_updateMsgCS);
                updateMsg = m_updateMsg;
                m_updateMsg = nullptr;
            }

            bool cont;
            if (updateMsg)
            {
                cont = m_win->Pulse(*updateMsg);
                m_message = *updateMsg;
                delete updateMsg;
            }
            else
                cont = m_win->Pulse();

            if (!cont)
            {
                m_canceled = true;
                Debug.AddLine("Canceled");
                m_bg->OnCancel();
                delete m_win;
                m_win = 0;
            }
        }
    }

    wxThread::ExitCode Entry()
    {
        bool err = m_bg->Entry();
        m_done = true;
        return (wxThread::ExitCode) err;
    }
};

RunInBg::RunInBg(wxWindow *parent, const wxString& title, const wxString& message)
    : m_impl(new RunInBgImpl(this, parent, title, message))
{
}

RunInBg::~RunInBg(void)
{
    delete m_impl;
}

void RunInBg::SetPopupDelay(unsigned int millis)
{
    m_impl->m_popupDelayMillis = millis;
}

bool RunInBg::Run(void)
{
    return m_impl->Run();
}

void RunInBg::SetMessage(const wxString& message)
{
    m_impl->SetMessage(message);
}

void RunInBg::SetErrorMsg(const wxString& msg)
{
    m_impl->m_errorMsg = msg;
}

wxString RunInBg::GetErrorMsg(void)
{
    return m_impl->m_errorMsg;
}

bool RunInBg::IsCanceled(void)
{
    return m_impl->m_canceled;
}

void RunInBg::OnCancel()
{
}

void RunInBg::OnKill()
{
}
