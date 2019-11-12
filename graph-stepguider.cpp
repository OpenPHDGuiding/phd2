/*
 *  graph-stepguider.cpp
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
 */

#include "phd.h"
#include <assert.h>
#include <wx/dcbuffer.h>
#include <wx/utils.h>
#include <wx/colordlg.h>

class GraphStepguiderClient : public wxWindow
{
public:

    static const unsigned int m_maxHistorySize = 64;

    struct
    {
        int dx;
        int dy;
    } m_history[m_maxHistorySize];

    PHD_Point m_avgPos;
    PHD_Point m_curBump;

    wxPen   *m_pPens[m_maxHistorySize];
    wxBrush *m_pBrushes[m_maxHistorySize];

    unsigned int m_nItems;    // # of items in the history
    unsigned int m_length;     // # of items to display

    int m_xMax;
    int m_yMax;

    int m_xBump;
    int m_yBump;

    void OnPaint(wxPaintEvent& evt);

    GraphStepguiderClient(wxWindow *parent);
    virtual ~GraphStepguiderClient(void);

    void SetLimits(unsigned int xMax, unsigned int yMax, unsigned int xBump, unsigned int yBump);
    void AppendData(const wxPoint& pos, const PHD_Point& avgPos);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(GraphStepguiderWindow, wxWindow)
    EVT_BUTTON(BUTTON_GRAPH_LENGTH,GraphStepguiderWindow::OnButtonLength)
    EVT_MENU_RANGE(MENU_LENGTH_BEGIN, MENU_LENGTH_END, GraphStepguiderWindow::OnMenuLength)
    EVT_BUTTON(BUTTON_GRAPH_CLEAR,GraphStepguiderWindow::OnButtonClear)
END_EVENT_TABLE()

GraphStepguiderWindow::GraphStepguiderWindow(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, _("AO Position"))
{
    SetBackgroundColour(*wxBLACK);

    m_visible = false;
    m_pClient = new GraphStepguiderClient(this);

    wxBoxSizer *pMainSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pLeftSizer = new wxBoxSizer(wxVERTICAL);

    pMainSizer->Add(pLeftSizer, wxSizerFlags().Expand());

    m_pClient->m_length = pConfig->Global.GetInt("/graph_stepguider/length", 1);
    wxString label = wxString::Format(_T("%3d"),m_pClient->m_length);
    LengthButton = new OptionsButton(this,BUTTON_GRAPH_LENGTH,label,wxPoint(10,10),wxSize(80,-1),wxALIGN_CENTER_HORIZONTAL);
    LengthButton->SetToolTip(_("Select the number of frames of history to display"));
    pLeftSizer->Add(LengthButton, wxSizerFlags().Border(wxALL, 3).Expand());

    ClearButton = new wxButton(this,BUTTON_GRAPH_CLEAR,_("Clear"),wxPoint(10,100),wxSize(80,-1));
    ClearButton->SetToolTip(_("Clear graph data"));
    pLeftSizer->Add(ClearButton, wxSizerFlags().Center().Border(wxALL, 3));

    pLeftSizer->AddStretchSpacer();

    m_hzText = new wxStaticText(this, wxID_ANY, wxEmptyString/*wxString::Format(_("%.1f Hz"), 999.9)*/, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxST_NO_AUTORESIZE);
    m_hzText->SetForegroundColour(*wxLIGHT_GREY);
    m_hzText->SetBackgroundColour(*wxBLACK);
    pLeftSizer->Add(m_hzText, wxSizerFlags().Border(wxALL, 3).Expand());

    pMainSizer->Add(m_pClient, wxSizerFlags().Border(wxALL,3).Expand().Proportion(1));

    SetSizer(pMainSizer);
    pMainSizer->SetSizeHints(this);

    Refresh();
}

GraphStepguiderWindow::~GraphStepguiderWindow()
{
    delete m_pClient;
}

void GraphStepguiderWindow::OnButtonLength(wxCommandEvent& WXUNUSED(evt))
{
    wxMenu *menu = new wxMenu();

    unsigned int val = 1;
    for (int id = MENU_LENGTH_BEGIN; id <= MENU_LENGTH_END; id++)
    {
        wxMenuItem *item = menu->AppendRadioItem(id, wxString::Format("%d", val));
        if (val == m_pClient->m_length)
            item->Check(true);
        val *= 4;
        if (val > 64)
            break;
    }

    PopupMenu(menu, LengthButton->GetPosition().x,
        LengthButton->GetPosition().y + LengthButton->GetSize().GetHeight());

    delete menu;
}

void GraphStepguiderWindow::OnMenuLength(wxCommandEvent& evt)
{
    unsigned int val = 1;
    for (int id = MENU_LENGTH_BEGIN; id < evt.GetId(); id++)
        val *= 4;

    m_pClient->m_length = val;

    LengthButton->SetLabel(wxString::Format(_T("%3d"), val));

    pConfig->Global.SetInt("/graph_stepguider/length", val);

    Refresh();
}

bool GraphStepguiderWindow::SetState(bool is_active)
{
    m_visible = is_active;

    if (m_visible)
    {
        Refresh();
    }

    return m_visible;
}

void GraphStepguiderWindow::SetLimits(unsigned int xMax, unsigned int yMax,
                                      unsigned int xBump, unsigned int yBump)
{
    assert(wxThread::IsMain());

    m_pClient->SetLimits(xMax, yMax, xBump, yBump);

    if (m_visible)
    {
        Refresh();
    }
}

void GraphStepguiderWindow::OnButtonClear(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_nItems = 0;

    if (m_visible)
    {
        Refresh();
    }
}

void GraphStepguiderWindow::AppendData(const wxPoint& pos, const PHD_Point& avgPos)
{
    assert(wxThread::IsMain());

    wxLongLong_t now = ::wxGetUTCTimeMillis().GetValue();

    if (m_pClient->m_nItems > 0)
    {
        double hz = 1000.0 / static_cast<double>(now - m_prevTimestamp);
        m_hzText->SetLabel(wxString::Format(_("%.1f Hz"), hz));
    }
    else
        m_hzText->SetLabel(wxEmptyString);

    m_prevTimestamp = now;

    m_pClient->AppendData(pos, avgPos);

    if (m_visible)
    {
        Refresh();
    }
}

void GraphStepguiderWindow::ShowBump(const PHD_Point& curBump)
{
    assert(wxThread::IsMain());

    m_pClient->m_curBump = curBump;

    if (m_visible)
    {
        Refresh();
    }
}

BEGIN_EVENT_TABLE(GraphStepguiderClient, wxWindow)
  EVT_PAINT(GraphStepguiderClient::OnPaint)
END_EVENT_TABLE()

GraphStepguiderClient::GraphStepguiderClient(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(201,201), wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    m_nItems = 0;
    m_length = 1;

    for (unsigned int i = 0; i < m_maxHistorySize; i++)
    {
        int color = (int)(i * 255 / m_maxHistorySize);
        m_pPens[i] = new wxPen(wxColor(color,color,color));
        m_pBrushes[i] = new wxBrush(wxColor(color,color,color), wxBRUSHSTYLE_SOLID);
    }

    SetLimits(0, 0, 0, 0);
}

GraphStepguiderClient::~GraphStepguiderClient(void)
{
    for (unsigned int i = 0; i < m_maxHistorySize; i++)
    {
        delete m_pBrushes[i];
        delete m_pPens[i];
    }
}

void GraphStepguiderClient::SetLimits(unsigned int xMax, unsigned int yMax,
                                      unsigned int xBump, unsigned int yBump)
{
    m_xMax = xMax;
    m_yMax = yMax;

    m_xBump = xBump;
    m_yBump = yBump;
}

void GraphStepguiderClient::AppendData(const wxPoint& pos, const PHD_Point& avgPos)
{
    memmove(&m_history, &m_history[1], sizeof(m_history[0]) * (m_maxHistorySize - 1));

    m_history[m_maxHistorySize - 1].dx = pos.x;
    m_history[m_maxHistorySize - 1].dy = pos.y;

    if (m_nItems < m_maxHistorySize)
    {
        ++m_nItems;
    }

    m_avgPos = avgPos;
}

void GraphStepguiderClient::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxAutoBufferedPaintDC dc(this);

    dc.SetBackground(*wxBLACK_BRUSH);
    //dc.SetBackground(wxColour(10,0,0));
    dc.Clear();

    wxSize size = GetClientSize();

    if (m_xMax == 0 || m_yMax == 0)
    {
        wxString txt(_("AO not connected"));
        wxSize txtsize = dc.GetTextExtent(txt);
        dc.SetTextForeground(*wxRED);
        dc.DrawText(txt, (size.x - txtsize.x) / 2, (size.y - txtsize.y) / 2);
        return;
    }

    wxPen GreySolidPen = wxPen(wxColour(200,200,200),2, wxPENSTYLE_SOLID);
    wxPen GreyDashPen = wxPen(wxColour(200,200,200),1, wxPENSTYLE_DOT);

    const int stepsPerDivision = 10;

    wxPoint center(size.x / 2, size.y / 2);

    int xSteps = ((m_xMax + stepsPerDivision - 1) / stepsPerDivision) * stepsPerDivision;
    int xDivisions = xSteps / stepsPerDivision;
    int xPixelsPerStep = (size.x - 1) / (2 * xSteps);

    int ySteps = ((m_yMax + stepsPerDivision - 1) / stepsPerDivision) * stepsPerDivision;
    int yDivisions = ySteps / stepsPerDivision;
    int yPixelsPerStep = (size.y - 1) / (2 * ySteps);

    int leftEdge     = center.x - xDivisions * stepsPerDivision * xPixelsPerStep;
    int rightEdge    = center.x + xDivisions * stepsPerDivision * xPixelsPerStep;

    int topEdge      = center.y - yDivisions * stepsPerDivision * yPixelsPerStep;
    int bottomEdge   = center.y + yDivisions * stepsPerDivision * yPixelsPerStep;

    // Draw axes
    dc.SetPen(GreySolidPen);

    dc.DrawLine(leftEdge, center.y , rightEdge, center.y);
    dc.DrawLine(center.x, topEdge, center.x, bottomEdge);

    // Draw divisions
    dc.SetPen(GreyDashPen);

    for (int division = 1; division <= xDivisions; division++)
    {
        int pos = stepsPerDivision * xPixelsPerStep * division;
        dc.DrawLine(center.x - pos, topEdge, center.x - pos, bottomEdge);
        dc.DrawLine(center.x + pos, topEdge, center.x + pos, bottomEdge);
    }

    for (int division = 1; division <= yDivisions; division++)
    {
        int pos = stepsPerDivision * yPixelsPerStep * division;
        dc.DrawLine(leftEdge, center.y - pos, rightEdge, center.y - pos);
        dc.DrawLine(leftEdge, center.y + pos, rightEdge, center.y + pos);
    }

    int xOffset = m_xBump * xPixelsPerStep;
    int yOffset = m_yBump * yPixelsPerStep;

    dc.SetPen(*wxYELLOW_PEN);
    dc.DrawLine(center.x - xOffset, center.y - yOffset, center.x + xOffset, center.y - yOffset);
    dc.DrawLine(center.x + xOffset, center.y - yOffset, center.x + xOffset, center.y + yOffset);
    dc.DrawLine(center.x + xOffset, center.y + yOffset, center.x - xOffset, center.y + yOffset);
    dc.DrawLine(center.x - xOffset, center.y + yOffset, center.x - xOffset, center.y - yOffset);

    xOffset = m_xMax * xPixelsPerStep;
    yOffset = m_yMax * yPixelsPerStep;

    dc.SetPen(*wxRED_PEN);
    dc.DrawLine(center.x - xOffset, center.y - yOffset, center.x + xOffset, center.y - yOffset);
    dc.DrawLine(center.x + xOffset, center.y - yOffset, center.x + xOffset, center.y + yOffset);
    dc.DrawLine(center.x + xOffset, center.y + yOffset, center.x - xOffset, center.y + yOffset);
    dc.DrawLine(center.x - xOffset, center.y + yOffset, center.x - xOffset, center.y - yOffset);

    // draw ra-dec directions
    if (pMount && pMount->IsCalibrated() && pSecondaryMount && pSecondaryMount->IsCalibrated())
    {
        double const LEN = 8.0;
        PHD_Point mnt, cam, ao;

        // RA vector
        mnt.SetXY(1., 0.);
        pSecondaryMount->TransformMountCoordinatesToCameraCoordinates(mnt, cam, false);
        pMount->TransformCameraCoordinatesToMountCoordinates(cam, ao, false);
        ao *= LEN / ao.Distance();
        ao.X *= xPixelsPerStep;
        ao.Y *= yPixelsPerStep;
        dc.SetPen(wxPen(pFrame->pGraphLog->GetRaOrDxColor(), 1, wxPENSTYLE_DOT));
        dc.DrawLine(center.x - ao.X, center.y - ao.Y, center.x + ao.X, center.y + ao.Y);

        // Dec vector
        mnt.SetXY(0., 1.);
        pSecondaryMount->TransformMountCoordinatesToCameraCoordinates(mnt, cam, false);
        pMount->TransformCameraCoordinatesToMountCoordinates(cam, ao, false);
        ao *= LEN / ao.Distance();
        ao.X *= xPixelsPerStep;
        ao.Y *= yPixelsPerStep;
        dc.SetPen(wxPen(pFrame->pGraphLog->GetDecOrDyColor(), 1, wxPENSTYLE_DOT));
        dc.DrawLine(center.x - ao.X, center.y - ao.Y, center.x + ao.X, center.y + ao.Y);
    }

    dc.SetPen(*wxWHITE_PEN);

    unsigned int startPoint = m_maxHistorySize - m_length;

    if (m_nItems < m_length)
    {
        startPoint = m_maxHistorySize - m_nItems;
    }

    int dotSize = (xPixelsPerStep > yPixelsPerStep ? yPixelsPerStep : xPixelsPerStep) / 2;

    if (startPoint == m_maxHistorySize)
    {
        dc.DrawCircle(center, dotSize);
    }

    for (unsigned int i = startPoint; i < m_maxHistorySize; i++)
    {
        if (i == m_maxHistorySize-1)
        {
            dotSize *= 2;
        }
        dc.SetPen(*m_pPens[i]);
        dc.SetBrush(*m_pBrushes[i]);
        dc.DrawCircle(center.x + m_history[i].dx * xPixelsPerStep, center.y + m_history[i].dy * yPixelsPerStep, dotSize);
    }

    if (m_avgPos.IsValid())
    {
        if (m_curBump.IsValid())
            dc.SetPen(*wxRED_PEN);
        else
            dc.SetPen(*wxGREEN_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);

        dc.DrawCircle(center.x + (int)(m_avgPos.X * xPixelsPerStep),
                      center.y + (int)(m_avgPos.Y * yPixelsPerStep), dotSize);

        if (m_curBump.IsValid())
        {
            dc.SetPen(*wxGREEN_PEN);
            dc.DrawLine(center.x + (int)(m_avgPos.X * xPixelsPerStep),
                    center.y + (int)(m_avgPos.Y * yPixelsPerStep),
                    center.x + (int)((m_avgPos.X + m_curBump.X * 2.0) * xPixelsPerStep),
                    center.y + (int)((m_avgPos.Y + m_curBump.Y * 2.0) * yPixelsPerStep));
        }
    }
}
