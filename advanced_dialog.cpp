/*
 *  advanced_dialog.cpp
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

#include "phd.h"

#if defined(__WXOSX__)
# include <wx/choicebk.h>
#endif

// a place to save id of selected panel so we can select the same panel next time the dialog is opened
static int s_selectedPage = -1;

Mount* AdvancedDialog::RealMount()
{
    Mount *mount = NULL;
    if (pSecondaryMount)
        mount = pSecondaryMount;
    else if (pMount && !pMount->IsStepGuider())
        mount = pMount;
    return mount;

}
AdvancedDialog::AdvancedDialog(MyFrame *pFrame) :
    wxDialog(pFrame, wxID_ANY, _("Advanced setup"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    /*
     * the advanced dialog is made up of a number of "on the fly" generated slices that configure different things.
     *
     * pTopLevelSizer is a top level Box Sizer in wxVERTICAL mode that contains a pair of sizers,
     * pConfigSizer to hold all the configuration panes and an unamed Button sizer and the OK and CANCEL buttons.
     *
     * pConfigSizer is a Horizontal Box Sizer which contains two Vertical Box sizers, one
     * for each column of panes
     *
     * +------------------------------------+------------------------------------+
     * |    General (Frame) Settings        |   Guider Base Class Settings       |
     * +------------------------------------|                                    |
     * |    Mount  Base Class Settings      |   Ra Guide Algorithm Settings      |
     * |                                    |                                    |
     * |    Mount  Sub Class Settings       |   Dec Guide Alogrithm Settings     |
     * +------------------------------------|                                    |
     * |    Camera Base Class Settings      |   Guider Sub Class Settings        |
     * |                                    |------------------------------------+
     * |    Camera Sub  Calss Settings      |                                    |
     * +------------------------------------|                                    |
     * |    Camera Base Class Settings      |                                    |
     * +-------------------------------------------------------------------------|
     * |                              OK and Cancel Buttons                      |
     * +-------------------------------------------------------------------------+
     *
     */

#if defined(__WXOSX__)
    m_pNotebook = new wxChoicebook(this, wxID_ANY);
#else
    m_pNotebook = new wxNotebook(this, wxID_ANY);
#endif
    m_pFrame = pFrame;      // We get called before global var is initialized

    wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL).Border(wxALL,2).Expand();

    // build tabs -- each needs the tab, and a sizer.  Once built
    // it needs to be populated

    // Build all the panels first
    m_pGlobalSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pGlobalTabSizer = new wxBoxSizer(wxVERTICAL);
    m_pGlobalSettingsPanel->SetSizer(pGlobalTabSizer);
    m_pNotebook->AddPage(m_pGlobalSettingsPanel, _("Global"), true);
    // Camera pane
    m_pCameraSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pCameraTabSizer = new wxBoxSizer(wxVERTICAL);
    m_pCameraSettingsPanel->SetSizer(pCameraTabSizer);
    m_pNotebook->AddPage(m_pCameraSettingsPanel, _("Camera"), false);
    // Build the guider pane
    m_pGuiderSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pGuidingTabSizer = new wxBoxSizer(wxVERTICAL);
    m_pGuiderSettingsPanel->SetSizer(pGuidingTabSizer);
    m_pNotebook->AddPage(m_pGuiderSettingsPanel, _("Guiding"));
    // Mount pane
    m_pScopeSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pScopeTabSizer = new wxBoxSizer(wxVERTICAL);
    m_pScopeSettingsPanel->SetSizer(pScopeTabSizer);
    m_pNotebook->AddPage(m_pScopeSettingsPanel, _("Algorithms"));
    // Devices pane - home for AO and rotator
    m_pDevicesSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pDevicesTabSizer = new wxBoxSizer(wxVERTICAL);
    m_pDevicesSettingsPanel->SetSizer(pDevicesTabSizer);
    m_pNotebook->AddPage(m_pDevicesSettingsPanel, _("Devices"));

    // Build the initial ConfigControlSets
    m_pGlobalCtrlSet = pFrame->GetConfigDlgCtrlSet(pFrame, this, m_brainCtrls);
    if (pCamera)
        m_pCameraCtrlSet = pCamera->GetConfigDlgCtrlSet(m_pCameraSettingsPanel, pCamera, this, m_brainCtrls);
    else
        m_pCameraCtrlSet = NULL;
    m_pGuiderCtrlSet = pFrame->pGuider->GetConfigDialogCtrlSet(m_pGuiderSettingsPanel, pFrame->pGuider, this, m_brainCtrls);

    if (pMount && pMount->IsStepGuider())
        m_pAOCtrlSet = new StepGuiderConfigDialogCtrlSet(m_pDevicesSettingsPanel, (StepGuider*)pMount, this, m_brainCtrls);
    else
        m_pAOCtrlSet = NULL;
    if (pRotator)
        m_pRotatorCtrlSet = new RotatorConfigDialogCtrlSet(m_pDevicesSettingsPanel, pRotator, this, m_brainCtrls);
    else
        m_pRotatorCtrlSet = NULL;

    Mount* bigMount = RealMount();
    m_pScopeCtrlSet = new ScopeConfigDialogCtrlSet(m_pGuiderSettingsPanel, (Scope*)bigMount, this, m_brainCtrls);

    // Populate global pane
    m_pGlobalPane = pFrame->GetConfigDialogPane(m_pGlobalSettingsPanel);
    m_pGlobalPane->LayoutControls(m_brainCtrls);
    pGlobalTabSizer->Add(m_pGlobalPane, sizer_flags);

    // Populate the camera pane
    AddCameraPage();

    // Populate the guiding pane
    m_pGuiderPane = pFrame->pGuider->GetConfigDialogPane(m_pGuiderSettingsPanel);
    m_pGuiderPane->LayoutControls(pFrame->pGuider, m_brainCtrls);
    pGuidingTabSizer->Add(m_pGuiderPane, sizer_flags);

    // Build Mount tab
    AddMountPage();

    // Build AO tab
    AddAoPage();

    // Add page for rotator
    AddRotatorPage();

    wxBoxSizer *pTopLevelSizer = new wxBoxSizer(wxVERTICAL);
    pTopLevelSizer->Add(m_pNotebook, wxSizerFlags(0).Expand().Border(wxALL, 5));
    pTopLevelSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags(0).Expand().Border(wxALL, 5));
    SetSizerAndFit(pTopLevelSizer);

    m_rebuildPanels = false;
}

AdvancedDialog::~AdvancedDialog()
{
}

void AdvancedDialog::RebuildPanels(void)
{
    wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL).Border(wxALL,2).Expand();

    delete m_pGlobalCtrlSet;
    m_pGlobalCtrlSet = NULL;
    delete m_pCameraCtrlSet;
    m_pCameraCtrlSet = NULL;
    delete m_pGuiderCtrlSet;
    m_pGuiderCtrlSet = NULL;
    delete m_pScopeCtrlSet;
    m_pScopeCtrlSet = NULL;
    delete m_pAOCtrlSet;
    m_pAOCtrlSet = NULL;
    delete m_pRotatorCtrlSet;
    m_pRotatorCtrlSet = NULL;

    m_pGlobalPane->Clear(true);
    m_pCameraPane->Clear(true);
    
    m_pCameraSettingsPanel->GetSizer()->Clear(true);
    m_pGuiderPane->Clear(true);

    if (m_pMountPane)
    {
        m_pMountPane->Clear(true);
        m_pScopeSettingsPanel->GetSizer()->Clear(true);
    }
    if (m_pAOPane)
    {
        m_pAOPane->Clear(true);
    }
    if (m_pRotatorPane)
    {
        m_pRotatorPane->Clear(true);
    }
    if (m_pRotatorPane != NULL || m_pAOPane != NULL)
        m_pDevicesSettingsPanel->GetSizer()->Clear(true);

    m_brainCtrls.clear();

    m_pGlobalCtrlSet = m_pFrame->GetConfigDlgCtrlSet(m_pFrame, this, m_brainCtrls);
    if (pCamera)
        m_pCameraCtrlSet = pCamera->GetConfigDlgCtrlSet(m_pCameraSettingsPanel, pCamera, this, m_brainCtrls);
    else
        m_pCameraCtrlSet = NULL;
    m_pGuiderCtrlSet = m_pFrame->pGuider->GetConfigDialogCtrlSet(m_pGuiderSettingsPanel, m_pFrame->pGuider, this, m_brainCtrls);

    Mount *bigMount = RealMount();

    if (pMount && pMount->IsStepGuider())
        m_pAOCtrlSet = new StepGuiderConfigDialogCtrlSet(m_pDevicesSettingsPanel, pMount, this, m_brainCtrls);
    if (pRotator)
        m_pRotatorCtrlSet = new RotatorConfigDialogCtrlSet(m_pDevicesSettingsPanel, pRotator, this, m_brainCtrls);

    // Need a scope ctrl set even if pMount is null - it exports generic controls needed by other panes
    m_pScopeCtrlSet = new ScopeConfigDialogCtrlSet(m_pGuiderSettingsPanel, (Scope*)bigMount, this, m_brainCtrls);

    m_pGlobalPane->LayoutControls(m_brainCtrls);
    m_pGlobalPane->Layout();

    AddCameraPage();

    m_pGuiderPane->LayoutControls(m_pFrame->pGuider, m_brainCtrls);     // Guider pane doesn't have specific device dependencies
    m_pGuiderPane->Layout();
 
    AddMountPage();

    AddAoPage();            // Will handle no AO case
    AddRotatorPage();

    if (m_pAOPane == NULL && m_pRotatorPane == NULL)
    {
            int idx = m_pNotebook->FindPage(m_pDevicesSettingsPanel);
            if (idx != wxNOT_FOUND)
                m_pNotebook->RemovePage(idx);
    }
    else
    {
        int idx = m_pNotebook->FindPage(m_pDevicesSettingsPanel);
        if (idx == wxNOT_FOUND)
            m_pNotebook->AddPage(m_pDevicesSettingsPanel, _("Devices"));
    }

    GetSizer()->Layout();
    GetSizer()->Fit(this);
    m_rebuildPanels = false;
}

wxWindow* AdvancedDialog::GetTabLocation(BRAIN_CTRL_IDS id)
{
    if (id < GLOBAL_TAB_BOUNDARY)
        return (wxWindow*)m_pGlobalSettingsPanel;
    else
    if (id < CAMERA_TAB_BOUNDARY)
        return (wxWindow*)m_pCameraSettingsPanel;
    else
    if (id < GUIDER_TAB_BOUNDARY)
        return (wxWindow*)m_pGuiderSettingsPanel;
    else
    if (id < MOUNT_TAB_BOUNDARY)
        return (wxWindow*)m_pScopeSettingsPanel;
    else
    if (id < DEVICES_TAB_BOUNDARY)
        return (wxWindow*)m_pDevicesSettingsPanel;
    else
        return NULL;              // FIX THIS
}

void AdvancedDialog::AddCameraPage(void)
{
    // Even if pCamera is null, the pane hosts other controls
    if (pCamera)
        m_pCameraPane = pCamera->GetConfigDialogPane(m_pCameraSettingsPanel);
    else
        m_pCameraPane = new CameraConfigDialogPane(m_pCameraSettingsPanel, pCamera);
    m_pCameraPane->LayoutControls(pCamera, m_brainCtrls);
    m_pCameraPane->Layout();

    m_pCameraSettingsPanel->GetSizer()->Add(m_pCameraPane);
    m_pScopeSettingsPanel->Layout();
}

void AdvancedDialog::AddMountPage(void)
{
    const long ID_NOMOUNT = 99999;
    Mount* mount = RealMount();

    if (mount)
    {
        wxWindow* noMsgWindow = m_pScopeSettingsPanel->FindWindow(ID_NOMOUNT);
        if (noMsgWindow)
            noMsgWindow->Destroy();
        m_pMountPane = mount->GetConfigDialogPane(m_pScopeSettingsPanel);
        m_pMountPane->LayoutControls(m_pScopeSettingsPanel, m_brainCtrls);
        m_pMountPane->Layout();
    }
    else
    {
        m_pMountPane = new Mount::MountConfigDialogPane(m_pScopeSettingsPanel, _("Mount"), mount);
        wxStaticText *pNoMount = new wxStaticText(m_pScopeSettingsPanel, ID_NOMOUNT, _("No mount specified"));
        m_pMountPane->Add(pNoMount);
    }

    m_pScopeSettingsPanel->GetSizer()->Add(m_pMountPane);
    m_pScopeSettingsPanel->Layout();
}

void AdvancedDialog::AddAoPage(void)
{
    if (pMount && pMount->IsStepGuider())
    {
        // We have an AO selected

        m_pAOPane = pMount->GetConfigDialogPane(m_pDevicesSettingsPanel);
        m_pAOPane->LayoutControls(m_pDevicesSettingsPanel, m_brainCtrls);
        m_pAOPane->Layout();

        m_pDevicesSettingsPanel->GetSizer()->Add(m_pAOPane, wxSizerFlags(0).Border(wxTOP, 10).Expand());
        m_pDevicesSettingsPanel->Layout();
    }
    else
    {
        m_pAOPane = NULL;
    }
}

void AdvancedDialog::AddRotatorPage(void)
{
    if (pRotator)
    {
        // We have a rotator selected
        m_pRotatorPane = new RotatorConfigDialogPane(m_pDevicesSettingsPanel, pRotator);
        m_pRotatorPane->LayoutControls(m_pDevicesSettingsPanel, m_brainCtrls);
        m_pRotatorPane->Layout();

        m_pDevicesSettingsPanel->GetSizer()->Add(m_pRotatorPane, wxSizerFlags(0).Border(wxTOP, 10).Expand());
        m_pDevicesSettingsPanel->Layout();
    }
    else
    {
        m_pRotatorPane = NULL;
    }
}

void AdvancedDialog::UpdateCameraPage(void)
{
    // This can be late-binding - one call to RebuildPanels when it's actually needed
    m_rebuildPanels = true;

}

void AdvancedDialog::UpdateMountPage(void)
{
    // This can be late-binding - one call to RebuildPanels when it's actually needed
    m_rebuildPanels = true;
}

void AdvancedDialog::UpdateAoPage(void)
{
    m_rebuildPanels = true;
}

void AdvancedDialog::UpdateRotatorPage(void)
{
    m_rebuildPanels = true;
}

void AdvancedDialog::LoadValues(void)
{
    Mount* bigMount = RealMount();
    // Late-binding rebuild of all the panels
    if (m_rebuildPanels)
        RebuildPanels();
    // Load all the current params
    m_pGlobalCtrlSet->LoadValues();
    if (m_pCameraCtrlSet)
        m_pCameraCtrlSet->LoadValues();
    if (m_pGuiderCtrlSet)
        m_pGuiderCtrlSet->LoadValues();
    if (pMount)
    {
        if (pMount->IsStepGuider())
        {
            m_pAOCtrlSet->LoadValues();
            m_pAOPane->LoadValues();
        }
        if (bigMount)
        {
            m_pScopeCtrlSet->LoadValues();
            m_pMountPane->LoadValues();
        }
    }
    if (s_selectedPage != -1)
        m_pNotebook->ChangeSelection(s_selectedPage);

}

void AdvancedDialog::UnloadValues(void)
{
    Mount* bigMount = RealMount();
    // Unload all the current params
    m_pGlobalCtrlSet->UnloadValues();
    if (m_pCameraCtrlSet)
        m_pCameraCtrlSet->UnloadValues();
    if (m_pGuiderCtrlSet)
        m_pGuiderCtrlSet->UnloadValues();
    if (pMount)
    {
        if (pMount->IsStepGuider())
        {
            m_pAOCtrlSet->LoadValues();
            m_pAOPane->UnloadValues();
        }
        if (bigMount)
        {
            m_pScopeCtrlSet->UnloadValues();
            m_pMountPane->UnloadValues();
        }
    }

}

void AdvancedDialog::Undo(void)
{
    ConfigDialogPane *const panes[] =
        { m_pGlobalPane, m_pGuiderPane, m_pCameraPane, m_pMountPane, m_pAOPane, m_pRotatorPane };

    for (unsigned int i = 0; i < WXSIZEOF(panes); i++)
    {
        ConfigDialogPane *const pane = panes[i];
        if (pane)
            pane->Undo();
    }
}

void AdvancedDialog::EndModal(int retCode)
{
    s_selectedPage = m_pNotebook->GetSelection();
    wxDialog::EndModal(retCode);
}

int AdvancedDialog::GetFocalLength(void)
{
    return m_pGlobalCtrlSet->GetFocalLength();
}

void AdvancedDialog::SetFocalLength(int val)
{
    m_pGlobalCtrlSet->SetFocalLength(val);
}

double AdvancedDialog::GetPixelSize(void)
{
    return m_pCameraCtrlSet ? m_pCameraCtrlSet->GetPixelSize() : 0.0;
}

void AdvancedDialog::SetPixelSize(double val)
{
    if (m_pCameraCtrlSet)
        m_pCameraCtrlSet->SetPixelSize(val);
}
