/*
 *  darks_dialog.h
 *  PHD Guiding
 *
 *  Created by Bruce Waddington in collaboration with David Ault and Andy Galasso
 *  Copyright (c) 2014 Bruce Waddington
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

#ifndef DarksDialog_h_included
#define DarksDialog_h_included

class DarksDialog : public wxDialog
{
private:
    bool m_cancelling;
    bool m_started;
    // wx UI controls
    wxComboBox *m_pDarkMinExpTime;
    wxComboBox *m_pDarkMaxExpTime;
    wxSpinCtrl *m_pDarkCount;
    wxSpinCtrl *m_pDefectExpTime;
    wxSpinCtrl *m_pNumDefExposures;
    wxRadioButton *m_rbModifyDarkLib;
    wxRadioButton *m_rbNewDarkLib;
    wxTextCtrl *m_pNotes;
    wxGauge *m_pProgress;
    wxButton *m_pStartBtn;
    wxButton *m_pResetBtn;
    wxStatusBar *m_pStatusBar;
    wxButton *m_pStopBtn;
    wxArrayString m_expStrings;
    void OnStart(wxCommandEvent& evt);
    void OnStop(wxCommandEvent& evt);
    void OnReset(wxCommandEvent& evt);
    void SaveProfileInfo();
    void ShowStatus(const wxString msg, bool appending);
    bool CreateMasterDarkFrame(usImage& dark, int expTime, int frameCount);

public:
    DarksDialog(wxWindow *parent, bool darkLibrary);
    ~DarksDialog(void);

private:
    bool buildDarkLib;

};

#endif
