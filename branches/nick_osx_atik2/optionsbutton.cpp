/*
 *  optionsbutton.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2013 Andy Galasso
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
 *    Neither the name of Craig Stark, Stark Labs,
 *     Bret McKee, Dad Dog Development, Ltd, nor the names of its
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
#include "optionsbutton.h"

#include "icons/down_arrow.xpm"
#include "icons/down_arrow_bold.xpm"

BEGIN_EVENT_TABLE(OptionsButton, wxPanel)
    EVT_ENTER_WINDOW(OptionsButton::OnMouseEnter)
    EVT_MOTION(OptionsButton::OnMouseMove)
    EVT_LEAVE_WINDOW(OptionsButton::OnMouseLeave)
    EVT_PAINT(OptionsButton::OnPaint)
    EVT_LEFT_UP(OptionsButton::OnClick)
END_EVENT_TABLE()

enum
{
    PADX = 5,
    PADY = 5,
};

OptionsButton::OptionsButton(wxWindow *parent, wxWindowID id, const wxString& label,
                             const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    :
    wxPanel(parent, id, pos, size, style, name),
    m_highlighted(false),
    m_label(label)
{
    m_bmp = new wxBitmap(down_arrow);
    m_bmp_bold = new wxBitmap(down_arrow_bold);

    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

OptionsButton::~OptionsButton()
{
    delete m_bmp;
    delete m_bmp_bold;
}

wxSize OptionsButton::GetMinSize() const
{
    wxSize txtsz = GetTextExtent(m_label);
    return wxSize(PADX + txtsz.x + PADX + m_bmp->GetWidth() + PADX,
        PADY + txtsz.y + PADY);
}

void OptionsButton::SetLabel(const wxString& label)
{
    m_label = label;
    Refresh();
}

void OptionsButton::OnPaint(wxPaintEvent & evt)
{
    wxBufferedPaintDC dc(this);

    if (m_highlighted)
        dc.SetPen(wxPen(wxColor(0, 0, 0)));
    else
        dc.SetPen(wxPen(wxColor(56, 56, 56)));

    dc.SetBrush(wxBrush(wxColor(200, 200, 200)));

    wxSize txtsz = GetTextExtent(m_label);
    wxSize size(GetSize());
    int xbmp = size.GetWidth() - m_bmp->GetWidth() - PADX;
    wxPoint bmp_pos(xbmp, 7);
    int xtxt;
    if (GetWindowStyleFlag() & wxALIGN_CENTER_HORIZONTAL)
    {
        xtxt = (size.GetWidth() - txtsz.GetWidth()) / 2;
    }
    else
    {
        xtxt = PADX;
    }

    dc.DrawRectangle(wxPoint(0, 0), size);
    dc.SetTextBackground(wxColor(200, 200, 200));

    if (m_highlighted)
    {
        dc.SetTextForeground(wxColor(0, 0, 0));
        dc.DrawText(m_label, xtxt, PADY);
        dc.DrawBitmap(*m_bmp_bold, bmp_pos);
    }
    else
    {
        dc.SetTextForeground(wxColor(56, 56, 56));
        dc.DrawText(m_label, xtxt, PADY);
        dc.DrawBitmap(*m_bmp, bmp_pos);
    }
}

void OptionsButton::OnMouseEnter(wxMouseEvent& event)
{
    m_highlighted = true;
    Refresh();
}

void OptionsButton::OnMouseMove(wxMouseEvent& event)
{
    if (!m_highlighted)
    {
        m_highlighted = true;
        Refresh();
    }
}

void OptionsButton::OnMouseLeave(wxMouseEvent& event)
{
    m_highlighted = false;
    Refresh();
}

void OptionsButton::OnClick(wxMouseEvent& event)
{
    wxCommandEvent cmd(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
#ifdef  __WXGTK__  // Process the event as in wxgtk_button_clicked_callback()
    HandleWindowEvent(cmd);
#else
    ::wxPostEvent(GetParent(), cmd);
#endif
    m_highlighted = false;
    Refresh();
}
