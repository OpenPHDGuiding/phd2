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

static AdvancedDialog *s_advancedDialogInstance;

AdvancedDialog::AdvancedDialog() :
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

    s_advancedDialogInstance = this;

#if defined(__WXOSX__)
    m_pNotebook = new wxChoicebook(this, wxID_ANY);
#else
    m_pNotebook = new wxNotebook(this, wxID_ANY);
#endif

    wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL).Border(wxALL,2).Expand();

    // build tabs -- each needs the tab, and a sizer.  Once built
    // it needs to be populated

    // Build the global tab pane
    wxPanel *pGlobalSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pGlobalTabSizer = new wxBoxSizer(wxVERTICAL);
    pGlobalSettingsPanel->SetSizer(pGlobalTabSizer);
    m_pNotebook->AddPage(pGlobalSettingsPanel, _("Global"), true);

    // and populate it
    m_pFramePane = pFrame->GetConfigDialogPane(pGlobalSettingsPanel);
    pGlobalTabSizer->Add(m_pFramePane, sizer_flags);

    // Build the guider tab
    wxPanel *pGuiderSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pGuidingTabSizer = new wxBoxSizer(wxVERTICAL);
    pGuiderSettingsPanel->SetSizer(pGuidingTabSizer);
    m_pNotebook->AddPage(pGuiderSettingsPanel, _("Guiding"));

    // and populate it
    m_pGuiderPane = pFrame->pGuider->GetConfigDialogPane(pGuiderSettingsPanel);
    pGuidingTabSizer->Add(m_pGuiderPane, sizer_flags);

    // Build the camera tab
    wxPanel *pCameraSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pCameraTabSizer = new wxBoxSizer(wxVERTICAL);
    pCameraSettingsPanel->SetSizer(pCameraTabSizer);
    m_pNotebook->AddPage(pCameraSettingsPanel, _("Camera"));

    // and populate it
    if (pCamera)
    {
        m_pCameraPane = pCamera->GetConfigDialogPane(pCameraSettingsPanel);
        if (m_pCameraPane)
        {
            pCameraTabSizer->Add(m_pCameraPane, sizer_flags);
        }
    }
    else
    {
        m_pCameraPane=NULL;
        wxStaticBoxSizer *pBox = new wxStaticBoxSizer(new wxStaticBox(pCameraSettingsPanel, wxID_ANY, _("Camera Settings")), wxVERTICAL);
        wxStaticText *pText = new wxStaticText(pCameraSettingsPanel, wxID_ANY, _("No Camera Selected"),wxPoint(-1,-1),wxSize(-1,-1));
        pBox->Add(pText);
        pCameraTabSizer->Add(pBox, sizer_flags);
    }

    // Build scope and AO tabs
    wxPanel *pScopeSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pScopeTabSizer = new wxBoxSizer(wxVERTICAL);
    pScopeSettingsPanel->SetSizer(pScopeTabSizer);
    m_pNotebook->AddPage(pScopeSettingsPanel, _("Mount"));

    wxPanel *pAoSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pAoTabSizer = new wxBoxSizer(wxVERTICAL);
    pAoSettingsPanel->SetSizer(pAoTabSizer);
    m_pNotebook->AddPage(pAoSettingsPanel, _("AO"));

    // and populate them
    m_pSecondaryMountPane = NULL;
    m_pMountPane          = NULL;

    if (pSecondaryMount)
    {
        assert(pMount);
        // Since there are two mounts, we have an AO connected and can populatate both tabs.
        m_pMountPane = pMount->GetConfigDialogPane(pAoSettingsPanel);
        m_pSecondaryMountPane = pSecondaryMount->GetConfigDialogPane(pScopeSettingsPanel);

        // The secondary mount config goes on the scope tab
        pScopeTabSizer->Add(m_pSecondaryMountPane, sizer_flags);

        // and the primary mount config goes on the Adaptive Optics tab
        pAoTabSizer->Add(m_pMountPane, sizer_flags);

    }
    else
    {
        // Add a text box to the AO tab informing the user there is no AO

        wxStaticBoxSizer *pBox = new wxStaticBoxSizer(new wxStaticBox(pAoSettingsPanel, wxID_ANY, _("AO Settings")), wxVERTICAL);
        wxStaticText *pText = new wxStaticText(pAoSettingsPanel, wxID_ANY, _("No AO Selected"),wxPoint(-1,-1),wxSize(-1,-1));
        pBox->Add(pText);
        pAoTabSizer->Add(pBox, sizer_flags);

        // and then either add the mount config dialog or a text box

        if (pMount)
        {
            // We only have a scope
            m_pMountPane = pMount->GetConfigDialogPane(pScopeSettingsPanel);

            // so it goes on the scope tab
            pScopeTabSizer->Add(m_pMountPane, sizer_flags);
        }
        else
        {
            // Add a text box to the Mount tab informing the user there is no Mount
            pBox = new wxStaticBoxSizer(new wxStaticBox(pScopeSettingsPanel, wxID_ANY, _("Mount Settings")), wxVERTICAL);
            pText = new wxStaticText(pScopeSettingsPanel, wxID_ANY, _("No Mount Selected"),wxPoint(-1,-1),wxSize(-1,-1));
            pBox->Add(pText);

            pScopeTabSizer->Add(pBox, sizer_flags);
        }
    }

    wxBoxSizer *pTopLevelSizer = new wxBoxSizer(wxVERTICAL);
    pTopLevelSizer->Add(m_pNotebook, wxSizerFlags(0).Expand().Border(wxALL, 5));
    pTopLevelSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags(0).Expand().Border(wxALL, 5));
    SetSizerAndFit(pTopLevelSizer);
}

AdvancedDialog::~AdvancedDialog()
{
    s_advancedDialogInstance = 0;
}

AdvancedDialog *AdvancedDialog::Instance()
{
    return s_advancedDialogInstance;
}

void AdvancedDialog::LoadValues(void)
{
    ConfigDialogPane *const panes[] =
        { m_pFramePane, m_pMountPane, m_pSecondaryMountPane, m_pGuiderPane, m_pCameraPane };

    for (unsigned int i = 0; i < WXSIZEOF(panes); i++)
    {
        ConfigDialogPane *const pane = panes[i];
        if (pane)
            pane->LoadValues();
    }

    if (s_selectedPage != -1)
        m_pNotebook->ChangeSelection(s_selectedPage);
}

void AdvancedDialog::UnloadValues(void)
{
    ConfigDialogPane *const panes[] =
        { m_pFramePane, m_pMountPane, m_pSecondaryMountPane, m_pGuiderPane, m_pCameraPane };

    for (unsigned int i = 0; i < WXSIZEOF(panes); i++)
    {
        ConfigDialogPane *const pane = panes[i];
        if (pane)
            pane->UnloadValues();
    }
}

void AdvancedDialog::Undo(void)
{
    ConfigDialogPane *const panes[] =
        { m_pFramePane, m_pMountPane, m_pSecondaryMountPane, m_pGuiderPane, m_pCameraPane };

    for (unsigned int i = 0; i < WXSIZEOF(panes); i++)
    {
        ConfigDialogPane *const pane = panes[i];
        if (pane)
            pane->Undo();
    }
}

void AdvancedDialog::OnSetupCamera(wxCommandEvent& WXUNUSED(event))
{
    // Prior to this we check to make sure the current camera is a WDM camera (main dialog) but...

    if (pFrame->CaptureActive || !pCamera || !pCamera->Connected || pCamera->PropertyDialogType != PROPDLG_WHEN_CONNECTED)
        return;  // One more safety check

    /*if (pCamera == &Camera_WDM)
        Camera_WDM.ShowPropertyDialog();
    else if (pCamera == &Camera_VFW)
        Camera_VFW.ShowPropertyDialog();*/
    pCamera->ShowPropertyDialog();
}

void AdvancedDialog::EndModal(int retCode)
{
    s_selectedPage = m_pNotebook->GetSelection();
    wxDialog::EndModal(retCode);
}

int AdvancedDialog::GetFocalLength(void)
{
    return m_pFramePane->GetFocalLength();
}

void AdvancedDialog::SetFocalLength(int val)
{
    m_pFramePane->SetFocalLength(val);
}

double AdvancedDialog::GetPixelSize(void)
{
    return m_pCameraPane ? m_pCameraPane->GetPixelSize() : 0.0;
}

void AdvancedDialog::SetPixelSize(double val)
{
    if (m_pCameraPane)
        m_pCameraPane->SetPixelSize(val);
}

BEGIN_EVENT_TABLE(AdvancedDialog, wxDialog)
    EVT_BUTTON(BUTTON_CAM_PROPERTIES,AdvancedDialog::OnSetupCamera)
END_EVENT_TABLE()
