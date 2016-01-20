/*
*  phd_statusbar.h
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

#ifndef _PHD_STATUSBAR_H_
#define _PHD_STATUSBAR_H_

// Types of fields in the statusbar
enum SBFieldTypes
{
    Field_StatusMsg,
    Field_Sat,
    Field_SNR,
    Field_RAInfo,
    Field_DecInfo,
    Field_Darks,
    Field_Calib,
    Field_Gear,
    Field_Max
};

class PHDStatusBar;

class PHDToolBarArt:public wxAuiDefaultToolBarArt
{
    virtual void DrawPlainBackground(wxDC& dc, wxWindow* parent, const wxRect& rect);
    virtual void DrawBackground(wxDC& dc, wxWindow* parent, const wxRect& rect);
    virtual wxAuiToolBarArt* Close() { return new PHDToolBarArt(*this); }
};

// Self-drawn panel for hosting controls in the wxStatusBar
class SBPanel : public wxPanel
{
    std::vector <int> fieldOffsets;

public:
    SBPanel(wxStatusBar* parent, wxSize panelSize);

    void OnPaint(wxPaintEvent& evt);
    void BuildFieldOffsets(std::vector <int> &fldWidths);
    wxPoint FieldLoc(int fieldId);
    int emWidth;

    wxDECLARE_EVENT_TABLE();
};

// Class for color-coded state indicators
class SBStateIndicatorItem
{
public:
    SBFieldTypes type;
    int txtHeight;
    int txtWidth;
    int fieldId;
    int lastState;
    SBPanel* parentPanel;
    wxStaticText* ctrl;
    wxStaticBitmap* pic;
    wxString otherInfo;

public:
    SBStateIndicatorItem(SBPanel* parent, int indField, const wxString &indLabel, SBFieldTypes indType, std::vector <int> &fldWidths);
    void PositionControl();
    void UpdateState();
    wxString IndicatorToolTip(SBFieldTypes indType, int triState);
};

class SBStateIndicators
{
    SBStateIndicatorItem* stateItems[Field_Max - Field_Darks];
    int numItems = Field_Max - Field_Darks;
    SBPanel* parentPanel;


public:
    SBStateIndicators(SBPanel* parent, std::vector <int> &fldWidths);
    ~SBStateIndicators();
    void PositionControls();
    void UpdateState();

};

class SBGuideIndicators
{
    wxStaticBitmap* bitmapRA;
    wxStaticBitmap* bitmapDec;
    wxStaticText* txtRaAmounts;
    wxStaticText* txtDecAmounts;
    wxIcon icoLeft;
    wxIcon icoRight;
    wxIcon icoUp;
    wxIcon icoDown;
    SBPanel* parentPanel;

public:
    SBGuideIndicators(SBPanel* parent, std::vector <int> &fldWidths);
    void PositionControls();
    void UpdateState(GUIDE_DIRECTION raDirection, GUIDE_DIRECTION decDirection, double raPx, double raPulse, double decPx, double decPulse);
    void ClearState() { UpdateState(LEFT, UP, 0, 0, 0, 0); }

};

class SBStarIndicators
{
    wxStaticText* txtMassPct;
    wxStaticText* txtSNRLabel;
    wxStaticText* txtSNRValue;
    wxStaticText* txtSaturated;
    const wxString massStr = _("Mass");
    const wxString SNRStr = _("SNR");
    const wxString satStr = _("SAT");
    int txtHeight;
    int snrLabelWidth;
    SBPanel* parentPanel;

public:
    SBStarIndicators(SBPanel *parent, std::vector <int> &fldWidths);
    void PositionControls();
    void UpdateState(double MassPct, double SNR, bool Saturated);

};

// Child of normal status bar - used for status bar with color-coded messages and state indicators
class PHDStatusBar : public wxStatusBar
{
public:
    PHDStatusBar(wxWindow *parent, long style = wxSTB_DEFAULT_STYLE);
    virtual ~PHDStatusBar();

    wxIcon yellowLight;
    wxIcon redLight;
    wxIcon greenLight;
    void SetStatusText(const wxString &text, int number = 0);
    void UpdateStates();
    void UpdateStarInfo(double SNR, bool Saturated);
    void ClearStarInfo() { UpdateStarInfo(-1, 0); }
    void UpdateGuiderInfo(GUIDE_DIRECTION raDirection, GUIDE_DIRECTION decDirection, double raPx, double raPulse, double decPx, double decPulse);
    void ClearGuiderInfo();

    // event handlers
    void OnSize(wxSizeEvent& event);

private:
    SBPanel *m_ctrlPanel;
    SBStateIndicators* m_StateIndicators;
    SBStarIndicators* m_StarIndicators;
    SBGuideIndicators* m_GuideIndicators;
    wxStaticText* m_Msg1;
    int m_NumIndicators;

    wxDECLARE_EVENT_TABLE();
};
#endif      // PHD_STATUS_H