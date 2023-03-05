/*
 *  graph-stepguider.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
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

 */

#ifndef GRAPH_STEPGUIDER_H_INCLUDED
#define GRAPH_STEPGUIDER_H_INCLUDED

class GraphStepguiderClient;

class GraphStepguiderWindow : public wxWindow
{
public:
    GraphStepguiderWindow(wxWindow *parent);
    ~GraphStepguiderWindow(void);

    void OnButtonLength(wxCommandEvent& evt);
    void OnMenuLength(wxCommandEvent& evt);
    void OnButtonClear(wxCommandEvent& evt);

    void SetLimits(unsigned int xMax, unsigned int yMax, unsigned int xBump, unsigned int yBump);
    void AppendData(const wxPoint& pos, const PHD_Point& avgPos);
    void ShowBump(const PHD_Point& curBump);

    bool SetState(bool is_active);

private:
    OptionsButton *LengthButton;
    wxButton *ClearButton;
    wxStaticText *m_hzText;
    GraphStepguiderClient *m_pClient;

    wxLongLong_t m_prevTimestamp;

    bool m_visible;

    DECLARE_EVENT_TABLE()
};

#endif // GRAPH_STEPGUIDER_H_INCLUDED
