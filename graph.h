/*
 *  graph.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

#ifndef GRAPHCLASS
#define GRAPHCLASS

class GraphControlPane;

class GraphLogClientWindow : public wxWindow
{
    GraphLogClientWindow(wxWindow *parent);
    ~GraphLogClientWindow(void);

    bool SetMinLength(int minLength);
    bool SetMaxLength(int maxLength);
    bool SetMinHeight(int minLength);
    bool SetMaxHeight(int minHeight);

    wxColour m_raOrDxColor, m_decOrDyColor;
    wxStaticText *m_pOscRMS, *m_pOscIndex;

    void AppendData (float dx, float dy, float RA, float Dec);
    void OnPaint(wxPaintEvent& evt);

    int m_minLength;
    int m_maxLength;

    int m_minHeight;
    int m_maxHeight;

    static const int m_xSamplesPerDivision = 50;
    static const int m_yDivisions = 3;

    struct S_HISTORY
    {
        double dx;
        double dy;
        double ra;
        double dec;
    } *m_pHistory;

    int m_nItems;    // # items in the history

    enum
    {
        MODE_RADEC,
        MODE_DXDY,
    } m_mode;

    int m_length;
    int m_height;

    friend class GraphLogWindow;

    DECLARE_EVENT_TABLE()
};

class GraphLogWindow : public wxWindow {
    wxWindow *m_pParent;
public:
    GraphLogWindow(wxWindow *parent);
    ~GraphLogWindow(void);
    void AppendData (float dx, float dy, float RA, float Dec);
    void UpdateControls(void);
    void SetState (bool is_active);
    void OnPaint(wxPaintEvent& evt);
    void OnButtonMode(wxCommandEvent& evt);
    void OnButtonLength(wxCommandEvent& evt);
    void OnButtonHeight(wxCommandEvent& evt);
    void OnButtonClear(wxCommandEvent& evt);
    void OnButtonZoomIn(wxCommandEvent& evt);
    void OnButtonZoomOut(wxCommandEvent& evt);

    wxStaticText *m_pLabel1, *m_pLabel2;

    wxColor GetRaOrDxColor(void);
    wxColor GetDecOrDyColor(void);

private:
    wxButton *m_pLengthButton;
    wxButton *m_pHeightButton;
    wxButton *m_pModeButton;
    wxButton *m_pClearButton;
    wxStaticText *RALabel;
    wxStaticText *DecLabel;
    wxStaticText *OscIndexLabel;
    wxStaticText *RMSLabel;
    wxBoxSizer *m_pControlSizer;
    GraphControlPane *m_pXControlPane;
    GraphControlPane *m_pYControlPane;
    GraphControlPane *m_pScopePane;

    float hdx[500]; // History of dx
    float hdy[500];
    float hra[500];
    float hdec[500];
    int n_items;    // # items in the history
    bool visible;
    int mode;   // 0 = RA/Dec, 1=dx, dy
    int length;
    float vertical_scale;

    int StringWidth(wxString string);

    bool m_visible;

    GraphLogClientWindow *m_pClient;

    DECLARE_EVENT_TABLE()
};

class GraphControlPane : public wxWindow
{
public:
    GraphControlPane(wxWindow *pParent, wxString label);
    ~GraphControlPane(void);
protected:
    wxWindow *m_pParent;
    wxBoxSizer *m_pControlSizer;
    //wxStaticBoxSizer *m_pControlSizer;

    int StringWidth(wxString string);
    void DoAdd(wxControl *pCtrl, wxString lbl);
};

#endif
