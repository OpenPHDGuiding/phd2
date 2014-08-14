/*
*  statswindow.cpp
*  PHD Guiding
*
*  Created by Andy Galasso
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

#include "phd.h"
#include "statswindow.h"

wxBEGIN_EVENT_TABLE(StatsWindow, wxWindow)
    EVT_BUTTON(BUTTON_GRAPH_LENGTH, StatsWindow::OnButtonLength)
    EVT_MENU_RANGE(MENU_LENGTH_BEGIN, MENU_LENGTH_END, StatsWindow::OnMenuLength)
    EVT_BUTTON(BUTTON_GRAPH_CLEAR, StatsWindow::OnButtonClear)
wxEND_EVENT_TABLE()

StatsWindow::StatsWindow(wxWindow *parent)
    : wxWindow(parent, wxID_ANY),
    m_visible(false)
{
    SetBackgroundColour(*wxBLACK);

    m_grid = new wxGrid(this, wxID_ANY);

    m_grid->CreateGrid(7, 3);
    m_grid->SetRowLabelSize(1);
    m_grid->SetColLabelSize(1);
    m_grid->EnableEditing(false);
    m_grid->SetCellBackgroundColour(*wxBLACK);
    m_grid->SetCellTextColour(*wxLIGHT_GREY);
    m_grid->SetGridLineColour(wxColour(40, 40, 40));

    int col = 0;
    int row = 0;
    m_grid->SetCellValue("", row, col++);
    m_grid->SetCellValue(_("RMS"), row, col++);
    m_grid->SetCellValue(_("Peak"), row, col++);
    ++row, col = 0;
    m_grid->SetCellValue(_("RA"), row, col++);
    m_grid->SetCellValue(_(" MM.MM (MM.MM'')"), row, col++);
    m_grid->SetCellValue(_(" MM.MM (MM.MM'')"), row, col++);
    ++row, col = 0;
    m_grid->SetCellValue(_("Dec"), row, col++);
    ++row, col = 0;
    m_grid->SetCellValue(_("Total"), row, col++);
    row += 2, col = 0;
    m_grid->SetCellValue(_("RA Osc"), row, col++);
    ++row, col = 0;
    m_grid->SetCellValue(_("Star lost"), row, col++);

    m_grid->AutoSize();
    m_grid->ClearSelection();

    m_grid->SetCellValue(1, 1, "");
    m_grid->SetCellValue(1, 2, "");

    wxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);

    wxButton *clearButton = new wxButton(this, BUTTON_GRAPH_CLEAR, _("Clear"));
    clearButton->SetToolTip(_("Clear graph data and stats"));
    clearButton->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    sizer1->Add(clearButton, 0, wxALL, 10);

    m_pLengthButton = new OptionsButton(this, BUTTON_GRAPH_LENGTH, _T("XXXXXXX:888888"), wxDefaultPosition, wxSize(220, -1));
    m_pLengthButton->SetToolTip(_("Select the number of frames of history for stats and the graph"));
    m_pLengthButton->SetLabel(wxString::Format(_T("x:%3d"), dynamic_cast<MyFrame *>(parent)->pGraphLog->GetLength()));
    sizer1->Add(m_pLengthButton, 0, wxALL, 10);

    wxSizer *sizer2 = new wxBoxSizer(wxVERTICAL);

    sizer2->Add(sizer1, 0, wxEXPAND, 10);
    sizer2->Add(m_grid, wxSizerFlags(0).Border(wxALL, 10));

    SetSizerAndFit(sizer2);
}

StatsWindow::~StatsWindow(void)
{
}

void StatsWindow::SetState(bool is_active)
{
    m_visible = is_active;
    if (m_visible)
    {
        UpdateStats();
    }
}

static wxString arcsecs(double px, double sampling)
{
    if (sampling != 1.0)
        return wxString::Format("% 4.2f (%.2f'')", px, px * sampling);
    else
        return wxString::Format("% 4.2f", px);
}

void StatsWindow::UpdateStats(void)
{
    if (!m_visible)
        return;

    if (!pFrame || !pFrame->pGraphLog)
        return;

    m_pLengthButton->SetLabel(wxString::Format(_T("x:%3d"), pFrame->pGraphLog->GetLength()));

    const SummaryStats& stats = pFrame->pGraphLog->Stats();

    const double sampling = pFrame ? pFrame->GetCameraPixelScale() : 1.0;

    m_grid->BeginBatch();

    int row = 1, col = 1;
    m_grid->SetCellValue(arcsecs(stats.rms_ra, sampling), row++, col);
    m_grid->SetCellValue(arcsecs(stats.rms_dec, sampling), row++, col);
    m_grid->SetCellValue(arcsecs(stats.rms_tot, sampling), row++, col);
    ++row;
    if (stats.osc_alert)
        m_grid->SetCellTextColour(wxColour(185, 20, 0), row, col);
    else
        m_grid->SetCellTextColour(*wxLIGHT_GREY, row, col);
    m_grid->SetCellValue(wxString::Format("% .02f", stats.osc_index), row++, col);
    m_grid->SetCellValue(wxString::Format(" %u", stats.star_lost_cnt), row++, col);

    row = 1, col = 2;
    m_grid->SetCellValue(arcsecs(stats.ra_peak, sampling), row++, col);
    m_grid->SetCellValue(arcsecs(stats.dec_peak, sampling), row++, col);

    m_grid->EndBatch();
}

void StatsWindow::OnButtonLength(wxCommandEvent& WXUNUSED(evt))
{
    wxMenu *menu = pFrame->pGraphLog->GetLengthMenu();

    PopupMenu(menu, m_pLengthButton->GetPosition().x,
        m_pLengthButton->GetPosition().y + m_pLengthButton->GetSize().GetHeight());

    delete menu;
}

void StatsWindow::OnMenuLength(wxCommandEvent& evt)
{
    pFrame->pGraphLog->OnMenuLength(evt);
}

void StatsWindow::OnButtonClear(wxCommandEvent& evt)
{
    pFrame->pGraphLog->OnButtonClear(evt);
}
