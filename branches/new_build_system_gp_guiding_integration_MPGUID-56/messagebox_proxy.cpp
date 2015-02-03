/*
 *  messagebox_proxy.cpp
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

void wxMessageBoxProxy::showMessageBox(void)
{
    m_result = ::wxMessageBox(m_message, m_caption, m_style, m_parent, m_x, m_y);
    m_semaphore.Post();
}

int wxMessageBoxProxy::wxMessageBox(const wxString& message, const wxString& caption, int style, wxWindow *parent, int x, int y)
{
    int ret;

    if (wxThread::IsMain())
    {
        Debug.AddLine(wxString::Format(_T("wxMessageBoxProxy(%s)"), message));
        ret = ::wxMessageBox(message, caption, style, parent, x, y);
    }
    else
    {
        m_message = message;
        m_caption = caption;
        m_style = style;
        m_parent = parent;
        m_x = x;
        m_y = y;

        wxCommandEvent evt(WXMESSAGEBOX_PROXY_EVENT, wxID_ANY);
        evt.SetClientData(this);
        wxQueueEvent(pFrame, evt.Clone());

        // wait for the request to complete
        m_semaphore.Wait();

        ret = m_result;
    }

    return ret;
}

void MyFrame::OnMessageBoxProxy(wxCommandEvent& evt)
{
    wxMessageBoxProxy *pRequest = (wxMessageBoxProxy *)evt.GetClientData();

    pRequest->showMessageBox();
}
