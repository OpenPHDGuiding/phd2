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

#include "icons/Ball_Green.xpm"
#include "icons/Ball_Yellow.xpm"
#include "icons/Ball_Red.xpm"
#include "icons/guide_arrow_left_16.xpm"
#include "icons/guide_arrow_right_16.xpm"
#include "icons/guide_arrow_up_16.xpm"
#include "icons/guide_arrow_down_16.xpm"

wxBEGIN_EVENT_TABLE(PHDStatusBar, wxStatusBar)
EVT_SIZE(PHDStatusBar::OnSize)

wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(SBPanel, wxPanel)
EVT_PAINT(SBPanel::OnPaint)
//EVT_ERASE_BACKGROUND(SBPanel::OnEraseBkgrnd)

wxEND_EVENT_TABLE()

// How this works: 
// PHDStatusBar is a child of wxStatusBar and is composed of various control groups - properties of the guide star, info about 
// current guide commands, and state information about the current app session.  Each group is managed by its own class, and that class 
// is responsible for building, positioning, and updating its controls. The various controls are positioned (via the OnSize event) on top of the SBPanel that
// is the single underlying field in the base-class statusbar.  The SBPanel class handles its own Paint event in order to render
// the borders and field separators the way we want.

// ----------------------------------------------------------------------------
// SBPanel - parent control used for all the status bar items
//
SBPanel::SBPanel(wxStatusBar* parent, wxSize panelSize)
: wxPanel(parent, wxID_ANY, wxDefaultPosition, panelSize)
{
    int txtHeight;

    fieldOffsets.reserve(12);
    parent->GetTextExtent("M", &emWidth, &txtHeight);       // Horizontal spacer used by various controls
    //SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetDoubleBuffered(true);
}

// We want a vector with the integer offset of each field relative to the righthand end of the panel
void SBPanel::BuildFieldOffsets(std::vector <int> &fldWidths)
{
    int cum = 0;

    for (std::vector<int>::reverse_iterator it = fldWidths.rbegin(); it != fldWidths.rend(); it++)
    {
        cum += *it;
        fieldOffsets.push_back(cum);
    }
    std::reverse(fieldOffsets.begin(), fieldOffsets.end());
}

wxPoint SBPanel::FieldLoc(int fieldId)
{
    wxSize panelSize = GetClientSize();
    int x = panelSize.x - fieldOffsets.at(fieldId);
    return (wxPoint(x, 3));
}

//void SBPanel::OnEraseBkgrnd(wxEraseEvent& evt)
//{
//
//}

void SBPanel::OnPaint(wxPaintEvent& evt)
{
    //wxAutoBufferedPaintDC dc(this);
    wxPaintDC dc(this);
    wxPen pen(*wxWHITE, 1);
    wxSize panelSize = GetClientSize();
    int x;

    dc.SetPen(pen);
    // Draw vertical white lines slightly in front of each field
    for (std::vector<int>::iterator it = fieldOffsets.begin() + 1; it != fieldOffsets.end(); it++)
    {
        x = panelSize.x - *it - 4;
        dc.DrawLine(wxPoint(x, 0), wxPoint(x, panelSize.y));
    }
    dc.DrawLine(wxPoint(0, 0), wxPoint(panelSize.x, 0));
    dc.SetPen(wxNullPen);
}

//-----------------------------------------------------------------------------
// SBStarIndicators - properties of the guide star
//
SBStarIndicators::SBStarIndicators(SBPanel *parent, std::vector <int> &fldWidths)
{
    int snrValueWidth;
    int satWidth;

    parent->GetTextExtent(_("SNR "), &snrLabelWidth, &txtHeight);
    parent->GetTextExtent("999.9", &snrValueWidth, &txtHeight);
    parent->GetTextExtent(_("SAT"), &satWidth, &txtHeight);
    fldWidths.push_back(satWidth + 1 * parent->emWidth);
    fldWidths.push_back(snrLabelWidth + snrValueWidth + 2 * parent->emWidth);


    txtSaturated = new wxStaticText(parent, wxID_ANY, satStr, wxDefaultPosition, wxSize(satWidth, -1));
    txtSaturated->SetBackgroundColour("BLACK");
    txtSaturated->SetForegroundColour("RED");
    txtSaturated->Show(false);
    // Label and value fields separated to allow different foreground colors for each
    txtSNRLabel = new wxStaticText(parent, wxID_ANY, _("SNR"), wxPoint(1,3), wxSize(snrLabelWidth, -1));
    txtSNRValue = new wxStaticText(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(snrValueWidth, 3));
    txtSNRLabel->SetBackgroundColour("BLACK");
    txtSNRLabel->SetForegroundColour("WHITE");
    txtSNRLabel->Show(false);
    txtSNRValue->SetBackgroundColour("BLACK");
    txtSNRValue->SetForegroundColour("GREEN");
    txtSNRValue->SetToolTip(_("Signal-to-noise ratio of guide star\nGreen means SNR >= 10\nYellow means  4 <= SNR < 10\nRed means SNR < 4"));

    parentPanel = parent;
}

void SBStarIndicators::PositionControls()
{
    int fieldNum = (int)Field_Sat;
    wxPoint snrLeft;

    txtSaturated->SetPosition(parentPanel->FieldLoc(fieldNum++));
    snrLeft = parentPanel->FieldLoc(fieldNum++);
    txtSNRLabel->SetPosition(snrLeft);
    txtSNRValue->SetPosition(wxPoint(snrLeft.x + snrLabelWidth + 6, snrLeft.y));
}

void SBStarIndicators::UpdateState(double MassPct, double SNR, bool Saturated)
{
    if (SNR >= 0)
    {
        if (SNR >= 10.0)
            txtSNRValue->SetForegroundColour("Green");
        else
        {
            if (SNR >= 4.0)
                txtSNRValue->SetForegroundColour("Yellow");
            else
                txtSNRValue->SetForegroundColour("Red");
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
SBGuideIndicators::SBGuideIndicators(SBPanel* parent, std::vector <int> &fldWidths)
{
    icoLeft = wxIcon(guide_arrow_left_16_xpm);
    icoRight = wxIcon(guide_arrow_right_16_xpm);
    icoUp = wxIcon(guide_arrow_up_16_xpm);
    icoDown = wxIcon(guide_arrow_down_16_xpm);
    int guideAmtWidth;
    int txtHeight;
    wxSize bitmapSize;

    wxColor fgColor = wxColor(200, 200, 200);           // reduced brightness
    parent->GetTextExtent("5555 ms, 555 px", &guideAmtWidth, &txtHeight);

    bitmapRA = new wxStaticBitmap(parent, wxID_ANY, icoLeft);
    bitmapSize = bitmapRA->GetSize();
    bitmapRA->Show(false);
    txtRaAmounts = new wxStaticText(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(guideAmtWidth, bitmapSize.y), wxALIGN_CENTER);
    txtRaAmounts->SetBackgroundColour("BLACK");
    txtRaAmounts->SetForegroundColour(fgColor);
    txtDecAmounts = new wxStaticText(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(guideAmtWidth, bitmapSize.y), wxALIGN_RIGHT);
    txtDecAmounts->SetBackgroundColour("Black");
    txtDecAmounts->SetForegroundColour(fgColor);
    bitmapDec = new wxStaticBitmap(parent, wxID_ANY, icoUp);
    bitmapDec->Show(false);
    parentPanel = parent;
    // Since we don't want separators between the arrows and the text info, we lump the two together and treat them as one field for the purpose
    // of positioning
    fldWidths.push_back(bitmapSize.x + guideAmtWidth + 2 * parent->emWidth);          // RA info
    fldWidths.push_back(bitmapSize.x + guideAmtWidth + 2 * parent->emWidth);          // Dec info
}

void SBGuideIndicators::PositionControls()
{
    int fieldNum = (int)Field_RAInfo;
    int txtWidth;
    int txtHeight;
    wxPoint loc;

    loc = parentPanel->FieldLoc(fieldNum);
    bitmapRA->SetPosition(wxPoint(loc.x, loc.y - 1));
    wxString txtSizer = "38 ms, 0.45 px";
    parentPanel->GetTextExtent(txtSizer, &txtWidth, &txtHeight);
    wxPoint raPosition = parentPanel->FieldLoc(fieldNum);
    raPosition.x += 20;
    txtRaAmounts->SetPosition(raPosition);

    fieldNum++;
    txtSizer = "120 ms, 4.38 px";
    parentPanel->GetTextExtent(txtSizer, &txtWidth, &txtHeight);
    wxPoint decPosition = parentPanel->FieldLoc(fieldNum);
    txtDecAmounts->SetPosition(decPosition);

    decPosition.x += txtWidth + 8;
    decPosition.y -= 2;
    bitmapDec->SetPosition(decPosition);
}

void SBGuideIndicators::UpdateState(GUIDE_DIRECTION raDirection, GUIDE_DIRECTION decDirection, double raPx, double raPulse, double decPx, double decPulse)
{

    wxString raInfo;
    wxString decInfo;

    if (raPx > 0)
    {
        if (raDirection == RIGHT)
            bitmapRA->SetIcon(icoRight);
        else
            bitmapRA->SetIcon(icoLeft);

        bitmapRA->Show(true);
        raInfo = wxString::Format("%0.0f ms, %0.1f px", raPulse, raPx);
    }
    else
    {
        bitmapRA->Show(false);
        raInfo = wxEmptyString;
    }
    if (decPx > 0)
    {
        if (decDirection == UP)
            bitmapDec->SetIcon(icoUp);
        else
            bitmapDec->SetIcon(icoDown);
        bitmapDec->Show(true);
        decInfo = wxString::Format("%0.0f ms, %0.1f px", decPulse, decPx);
    }
    else
    {
        bitmapDec->Show(false);
        decInfo = wxEmptyString;
    }

    txtRaAmounts->SetLabelText(raInfo);
    txtDecAmounts->SetLabelText(decInfo);

}
//------------------------------------------------------------------------------------------
// ---SBStateIndicatorItem - individual state indicators
//
SBStateIndicatorItem::SBStateIndicatorItem(SBPanel* parent, int indField, const wxString &indLabel, SBFieldTypes indType, std::vector <int> &fldWidths)
{
    type = indType;
    lastState = -2;
    parentPanel = parent;
    fieldId = indField;
    otherInfo = wxEmptyString;
    parentPanel->GetTextExtent(indLabel, &txtWidth, &txtHeight);
    if (indType != Field_Gear)
    {
        ctrl = new wxStaticText(parentPanel, wxID_ANY, indLabel, wxDefaultPosition, wxSize(txtWidth + parent->emWidth, -1), wxALIGN_CENTER);
        fldWidths.push_back(txtWidth + 2 * parent->emWidth);
    }
    else
    {
        pic = new wxStaticBitmap(parentPanel, wxID_ANY, wxIcon(Ball_Green_xpm), wxDefaultPosition, wxSize(16, 16));
        fldWidths.push_back(20 + 1 * parent->emWidth);
    }
}

void SBStateIndicatorItem::PositionControl()
{
    if (type == Field_Gear)
        pic->SetPosition(parentPanel->FieldLoc(fieldId));
    else
        ctrl->SetPosition(parentPanel->FieldLoc(fieldId));
}

void SBStateIndicatorItem::UpdateState()
{
    int quadState = -1;
    bool problems = false;
    bool partials = false; 
    wxString currLabel;
    wxString MIAs = "";

    switch (type)
    {
    case Field_Gear:
        if (pCamera && pCamera->Connected)
            partials = true;
        else
        {
            MIAs += _("Camera, ");
            problems = true;
        }

        if ((pMount && pMount->IsConnected()) || pSecondaryMount && pSecondaryMount->IsConnected())
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
                pic->SetIcon(wxIcon(Ball_Green_xpm));
                quadState = 1;
                otherInfo = "";
            }
            else
            {
                pic->SetIcon(wxIcon(Ball_Yellow_xpm));
                quadState = 0;
                otherInfo = MIAs.Mid(0, MIAs.Length() - 2);
                pic->SetToolTip(IndicatorToolTip(type, quadState));
            }
        }
        else
        {
            pic->SetIcon(wxIcon(Ball_Red_xpm));
            quadState = -1;
        }
            
        break;

    case Field_Darks:
        if (pFrame)
        {
            wxString lastLabel = ctrl->GetLabelText();
            if (pFrame->m_useDarksMenuItem->IsChecked() || pFrame->m_useDefectMapMenuItem->IsChecked())
            {
                quadState = 1;
                currLabel = (pFrame->m_useDefectMapMenuItem->IsChecked() ? _("BPM") : _("Dark"));
                if (lastLabel != currLabel)
                {
                    ctrl->SetLabelText(currLabel);
                    ctrl->SetToolTip(IndicatorToolTip(type, quadState));
                }
            }
        }

        break;

    case Field_Calib:
        // For calib: -1 => no cal, 0 => cal but no pointing compensation, 1 => golden
        bool uncal = (pMount && !pMount->IsCalibrated()) || (pSecondaryMount && !pSecondaryMount->IsCalibrated());
        if (!uncal)
        {
            if (pPointingSource && pPointingSource->IsConnected() && ((pMount && pMount->DecCompensationEnabled()) || (pSecondaryMount && pSecondaryMount->DecCompensationEnabled())))
                quadState = 1;
            else
                quadState = 0;
        }
        break;
    }
    // Don't flog the status bar unless something has changed
    if (lastState != quadState)
    {
        if (type != Field_Gear)
        {
            currLabel = ctrl->GetLabelText();

            switch (quadState)
            {
            case -2:
                ctrl->SetForegroundColour("Grey");
                break;
            case -1:
                ctrl->SetForegroundColour("Red");
                break;
            case 0:
                ctrl->SetForegroundColour("Yellow");
                break;
            case 1:
                ctrl->SetForegroundColour("Green");
                break;
            }
            ctrl->SetLabelText(currLabel);
            if (quadState != -2)
                ctrl->SetToolTip(IndicatorToolTip(type, quadState));
        }
        else
        if (quadState != -2)
        {
            pic->SetToolTip(IndicatorToolTip(type, quadState));
        }

        lastState = quadState;

    }
}

wxString SBStateIndicatorItem::IndicatorToolTip(SBFieldTypes indType, int triState)
{
    wxString rslt = "";

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
            rslt += _("Completed, but no scope pointing info available");
            break;
        case 1:
            rslt += _("Completed, scope pointing info in-use");
            break;
        }

    }
    return rslt;
}

//--------------------------------------------------------------------------------------
// ---SBStateIndicators - the group of all app/session state controls, a mix of static text and bitmap controls
//
SBStateIndicators::SBStateIndicators(SBPanel* parent, std::vector <int> &fldWidths)
{
    parentPanel = parent;
    wxString labels[] = { _("Dark"), _("Cal"), wxEmptyString};

    for (int inx = 0; inx < numItems; inx++)
    {
        stateItems[inx] = new SBStateIndicatorItem(parent, Field_Darks + inx, labels[inx], (SBFieldTypes)(Field_Darks + inx), fldWidths);
        stateItems[inx]->UpdateState();
    }
}

SBStateIndicators::~SBStateIndicators()
{
    for (int inx = 0; inx < numItems; inx++)
        delete stateItems[inx];
}

void SBStateIndicators::PositionControls()
{
    for (int inx = 0; inx < numItems; inx++)
    {
        stateItems[inx]->PositionControl();
    }
}

void SBStateIndicators::UpdateState()
{
    for (int inx = 0; inx < numItems; inx++)
    {
        stateItems[inx]->UpdateState();
    }
}


// -----------  PHDStatusBar Class
//
PHDStatusBar::PHDStatusBar(wxWindow *parent, long style)
: wxStatusBar(parent, wxID_ANY, wxSTB_SHOW_TIPS | wxSTB_ELLIPSIZE_END | wxFULL_REPAINT_ON_RESIZE, "PHDStatusBar")


{
    wxClientDC dc(this);
    int txtWidth;
    int txtHeight;

    std::vector <int> fieldWidths;
    const int bitmapSize = 16;

    m_NumIndicators = 6;


    redLight = wxIcon("Ball_Red_xpm");
    yellowLight = wxIcon("Ball_Yellow_xpm");
    greenLight = wxIcon("Ball_Green_xpm");

    // Set up the only field the wxStatusBar base class will know about
    int widths[] = {-1};

    SetFieldsCount(1);
    SetStatusWidths(1, widths);
    this->SetBackgroundColour("BLACK");

    m_ctrlPanel = new SBPanel(this, wxSize(500, 16));
    m_ctrlPanel->SetPosition(wxPoint(1, 2));

    // Build the leftmost text status field, the only field managed at this level
    m_Msg1 = new wxStaticText(m_ctrlPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1));
    GetTextExtent("Sample message", &txtWidth, &txtHeight);         // only care about the height
    m_Msg1->SetBackgroundColour("BLACK");
    m_Msg1->SetForegroundColour("WHITE");
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
    sb->SetMinHeight(16);
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
    int fldWidth;
    wxRect fldRect;

    GetFieldRect(0, fldRect);
    fldWidth = fldRect.GetWidth();
    m_ctrlPanel->SetSize(fldWidth - 1, fldRect.GetHeight());
    m_Msg1->SetPosition(wxPoint(2, 3));
    m_StarIndicators->PositionControls();
    m_GuideIndicators->PositionControls();
    m_StateIndicators->PositionControls();

    event.Skip();
}

// Let client force update to various statusbar components
void PHDStatusBar::UpdateStates()
{
    m_StateIndicators->UpdateState();

}

void PHDStatusBar::UpdateStarInfo(double SNR, bool Saturated)
{
    m_StarIndicators->UpdateState(0, SNR, Saturated);
}

void PHDStatusBar::UpdateGuiderInfo(GUIDE_DIRECTION raDirection, GUIDE_DIRECTION decDirection, double raPx, double raPulse, double decPx, double decPulse)
{
    m_GuideIndicators->UpdateState(raDirection, decDirection, raPx, raPulse, decPx, decPulse);
}

void PHDStatusBar::ClearGuiderInfo()
{
    m_GuideIndicators->ClearState();
}

// Override function to be sure status text updates actually go to static text field
void PHDStatusBar::SetStatusText(const wxString &text, int number)
{
    wxString lastMsg = m_Msg1->GetLabelText();
    //if (lastMsg != text) // && text != wxEmptyString)
        m_Msg1->SetLabelText(text);
}

// Trivial class to handle the background color on the toolbar control
void PHDToolBarArt::DrawBackground(wxDC& dc, wxWindow* parent, const wxRect& rect)
{
    dc.SetBrush(wxColour(100, 100, 100));
    dc.DrawRectangle(rect);
}