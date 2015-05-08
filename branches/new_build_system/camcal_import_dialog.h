/*
 *  camcal_import_dialog.h
 *  PHD Guiding
 *
 *  Created by Bruce Waddington
 *  Copyright (c) 2015 Bruce Waddington
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

#ifndef CamCalImportDialog_h_included
#define CamCalImportDialog_h_included

class CamCalImportDialog : public wxDialog
{
public:

    CamCalImportDialog(wxWindow *parent);
    ~CamCalImportDialog(void);

private:
    wxChoice* m_darksChoice;
    wxStaticText* m_darkCameraChoice;
    wxChoice* m_bpmChoice;
    wxStaticText* m_bpmCameraChoice;
    void FindCompatibleDarks(wxArrayString* pResults);
    void FindCompatibleBPMs(wxArrayString* pResults);
    void OnDarkProfileChoice(wxCommandEvent& evt);
    void OnBPMProfileChoice(wxCommandEvent& evt);
    void OnOk(wxCommandEvent& evt);
    wxArrayString m_profileNames;
    wxString m_activeProfileName;
    int m_sourceDarksProfileId;
    int m_sourceBpmProfileId;
    int m_thisProfileId;
};

#endif