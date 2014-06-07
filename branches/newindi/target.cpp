/*
 *  target.cpp
 *  PHD Guiding
 *
 *  Created by Sylvain Girard
 *  Copyright (c) 2013 Sylvain Girard
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
 *    Neither the name of Bret McKee, Dad Dog Development Ltd, nor the names of its
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
 *
 */

#include "phd.h"

static double const MIN_ZOOM = 0.25;

BEGIN_EVENT_TABLE(TargetWindow, wxWindow)
    EVT_BUTTON(BUTTON_GRAPH_LENGTH,TargetWindow::OnButtonLength)
    EVT_MENU_RANGE(MENU_LENGTH_BEGIN, MENU_LENGTH_END, TargetWindow::OnMenuLength)
    EVT_BUTTON(BUTTON_GRAPH_CLEAR,TargetWindow::OnButtonClear)
    EVT_BUTTON(BUTTON_GRAPH_ZOOMIN,TargetWindow::OnButtonZoomIn)
    EVT_BUTTON(BUTTON_GRAPH_ZOOMOUT,TargetWindow::OnButtonZoomOut)
END_EVENT_TABLE()

TargetWindow::TargetWindow(wxWindow *parent) :
    wxWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize, 0,_("Target"))
{
    //SetFont(wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
    SetBackgroundColour(*wxBLACK);

    m_visible = false;
    m_pClient = new TargetClient(this);

    wxBoxSizer *pMainSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pLeftSizer = new wxBoxSizer(wxVERTICAL);

    pMainSizer->Add(pLeftSizer);

    wxString label = wxString::Format("%3d", m_pClient->m_length);
    LengthButton = new OptionsButton(this,BUTTON_GRAPH_LENGTH,label,wxDefaultPosition,wxSize(80,-1),wxALIGN_CENTER_HORIZONTAL);
    LengthButton->SetToolTip(_("Select the number of frames of history to display"));

    wxBoxSizer *pZoomSizer = new wxBoxSizer(wxHORIZONTAL);

    ZoomInButton = new wxButton(this,BUTTON_GRAPH_ZOOMIN,_T("+"),wxDefaultPosition,wxSize(40,-1));
    ZoomInButton->SetToolTip(_("Zoom in"));

    ZoomOutButton = new wxButton(this,BUTTON_GRAPH_ZOOMOUT,_T("-"),wxDefaultPosition,wxSize(40,-1));
    ZoomOutButton->SetToolTip(_("Zoom out"));

    pZoomSizer->Add(ZoomInButton);
    pZoomSizer->Add(ZoomOutButton);

    ClearButton = new wxButton(this,BUTTON_GRAPH_CLEAR,_("Clear"),wxDefaultPosition,wxSize(80,-1));
    ClearButton->SetToolTip(_("Clear graph data"));

    pLeftSizer->Add(LengthButton, wxSizerFlags().Center().Border(wxTOP | wxRIGHT | wxLEFT,5).Expand());
    pLeftSizer->Add(pZoomSizer, wxSizerFlags().Center().Border(wxRIGHT | wxLEFT,5));
    pLeftSizer->Add(ClearButton, wxSizerFlags().Center().Border(wxRIGHT | wxLEFT,5));

    pMainSizer->Add(m_pClient, wxSizerFlags().Border(wxALL,3).Expand().Proportion(1));

    SetSizer(pMainSizer);
    pMainSizer->SetSizeHints(this);
}

TargetWindow::~TargetWindow()
{
    delete m_pClient;
}

void TargetWindow::SetState(bool is_active)
{
    m_visible = is_active;
    if (is_active)
        Refresh();
}

void TargetWindow::AppendData(const GuideStepInfo& step)
{
    m_pClient->AppendData(step);

    if (this->m_visible)
    {
        Refresh();
    }
}

void TargetWindow::OnButtonLength(wxCommandEvent& WXUNUSED(evt))
{
    wxMenu *menu = new wxMenu();

    unsigned int val = m_pClient->m_minLength;
    for (int id = MENU_LENGTH_BEGIN; id <= MENU_LENGTH_END; id++)
    {
        wxMenuItem *item = menu->AppendRadioItem(id, wxString::Format("%d", val));
        if (val == m_pClient->m_length)
            item->Check(true);
        val *= 2;
        if (val > m_pClient->m_maxLength)
            break;
    }

    PopupMenu(menu, LengthButton->GetPosition().x,
        LengthButton->GetPosition().y + LengthButton->GetSize().GetHeight());

    delete menu;
}

void TargetWindow::OnMenuLength(wxCommandEvent& evt)
{
    unsigned int val = m_pClient->m_minLength;
    for (int id = MENU_LENGTH_BEGIN; id < evt.GetId(); id++)
        val *= 2;

    m_pClient->m_length = val;

    pConfig->Global.SetInt("/target/length", val);

    LengthButton->SetLabel(wxString::Format(_T("%3d"), val));
    Refresh();
}

void TargetWindow::OnButtonClear(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_nItems = 0;
    Refresh();
}

void TargetWindow::OnButtonZoomIn(wxCommandEvent& evt)
{
    if (m_pClient->m_zoom < 3)
    {
        m_pClient->m_zoom *= 2;
        pConfig->Global.SetDouble("/target/zoom", m_pClient->m_zoom);
    }
    Refresh();
}

void TargetWindow::OnButtonZoomOut(wxCommandEvent& evt)
{
    if (m_pClient->m_zoom > MIN_ZOOM)
    {
        m_pClient->m_zoom /= 2.0;
        pConfig->Global.SetDouble("/target/zoom", m_pClient->m_zoom);
    }
    Refresh();
}

BEGIN_EVENT_TABLE(TargetClient, wxWindow)
    EVT_PAINT(TargetClient::OnPaint)
END_EVENT_TABLE()

TargetClient::TargetClient(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(201,201), wxFULL_REPAINT_ON_RESIZE )
{
    m_minLength = 50;
    m_maxLength = 400;

    m_nItems = 0;
    m_length = pConfig->Global.GetInt("/target/length", 100);
    m_zoom = pConfig->Global.GetDouble("/target/zoom", 1.0);
    if (m_zoom < MIN_ZOOM)
        m_zoom = MIN_ZOOM;
}

TargetClient::~TargetClient(void)
{
}

void TargetClient::AppendData(const GuideStepInfo& step)
{
    memmove(&m_history, &m_history[1], sizeof(m_history[0])*(m_maxHistorySize-1));

    m_history[m_maxHistorySize-1].ra = step.mountOffset->X;
    m_history[m_maxHistorySize-1].dec = step.mountOffset->Y;

    if (m_nItems < m_maxHistorySize)
    {
        m_nItems++;
    }
}

void TargetClient::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxPaintDC dc(this);

    dc.SetBackground(*wxBLACK_BRUSH);
    //dc.SetBackground(wxColour(10,0,0));
    dc.Clear();

    const double sampling = pFrame ? pFrame->GetCameraPixelScale() : 1.0;

    wxColour Grey(128,128,128);
    wxPen GreySolidPen = wxPen(Grey,1, wxSOLID);
    wxPen GreyDashPen = wxPen(Grey,1, wxDOT);

    dc.SetTextForeground(wxColour(200,200,200));
    dc.SetFont(wxFont(8,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
    dc.SetPen(GreySolidPen);
    dc.SetBrush(* wxTRANSPARENT_BRUSH);

    wxSize size = GetClientSize();
    wxPoint center(size.x/2, size.y/2);
    double radius_max = ((size.x < size.y ? size.x : size.y) - 6) / 2;

    int leftEdge     = center.x - radius_max;
    int topEdge      = center.y - radius_max;

    // Draw circles
    wxString l;
    wxSize sl;
    radius_max -= 18;

    for (int i = 1 ; i <= 4 ; i++)
    {
        double rr = radius_max / 4;
        dc.DrawCircle(center, rr*i);
        l = wxString::Format(_T("%g%s"), i/2.0 / m_zoom, sampling != 1 ? "''" : "");
        sl = dc.GetTextExtent(l);
        dc.DrawText(l, center.x - sl.x - 1, center.y - rr*i - sl.y);
    }

    // Draw axes
    dc.DrawLine(3, center.y , size.x - 3, center.y);
    dc.DrawLine(center.x, 3, center.x, size.y - 3);

    double r = radius_max / (2 / m_zoom);
    int g = size.x / 100;
    for (double x = 0 ; x < size.x ; x += r/4)
    {
        if (x != radius_max && x != r)
        {
            dc.DrawLine(center.x + x, center.y - g, center.x + x, center.y + g);
            dc.DrawLine(center.x - x, center.y - g, center.x - x, center.y + g);
        }
    }
    for (double y = 0 ; y < size.y ; y += r/4)
    {
        if (y != radius_max && y != r)
        {
            dc.DrawLine(center.x - g, center.y + y, center.x + g, center.y + y);
            dc.DrawLine(center.x - g, center.y - y, center.x + g, center.y - y);
        }
    }

    // Draw labels
    dc.DrawText(_("RA"), leftEdge, center.y - 15);
    dc.DrawText(_("Dec"), center.x + 5, topEdge - 3);

    // Draw impacts
    double scale = radius_max / 2 * sampling;
    unsigned int startPoint = m_maxHistorySize - m_length;

    if (m_nItems < m_length)
    {
        startPoint = m_maxHistorySize - m_nItems;
    }

    int dotSize = 1;

    if (startPoint == m_maxHistorySize)
    {
        dc.DrawCircle(center, dotSize);
    }

    dc.SetPen(wxPen(wxColour(127,127,255),1, wxSOLID));
    for (unsigned int i = startPoint; i < m_maxHistorySize; i++)
    {
        int ximpact = center.x + m_history[i].ra * scale * m_zoom;
        int yimpact = center.y + m_history[i].dec * scale * m_zoom;
        if (i == m_maxHistorySize - 1)
        {
            int lcrux = 4;
            dc.SetPen(*wxRED_PEN);
            dc.DrawLine(ximpact + lcrux, yimpact + lcrux, ximpact - lcrux - 1, yimpact - lcrux - 1);
            dc.DrawLine(ximpact + lcrux, yimpact - lcrux, ximpact - lcrux - 1, yimpact + lcrux + 1);
        }
        else
            dc.DrawCircle(ximpact, yimpact, dotSize);
    }
}
