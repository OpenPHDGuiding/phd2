/*
*  aui_controls.h
*  PHD Guiding
*
*  Created by Bruce Waddington
*  Copyright (c) 2016-2017 Bruce Waddington and Andy Galasso
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

#ifndef _AUI_CONTROLS_H_
#define _AUI_CONTROLS_H_

class SBPanel;
class SBStateIndicators;
class SBStarIndicators;
class SBGuideIndicators;

// Child of normal status bar - used for status bar with color-coded messages and state indicators
class PHDStatusBar : public wxStatusBar
{
    SBPanel *m_ctrlPanel;
    SBStateIndicators *m_StateIndicators;
    SBStarIndicators *m_StarIndicators;
    SBGuideIndicators *m_GuideIndicators;
    wxStaticText *m_Msg1;

    PHDStatusBar(wxWindow *parent, long style = wxSTB_DEFAULT_STYLE);
    virtual ~PHDStatusBar();

public:
    static PHDStatusBar *CreateInstance(wxWindow *parent, long style = wxSTB_DEFAULT_STYLE);

    void StatusMsg(const wxString& text);

    void OverlayMsg(const wxString& text);
    void ClearOverlayMsg();

    void UpdateStates();

    void UpdateStarInfo(double SNR, bool Saturated);
    void ClearStarInfo() { UpdateStarInfo(-1, 0); }

    void UpdateGuiderInfo(const GuideStepInfo& step);
    void ClearGuiderInfo();

    int GetMinSBWidth();

    // event handlers
    void OnSize(wxSizeEvent& event);

private:
    wxDECLARE_EVENT_TABLE();
};

// Minor subclass to force the toolbar background to be what we want
class PHDToolBarArt : public wxAuiDefaultToolBarArt
{
    virtual void DrawBackground(wxDC& dc, wxWindow *parent, const wxRect& rect);
    virtual wxAuiToolBarArt *Clone() { return new PHDToolBarArt(*this); }
};

#endif      // AUI_CONTROLS_H
