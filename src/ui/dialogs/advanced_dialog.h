/*
 *  advanced_dialog.h
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

#ifndef ADVANCED_DIALOG_H_INCLUDED
#define ADVANCED_DIALOG_H_INCLUDED

class MyFrame;
class MyFrameConfigDialogPane;
class MyFrameConfigDialogCtrlSet;
class MountConfigDialogCtrlSet;
class CameraConfigDialogPane;
class GuiderConfigDialogPane;
class wxTipWindow;

enum TAB_PAGES
{
    AD_GLOBAL_PAGE,
    AD_GUIDER_PAGE,
    AD_CAMERA_PAGE,
    AD_MOUNT_PAGE,
    AD_AO_PAGE,
    AD_ROTATOR_PAGE,
    AD_UNASSIGNED_PAGE
};

class AdvancedDialog : public wxDialog
{
    MyFrame *m_pFrame;
    wxBookCtrlBase *m_pNotebook;
    MyFrameConfigDialogPane *m_pGlobalPane;
    Guider::GuiderConfigDialogPane *m_pGuiderPane;
    CameraConfigDialogPane *m_pCameraPane;
    Mount::MountConfigDialogPane *m_pMountPane;
    AOConfigDialogPane *m_pAOPane;
    RotatorConfigDialogPane *m_pRotatorPane;

    BrainCtrlIdMap m_brainCtrls;
    bool m_rebuildPanels;
    MyFrameConfigDialogCtrlSet *m_pGlobalCtrlSet;
    CameraConfigDialogCtrlSet *m_pCameraCtrlSet;
    GuiderConfigDialogCtrlSet *m_pGuiderCtrlSet;
    MountConfigDialogCtrlSet *m_pScopeCtrlSet;
    AOConfigDialogCtrlSet *m_pAOCtrlSet;
    RotatorConfigDialogCtrlSet *m_pRotatorCtrlSet;
    wxPanel *m_pGlobalSettingsPanel;
    wxPanel *m_pCameraSettingsPanel;
    wxPanel *m_pGuiderSettingsPanel;
    wxPanel *m_pScopeSettingsPanel;
    wxPanel *m_pDevicesSettingsPanel;
    wxTipWindow *m_tip;
    wxTimer *m_tipTimer;
    bool m_imageScaleChanged;

public:
    static const double MIN_FOCAL_LENGTH;
    static const double MAX_FOCAL_LENGTH;

    AdvancedDialog(MyFrame *pFrame);
    ~AdvancedDialog();

    void EndModal(int retCode) override;

    void UpdateCameraPage();
    void UpdateMountPage();
    void UpdateAoPage();
    void UpdateRotatorPage();

    void LoadValues();
    void UnloadValues();
    void Undo();
    void Preload();

    bool Validate() override;
    void ShowInvalid(wxWindow *ctrl, const wxString& message);
    double PercentChange(double oldVal, double newVal);
    void FlagImageScaleChange()
    {
        m_imageScaleChanged = true;
    } // Allows image scale adjustment to be made only once when AD is closed
    int GetFocalLength();
    void SetFocalLength(int val);
    double GetPixelSize();
    void SetPixelSize(double val);
    int GetBinning();
    void SetBinning(int binning);
    void MakeImageScaleAdjustments();
    Mount::MountConfigDialogPane *GetCurrentMountPane() { return m_pMountPane; }

    wxWindow *GetTabLocation(BRAIN_CTRL_IDS id);

private:
    void AddCameraPage();
    void AddMountPage();
    void AddAoPage();
    void AddRotatorPage();
    size_t FindPage(wxWindow *ctrl);
    void RebuildPanels();
    void BuildCtrlSets();
    void CleanupCtrlSets();
    void ConfirmLayouts();
    double DetermineGuideSpeed();
};

#endif // ADVANCED_DIALOG_H_INCLUDED
