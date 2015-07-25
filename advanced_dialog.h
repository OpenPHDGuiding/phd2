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
class CameraConfigDialogPane;

enum TAB_PAGES {
    GLOBAL_PAGE,
    GUIDER_PAGE,
    CAMERA_PAGE,
    MOUNT_PAGE,
    AO_PAGE,
    ROTATOR_PAGE,
    UNASSIGNED_PAGE
};
// Segmented by the tab page location seen in the UI
enum BRAIN_CTRL_IDS
{
    UNASSIGNED,
    cbResetConfig,
    cbDontAsk,
    szImageLoggingFormat,
    szLanguage,
    szLogFileInfo,
    szDitherParams,
    GLOBAL_TAB_BOUNDARY,        //-----end of global tab controls
    cbUseSubFrames,
    szNoiseReduction,
    szAutoExposure,
    szCameraTimeout,
    szTimeLapse,
    szPixelSize,
    szGain,
    szDelay,
    szPort,
    CAMERA_TAB_BOUNDARY,        // ------ end of camera tab controls
    szFocalLength,
    cbAutoRestoreCal

};

struct BrainCtrlInfo
{
    BRAIN_CTRL_IDS ctrlId;
    wxObject* panelCtrl;
    TAB_PAGES ctrlHost;
    bool isPositioned;           // debug only

    BrainCtrlInfo::BrainCtrlInfo()
    {
        panelCtrl = NULL;
        ctrlId = UNASSIGNED;
        ctrlHost = UNASSIGNED_PAGE;
        isPositioned = false;
    }
    BrainCtrlInfo::BrainCtrlInfo(BRAIN_CTRL_IDS id, wxObject* ctrl)
    {
        panelCtrl = ctrl;
        ctrlId = id;
        ctrlHost = UNASSIGNED_PAGE;
        isPositioned = false;
    }

};

class AdvancedDialog : public wxDialog
{
    MyFrame *m_pFrame;
    wxBookCtrlBase *m_pNotebook;
    wxWindow *m_aoPage;
    wxWindow *m_rotatorPage;
    MyFrameConfigDialogPane *m_pGlobalPane;
    ConfigDialogPane *m_pGuiderPane;
    CameraConfigDialogPane *m_pCameraPane;
    ConfigDialogPane *m_pMountPane;
    ConfigDialogPane *m_pAoPane;
    ConfigDialogPane *m_rotatorPane;

    std::map <BRAIN_CTRL_IDS, BrainCtrlInfo> m_brainCtrls;
    MyFrameConfigDialogCtrlSet *m_pGlobalCtrlSet;
    CameraConfigDialogCtrlSet *m_pCameraCtrlSet;
    wxPanel *m_pGlobalSettingsPanel;
    wxPanel *m_pCameraSettingsPanel;

public:
    AdvancedDialog(MyFrame *pFrame);
    ~AdvancedDialog();

    void EndModal(int retCode);

    void UpdateCameraPage(void);
    void UpdateMountPage(void);
    void UpdateAoPage(void);
    void UpdateRotatorPage(void);

    void LoadValues(void);
    void UnloadValues(void);
    void Undo(void);

    int GetFocalLength(void);
    void SetFocalLength(int val);
    double GetPixelSize(void);
    void SetPixelSize(double val);

    wxWindow* GetTabLocation(BRAIN_CTRL_IDS id);


private:
    void AddCameraPage(void);
    void AddMountPage(void);
    void AddAoPage(void);
    void AddRotatorPage(void);
    void RebuildPanels(void);
};

#endif // ADVANCED_DIALOG_H_INCLUDED
