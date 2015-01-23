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

class wxGridBagSizer;

class GearDialog : public wxDialog
{
    GuideCamera *m_pCamera;
    Scope       *m_pScope;
    Scope       *m_pAuxScope;
    StepGuider  *m_pStepGuider;
    Rotator     *m_pRotator;

    bool m_cameraUpdated;
    bool m_mountUpdated;
    bool m_stepGuiderUpdated;
    bool m_rotatorUpdated;
    bool m_showDarksDialog;
    bool m_ascomScopeSelected;

    wxGridBagSizer *m_gearSizer;

    wxChoice *m_profiles;
    OptionsButton *m_btnProfileManage;
    wxMenu *m_menuProfileManage;

    wxChoice *m_pCameras;
    wxButton *m_pSetupCameraButton;
    wxToggleButton *m_pConnectCameraButton;

    wxChoice *m_pScopes;
    wxButton *m_pSetupScopeButton;
    wxToggleButton *m_pConnectScopeButton;

    wxChoice *m_pAuxScopes;
    wxButton *m_pSetupAuxScopeButton;
    wxToggleButton *m_pConnectAuxScopeButton;

    wxButton *m_moreButton;
    bool m_showMoreGear;

    wxChoice *m_pStepGuiders;
    wxButton *m_pSetupStepGuiderButton;
    wxToggleButton *m_pConnectStepGuiderButton;

    wxChoice *m_pRotators;
    wxButton *m_pSetupRotatorButton;
    wxToggleButton *m_pConnectRotatorButton;

    wxButton *m_pConnectAllButton;
    wxButton *m_pDisconnectAllButton;

public:
    GearDialog(wxWindow *pParent);
    ~GearDialog(void);

    void Initialize(void);
    int ShowGearDialog(bool autoConnect);
    void EndModal(int retCode);

    void ShowProfileWizard(void);
    bool SetProfile(int profileId, wxString *error);
    bool ConnectAll(wxString *error);
    bool DisconnectAll(wxString *error);
    void Shutdown(bool forced);

private:
    void LoadGearChoices(void);
    void UpdateGearPointers(void);

    void UpdateCameraButtonState(void);
    void UpdateScopeButtonState(void);
    void UpdateAuxScopeButtonState(void);
    void UpdateStepGuiderButtonState(void);
    void UpdateRotatorButtonState(void);
    void UpdateConnectAllButtonState(void);
    void UpdateDisconnectAllButtonState(void);
    void UpdateButtonState(void);
    void UpdateAdvancedDialog(void);

    void OnProfileChoice(wxCommandEvent& event);
    void OnButtonProfileManage(wxCommandEvent& event);
    void OnProfileNew(wxCommandEvent& event);
    void OnProfileDelete(wxCommandEvent& event);
    void OnProfileRename(wxCommandEvent& event);
    void OnProfileLoad(wxCommandEvent& event);
    void OnProfileSave(wxCommandEvent& event);
    void OnAdvanced(wxCommandEvent& event);

    void OnButtonConnectAll(wxCommandEvent& event);
    void OnButtonDisconnectAll(wxCommandEvent& event);
    void OnChar(wxKeyEvent& event);

    void OnChoiceCamera(wxCommandEvent& event);
    void OnButtonSetupCamera(wxCommandEvent& event);
    void OnButtonConnectCamera(wxCommandEvent& event);
    void OnButtonDisconnectCamera(wxCommandEvent& event);

    void OnChoiceScope(wxCommandEvent& event);
    void OnButtonSetupScope(wxCommandEvent& event);
    void OnButtonConnectScope(wxCommandEvent& event);
    void OnButtonDisconnectScope(wxCommandEvent& event);

    void OnChoiceAuxScope(wxCommandEvent& event);
    void OnButtonSetupAuxScope(wxCommandEvent& event);
    void OnButtonConnectAuxScope(wxCommandEvent& event);
    void OnButtonDisconnectAuxScope(wxCommandEvent& event);

    void OnButtonMore(wxCommandEvent& event);
    void ShowMoreGear();

    void OnChoiceStepGuider(wxCommandEvent& event);
    void OnButtonSetupStepGuider(wxCommandEvent& event);
    void OnButtonConnectStepGuider(wxCommandEvent& event);
    void OnButtonDisconnectStepGuider(wxCommandEvent& event);

    void OnChoiceRotator(wxCommandEvent& event);
    void OnButtonSetupRotator(wxCommandEvent& event);
    void OnButtonConnectRotator(wxCommandEvent& event);
    void OnButtonDisconnectRotator(wxCommandEvent& event);

    void OnButtonWizard(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // GEAR_DIALOG_H_INCLUDED
