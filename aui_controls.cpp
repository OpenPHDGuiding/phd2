/*
*  aui_controls.cpp
*  PHD Guiding
*
*  Created by Bruce Waddington
*  Copyright (c) 2016 Bruce Waddington and Andy Galasso
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
#include "aui_controls.h"

#include <algorithm>

//#define ICON_DEV
#ifndef ICON_DEV
#include "icons/sb_led_green.png.h"
#include "icons/sb_led_yellow.png.h"
#include "icons/sb_led_red.png.h"
#include "icons/sb_arrow_left_16.png.h"
#include "icons/sb_arrow_right_16.png.h"
#include "icons/sb_arrow_up_16.png.h"
#include "icons/sb_arrow_down_16.png.h"
#endif

wxBEGIN_EVENT_TABLE(PHDStatusBar, wxStatusBar)
  EVT_SIZE(PHDStatusBar::OnSize)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(SBPanel, wxPanel)
  EVT_PAINT(SBPanel::OnPaint)
wxEND_EVENT_TABLE()

// How this works: 
// PHDStatusBar is a child of wxStatusBar and is composed of various control groups - properties of the guide star, info about 
// current guide commands, and state information about the current app session.  Each group is managed by its own class, and that class 
// is responsible for building, positioning, and updating its controls. The various controls are positioned (via the OnSize event) on top of the SBPanel that
// is the single underlying field in the base-class statusbar.  The SBPanel class handles its own Paint event in order to render
// the borders and field separators the way we want.

// ----------------------------------------------------------------------------
// SBPanel - parent control is the parent for all the status bar items
//
SBPanel::SBPanel(wxStatusBar* parent, const wxSize& panelSize)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, panelSize)
{
    int txtHeight;

    fieldOffsets.reserve(12);
    parent->GetTextExtent("M", &emWidth, &txtHeight);       // Horizontal spacer used by various controls
    SetBackgroundStyle(wxBG_STYLE_PAINT);

#ifndef __APPLE__
    SetDoubleBuffered(true);
#endif
}

void SBPanel::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();

    wxPen pen(*wxWHITE, 1);
    wxSize panelSize = GetClientSize();
    int x;

    dc.SetPen(pen);
    // Draw vertical white lines slightly in front of each field
    for (auto it = fieldOffsets.begin() + 1; it != fieldOffsets.end(); it++)
    {
        x = panelSize.x - *it - 4;
        dc.DrawLine(wxPoint(x, 0), wxPoint(x, panelSize.y));
    }
    // Put a border on the top of the panel
    dc.DrawLine(wxPoint(0, 0), wxPoint(panelSize.x, 0));
    dc.SetPen(wxNullPen);
}

// We want a vector with the integer offset of each field relative to the righthand end of the panel
void SBPanel::BuildFieldOffsets(const std::vector<int>& fldWidths)
{
    int cum = 0;
    // Add up field widths starting at right end of panel
    for (auto it = fldWidths.rbegin(); it != fldWidths.rend(); it++)
    {
        cum += *it;
        fieldOffsets.push_back(cum);
    }
    // Reverse it because the fields are indexed from left to right
    std::reverse(fieldOffsets.begin(), fieldOffsets.end());
}

int SBPanel::GetMinPanelWidth()
{
    return fieldOffsets.at(0);
}

wxPoint SBPanel::FieldLoc(int fieldId)
{
    wxSize panelSize = GetClientSize();
    int x = panelSize.x - fieldOffsets.at(fieldId);
    return (wxPoint(x, 3));
}

//-----------------------------------------------------------------------------
// SBStarIndicators - properties of the guide star
//
SBStarIndicators::SBStarIndicators(SBPanel *panel, std::vector<int>& fldWidths)
{
    int snrValueWidth;
    int satWidth;

    panel->GetTextExtent(_("SNR"), &snrLabelWidth, &txtHeight);
    panel->GetTextExtent("999.9", &snrValueWidth, &txtHeight);
    panel->GetTextExtent(_("SAT"), &satWidth, &txtHeight);
    fldWidths.push_back(satWidth + 1 * panel->emWidth);
    fldWidths.push_back(snrLabelWidth + snrValueWidth + 2 * panel->emWidth);

    // Use default positions for control creation - positioning is handled explicitly in PositionControls()
    txtSaturated = new wxStaticText(panel, wxID_ANY, satStr, wxDefaultPosition, wxSize(satWidth, -1));
    txtSaturated->SetBackgroundColour(*wxBLACK);
    txtSaturated->SetForegroundColour(*wxRED);
    txtSaturated->Show(false);
    // Label and value fields separated to allow different foreground colors for each
    txtSNRLabel = new wxStaticText(panel, wxID_ANY, _("SNR"), wxDefaultPosition, wxDefaultSize);
    txtSNRValue = new wxStaticText(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(snrValueWidth, 3), wxALIGN_RIGHT);
    txtSNRLabel->SetBackgroundColour(*wxBLACK);
    txtSNRLabel->SetForegroundColour(*wxWHITE);
    txtSNRLabel->Show(false);
    txtSNRValue->SetBackgroundColour(*wxBLACK);
    txtSNRValue->SetForegroundColour(*wxGREEN);
    txtSNRValue->SetToolTip(_("Signal-to-noise ratio of guide star\nGreen means SNR >= 10\nYellow means  4 <= SNR < 10\nRed means SNR < 4"));

    parentPanel = panel;
}

void SBStarIndicators::PositionControls()
{
    int fieldNum = (int)Field_Sat;
    wxPoint snrPos;
    wxPoint satPos;

    satPos = parentPanel->FieldLoc(fieldNum++);
    txtSaturated->SetPosition(wxPoint(satPos.x + 1, satPos.y));
    snrPos = parentPanel->FieldLoc(fieldNum++);
    txtSNRLabel->SetPosition(wxPoint(snrPos.x + 3, snrPos.y));
    txtSNRValue->SetPosition(wxPoint(snrPos.x + 3 + snrLabelWidth + 6, snrPos.y));
}

void SBStarIndicators::UpdateState(double MassPct, double SNR, bool Saturated)
{
    if (SNR >= 0)
    {
        if (SNR >= 10.0)
            txtSNRValue->SetForegroundColour(*wxGREEN);
        else
        {
            if (SNR >= 4.0)
                txtSNRValue->SetForegroundColour(*wxYELLOW);
            else
                txtSNRValue->SetForegroundColour(*wxRED);
        }
        txtSNRLabel->Show(true);
        txtSNRValue->SetLabelText(wxString::Format("%3.1f", SNR));
        txtSNRValue->Show(true);
        txtSaturated->Show(Saturated);
    }
    else
    {
        txtSNRLabel->Show(false);
        txtSNRValue->Show(false);
        txtSaturated->Show(false);
    }
}

//-----------------------------------------------------------------------------
// SBGuideIndicators - info about the most recent guide commands
//
SBGuideIndicators::SBGuideIndicators(SBPanel* panel, std::vector<int>& fldWidths)
{
#ifdef ICON_DEV
    wxIcon arrow = wxIcon("SB_arrow_left_16.png", wxBITMAP_TYPE_PNG, 16, 16);
    arrowLeft.CopyFromIcon(arrow);
    arrow = wxIcon("SB_arrow_right_16.png", wxBITMAP_TYPE_PNG, 16, 16);
    arrowRight.CopyFromIcon(arrow);
    arrow = wxIcon("SB_arrow_up_16.png", wxBITMAP_TYPE_PNG, 16, 16);
    arrowUp.CopyFromIcon(arrow);
    arrow = wxIcon("SB_arrow_down_16.png", wxBITMAP_TYPE_PNG, 16, 16);
    arrowDown.CopyFromIcon(arrow);
#else
    arrowLeft = (wxBITMAP_PNG_FROM_DATA(sb_arrow_left_16));
    arrowRight = wxBitmap(wxBITMAP_PNG_FROM_DATA(sb_arrow_right_16));
    arrowUp = wxBitmap(wxBITMAP_PNG_FROM_DATA(sb_arrow_up_16));
    arrowDown = wxBitmap(wxBITMAP_PNG_FROM_DATA(sb_arrow_down_16));
#endif
    int guideAmtWidth;
    int txtHeight;
    wxSize bitmapSize;

    wxColor fgColor(200, 200, 200);           // reduced brightness
    panel->GetTextExtent("5555 ms, 555 px", &guideAmtWidth, &txtHeight);

    // Use default positions for control creation - positioning is handled explicitly in PositionControls()
    bitmapRA = new wxStaticBitmap(panel, wxID_ANY, arrowLeft);
    bitmapSize = bitmapRA->GetSize();
    bitmapRA->Show(false);
    txtRaAmounts = new wxStaticText(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(guideAmtWidth, bitmapSize.y), wxALIGN_CENTER);
    txtRaAmounts->SetBackgroundColour(*wxBLACK);
    txtRaAmounts->SetForegroundColour(fgColor);
    txtDecAmounts = new wxStaticText(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(guideAmtWidth, bitmapSize.y), wxALIGN_RIGHT);
    txtDecAmounts->SetBackgroundColour(*wxBLACK);
    txtDecAmounts->SetForegroundColour(fgColor);
    bitmapDec = new wxStaticBitmap(panel, wxID_ANY, arrowUp);
    bitmapDec->Show(false);
    parentPanel = panel;
    // Since we don't want separators between the arrows and the text info, we lump the two together and treat them as one field for the purpose
    // of positioning
    fldWidths.push_back(bitmapSize.x + guideAmtWidth + 2 * panel->emWidth);          // RA info
    fldWidths.push_back(bitmapSize.x + guideAmtWidth + 2 * panel->emWidth);          // Dec info
}

void SBGuideIndicators::PositionControls()
{
    int fieldNum = (int)Field_RAInfo;
    int txtWidth;
    int txtHeight;
    wxPoint loc;

    loc = parentPanel->FieldLoc(fieldNum);
    bitmapRA->SetPosition(wxPoint(loc.x, loc.y - 1));
    wxPoint raPosition = parentPanel->FieldLoc(fieldNum);
    raPosition.x += 20;
    txtRaAmounts->SetPosition(raPosition);

    fieldNum++;
    wxString txtSizer =  wxString::Format(_("%d ms, %0.1f px"), 120, 4.38);
    parentPanel->GetTextExtent(txtSizer, &txtWidth, &txtHeight);
    wxPoint decPosition = parentPanel->FieldLoc(fieldNum);
    txtDecAmounts->SetPosition(decPosition);

    decPosition.x += txtWidth + 8;
    decPosition.y -= 1;
    bitmapDec->SetPosition(decPosition);
}

void SBGuideIndicators::UpdateState(int raDirection, int decDirection, double raPx, int raPulse, double decPx, int decPulse)
{
    wxString raInfo;
    wxString decInfo;

    if (raPulse > 0)
    {
        if (raDirection == RIGHT)
            bitmapRA->SetBitmap(arrowRight);
        else
            bitmapRA->SetBitmap(arrowLeft);

        bitmapRA->Show(true);
        raInfo = wxString::Format(_("%d ms, %0.1f px"), raPulse, raPx);
    }
    else
    {
        bitmapRA->Show(false);
    }

    if (decPulse > 0)
    {
        if (decDirection == UP)
            bitmapDec->SetBitmap(arrowUp);
        else
            bitmapDec->SetBitmap(arrowDown);
        bitmapDec->Show(true);
        decInfo = wxString::Format(_("%d ms, %0.1f px"), decPulse, decPx);
    }
    else
    {
        bitmapDec->Show(false);
    }

    txtRaAmounts->SetLabelText(raInfo);
    txtDecAmounts->SetLabelText(decInfo);
}

//------------------------------------------------------------------------------------------
// ---SBStateIndicatorItem - individual state indicators
//
SBStateIndicatorItem::SBStateIndicatorItem(SBPanel* panel, SBStateIndicators* host, int indField, const wxString& indLabel, SBFieldTypes indType, std::vector<int>& fldWidths)
{
    type = indType;
    lastState = -2;
    parentPanel = panel;
    container = host;
    fieldId = indField;
    otherInfo = wxEmptyString;
    parentPanel->GetTextExtent(indLabel, &txtWidth, &txtHeight);
    // Use default positions for control creation - positioning is handled explicitly in PositionControls()
    if (indType != Field_Gear)
    {
        ctrl = new wxStaticText(parentPanel, wxID_ANY, indLabel, wxDefaultPosition, wxSize(txtWidth + parentPanel->emWidth, -1), wxALIGN_CENTER);
        fldWidths.push_back(txtWidth + 2 * parentPanel->emWidth);
    }
    else
    {
        pic = new wxStaticBitmap(parentPanel, wxID_ANY, container->icoGreenLed, wxDefaultPosition, wxSize(16, 16));
        fldWidths.push_back(20 + 1 * parentPanel->emWidth);
    }
}

void SBStateIndicatorItem::PositionControl()
{
    wxPoint loc;
    if (type == Field_Gear)
    {
        loc = parentPanel->FieldLoc(fieldId);
        pic->SetPosition(wxPoint(loc.x + 7, loc.y));
    }
    else
        ctrl->SetPosition(parentPanel->FieldLoc(fieldId));
}

void SBStateIndicatorItem::UpdateState()
{
    int quadState = -1;
    bool cameraOk = true;
    bool problems = false;
    bool partials = false;
    wxString MIAs;

    switch (type)
    {
    case Field_Gear:
        if (pCamera && pCamera->Connected)
        {
            partials = true;
        }
        else
        {
            MIAs += _("Camera, ");
            cameraOk = false;
            problems = true;
        }

        if ((pMount && pMount->IsConnected()) || (pSecondaryMount && pSecondaryMount->IsConnected()))
            partials = true;
        else
        {
            MIAs += _("Mount, ");
            problems = true;
        }

        if (pPointingSource && pPointingSource->IsConnected())
            partials = true;
        else
        {
            MIAs += _("Aux Mount, ");
            problems = true;
        }

        if (pMount && pMount->IsStepGuider())
        {
            if (pMount->IsConnected())
                partials = true;
            else
            {
                MIAs += _("AO, ");
                problems = true;
            }
        }

        if (pRotator)
        {
            if (pRotator->IsConnected())
                partials = true;
            else
            {
                MIAs += _("Rotator, ");
                problems = true;
            }
        }

        if (partials)
        {
            if (!problems)
            {
                pic->SetIcon(wxIcon(container->icoGreenLed));
                quadState = 1;
                otherInfo = "";
            }
            else
            {
                if (cameraOk)
                    pic->SetIcon(container->icoYellowLed);
                else
                    pic->SetIcon(container->icoRedLed);     // What good are we without a camera
                quadState = 0;
                otherInfo = MIAs.Mid(0, MIAs.Length() - 2);
                pic->SetToolTip(IndicatorToolTip(type, quadState));
            }
        }
        else
        {
            pic->SetIcon(container->icoRedLed);
            quadState = -1;
        }

        break;

    case Field_Darks:
        if (pFrame)
        {
            if (pFrame->m_useDarksMenuItem->IsChecked() || pFrame->m_useDefectMapMenuItem->IsChecked())
            {
                quadState = 1;
                wxString lastLabel = ctrl->GetLabelText();
                wxString currLabel = (pFrame->m_useDefectMapMenuItem->IsChecked() ? _("BPM") : _("Dark"));
                if (lastLabel != currLabel)
                {
                    ctrl->SetLabelText(currLabel);
                    ctrl->SetToolTip(IndicatorToolTip(type, quadState));
                }
            }
        }

        break;

    case Field_Calib: {
        // For calib: -1 => no cal, 0 => cal but no pointing compensation, 1 => golden
        bool calibrated = (!pMount || pMount->IsCalibrated()) && (!pSecondaryMount || pSecondaryMount->IsCalibrated());
        if (calibrated)
        {
            Scope *scope = TheScope();
            bool deccomp = scope && scope->DecCompensationActive();
            quadState = deccomp ? 1 : 0;
        }
        break;
      }

    default:
        break;
    }

    // Don't flog the status icons unless something has changed
    if (lastState != quadState)
    {
        if (type != Field_Gear)
        {
            switch (quadState)
            {
            case -2:
                ctrl->SetForegroundColour(*wxLIGHT_GREY);
                break;
            case -1:
                ctrl->SetForegroundColour(*wxRED);
                break;
            case 0:
                ctrl->SetForegroundColour(*wxYELLOW);
                break;
            case 1:
                ctrl->SetForegroundColour(*wxGREEN);
                break;
            }
            ctrl->Refresh();

            if (quadState != -2)
                ctrl->SetToolTip(IndicatorToolTip(type, quadState));
        }
        else if (quadState != -2)
        {
            pic->SetToolTip(IndicatorToolTip(type, quadState));
        }

        lastState = quadState;
    }
}

wxString SBStateIndicatorItem::IndicatorToolTip(SBFieldTypes indType, int triState)
{
    wxString rslt;

    switch (indType)
    {
    case Field_Gear:
        if (triState == 1)
            rslt = _("All devices connected");
        else
        if (triState == -1)
            rslt = _("No devices connected");
        else
            rslt = _("Devices not connected: " + otherInfo);
        break;

    case Field_Darks:
        if (ctrl->GetLabelText() == _("Dark"))
            rslt = _("Dark library: ") + (triState == 1 ? _("In-use") : _("Not in-use"));
        else
            rslt = _("Bad pixel map: ") + (triState == 1 ? _("In-use") : _("Not in-use"));
        break;
    case Field_Calib:
        rslt = _("Calibration: ");
        switch (triState)
        {
        case -1:
            rslt += _("Not completed");
            break;
        case 0:
            rslt += _("Completed, but scope pointing info not available/not in-use");
            break;
        case 1:
            rslt += _("Completed, scope pointing info in-use");
            break;
        }
    default:
        break;
    }

    return rslt;
}

//--------------------------------------------------------------------------------------
// ---SBStateIndicators - the group of all app/session state controls, a mix of static text and bitmap controls
//
SBStateIndicators::SBStateIndicators(SBPanel* panel, std::vector<int>& fldWidths)
{
    parentPanel = panel;
    wxString labels[] = { _("Dark"), _("Cal"), wxEmptyString};

#ifdef ICON_DEV
    icoGreenLed = wxIcon("SB_led_green.ico", wxBITMAP_TYPE_ICO, 16, 16);
    icoYellowLed = wxIcon("SB_led_yellow.ico", wxBITMAP_TYPE_ICO, 16, 16);
    icoRedLed = wxIcon("SB_led_red.ico", wxBITMAP_TYPE_ICO, 16, 16);
#else
    wxBitmap led(wxBITMAP_PNG_FROM_DATA(sb_led_green));
    icoGreenLed.CopyFromBitmap(led);
    led = wxBitmap(wxBITMAP_PNG_FROM_DATA(sb_led_yellow));
    icoYellowLed.CopyFromBitmap(led);
    led = wxBitmap(wxBITMAP_PNG_FROM_DATA(sb_led_red));
    icoRedLed.CopyFromBitmap(led);
#endif
    for (int inx = (int)Field_Darks; inx <= (int)Field_Gear; inx++)
    {
        SBStateIndicatorItem* item = new SBStateIndicatorItem(parentPanel, this, inx, labels[inx - Field_Darks], (SBFieldTypes)(inx), fldWidths);
        stateItems.push_back(item);
        item->UpdateState();
    }
}

SBStateIndicators::~SBStateIndicators()
{
    for (int inx = 0; inx < stateItems.size(); inx++)
        delete stateItems.at(inx);
}

void SBStateIndicators::PositionControls()
{
    for (auto it = stateItems.begin(); it != stateItems.end(); it++)
    {
        (*it)->PositionControl();
    }
}

void SBStateIndicators::UpdateState()
{
    for (auto it = stateItems.begin(); it != stateItems.end(); it++)
    {
        (*it)->UpdateState();
    }
}


enum {
    SB_HEIGHT = 16
};

// -----------  PHDStatusBar Class
//
PHDStatusBar::PHDStatusBar(wxWindow *parent, long style)
    : wxStatusBar(parent, wxID_ANY, wxSTB_SHOW_TIPS | wxSTB_ELLIPSIZE_END | wxFULL_REPAINT_ON_RESIZE, "PHDStatusBar")
{
    std::vector<int> fieldWidths;

    // Set up the only field the wxStatusBar base class will know about
    int widths[] = {-1};

    SetFieldsCount(1);
    SetStatusWidths(1, widths);
    this->SetBackgroundColour(*wxBLACK);

    m_ctrlPanel = new SBPanel(this, wxSize(500, SB_HEIGHT));
    m_ctrlPanel->SetPosition(wxPoint(1, 2));

    // Build the leftmost text status field, the only field managed at this level
    m_Msg1 = new wxStaticText(m_ctrlPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1));
    int txtWidth, txtHeight;
    GetTextExtent(_("Selected star at (999.9, 999.9)"), &txtWidth, &txtHeight);         // only care about the width
    m_Msg1->SetBackgroundColour(*wxBLACK);
    m_Msg1->SetForegroundColour(*wxWHITE);
    fieldWidths.push_back(txtWidth);                    // Doesn't matter but we need to occupy the position in fieldWidths

    // Build the star status fields
    m_StarIndicators = new SBStarIndicators(m_ctrlPanel, fieldWidths);

    // Build the guide indicators
    m_GuideIndicators = new SBGuideIndicators(m_ctrlPanel, fieldWidths);

    // Build the state indicator controls
    m_StateIndicators = new SBStateIndicators(m_ctrlPanel, fieldWidths);

    m_ctrlPanel->BuildFieldOffsets(fieldWidths);
}

// Helper function - not safe to call SetMinHeight in the constructor
PHDStatusBar* PHDStatusBar::CreateInstance(wxWindow *parent, long style)
{
    PHDStatusBar* sb = new PHDStatusBar(parent, style);
    sb->SetMinHeight(SB_HEIGHT);
    return sb;
}

// Destructor
PHDStatusBar::~PHDStatusBar()
{
    this->DestroyChildren();        // any wxWidgets objects will be deleted
    delete m_StateIndicators;
    delete m_GuideIndicators;
    delete m_StarIndicators;
}

void PHDStatusBar::OnSize(wxSizeEvent& event)
{
    wxRect fldRect;
    GetFieldRect(0, fldRect);
    int fldWidth = fldRect.GetWidth();
    m_ctrlPanel->SetSize(fldWidth - 1, fldRect.GetHeight());
    m_Msg1->SetPosition(wxPoint(2, 3));
    m_StarIndicators->PositionControls();
    m_GuideIndicators->PositionControls();
    m_StateIndicators->PositionControls();

    event.Skip();
}

// Let client force updates to various statusbar components
void PHDStatusBar::UpdateStates()
{
    m_StateIndicators->UpdateState();
}

void PHDStatusBar::UpdateStarInfo(double SNR, bool Saturated)
{
    m_StarIndicators->UpdateState(0, SNR, Saturated);
}

void PHDStatusBar::UpdateGuiderInfo(const GuideStepInfo& info)
{
    m_GuideIndicators->UpdateState(info.directionRA, info.directionDec, fabs(info.mountOffset.X),
        info.durationRA, fabs(info.mountOffset.Y), info.durationDec);
}

void PHDStatusBar::ClearGuiderInfo()
{
    m_GuideIndicators->ClearState();
}

int PHDStatusBar::GetMinSBWidth()
{
    return m_ctrlPanel->GetMinPanelWidth();
}

void PHDStatusBar::StatusMsg(const wxString& text)
{
    m_Msg1->SetLabelText(text);
    m_Msg1->Update();
}

// Trivial class to handle the background color on the toolbar control
void PHDToolBarArt::DrawBackground(wxDC& dc, wxWindow* parent, const wxRect& rect)
{
    dc.SetBrush(wxColour(100, 100, 100));
    dc.DrawRectangle(rect);
}
