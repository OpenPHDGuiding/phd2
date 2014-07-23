/*  profile_wizard.h
 *  PHD Guiding
 *
 *  Created by Bruce Waddington
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

#ifndef ProfileWizard_h_included
#define ProfileWizard_h_included

class ProfileWizard : public wxDialog
{
public:
    enum DialogState
    {
        STATE_GREETINGS = 0,
        STATE_CAMERA,
        STATE_MOUNT,
        STATE_AUXMOUNT,
        STATE_AO,
        STATE_WRAPUP,
        STATE_DONE, NUM_PAGES = STATE_DONE
    };

    enum CtrlIds
    {
        ID_COMBO = 10001,
        ID_PIXELSIZE,
        ID_DETECT_PIXELSIZE,
        ID_FOCALLENGTH,
        ID_PREV,
        ID_NEXT
    };

private:
    // wx UI controls
    wxBoxSizer *m_pvSizer;
    wxStaticBitmap *m_bitmap;
    wxStaticText *m_pInstructions;
    wxStaticText *m_pGearLabel;
    wxChoice *m_pGearChoice;
    wxSpinCtrlDouble *m_pPixelSize;
    wxButton *m_detectPixelSizeBtn;
    wxSpinCtrlDouble *m_pFocalLength;
    wxButton *m_pPrevBtn;
    wxButton *m_pNextBtn;
    wxStaticBoxSizer *m_pHelpGroup;
    wxStaticText *m_pHelpText;
    wxFlexGridSizer *m_pGearGrid;
    wxFlexGridSizer *m_pUserProperties;
    wxFlexGridSizer *m_pWrapUp;
    wxTextCtrl *m_pProfileName;
    wxCheckBox *m_pLaunchDarks;
    wxStatusBar *m_pStatusBar;

    wxString m_SelectedCamera;
    wxString m_SelectedMount;
    bool m_PositionAware;
    wxString m_SelectedAuxMount;
    wxString m_SelectedAO;
    int m_FocalLength;
    double m_PixelSize;
    wxString m_ProfileName;
    wxBitmap *m_bitmaps[NUM_PAGES];

    void OnNext(wxCommandEvent& evt);
    void OnPrev(wxCommandEvent& evt);
    void OnGearChoice(wxCommandEvent& evt);
    void OnDetectPixelSize(wxCommandEvent& evt);
    void OnPixelSizeChange(wxSpinDoubleEvent& evt);
    void OnFocalLengthChange(wxSpinDoubleEvent& evt);
    void ShowStatus(const wxString& msg, bool appending = false);
    void UpdateState(const int change);
    bool SemanticCheck(DialogState state, int change);
    void ShowHelp(DialogState state);
    void WrapUp();

    DialogState m_State;

public:

    bool m_launchDarks;

    ProfileWizard(wxWindow *parent, bool firstLight);
    ~ProfileWizard(void);

    wxDECLARE_EVENT_TABLE();
};

#endif
