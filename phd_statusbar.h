/*
*  phd_statusbar.h
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

#ifndef _PHD_STATUSBAR_H_
#define _PHD_STATUSBAR_H_

// Types of fields in the statusbar
enum SBFieldTypes
{
    Field_Msg1,
    Field_Msg2,
    Field_Darks,
    Field_Calib,
    Field_Cam,
    Field_Mount,
    Field_AO,
    Field_Rot,
    Field_Max
};

class PHDStatusBar;

// Class for color-coded state indicators
class SBStateIndicator
{
public:
    SBFieldTypes type;
    wxString label;
    int txtHeight;
    int txtWidth;
    int fieldId;
    int lastState;
    PHDStatusBar* parentSB;
    wxStaticText* ctrl;

public:
    SBStateIndicator(PHDStatusBar* parent, int indField, const wxString &indLabel, SBFieldTypes indType);
    void UpdateState();
    wxString IndicatorToolTip(SBFieldTypes indType, int triState);
};

// Child of normal status bar - used for status bar with color-coded messages and state indicators
class PHDStatusBar : public wxStatusBar
{
public:
    PHDStatusBar(wxWindow *parent, long style = wxSTB_DEFAULT_STYLE);
    virtual ~PHDStatusBar();

    wxPoint FieldLoc(int fieldNum, int crtlWidth, int ctrlHeight);
    void SetStatusText(const wxString &text, int number = 0);
    void UpdateStates();

    // event handlers
    void OnSize(wxSizeEvent& event);

#if wxUSE_TIMER
    wxTimer m_timer;
#endif

private:
    SBStateIndicator* m_StateIndicators[Field_Max];
    wxStaticText* m_Msg1;
    wxStaticText* m_Msg2;
    int m_NumIndicators;
    bool m_even = true;

    wxDECLARE_EVENT_TABLE();
};
#endif      // PHD_STATUS_H