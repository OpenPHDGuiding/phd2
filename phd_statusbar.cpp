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

wxBEGIN_EVENT_TABLE(PHDStatusBar, wxStatusBar)
EVT_SIZE(PHDStatusBar::OnSize)

wxEND_EVENT_TABLE()


// How this works: the PHDStatusBar is derived from wxStatusBar.  The parent class is given a list of fields with related field widths, and the parent handles the horizontal
// spacing of these fields as the statusbar is resized.  In order to get the desired color theme,  static text controls are created and managed by PHDStatusBar.  These
// controls, which hold the actual text content, are always positioned directly above the underlying statusbar fields.

// -----------  SBStatusIndicator Class
//
SBStateIndicator::SBStateIndicator(PHDStatusBar* parent, int indField, const wxString &indLabel, SBFieldTypes indType)
{
    type = indType;
    label = indLabel;
    lastState = -3;     
    parentSB = parent;
    fieldId = indField;
    parentSB->GetTextExtent(label, &txtWidth, &txtHeight);
    ctrl = new wxStaticText(parentSB, wxID_ANY, label);
}

// Update text color based on current state; Perform layout
void SBStateIndicator::UpdateState()
{
    int quadState = -1;

    switch (type)
    {
    case Field_Cam:
        if (pCamera && pCamera->Connected)
            quadState = 1;
        break;
    case Field_Mount:
        if ((pMount && pMount->IsConnected()) || pSecondaryMount && pSecondaryMount->IsConnected())
            quadState = 1;
        break;
    case Field_AO:
        if (pMount && pMount->IsStepGuider())
        {
            if (pMount->IsConnected())
                quadState = 1;
        }
        else
            quadState = -2;
        break;
    case Field_Rot:
        if (pRotator)
        {
            if (pRotator->IsConnected())
                quadState = 1;
        }
        else
            quadState = -2;
        break;
    case Field_Darks:
        if (pFrame)
            if (pFrame->m_useDarksMenuItem->IsChecked() || pFrame->m_useDefectMapMenuItem->IsChecked())
                quadState = 1;
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
        wxString currLabel = ctrl->GetLabelText();
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
        {
            ctrl->SetToolTip(IndicatorToolTip(type, quadState));
        }

        lastState = quadState;

    }
    ctrl->SetPosition(parentSB->FieldLoc(fieldId, txtWidth, txtHeight));
   
}

// Load context-sensitive tooltips for the state fields
wxString SBStateIndicator::IndicatorToolTip(SBFieldTypes indType, int quadState)
{
    wxString rslt = "";

    switch (indType)
    {
    case Field_Cam:
        rslt = _("Camera: ") + (quadState == 1 ? _("Connected") : _("Disconnected"));
        break;
    case Field_Mount:
        rslt = _("Mount: ") + (quadState == 1 ? _("Connected") : _("Disconnected"));
        break;
    case Field_AO:
        rslt = _("AO: ") + (quadState == 1 ? _("Connected") : _("Disconnected"));
        break;
    case Field_Rot:
        rslt = _("Rotator: ") + (quadState == 1 ? _("Connected") : _("Disconnected"));
        break;
    case Field_Darks:
        rslt = _("Darks/Bad-pix map: ") + (quadState == 1 ? _("In-use") : _("Not in-use"));
        break;
    case Field_Calib:
        rslt = _("Calibration: ");
        switch (quadState)
        {
        case -1:
            rslt += _("Not completed");
            break;
        case 0: 
            rslt += _("Completed, pointing adjustments inactive");
            break;
        case 1:
            rslt += _("Completed, pointing adjustments active");
            break;
        }

    }
    return rslt;
}

// -----------  PHDStatusBar Class
//
PHDStatusBar::PHDStatusBar(wxWindow *parent, long style)
        : wxStatusBar(parent, wxID_ANY, style, "PHDStatusBar")
#if wxUSE_TIMER
            , m_timer(this)
#endif

{
    wxClientDC dc(this);

    int widths[Field_Max];
    int txtWidth;
    int txtHeight;

    m_NumIndicators = 6;
    wxString labels[] = { _("Dk"), _("Cl"), _("Cm"), _("Mt"), _("AO"), _("Rt") };

    GetTextExtent(labels[0], &txtWidth, &txtHeight);

    widths[0] = -3; // growable, same proportions as before
    widths[1] = -5;
    for (int inx = 0; inx < m_NumIndicators; inx++)
        widths[inx + 2] = txtWidth + 4;

    SetFieldsCount(m_NumIndicators + 2);
    SetStatusWidths(m_NumIndicators + 2, widths);

    this->SetBackgroundColour("BLACK");
    // Build the two text status fields and get them positioned
    m_Msg1 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150,-1));
    m_Msg1->SetPosition(FieldLoc(0, -1, txtHeight));
    m_Msg1->SetBackgroundColour("BLACK");
    m_Msg1->SetForegroundColour("WHITE");
    m_Msg2 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(50, -1));
    m_Msg2->SetPosition(FieldLoc(1, -1, txtHeight));
    m_Msg2->SetBackgroundColour("BLACK");
    m_Msg2->SetForegroundColour("WHITE");
    // Build and position the state indicator controls
    for (int inx = 0; inx < m_NumIndicators; inx++)
    {
        m_StateIndicators[inx] = new SBStateIndicator(this, inx + 2, labels[inx], (SBFieldTypes)(Field_Darks + inx));
        m_StateIndicators[inx]->UpdateState();
    }

    SetMinHeight(txtHeight);
}

// Clean up the StateIndicator objects
PHDStatusBar::~PHDStatusBar()
{
    this->DestroyChildren();        // Handles the wxWidget controls
    for (int inx = 0; inx < m_NumIndicators; inx++)
        delete m_StateIndicators[inx];
}

// Util function to compute the correct horiz/vert location of the text relative to the underlying field
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

// Handle size event, which will force field positioning and state updates
void PHDStatusBar::OnSize(wxSizeEvent& event)
{
    int txtWidth;
    int txtHeight;

    // Position the custom fields on top of the auto-positioned fields
    GetTextExtent(m_Msg1->GetLabelText(), &txtWidth, &txtHeight);
    m_Msg1->SetPosition(FieldLoc(0, -1, txtHeight));
    m_Msg2->SetPosition(FieldLoc(1, -1, txtHeight));
    for (int inx = 0; inx < m_NumIndicators; inx++)
        m_StateIndicators[inx]->UpdateState();
    
    event.Skip();
}

// Let client force update to state indicators
void PHDStatusBar::UpdateStates()
{
    for (int inx = 0; inx < m_NumIndicators; inx++)
        m_StateIndicators[inx]->UpdateState();
}

// OVerride function to be sure status text updates actually go to static text fields
void PHDStatusBar::SetStatusText(const wxString &text, int number)
{
    if (number == 0)
        m_Msg1->SetLabelText(text);
    else
    if (number == 1)
        m_Msg2->SetLabelText(text);
}
