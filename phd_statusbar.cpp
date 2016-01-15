/*
*  phd_statusbar.cpp
*  PHD Guiding
*
*  Created by Bruce Waddington
*  Copyright (c) 2016 Bruce Waddington
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
#include "phd_statusbar.h"

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


// How this works: 
// PHDStatusBar is a child of wxStatusBar and is composed of various control groups - properties of the guide star, info about 
// current guide commands, and state information about the current app session.  Each group is managed by its own class, and that class 
// is responsible for building, positioning, and updating its controls. The various controls are positioned on top of the 
// underlying fields in the base-class statusbar via the OnSize event.


//-----------------------------------------------------------------------------
// SBStarIndicators - properties of the guide star
//
SBStarIndicators::SBStarIndicators(PHDStatusBar *parent, std::vector <int> &fldWidths)
{

    parent->GetTextExtent(_("Mass 125% "), &massWidth, &txtHeight);
    parent->GetTextExtent(_("SNR 100% "), &snrWidth, &txtHeight);
    parent->GetTextExtent(_("SAT"), &satWidth, &txtHeight);
    //fldWidths.push_back(massWidth);
    fldWidths.push_back(satWidth);
    fldWidths.push_back(snrWidth);


    //txtMassPct = new wxStaticText(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(massWidth, -1));
    //txtMassPct->SetBackgroundColour("BLACK");
    //txtMassPct->SetForegroundColour("GREEN");
    txtSaturated = new wxStaticText(parent, wxID_ANY, satStr, wxDefaultPosition, wxSize(satWidth, -1));
    txtSaturated->SetBackgroundColour("BLACK");
    txtSaturated->SetForegroundColour("RED");
    txtSaturated->Show(false);
    txtSNR = new wxStaticText(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(snrWidth, -1));
    txtSNR->SetBackgroundColour("BLACK");
    txtSNR->SetForegroundColour("GREEN");
    txtSNR->SetToolTip(_("Signal-to-noise ratio of guide star\nGreen means SNR >= 10\nYellow means  4 <= SNR < 10\nRed means SNR < 4"));

    parentSB = parent;
}

void SBStarIndicators::PositionControls()
{
    int fieldNum = (int)Field_Sat;

    //txtMassPct->SetPosition(parentSB->FieldLoc(fieldNum++, massWidth, txtHeight));
    txtSaturated->SetPosition(parentSB->FieldLoc(fieldNum++, satWidth, txtHeight));
    txtSNR->SetPosition(parentSB->FieldLoc(fieldNum++, snrWidth, txtHeight));

}

void SBStarIndicators::UpdateState(double MassPct, double SNR, bool Saturated)
{
    if (SNR >= 0)
    {
        //if (MassPct < 50.0)
        //    txtMassPct->SetForegroundColour("RED");
        //else
        //    txtMassPct->SetForegroundColour("GREEN");
        //txtMassPct->SetLabelText(wxString::Format("%s %0.0f%%", massStr, MassPct));
        if (SNR >= 10.0)
            txtSNR->SetForegroundColour("Green");
        else
        {
            if (SNR >= 4.0)
                txtSNR->SetForegroundColour("Yellow");
            else
                txtSNR->SetForegroundColour("Red");

            //txtMassPct->SetForegroundColour("GREEN");
        }
        txtSNR->Show(true);
        txtSNR->SetLabelText(wxString::Format("%s %0.1f", SNRStr, SNR));
        txtSaturated->Show(Saturated);
    }
    else
    {
        txtSaturated->Show(false);
        txtSNR->Show(false);
    }
}

//-----------------------------------------------------------------------------
// SBGuideIndicators - info about the most recent guide commands
//
SBGuideIndicators::SBGuideIndicators(PHDStatusBar* parent, std::vector <int> &fldWidths)
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
    txtRaAmounts = new wxStaticText(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(guideAmtWidth, bitmapSize.y));
    txtRaAmounts->SetBackgroundColour("BLACK");
    txtRaAmounts->SetForegroundColour(fgColor);
    txtDecAmounts = new wxStaticText(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(guideAmtWidth, bitmapSize.y));
    txtDecAmounts->SetBackgroundColour("Black");
    txtDecAmounts->SetForegroundColour(fgColor);
    bitmapDec = new wxStaticBitmap(parent, wxID_ANY, icoUp);
    bitmapDec->Show(false);
    parentSB = parent;

    fldWidths.push_back(bitmapSize.x + guideAmtWidth + 6);          // RA info
    fldWidths.push_back(bitmapSize.x + guideAmtWidth + 6);          // Dec info
}

void SBGuideIndicators::PositionControls()
{
    int fieldNum = (int)Field_RAInfo;
    int txtWidth;
    int txtHeight;

    bitmapRA->SetPosition(parentSB->FieldLoc(fieldNum, -1, -1));
    wxString txtSizer = "38 ms, 0.45 px";
    parentSB->GetTextExtent(txtSizer, &txtWidth, &txtHeight);
    wxPoint raPosition = parentSB->FieldLoc(fieldNum, txtWidth, -1);
    raPosition.x += 6;
    txtRaAmounts->SetPosition(raPosition);

    fieldNum++;
    txtSizer = "120 ms, 4.38 px";
    parentSB->GetTextExtent(txtSizer, &txtWidth, &txtHeight);
    wxPoint decPosition = parentSB->FieldLoc(fieldNum, -1, -1);
    txtDecAmounts->SetPosition(decPosition);

    decPosition.x += txtWidth + 2;
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
SBStateIndicatorItem::SBStateIndicatorItem(PHDStatusBar* parent, int indField, const wxString &indLabel, SBFieldTypes indType, std::vector <int> &fldWidths)
{
    type = indType;
    lastState = -2;
    parentSB = parent;
    fieldId = indField;
    otherInfo = wxEmptyString;
    parentSB->GetTextExtent(indLabel, &txtWidth, &txtHeight);
    if (indType != Field_Gear)
    {
        ctrl = new wxStaticText(parentSB, wxID_ANY, indLabel);
        fldWidths.push_back(txtWidth);
    }
    else
    {
        pic = new wxStaticBitmap(parentSB, wxID_ANY, wxIcon(Ball_Green_xpm));
        fldWidths.push_back(16);
    }
}

void SBStateIndicatorItem::PositionControl()
{
    if (type == Field_Gear)
        pic->SetPosition(parentSB->FieldLoc(fieldId, 16, 16));
    else
        ctrl->SetPosition(parentSB->FieldLoc(fieldId, txtWidth, txtHeight));
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
    //case Field_Mount:
    //    rslt = _("Mount: ") + (triState == 1 ? _("Connected") : _("Disconnected"));
    //    break;
    //case Field_AO:
    //    rslt = _("AO: ") + (triState == 1 ? _("Connected") : _("Disconnected"));
    //    break;
    //case Field_Rot:
    //    rslt = _("Rotator: ") + (triState == 1 ? _("Connected") : _("Disconnected"));
    //    break;
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
SBStateIndicators::SBStateIndicators(PHDStatusBar* parent, std::vector <int> &fldWidths)
{
    parentSB = parent;
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
: wxStatusBar(parent, wxID_ANY, style, "PHDStatusBar")


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

    // Set up some dummy widths just to get the statusbar control built
    int widths[] = { -1, 10, 10, 10, 10, 10, 10, 10 };

    SetFieldsCount(Field_Max);
    SetStatusWidths(Field_Max, widths);
    this->SetBackgroundColour("BLACK");

    // Build the leftmost text status field, the only field managed at this level
    m_Msg1 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1));
    GetTextExtent("Sample message", &txtWidth, &txtHeight);         // only care about the height
    m_Msg1->SetPosition(FieldLoc(0, -1, txtHeight));
    m_Msg1->SetBackgroundColour("BLACK");
    m_Msg1->SetForegroundColour("WHITE");
    fieldWidths.push_back(-1);

    // Build the star status fields
    m_StarIndicators = new SBStarIndicators(this, fieldWidths);

    // Build the guide indicators
    m_GuideIndicators = new SBGuideIndicators(this, fieldWidths);

    // Build the state indicator controls
    m_StateIndicators = new SBStateIndicators(this, fieldWidths);

    SetStatusWidths(Field_Max, &fieldWidths[0]);
    SetMinHeight(txtHeight);
}

// Destructor
PHDStatusBar::~PHDStatusBar()
{
    this->DestroyChildren();        // any wxWidgets objects will be deleted
    delete m_StateIndicators;
    delete m_GuideIndicators;
    delete m_StarIndicators;
}

// Utility function to position our controls over the fields in the statusbar
wxPoint PHDStatusBar::FieldLoc(int fieldNum, int ctrlWidth, int ctrlHeight)
{
    wxRect fieldRect;
    int ctrlX;
    int ctrlY;
    GetFieldRect(fieldNum, fieldRect);
    if (ctrlWidth != -1)
        ctrlX = fieldRect.x + (fieldRect.width - ctrlWidth) / 2;
    else
        ctrlX = fieldRect.x + 2;
    if (ctrlHeight != -1)
        ctrlY = fieldRect.y + (fieldRect.height - ctrlHeight) / 2;
    else
        ctrlY = fieldRect.y;

    return(wxPoint(ctrlX, ctrlY));
}

void PHDStatusBar::OnSize(wxSizeEvent& event)
{
    int txtWidth;
    int txtHeight;

    // Position the custom fields on top of the auto-positioned fields
    GetTextExtent(m_Msg1->GetLabelText(), &txtWidth, &txtHeight);
    m_Msg1->SetPosition(FieldLoc(0, -1, txtHeight));
    m_StarIndicators->PositionControls();
    m_GuideIndicators->PositionControls();
    m_StateIndicators->PositionControls();

    event.Skip();
}

// Let client force update to state indicators
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
    m_Msg1->SetLabelText(text);
}
