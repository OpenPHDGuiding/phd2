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

    m_grid1 = new wxGrid(this, wxID_ANY);

    m_grid1->CreateGrid(4, 3);
    m_grid1->SetRowLabelSize(1);
    m_grid1->SetColLabelSize(1);
    m_grid1->EnableEditing(false);
    m_grid1->SetCellBackgroundColour(*wxBLACK);
    m_grid1->SetCellTextColour(*wxLIGHT_GREY);
    m_grid1->SetGridLineColour(wxColour(40, 40, 40));

    int col = 0;
    int row = 0;
    m_grid1->SetCellValue(row, col++, "");
    m_grid1->SetCellValue(row, col++, _("RMS"));
    m_grid1->SetCellValue(row, col++, _("Peak"));
    ++row, col = 0;
    m_grid1->SetCellValue(row, col++, _("RA"));
    m_grid1->SetCellValue(row, col++, _T(" 99.99 (99.99'')"));
    m_grid1->SetCellValue(row, col++, _(" 99.99 (99.99'')"));
    ++row, col = 0;
    m_grid1->SetCellValue(row, col++, _("Dec"));
    ++row, col = 0;
    m_grid1->SetCellValue(row, col++, _("Total"));

    m_grid1->AutoSize();
    m_grid1->SetCellValue(1, 1, "");
    m_grid1->SetCellValue(1, 2, "");
    m_grid1->ClearSelection();

    m_grid2 = new wxGrid(this, wxID_ANY);
    m_grid2->CreateGrid(7, 2);
    m_grid2->SetRowLabelSize(1);
    m_grid2->SetColLabelSize(1);
    m_grid2->EnableEditing(false);
    m_grid2->SetCellBackgroundColour(*wxBLACK);
    m_grid2->SetCellTextColour(*wxLIGHT_GREY);
    m_grid2->SetGridLineColour(wxColour(40, 40, 40));

    row = 0, col = 0;
    m_grid2->SetCellValue(row, col++, _("RA Osc"));
    ++row, col = 0;
    m_grid2->SetCellValue(row, col++, _("RA Limited"));
    ++row, col = 0;
    m_grid2->SetCellValue(row, col++, _("Dec Limited"));
    ++row, col = 0;
    m_grid2->SetCellValue(row, col++, _("Star lost"));
    ++row, col = 0;
    m_grid2->SetCellValue(row, col++, _("Declination"));
    ++row, col = 0;
    m_grid2->SetCellValue(row, col++, _("Pier Side"));
    m_grid2->SetCellValue(3, 1, _T("MMMMMM"));
    ++row, col = 0;
    m_grid2->SetCellValue(row, col++, _("Rotator Pos"));

    m_grid2->AutoSize();
    m_grid2->SetCellValue(3, 1, _T(""));
    m_grid2->ClearSelection();

    wxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);

    wxButton *clearButton = new wxButton(this, BUTTON_GRAPH_CLEAR, _("Clear"));
    clearButton->SetToolTip(_("Clear graph data and stats"));
    clearButton->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    sizer1->Add(clearButton, 0, wxALL, 10);

    m_pLengthButton = new OptionsButton(this, BUTTON_GRAPH_LENGTH, _T("XXXXXXX:888888"), wxDefaultPosition, wxSize(220, -1));
    m_pLengthButton->SetToolTip(_("Select the number of frames of history for stats and the graph"));
    m_length = dynamic_cast<MyFrame *>(parent)->pGraphLog->GetLength();
    m_pLengthButton->SetLabel(wxString::Format(_T("x:%3d"), m_length));
    sizer1->Add(m_pLengthButton, 0, wxALL, 10);

    wxSizer *sizer2 = new wxBoxSizer(wxVERTICAL);

    sizer2->Add(sizer1, 0, wxEXPAND, 10);
    sizer2->Add(m_grid1, wxSizerFlags(0).Border(wxALL, 10));
    sizer2->Add(m_grid2, wxSizerFlags(0).Border(wxALL, 10));

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

    int length = pFrame->pGraphLog->GetLength();
    if (m_length != length)
    {
        m_pLengthButton->SetLabel(wxString::Format(_T("x:%3d"), length));
        m_length = length;
    }

    const SummaryStats& stats = pFrame->pGraphLog->Stats();

    const double sampling = pFrame ? pFrame->GetCameraPixelScale() : 1.0;

    m_grid1->BeginBatch();
    m_grid2->BeginBatch();

    int row = 1, col = 1;
    m_grid1->SetCellValue(arcsecs(stats.rms_ra, sampling), row++, col);
    m_grid1->SetCellValue(arcsecs(stats.rms_dec, sampling), row++, col);
    m_grid1->SetCellValue(arcsecs(stats.rms_tot, sampling), row++, col);

    row = 1, col = 2;
    m_grid1->SetCellValue(arcsecs(stats.ra_peak, sampling), row++, col);
    m_grid1->SetCellValue(arcsecs(stats.dec_peak, sampling), row++, col);

    row = 0, col = 1;
    if (stats.osc_alert)
        m_grid2->SetCellTextColour(wxColour(185, 20, 0), row, col);
    else
        m_grid2->SetCellTextColour(*wxLIGHT_GREY, row, col);
    m_grid2->SetCellValue(wxString::Format("% .02f", stats.osc_index), row++, col);

    unsigned int historyItems = wxMax(pFrame->pGraphLog->GetHistoryItemCount(), 1); // avoid divide-by-zero
    if (stats.ra_limit_cnt > 0)
        m_grid2->SetCellTextColour(wxColour(185, 20, 0), row, col);
    else
        m_grid2->SetCellTextColour(*wxLIGHT_GREY, row, col);
    m_grid2->SetCellValue(wxString::Format(" %u (%.f%%)", stats.ra_limit_cnt, stats.ra_limit_cnt * 100. / historyItems), row++, col);

    if (stats.dec_limit_cnt > 0)
        m_grid2->SetCellTextColour(wxColour(185, 20, 0), row, col);
    else
        m_grid2->SetCellTextColour(*wxLIGHT_GREY, row, col);
    m_grid2->SetCellValue(wxString::Format(" %u (%.f%%)", stats.dec_limit_cnt, stats.dec_limit_cnt * 100. / historyItems), row++, col);

    m_grid2->SetCellValue(wxString::Format(" %u", stats.star_lost_cnt), row++, col);

    m_grid1->EndBatch();
    m_grid2->EndBatch();
}

static wxString RotatorPosStr()
{
    if (!pRotator)
        return _("N/A");
    double pos = Rotator::RotatorPosition();
    if (pos == Rotator::POSITION_UNKNOWN)
        return _("Unknown");
    else
        return wxString::Format("%5.1f", norm(pos, 0.0, 360.0));
}

void StatsWindow::UpdateScopePointing()
{
    if (pPointingSource)
    {
        double declination = pPointingSource->GetGuidingDeclination();
        PierSide pierSide = pPointingSource->SideOfPier();

        m_grid2->BeginBatch();
        int row = 4, col = 1;
        m_grid2->SetCellValue(wxString::Format("% .1f deg", degrees(declination)), row++, col);
        m_grid2->SetCellValue(Mount::PierSideStr(pierSide), row++, col);
        m_grid2->SetCellValue(RotatorPosStr(), row++, col);
        m_grid2->EndBatch();
    }
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
