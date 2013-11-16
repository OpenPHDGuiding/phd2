/*
 *  graph.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2008-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Seriously modified by Bret McKee
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
#include <wx/dcbuffer.h>
#include <wx/utils.h>
#include <wx/colordlg.h>

static const int DefaultMinLength =  50;
static const int DefaultMaxLength = 400;
static const int DefaultMinHeight =  1;
static const int DefaultMaxHeight = 16;

BEGIN_EVENT_TABLE(GraphLogWindow, wxWindow)
    EVT_PAINT(GraphLogWindow::OnPaint)
    EVT_BUTTON(BUTTON_GRAPH_SETTINGS,GraphLogWindow::OnButtonSettings)
    EVT_MENU_RANGE(GRAPH_RADEC, GRAPH_DXDY, GraphLogWindow::OnRADecDxDy)
    EVT_MENU_RANGE(GRAPH_ARCSECS, GRAPH_PIXELS, GraphLogWindow::OnArcsecsPixels)
    EVT_MENU(GRAPH_RADX_COLOR, GraphLogWindow::OnRADxColor)
    EVT_MENU(GRAPH_DECDY_COLOR, GraphLogWindow::OnDecDyColor)
    EVT_BUTTON(BUTTON_GRAPH_LENGTH,GraphLogWindow::OnButtonLength)
    EVT_MENU_RANGE(MENU_LENGTH_BEGIN, MENU_LENGTH_END, GraphLogWindow::OnMenuLength)
    EVT_BUTTON(BUTTON_GRAPH_HEIGHT,GraphLogWindow::OnButtonHeight)
    EVT_MENU_RANGE(MENU_HEIGHT_BEGIN, MENU_HEIGHT_END, GraphLogWindow::OnMenuHeight)
    EVT_BUTTON(BUTTON_GRAPH_CLEAR,GraphLogWindow::OnButtonClear)
    EVT_CHECKBOX(CHECKBOX_GRAPH_TRENDLINES,GraphLogWindow::OnCheckboxTrendlines)
    EVT_CHECKBOX(CHECKBOX_GRAPH_CORRECTIONS,GraphLogWindow::OnCheckboxCorrections)
END_EVENT_TABLE()

#ifdef __WXOSX__
# define OSX_SMALL_FONT(lbl) do { (lbl)->SetFont(*wxSMALL_FONT); } while (0)
#else
# define OSX_SMALL_FONT(lbl)
#endif

GraphLogWindow::GraphLogWindow(wxWindow *parent) :
    wxWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize, wxFULL_REPAINT_ON_RESIZE,_T("Graph"))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    wxBoxSizer *pMainSizer   = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *pClientSizer = new wxBoxSizer(wxVERTICAL);

    m_pClient = new GraphLogClientWindow(this);

    m_pControlSizer = new wxBoxSizer(wxHORIZONTAL);

    if (pMount)
    {
        m_pXControlPane = pMount->GetXGuideAlgorithmControlPane(this);
        m_pYControlPane = pMount->GetYGuideAlgorithmControlPane(this);
        m_pScopePane    = pMount->GetGraphControlPane(this, _("Scope:"));
    }
    else
    {
        m_pXControlPane = NULL;
        m_pYControlPane = NULL;
        m_pScopePane    = NULL;
    }

    if (m_pXControlPane != NULL)
    {
        m_pControlSizer->Add(m_pXControlPane, wxSizerFlags().Expand());
    }

    if (m_pYControlPane != NULL)
    {
        m_pControlSizer->Add(m_pYControlPane, wxSizerFlags().Expand());
    }

    if (m_pScopePane != NULL)
    {
        m_pControlSizer->Add(m_pScopePane, wxSizerFlags().Expand());
    }


    m_visible = false;

    SetBackgroundColour(*wxBLACK);

    m_pLengthButton = new OptionsButton(this,BUTTON_GRAPH_LENGTH, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    m_pLengthButton->SetToolTip(_("Select the number of frames of history to display on the X-axis"));
    m_pLengthButton->SetLabel(wxString::Format(_T("x:%3d"), m_pClient->m_length));
    pButtonSizer->Add(m_pLengthButton, wxSizerFlags().Border(wxTOP, 5).Expand());

    m_pHeightButton = new OptionsButton(this,BUTTON_GRAPH_HEIGHT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    m_heightButtonLabelVal = 0;
    UpdateHeightButtonLabel();
    pButtonSizer->Add(m_pHeightButton, wxSizerFlags().Expand());

    m_pSettingsButton = new OptionsButton(this, BUTTON_GRAPH_SETTINGS, _("Settings"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    m_pSettingsButton->SetToolTip(_("Graph settings"));
    pButtonSizer->Add(m_pSettingsButton, wxSizerFlags().Expand());

    m_pClearButton = new wxButton(this,BUTTON_GRAPH_CLEAR,_("Clear"));
    m_pClearButton->SetToolTip(_("Clear graph data"));
    m_pClearButton->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    pButtonSizer->Add(m_pClearButton, wxSizerFlags().Expand());

    m_pCheckboxTrendlines = new wxCheckBox(this,CHECKBOX_GRAPH_TRENDLINES,_("Trendlines"));
#if defined(__WXOSX__)
    // workaround inability to set checkbox foreground color
    m_pCheckboxTrendlines->SetBackgroundColour(wxColor(200, 200, 200));
#else
    m_pCheckboxTrendlines->SetForegroundColour(*wxLIGHT_GREY);
#endif
    m_pCheckboxTrendlines->SetToolTip(_("Plot trend lines"));
    pButtonSizer->Add(m_pCheckboxTrendlines, wxSizerFlags().Expand().Border(wxTOP, 1));

    m_pCheckboxCorrections = new wxCheckBox(this,CHECKBOX_GRAPH_CORRECTIONS,_("Corrections"));
#if defined(__WXOSX__)
    // workaround inability to set checkbox foreground color
    m_pCheckboxCorrections->SetBackgroundColour(wxColor(200, 200, 200));
#else
    m_pCheckboxCorrections->SetForegroundColour(*wxLIGHT_GREY);
#endif
    m_pCheckboxCorrections->SetToolTip(_("Display mount corrections"));
    m_pCheckboxCorrections->SetValue(m_pClient->m_showCorrections);
    pButtonSizer->Add(m_pCheckboxCorrections, wxSizerFlags().Expand());

    wxBoxSizer *pLabelSizer = new wxBoxSizer(wxHORIZONTAL);

    m_pLabel1 = new wxStaticText(this, wxID_ANY, _("RA"));
    m_pLabel1->SetForegroundColour(m_pClient->m_raOrDxColor);
    m_pLabel1->SetBackgroundColour(*wxBLACK);
    pLabelSizer->Add(m_pLabel1, wxSizerFlags().Left());

    m_pLabel2 = new wxStaticText(this, wxID_ANY, _("Dec"));
    m_pLabel2->SetForegroundColour(m_pClient->m_decOrDyColor);
    m_pLabel2->SetBackgroundColour(*wxBLACK);

    pLabelSizer->AddStretchSpacer();
    pLabelSizer->Add(m_pLabel2, wxSizerFlags().Right());

    pButtonSizer->Add(pLabelSizer, wxSizerFlags().Expand());

    wxStaticText *lbl;

    lbl = new wxStaticText(this, wxID_ANY, _("RMS Error:"), wxDefaultPosition, wxDefaultSize);
    OSX_SMALL_FONT(lbl);
    lbl->SetForegroundColour(*wxLIGHT_GREY);
    lbl->SetBackgroundColour(*wxBLACK);
    pButtonSizer->Add(lbl);

    wxSize size1 = GetTextExtent(_T("XXXX"));

    wxBoxSizer *szRaRMS = new wxBoxSizer(wxHORIZONTAL);
    lbl = new wxStaticText(this, wxID_ANY, _("RA"), wxDefaultPosition, size1, wxALIGN_RIGHT);
    OSX_SMALL_FONT(lbl);
    lbl->SetForegroundColour(*wxLIGHT_GREY);
    lbl->SetBackgroundColour(*wxBLACK);
    m_pClient->m_pRaRMS = new wxStaticText(this, wxID_ANY, _T("0.00"), wxDefaultPosition, wxSize(80,-1));
    OSX_SMALL_FONT(m_pClient->m_pRaRMS);
    m_pClient->m_pRaRMS->SetForegroundColour(*wxLIGHT_GREY);
    m_pClient->m_pRaRMS->SetBackgroundColour(*wxBLACK);
    szRaRMS->Add(lbl, wxSizerFlags().Border(wxRIGHT, 5));
    szRaRMS->Add(m_pClient->m_pRaRMS);
    pButtonSizer->Add(szRaRMS);

    wxBoxSizer *szDecRMS = new wxBoxSizer(wxHORIZONTAL);
    lbl = new wxStaticText(this, wxID_ANY, _("Dec"), wxDefaultPosition, size1, wxALIGN_RIGHT);
    OSX_SMALL_FONT(lbl);
    lbl->SetForegroundColour(*wxLIGHT_GREY);
    lbl->SetBackgroundColour(*wxBLACK);
    m_pClient->m_pDecRMS = new wxStaticText(this, wxID_ANY, _T("0.00"), wxDefaultPosition, wxSize(80,-1));
    OSX_SMALL_FONT(m_pClient->m_pDecRMS);
    m_pClient->m_pDecRMS->SetForegroundColour(*wxLIGHT_GREY);
    m_pClient->m_pDecRMS->SetBackgroundColour(*wxBLACK);
    szDecRMS->Add(lbl, wxSizerFlags().Border(wxRIGHT, 5));
    szDecRMS->Add(m_pClient->m_pDecRMS);
    pButtonSizer->Add(szDecRMS);

    wxBoxSizer *szTotRMS = new wxBoxSizer(wxHORIZONTAL);
    lbl = new wxStaticText(this, wxID_ANY, _("Tot"), wxDefaultPosition, size1, wxALIGN_RIGHT);
    OSX_SMALL_FONT(lbl);
    lbl->SetForegroundColour(*wxLIGHT_GREY);
    lbl->SetBackgroundColour(*wxBLACK);
    m_pClient->m_pTotRMS = new wxStaticText(this, wxID_ANY, _T("0.00"), wxDefaultPosition, wxSize(80,-1));
    OSX_SMALL_FONT(m_pClient->m_pTotRMS);
    m_pClient->m_pTotRMS->SetForegroundColour(*wxLIGHT_GREY);
    m_pClient->m_pTotRMS->SetBackgroundColour(*wxBLACK);
    szTotRMS->Add(lbl, wxSizerFlags().Border(wxRIGHT, 5));
    szTotRMS->Add(m_pClient->m_pTotRMS);
    pButtonSizer->Add(szTotRMS);

    m_pClient->m_pOscIndex = new wxStaticText(this, wxID_ANY, _("RA Osc: 0.00"));
    OSX_SMALL_FONT(m_pClient->m_pOscIndex);
    m_pClient->m_pOscIndex->SetForegroundColour(*wxLIGHT_GREY);
    m_pClient->m_pOscIndex->SetBackgroundColour(*wxBLACK);
    pButtonSizer->Add(m_pClient->m_pOscIndex);

    pClientSizer->Add(m_pClient, wxSizerFlags().Expand().Proportion(1));
    pClientSizer->Add(m_pControlSizer, wxSizerFlags().Expand().Border(wxALL, 10));
    pMainSizer->Add(pButtonSizer, wxSizerFlags().Left().DoubleHorzBorder().Expand());
    pMainSizer->Add(pClientSizer, wxSizerFlags().Expand().Proportion(1));

    SetSizer(pMainSizer);
    pMainSizer->SetSizeHints(this);
}

GraphLogWindow::~GraphLogWindow()
{
    delete m_pClient;
}

wxColor GraphLogWindow::GetRaOrDxColor(void)
{
    return m_pClient->m_raOrDxColor;
}

wxColor GraphLogWindow::GetDecOrDyColor(void)
{
    return m_pClient->m_decOrDyColor;
}

int GraphLogWindow::StringWidth(const wxString& string)
{
    int width, height;

    GetParent()->GetTextExtent(string, &width, &height);

    return width;
}

void GraphLogWindow::OnButtonSettings(wxCommandEvent& WXUNUSED(evt))
{
    wxMenu *menu = new wxMenu();
    wxMenuItem *item1, *item2;

    // setup RA/Dec / dx/dy items
    item1 = menu->Append(wxID_ANY, _("Plot mode"));
    item1->Enable(false);
    item1 = menu->AppendRadioItem(GRAPH_RADEC, _("RA / Dec"));
    item2 = menu->AppendRadioItem(GRAPH_DXDY, _("dx / dy"));
    if (m_pClient->m_mode == GraphLogClientWindow::MODE_RADEC)
        item1->Check();
    else
        item2->Check();
    menu->AppendSeparator();

    // setup Arcses / pixels items
    item1 = menu->Append(wxID_ANY, _("Y-axis units"));
    item1->Enable(false);
    item1 = menu->AppendRadioItem(GRAPH_ARCSECS, _("Arc-seconds"));
    bool enable_arcsecs = pFrame->GetCameraPixelScale() != 1.0;
    if (!enable_arcsecs)
        item1->Enable(false);
    item2 = menu->AppendRadioItem(GRAPH_PIXELS, _("Pixels"));
    if (m_pClient->m_heightUnits == UNIT_ARCSEC && enable_arcsecs)
        item1->Check();
    else
        item2->Check();
    menu->AppendSeparator();

    // setup color selection items
    if (m_pClient->m_mode == GraphLogClientWindow::MODE_RADEC)
    {
        menu->Append(GRAPH_RADX_COLOR, _("RA Color..."));
        menu->Append(GRAPH_DECDY_COLOR, _("Dec Color..."));
    }
    else
    {
        menu->Append(GRAPH_RADX_COLOR, _("dx Color..."));
        menu->Append(GRAPH_DECDY_COLOR, _("dy Color..."));
    }

    PopupMenu(menu, m_pSettingsButton->GetPosition().x,
        m_pSettingsButton->GetPosition().y + m_pSettingsButton->GetSize().GetHeight());

    delete menu;
}

void GraphLogWindow::OnRADecDxDy(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
    case GRAPH_DXDY:
        m_pClient->m_mode = GraphLogClientWindow::MODE_DXDY;
        break;
    case GRAPH_RADEC:
        m_pClient->m_mode = GraphLogClientWindow::MODE_RADEC;
        break;
    }
    pConfig->Global.SetInt ("/graph/ScopeOrCameraUnits", (int)m_pClient->m_mode);
    Refresh();
}

void GraphLogWindow::OnArcsecsPixels(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
    case GRAPH_ARCSECS:
        m_pClient->m_heightUnits = UNIT_ARCSEC;
        break;
    case GRAPH_PIXELS:
        m_pClient->m_heightUnits = UNIT_PIXELS;
        break;
    }
    pConfig->Global.SetInt ("/graph/HeightUnits", (int)m_pClient->m_heightUnits);
    Refresh();
}

void GraphLogWindow::OnRADxColor(wxCommandEvent& evt)
{
    wxColourData cdata;
    cdata.SetColour(m_pClient->m_raOrDxColor);
    wxColourDialog cdialog(this, &cdata);
    cdialog.SetTitle(_("RA or dx Color"));
    if (cdialog.ShowModal() == wxID_OK)
    {
        cdata = cdialog.GetColourData();
        m_pClient->m_raOrDxColor = cdata.GetColour();
        pConfig->Global.SetString("/graph/RAColor", m_pClient->m_raOrDxColor.GetAsString(wxC2S_HTML_SYNTAX));
        m_pLabel1->SetForegroundColour(m_pClient->m_raOrDxColor);
        Refresh();
    }
}

void GraphLogWindow::OnDecDyColor(wxCommandEvent& evt)
{
    wxColourData cdata;
    cdata.SetColour(m_pClient->m_decOrDyColor);
    wxColourDialog cdialog(this, &cdata);
    cdialog.SetTitle(_("Dec or dy Color"));
    if (cdialog.ShowModal() == wxID_OK)
    {
        cdata = cdialog.GetColourData();
        m_pClient->m_decOrDyColor = cdata.GetColour();
        pConfig->Global.SetString("/graph/DecColor", m_pClient->m_decOrDyColor.GetAsString(wxC2S_HTML_SYNTAX));
        m_pLabel2->SetForegroundColour(m_pClient->m_decOrDyColor);
        Refresh();
    }
}

void GraphLogWindow::OnButtonLength(wxCommandEvent& WXUNUSED(evt))
{
    wxMenu *menu = new wxMenu();
    unsigned int val = m_pClient->m_minLength;
    for (int id = MENU_LENGTH_BEGIN; id <= MENU_LENGTH_END; id++)
    {
        wxMenuItem *item = menu->AppendRadioItem(id, wxString::Format("%d", val));
        if (val == m_pClient->m_length)
            item->Check(true);
        val *= 2;
        if (val > m_pClient->m_history.capacity())
            break;
    }

    PopupMenu(menu, m_pLengthButton->GetPosition().x,
        m_pLengthButton->GetPosition().y + m_pLengthButton->GetSize().GetHeight());

    delete menu;
}

void GraphLogWindow::OnMenuLength(wxCommandEvent& evt)
{
    unsigned int val = m_pClient->m_minLength;
    for (int id = MENU_LENGTH_BEGIN; id < evt.GetId(); id++)
        val *= 2;

    m_pClient->m_length = val;

    m_pClient->RecalculateTrendLines();

    pConfig->Global.SetInt("/graph/length", val);

    m_pLengthButton->SetLabel(wxString::Format(_T("x:%3d"), val));

    Refresh();
}

void GraphLogWindow::OnButtonHeight(wxCommandEvent& WXUNUSED(evt))
{
    wxMenu *menu = new wxMenu();

    unsigned int val = m_pClient->m_minHeight;
    for (int id = MENU_HEIGHT_BEGIN; id <= MENU_HEIGHT_END; id++)
    {
        wxMenuItem *item = menu->AppendRadioItem(id, wxString::Format("%d", val));
        if (val == m_pClient->m_height)
            item->Check(true);
        val *= 2;
        if (val > m_pClient->m_maxHeight)
            break;
    }

    PopupMenu(menu, m_pHeightButton->GetPosition().x,
        m_pHeightButton->GetPosition().y + m_pHeightButton->GetSize().GetHeight());

    delete menu;
}

void GraphLogWindow::OnMenuHeight(wxCommandEvent& evt)
{
    unsigned int val = m_pClient->m_minHeight;
    for (int id = MENU_HEIGHT_BEGIN; id < evt.GetId(); id++)
        val *= 2;

    m_pClient->m_height = val;

    pConfig->Global.SetInt("/graph/height", m_pClient->m_height);

    UpdateHeightButtonLabel();

    Refresh();
}

void GraphLogWindow::SetState(bool is_active)
{
    this->m_visible = is_active;
    if (is_active)
    {
        UpdateControls();
    }
    this->Show(is_active);
}

void GraphLogWindow::EnableTrendLines(bool enable)
{
    m_pCheckboxTrendlines->SetValue(enable);
    wxCommandEvent dummy;
    OnCheckboxTrendlines(dummy);
}

void GraphLogWindow::AppendData(const GuideStepInfo& step)
{
    m_pClient->AppendData(step);

    if (m_visible)
    {
        Refresh();
    }
}

void GraphLogWindow::UpdateControls()
{
    if (m_pXControlPane != NULL)
    {
        m_pControlSizer->Detach(m_pXControlPane);
        m_pXControlPane->Destroy();
    }

    if (m_pYControlPane != NULL)
    {
        m_pControlSizer->Detach(m_pYControlPane);
        m_pYControlPane->Destroy();
    }

    if (m_pScopePane != NULL)
    {
        m_pControlSizer->Detach(m_pScopePane);
        m_pScopePane->Destroy();
    }

    if (pMount && pMount->IsConnected())
    {
        m_pXControlPane = pMount->GetXGuideAlgorithmControlPane(this);
        m_pYControlPane = pMount->GetYGuideAlgorithmControlPane(this);
        m_pScopePane    = pMount->GetGraphControlPane(this, _("Scope:"));
    }
    else
    {
        m_pXControlPane = NULL;
        m_pYControlPane = NULL;
        m_pScopePane    = NULL;
    }

    if (m_pXControlPane != NULL)
    {
        m_pControlSizer->Add(m_pXControlPane, wxSizerFlags().Expand());
    }

    if (m_pYControlPane != NULL)
    {
        m_pControlSizer->Add(m_pYControlPane, wxSizerFlags().Expand());
    }

    if (m_pScopePane != NULL)
    {
        m_pControlSizer->Add(m_pScopePane, wxSizerFlags().Expand());
    }

    Layout();
    Refresh();
}

void GraphLogWindow::OnButtonClear(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->ResetData();
    Refresh();
}

void GraphLogWindow::OnCheckboxTrendlines(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_showTrendlines = m_pCheckboxTrendlines->IsChecked();
    if (!m_pClient->m_showTrendlines)
    {
        // clear the polar alignment circle
        pFrame->pGuider->SetPolarAlignCircle(PHD_Point(), 0);
    }
    Refresh();
}

void GraphLogWindow::OnCheckboxCorrections(wxCommandEvent& WXUNUSED(evt))
{
    m_pClient->m_showCorrections = m_pCheckboxCorrections->IsChecked();
    pConfig->Global.SetBoolean("/graph/showCorrections", m_pClient->m_showCorrections);
    Refresh();
}

void GraphLogWindow::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    wxAutoBufferedPaintDC dc(this);

    dc.SetBackground(*wxBLACK_BRUSH);
    //dc.SetBackground(wxColour(10,0,0));
    dc.Clear();

    switch (m_pClient->m_mode)
    {
        case GraphLogClientWindow::MODE_RADEC:
            m_pLabel1->SetLabel(_("RA"));
            m_pLabel2->SetLabel(_("Dec"));
            break;
        case GraphLogClientWindow::MODE_DXDY:
            m_pLabel1->SetLabel(_("dx"));
            m_pLabel2->SetLabel(_("dy"));
            break;
    }

    UpdateHeightButtonLabel();
}

void GraphLogWindow::UpdateHeightButtonLabel(void)
{
    int val = m_pClient->m_height;

    if (pFrame && pFrame->GetCameraPixelScale() != 1.0 && m_pClient->m_heightUnits == UNIT_ARCSEC)
        val = -val; // <0 indicates arc-sec

    if (m_heightButtonLabelVal != val)
    {
        if (val > 0)
        {
            m_pHeightButton->SetLabel(wxString::Format(_T("y:+/-%d"), m_pClient->m_height));
            m_pHeightButton->SetToolTip(_("Select the Y-axis scale, pixels per Y division"));
        }
        else
        {
            m_pHeightButton->SetLabel(wxString::Format(_T("y:+/-%d''"), m_pClient->m_height));
            m_pHeightButton->SetToolTip(_("Select the Y-axis scale, arc-seconds per Y division"));
        }
        m_heightButtonLabelVal = val;
    }
}

BEGIN_EVENT_TABLE(GraphLogClientWindow, wxWindow)
EVT_PAINT(GraphLogClientWindow::OnPaint)
END_EVENT_TABLE()

GraphLogClientWindow::GraphLogClientWindow(wxWindow *parent) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(401,200), wxFULL_REPAINT_ON_RESIZE)
{
    ResetData();
    m_mode = (GRAPH_MODE) pConfig->Global.GetInt ("/graph/ScopeOrCameraUnits", (int) MODE_RADEC);

    if (!m_raOrDxColor.Set(pConfig->Global.GetString("/graph/RAColor", wxEmptyString)))
    {
        m_raOrDxColor  = wxColour(100,100,255);
        pConfig->Global.SetString("/graph/RAColor", m_raOrDxColor.GetAsString(wxC2S_HTML_SYNTAX));
    }
    if (!m_decOrDyColor.Set(pConfig->Global.GetString("/graph/DecColor", wxEmptyString)))
    {
        m_decOrDyColor = wxColour(255,0,0);
        pConfig->Global.SetString("/graph/DecColor", m_decOrDyColor.GetAsString(wxC2S_HTML_SYNTAX));
    }

    int minLength = pConfig->Global.GetInt("/graph/minLength", DefaultMinLength);
    SetMinLength(minLength);

    int maxLength = pConfig->Global.GetInt("/graph/maxLength", DefaultMaxLength);
    SetMaxLength(maxLength);

    int minHeight = pConfig->Global.GetInt("/graph/minHeight", DefaultMinHeight);
    SetMinHeight(minHeight);

    int maxHeight = pConfig->Global.GetInt("/graph/maxHeight", DefaultMaxHeight);
    SetMaxHeight(maxHeight);

    m_length = pConfig->Global.GetInt("/graph/length", m_minLength * 2);
    m_height = pConfig->Global.GetInt("/graph/height", m_minHeight * 2 * 2); // match PHD1 4-pixel scale for new users
    m_heightUnits = (GRAPH_UNITS) pConfig->Global.GetInt ("graph/HeightUnits", (int) UNIT_ARCSEC); // preferred units, will still display pixels if camera pixel scale not available

    m_showTrendlines = false;
    m_showCorrections = pConfig->Global.GetBoolean("/graph/showCorrections", true);
}

GraphLogClientWindow::~GraphLogClientWindow(void)
{
}

static void reset_trend_accums(TrendLineAccum accums[4])
{
    for (int i = 0; i < 4; i++)
    {
        accums[i].sum_xy = 0.0;
        accums[i].sum_y = 0.0;
        accums[i].sum_y2 = 0.0;
    }
}

void GraphLogClientWindow::ResetData(void)
{
    m_history.clear();
    reset_trend_accums(m_trendLineAccum);
    m_raSameSides = 0;
}

bool GraphLogClientWindow::SetMinLength(unsigned int minLength)
{
    bool bError = false;

    try
    {
        if (minLength < 1)
        {
            throw ERROR_INFO("minLength < 1");
        }
        m_minLength = minLength;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minLength = DefaultMinLength;
    }

    pConfig->Global.SetInt("/graph/minLength", m_minLength);

    return bError;
}

bool GraphLogClientWindow::SetMaxLength(unsigned int maxLength)
{
    bool bError = false;

    try
    {
        if (maxLength <= m_minLength)
        {
            throw ERROR_INFO("maxLength < m_minLength");
        }
        m_history.resize(maxLength);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minLength = DefaultMinLength;
        m_history.resize(DefaultMaxLength);
    }

    pConfig->Global.SetInt("/graph/maxLength", m_history.capacity());

    return bError;
}

bool GraphLogClientWindow::SetMinHeight(unsigned int minHeight)
{
    bool bError = false;

    try
    {
        if (minHeight < 1)
        {
            throw ERROR_INFO("minHeight < 1");
        }
        m_minHeight = minHeight;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minHeight = DefaultMinHeight;
    }

    pConfig->Global.SetInt("/graph/minHeight", m_minHeight);

    return bError;
}

bool GraphLogClientWindow::SetMaxHeight(unsigned int maxHeight)
{
    bool bError = false;

    try
    {
        if (maxHeight <= m_minHeight)
        {
            throw ERROR_INFO("maxHeight < m_minHeight");
        }
        m_maxHeight = maxHeight;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_minHeight = DefaultMinHeight;
        m_maxHeight = DefaultMaxHeight;
    }

    pConfig->Global.SetInt("/graph/maxHeight", m_maxHeight);

    return bError;
}

// update_trend - update running accumulators for trend line calculations
//
static void update_trend(int nr, int max_nr, double newval, const double& oldval, TrendLineAccum *accum)
{
    // note: not safe to dereference oldval when nr == 0

    if (nr < max_nr)
    {
        // number of items is increasing, increment sums
        accum->sum_y += newval;
        accum->sum_xy += nr * newval;
        accum->sum_y2 += newval * newval;
    }
    else
    {
        // number of items has reached limit. Update counters to reflect
        // removal of oldest value (oldval) and addition of new value.
        accum->sum_xy += (max_nr - 1) * newval + oldval - accum->sum_y;
        accum->sum_y += newval - oldval;
        accum->sum_y2 += newval * newval - oldval * oldval;
    }
}

void GraphLogClientWindow::AppendData(const GuideStepInfo& step)
{
    unsigned int trend_items = m_length;
    if (trend_items > m_history.size())
        trend_items = m_history.size();
    const int oldest_idx = m_history.size() - trend_items;

    S_HISTORY oldest;
    if (m_history.size() > 0)
        oldest = m_history[oldest_idx];
    update_trend(trend_items, m_length, step.cameraOffset->X, oldest.dx, &m_trendLineAccum[0]);
    update_trend(trend_items, m_length, step.cameraOffset->Y, oldest.dy, &m_trendLineAccum[1]);
    update_trend(trend_items, m_length, step.mountOffset->X, oldest.ra, &m_trendLineAccum[2]);
    update_trend(trend_items, m_length, step.mountOffset->Y, oldest.dec, &m_trendLineAccum[3]);

    // update counter for osc index
    if (trend_items >= 1)
    {
        if (step.mountOffset->X * m_history[m_history.size() - 1].ra > 0.0)
            ++m_raSameSides;
        if (trend_items >= m_length)
        {
            if (m_history[oldest_idx].ra * m_history[oldest_idx + 1].ra > 0.0)
                --m_raSameSides;
        }
    }

    m_history.push_back(S_HISTORY(step));
}

void GraphLogClientWindow::RecalculateTrendLines(void)
{
    reset_trend_accums(m_trendLineAccum);
    unsigned int trend_items = m_history.size();
    if (trend_items > m_length)
        trend_items = m_length;
    const int begin = m_history.size() - trend_items;
    const int end = m_history.size() - 1;
    for (unsigned int x = 0, i = begin; x < trend_items; i++, x++) {
        const S_HISTORY& h = m_history[i];
        update_trend(x, trend_items, h.dx, 0.0, &m_trendLineAccum[0]);
        update_trend(x, trend_items, h.dy, 0.0, &m_trendLineAccum[1]);
        update_trend(x, trend_items, h.ra, 0.0, &m_trendLineAccum[2]);
        update_trend(x, trend_items, h.dec, 0.0, &m_trendLineAccum[3]);
    }
    // recalculate ra same side counter
    m_raSameSides = 0;
    if (trend_items >= 2)
        for (int i = begin; i < end; i++)
            if (m_history[i].ra * m_history[i + 1].ra > 0.0)
                ++m_raSameSides;
}

// trendline - calculate the the trendline slope and intercept. We can do this
// in O(1) without iterating over the history data since we have kept running
// sums sum(y), sum(xy), and since sum(x) and sum(x^2) can be computed directly
// in a single expression (without iterating) for x from 0..n-1
//
static std::pair<double, double> trendline(const TrendLineAccum& accum, int nn)
{
    assert(nn > 1);
    double n = (double) nn;
    // sum_x is: sum(x) for x from 0 .. n-1
    double sum_x = 0.5 * n * (n - 1.0);
    // denom is: (n sum(x^2) - sum(x)^2) for x from 0 .. n-1
    double denom = n * n * (n - 1.0) * ((2.0 * n - 1.0) / 6.0 - 0.25 * (n - 1));

    double a = (n * accum.sum_xy - sum_x * accum.sum_y) / denom;
    double b = (accum.sum_y - a * sum_x) / n;

    return std::make_pair(a, b);
}

// helper class to scale and translate points
struct ScaleAndTranslate
{
    int m_xorig, m_yorig;
    double m_xmag, m_ymag;
    ScaleAndTranslate(int xorig, int yorig, double xmag, double ymag) : m_xorig(xorig), m_yorig(yorig), m_xmag(xmag), m_ymag(ymag) { }
    wxPoint pt(double x, double y) const {
        return wxPoint(m_xorig + (int)(x * m_xmag), m_yorig + (int)(y * m_ymag));
    }
};

static double rms(unsigned int nr, const TrendLineAccum *accum)
{
    if (nr == 0)
        return 0.0;
    double const n = (double) nr;
    double const s1 = accum->sum_y;
    double const s2 = accum->sum_y2;
    return sqrt(n * s2 - s1 * s1) / n;
}

static wxString rms_label(double rms, double sampling)
{
    if (sampling != 1.0)
        return wxString::Format("%4.2f (%.2f'')", rms, rms * sampling);
    else
        return wxString::Format("%4.2f", rms);
}

static double GetMaxDuration(const circular_buffer<S_HISTORY>& history, int start_item)
{
    double maxdur = 1.0; // always return at least 1.0 to protect against divide-by-zero
    for (unsigned int i = start_item; i < history.size(); i++)
    {
        const S_HISTORY& h = history[i];
        double d = fabs(h.raDur);
        if (d > maxdur)
            maxdur = d;
        d = fabs(h.decDur);
        if (d > maxdur)
            maxdur = d;
    }
    return maxdur;
}

void GraphLogClientWindow::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
    //wxAutoBufferedPaintDC dc(this);
    wxPaintDC dc(this);

    wxSize size = GetClientSize();
    wxSize center(size.x/2, size.y/2);

    const int leftEdge = 0;
    const int rightEdge = size.x-5;

    const int topEdge = 5;
    const int bottomEdge = size.y-5;

    const int xorig = 0;
    const int yorig = size.y/2;

    const int xDivisions = m_length/m_xSamplesPerDivision-1;
    const int xPixelsPerDivision = size.x/2/(xDivisions+1);
    const int yPixelsPerDivision = size.y/2/(m_yDivisions+1);

    const double sampling = pFrame ? pFrame->GetCameraPixelScale() : 1.0;
    GRAPH_UNITS units = m_heightUnits;
    if (sampling == 1.0)
    {
        // force units to pixels if pixel scale not available
        units = UNIT_PIXELS;
    }

    dc.SetBackground(*wxBLACK_BRUSH);
    //dc.SetBackground(wxColour(10,0,0));
    dc.Clear();

    wxPen GreyDashPen;
    GreyDashPen = wxPen(wxColour(200,200,200),1, wxDOT);

    // Draw axes
    dc.SetPen(*wxGREY_PEN);
    dc.DrawLine(center.x,topEdge,center.x,bottomEdge);
    dc.DrawLine(leftEdge,center.y,rightEdge,center.y);

    // draw a box around the client area
    dc.SetPen(*wxGREY_PEN);
    dc.DrawLine(leftEdge, topEdge, rightEdge, topEdge);
    dc.DrawLine(rightEdge, topEdge, rightEdge, bottomEdge);
    dc.DrawLine(rightEdge, bottomEdge, leftEdge, bottomEdge);
    dc.DrawLine(leftEdge, bottomEdge, leftEdge, topEdge);

    // Draw horiz rule (scale is 1 pixel error per 25 pixels) + scale labels
    dc.SetPen(GreyDashPen);
    dc.SetTextForeground(*wxLIGHT_GREY);
#if defined(__WXOSX__)
    dc.SetFont(*wxSMALL_FONT);
#else
    dc.SetFont(*wxSWISS_FONT);
#endif

    for (int i = 1; i <= m_yDivisions; i++)
    {
        double div_y = center.y-i*yPixelsPerDivision;
        dc.DrawLine(leftEdge,div_y, rightEdge, div_y);
        dc.DrawText(wxString::Format("%g%s", i * (double)m_height / (m_yDivisions + 1), units == UNIT_ARCSEC ? "''" : ""), leftEdge + 3, div_y - 13);

        div_y = center.y+i*yPixelsPerDivision;
        dc.DrawLine(leftEdge, div_y, rightEdge, div_y);
        dc.DrawText(wxString::Format("%g%s", -i * (double)m_height / (m_yDivisions + 1), units == UNIT_ARCSEC ? "''" : ""), leftEdge + 3, div_y - 13);
    }

    for (int i = 1; i <= xDivisions; i++)
    {
        dc.DrawLine(center.x-i*xPixelsPerDivision, topEdge, center.x-i*xPixelsPerDivision, bottomEdge);
        dc.DrawLine(center.x+i*xPixelsPerDivision, topEdge, center.x+i*xPixelsPerDivision, bottomEdge);
    }

    const double xmag = size.x / (double)m_length;
    const double ymag = yPixelsPerDivision * (double)(m_yDivisions + 1) / (double)m_height * (units == UNIT_ARCSEC ? sampling : 1.0);

    ScaleAndTranslate sctr(xorig, yorig, xmag, ymag);

    // Draw data
    if (m_history.size() > 0)
    {
        unsigned int plot_length = m_length;
        if (plot_length > m_history.size())
        {
            plot_length = m_history.size();
        }

        wxPoint *pRaOrDxLine  = new wxPoint[plot_length];
        wxPoint *pDecOrDyLine = new wxPoint[plot_length];

        unsigned int start_item = m_history.size() - plot_length;

        if (m_showCorrections)
        {
            double maxDur = GetMaxDuration(m_history, start_item);

            const double ymag = (size.y - 10) * 0.5 / maxDur;
            ScaleAndTranslate sctr(xorig, yorig, xmag, ymag);

            dc.SetBrush(*wxTRANSPARENT_BRUSH);

            dc.SetPen(wxPen(m_raOrDxColor.ChangeLightness(60)));

            for (unsigned int i = start_item, j = 0; i < m_history.size(); i++, j++)
            {
                const S_HISTORY& h = m_history[i];

                const double raDur = h.ra > 0.0 ? -h.raDur : h.raDur;
                wxPoint pt(sctr.pt(j, raDur));
                if (raDur <= -0.5)
                    dc.DrawRectangle(pt,wxSize(4, yorig - pt.y));
                else if (raDur >= 0.5)
                    dc.DrawRectangle(wxPoint(pt.x, yorig), wxSize(4, pt.y - yorig));
            }

            dc.SetPen(wxPen(m_decOrDyColor.ChangeLightness(60)));

            for (unsigned int i = start_item, j = 0; i < m_history.size(); i++, j++)
            {
                const S_HISTORY& h = m_history[i];

                const double decDur = h.dec > 0.0 ? -h.decDur : h.decDur;
                wxPoint pt(sctr.pt(j, decDur));
                pt.x += 5;
                if (decDur <= -0.5)
                    dc.DrawRectangle(pt,wxSize(4, yorig - pt.y));
                else if (decDur >= 0.5)
                    dc.DrawRectangle(wxPoint(pt.x, yorig), wxSize(4, pt.y - yorig));
            }
        }

        for (unsigned int i = start_item, j = 0; i < m_history.size(); i++, j++)
        {
            const S_HISTORY& h = m_history[i];

            switch (m_mode)
            {
            case MODE_RADEC:
                pRaOrDxLine[j] = sctr.pt(j, h.ra);
                pDecOrDyLine[j] = sctr.pt(j, h.dec);
                break;
            case MODE_DXDY:
                pRaOrDxLine[j] = sctr.pt(j, h.dx);
                pDecOrDyLine[j] = sctr.pt(j, h.dy);
                break;
            }
        }

        wxPen raOrDxPen(m_raOrDxColor, 2);
        dc.SetPen(raOrDxPen);
        dc.DrawLines(plot_length, pRaOrDxLine);

        wxPen decOrDyPen(m_decOrDyColor, 2);
        dc.SetPen(decOrDyPen);
        dc.DrawLines(plot_length, pDecOrDyLine);

        // draw trend lines
        unsigned int polarAlignCircleRadius = 0;
        if (m_showTrendlines && plot_length >= 5)
        {
            std::pair<double, double> trendRaOrDx;
            std::pair<double, double> trendDecOrDy;
            switch (m_mode)
            {
            case MODE_RADEC:
                trendRaOrDx = trendline(m_trendLineAccum[2], plot_length);
                trendDecOrDy = trendline(m_trendLineAccum[3], plot_length);
                break;
            case MODE_DXDY:
                trendRaOrDx = trendline(m_trendLineAccum[0], plot_length);
                trendDecOrDy = trendline(m_trendLineAccum[1], plot_length);
                break;
            }

            wxPoint lineRaOrDx[2];
            lineRaOrDx[0] = sctr.pt(0.0, trendRaOrDx.second);
            lineRaOrDx[1] = sctr.pt(m_length, trendRaOrDx.first * m_length + trendRaOrDx.second);

            wxPoint lineDecOrDy[2];
            lineDecOrDy[0] = sctr.pt(0.0, trendDecOrDy.second);
            lineDecOrDy[1] = sctr.pt(m_length, trendDecOrDy.first * m_length + trendDecOrDy.second);

            raOrDxPen.SetStyle(wxLONG_DASH);
            dc.SetPen(raOrDxPen);
            dc.DrawLines(2, lineRaOrDx, 0, 0);

            decOrDyPen.SetStyle(wxLONG_DASH);
            dc.SetPen(decOrDyPen);
            dc.DrawLines(2, lineDecOrDy, 0, 0);

            // show polar alignment error
            if (m_mode == MODE_RADEC && sampling != 1.0)
            {
                double declination = pMount ? pMount->GetDeclination() : M_PI / 2.0;

                if (fabs(declination) <= Mount::DEC_COMP_LIMIT)
                {
                    const S_HISTORY& h0 = m_history[start_item];
                    const S_HISTORY& h1 = m_history[m_history.size() - 1];
                    double dt = (double)(h1.timestamp - h0.timestamp) / (1000.0 * 60.0); // time span in minutes
                    double ddec = (double) (plot_length - 1) * trendDecOrDy.first;
                    // From Frank Barrett, "Determining Polar Axis Alignment Accuracy"
                    // http://celestialwonders.com/articles/polaralignment/PolarAlignmentAccuracy.pdf
                    double err_arcmin = (3.82 * ddec) / (dt * cos(declination));
                    unsigned int err_px = (unsigned int) floor(fabs(err_arcmin * sampling * 60.0) + 0.5);
                    polarAlignCircleRadius = err_px;
                    dc.DrawText(wxString::Format("Polar alignment error: %.2f' (%u px)", err_arcmin, err_px),
                        leftEdge + 30, bottomEdge - 18);
                }
            }
        }
        pFrame->pGuider->SetPolarAlignCircle(pFrame->pGuider->CurrentPosition(), polarAlignCircleRadius);

        double rms_ra = rms(plot_length, &m_trendLineAccum[2]);
        double rms_dec = rms(plot_length, &m_trendLineAccum[3]);
        double rms_tot = sqrt(rms_ra * rms_ra + rms_dec * rms_dec);
        m_pRaRMS->SetLabel(rms_label(rms_ra, sampling));
        m_pDecRMS->SetLabel(rms_label(rms_dec, sampling));
        m_pTotRMS->SetLabel(rms_label(rms_tot, sampling));

        // Figure oscillation score

        double osc_index = 0.0;
        if (plot_length >= 2)
            osc_index = 1.0 - (double) m_raSameSides / (double) (plot_length - 1);

        if ((osc_index > 0.6) || (osc_index < 0.15))
        {
            m_pOscIndex->SetForegroundColour(wxColour(185,20,0));
        }
        else
        {
            m_pOscIndex->SetForegroundColour(*wxLIGHT_GREY);
        }

        m_pOscIndex->SetLabel(wxString::Format("RA Osc: %4.2f", osc_index));

        delete [] pRaOrDxLine;
        delete [] pDecOrDyLine;
    }
}

GraphControlPane::GraphControlPane(wxWindow *pParent, const wxString& label)
    : wxWindow(pParent, wxID_ANY)
{
    m_pControlSizer = new wxBoxSizer(wxHORIZONTAL);

    SetBackgroundColour(*wxBLACK);

    int width  = StringWidth(label);
    wxStaticText *pLabel = new wxStaticText(this,wxID_ANY,label, wxDefaultPosition, wxSize(width + 5, -1));
    wxFont f = pLabel->GetFont();
    f.SetWeight(wxBOLD);
    pLabel->SetFont(f);
    pLabel->SetForegroundColour(*wxWHITE);
    pLabel->SetBackgroundColour(*wxBLACK);

    m_pControlSizer->Add(pLabel, wxSizerFlags().Right());
    SetSizer(m_pControlSizer);
}

GraphControlPane::~GraphControlPane(void)
{
}

int GraphControlPane::StringWidth(const wxString& string)
{
    int width, height;

    GetParent()->GetTextExtent(string, &width, &height);

    return width;
}

void GraphControlPane::DoAdd(wxControl *pCtrl, const wxString& lbl)
{
    wxStaticText *pLabel = new wxStaticText(this,wxID_ANY,lbl);
    pLabel->SetForegroundColour(*wxWHITE);
    pLabel->SetBackgroundColour(*wxBLACK);

    m_pControlSizer->Add(pLabel, wxSizerFlags().Right());
    m_pControlSizer->AddSpacer(5);
    m_pControlSizer->Add(pCtrl, wxSizerFlags().Left());
    m_pControlSizer->AddSpacer(10);
}
