/*
 *  target.h
 *  PHD Guiding
 *
 *  Created by Sylvain Girard
 *  Copyright (c) 2013 Sylvain Girard
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
 *    Neither the name of Bret McKee, Dad Dog Development Ltd, nor the names of its
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
 *
 */

#ifndef TARGET_H_INCLUDED
#define TARGET_H_INCLUDED

class TargetWindow;

class TargetClient : public wxWindow
{
    TargetClient(wxWindow *parent);
    ~TargetClient(void);

    static const unsigned int m_maxHistorySize = 400;

    unsigned int m_minLength;
    unsigned int m_maxLength;

    unsigned int m_minHeight;
    unsigned int m_maxHeight;

    struct
    {
        double ra;
        double dec;
    } m_history[m_maxHistorySize];

    unsigned int m_nItems;    // # of items in the history
    unsigned int m_length;     // # of items to display
    double m_zoom;
    double m_refCircleRadius;

    void AppendData(const GuideStepInfo& step);

    void OnPaint(wxPaintEvent& evt);

    friend class TargetWindow;

    DECLARE_EVENT_TABLE()
};

class TargetWindow : public wxWindow
{
    OptionsButton *m_lengthButton;
    wxCheckBox *m_enableRefCircle;
    wxSpinCtrlDouble *m_refCircleRadius;
    TargetClient *m_pClient;
    bool m_visible;

public:
    TargetWindow(wxWindow *parent);
    ~TargetWindow(void);

    void AppendData(const GuideStepInfo& step);
    void SetState(bool is_active);
    void UpdateControls(void);

private:
    void OnButtonLength(wxCommandEvent& evt);
    void OnMenuLength(wxCommandEvent& evt);
    void OnButtonClear(wxCommandEvent& evt);
    void OnButtonZoomIn(wxCommandEvent& evt);
    void OnButtonZoomOut(wxCommandEvent& evt);
    void OnCheckBoxRefCircle(wxCommandEvent& event);
    void OnRefCircleRadius(wxSpinDoubleEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // TARGET_H_INCLUDED
