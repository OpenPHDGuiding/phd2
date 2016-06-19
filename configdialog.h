/*
 *  configdialog.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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

#ifndef CONFIG_DIALOG_H_INCLUDED
#define CONFIG_DIALOG_H_INCLUDED

// Segmented by the tab page location seen in the UI
enum BRAIN_CTRL_IDS
{
    AD_UNASSIGNED,
    AD_cbResetConfig,
    AD_cbDontAsk,
    AD_szImageLoggingFormat,
    AD_szLanguage,
    AD_szLogFileInfo,
    AD_szDither,
    AD_GLOBAL_TAB_BOUNDARY,        //-----end of global tab controls
    AD_cbUseSubFrames,
    AD_szNoiseReduction,
    AD_szAutoExposure,
    AD_szCameraTimeout,
    AD_szTimeLapse,
    AD_szPixelSize,
    AD_szGain,
    AD_szDelay,
    AD_szPort,
    AD_binning,
    AD_cooler,
    AD_CAMERA_TAB_BOUNDARY,        // ------ end of camera tab controls
    AD_cbScaleImages,
    AD_szFocalLength,
    AD_cbAutoRestoreCal,
    AD_cbFastRecenter,
    AD_szStarTracking,
    AD_cbClearCalibration,
    AD_cbEnableGuiding,
    AD_szCalibrationDuration,
    AD_cbReverseDecOnFlip,
    AD_cbAssumeOrthogonal,
    AD_cbSlewDetection,
    AD_cbUseDecComp,
    AD_GUIDER_TAB_BOUNDARY,        // --------------- end of guiding tab controls
    AD_cbDecComp,
    AD_szDecCompAmt,
    AD_szMaxRAAmt,
    AD_szMaxDecAmt,
    AD_szDecGuideMode,
    AD_MOUNT_TAB_BOUNDARY,          // ----------- end of mount tab controls
    AD_AOTravel,
    AD_szCalStepsPerIteration,
    AD_szSamplesToAverage,
    AD_szBumpPercentage,
    AD_szBumpSteps,
    AD_cbBumpOnDither,
    AD_cbClearAOCalibration,
    AD_cbEnableAOGuiding,
    AD_cbRotatorReverse,
    AD_DEVICES_TAB_BOUNDARY         // ----------- end of devices tab controls
};

struct BrainCtrlInfo
{
    wxObject *panelCtrl;
    bool isPositioned;           // debug only

    BrainCtrlInfo()
        :
        panelCtrl(0),
        isPositioned(false)
    {
    }

    BrainCtrlInfo(BRAIN_CTRL_IDS id, wxObject *ctrl)
        :
        panelCtrl(ctrl),
        isPositioned(false)
    {
    }
};

typedef std::map<BRAIN_CTRL_IDS, BrainCtrlInfo> BrainCtrlIdMap;

class ConfigDialogPane : public wxStaticBoxSizer
{
protected:
    wxWindow *m_pParent;
public:
    ConfigDialogPane(const wxString& heading, wxWindow *pParent);
    virtual ~ConfigDialogPane(void) {};

    virtual void LoadValues(void) = 0;
    virtual void UnloadValues(void) = 0;
    virtual void Undo();

    wxWindow *GetSingleCtrl(BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS id);
    wxSizer *GetSizerCtrl(BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS id);
    void CondAddCtrl(wxSizer *szr, BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS id, const wxSizerFlags& flags = 0);

protected:
    wxSizer *MakeLabeledControl(const wxString& label, wxWindow *pControl, const wxString& toolTip, wxWindow *pControl2 = NULL);
    void DoAdd(wxSizer *pSizer);
    void DoAdd(wxWindow *pWindow);
    void DoAdd(wxWindow *pWindow, const wxString& toolTip);
    void DoAdd(const wxString& Label, wxWindow *pControl, const wxString& toolTip, wxWindow *pControl2 = NULL);

    int StringWidth(const wxString& string);
    int StringArrayWidth(wxString string[], int nElements);
};

class MyFrame;
class AdvancedDialog;

// ConfigDialogCtrlSet objects create and manage the UI controls and associated semantics - but they don't control tab locations or layout - those functions
// are done by the ConfigDialogPane objects
class ConfigDialogCtrlSet
{
protected:
    wxWindow *m_pParent;
    AdvancedDialog *m_pAdvDlg;

public:
    ConfigDialogCtrlSet(wxWindow *pParent, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap);
    virtual ~ConfigDialogCtrlSet(void) {};

    virtual void LoadValues(void) = 0;
    virtual void UnloadValues(void) = 0;

public:
    wxSizer *MakeLabeledControl(BRAIN_CTRL_IDS id, const wxString& label, wxWindow *pControl, const wxString& toolTip);
    void AddMapElement(BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS, wxObject *pElem);
    void AddGroup(BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS id, wxSizer *pSizer);      // Sizer
    void AddCtrl(BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS id, wxControl *pCtrl);      // Bare control
    void AddLabeledCtrl(BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS id, const wxString& Label, wxControl *pCtrl, const wxString& toolTip);
    void AddCtrl(BrainCtrlIdMap& CtrlMap, BRAIN_CTRL_IDS id, wxControl *pCtrl, const wxString& toolTip);     // Control with tooltip

    wxWindow *GetParentWindow(BRAIN_CTRL_IDS id);

    int StringWidth(const wxString& string);
    int StringArrayWidth(wxString string[], int nElements);
    int StringArrayWidth(const wxArrayString& ary);
};
#endif // CONFIG_DIALOG_H_INCLUDED
