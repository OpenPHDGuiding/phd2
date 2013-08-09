/*
 *  gear_dialog.h
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
 *    Neither the name of Bret McKee, Dad Dog Development, Ltd. nor the names of its
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

#ifndef GEAR_DIALOG_H_INCLUDED
#define GEAR_DIALOG_H_INCLUDED

class GearDialog: public wxDialog
{
    GuideCamera *m_pCamera;
    Scope       *m_pScope;
    StepGuider  *m_pStepGuider;

    wxChoice *m_pCameras;
    wxToggleButton *m_pConnectCameraButton;

    wxChoice *m_pScopes;
    wxToggleButton *m_pConnectScopeButton;

    wxChoice *m_pStepGuiders;
    wxToggleButton *m_pConnectStepGuiderButton;

    wxButton *m_pConnectAllButton;
    wxButton *m_pDisconnectAllButton;

public:
    GearDialog(wxWindow *pParent);
    ~GearDialog(void);

    void Initialize(void);
    int ShowModal(bool autoConnect);
    void EndModal(int retCode);

private:
    void UpdateGearPointers(void);

    void UpdateCameraButtonState(void);
    void UpdateScopeButtonState(void);
    void UpdateStepGuiderButtonState(void);
    void UpdateConnectAllButtonState(void);
    void UpdateDisconnectAllButtonState(void);
    void UpdateButtonState(void);

    void OnButtonConnectAll(wxCommandEvent& event);
    void OnButtonDisconnectAll(wxCommandEvent& event);

    void OnChoiceCamera(wxCommandEvent& event);
    void OnButtonConnectCamera(wxCommandEvent& event);
    void OnButtonDisconnectCamera(wxCommandEvent& event);

    void OnChoiceScope(wxCommandEvent& event);
    void OnButtonConnectScope(wxCommandEvent& event);
    void OnButtonDisconnectScope(wxCommandEvent& event);

    void OnChoiceStepGuider(wxCommandEvent& event);
    void OnButtonConnectStepGuider(wxCommandEvent& event);
    void OnButtonDisconnectStepGuider(wxCommandEvent& event);
private:
    DECLARE_EVENT_TABLE()
};

#endif // GEAR_DIALOG_H_INCLUDED
