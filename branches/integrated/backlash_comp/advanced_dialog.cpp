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

enum {
    GLOBAL_PAGE,
    GUIDER_PAGE,
    CAMERA_PAGE,
    MOUNT_PAGE,
    AO_PAGE,
    ROTATOR_PAGE,
};

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

    m_aoPage = 0;
    m_rotatorPage = 0;

    wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL).Border(wxALL,2).Expand();

    // build tabs -- each needs the tab, and a sizer.  Once built
    // it needs to be populated

    // Build the global tab pane
    wxPanel *pGlobalSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pGlobalTabSizer = new wxBoxSizer(wxVERTICAL);
    pGlobalSettingsPanel->SetSizer(pGlobalTabSizer);
    m_pNotebook->AddPage(pGlobalSettingsPanel, _("Global"), true);

    // and populate it
    m_pGlobalPane = pFrame->GetConfigDialogPane(pGlobalSettingsPanel);
    pGlobalTabSizer->Add(m_pGlobalPane, sizer_flags);

    // Build the guider tab
    wxPanel *pGuiderSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pGuidingTabSizer = new wxBoxSizer(wxVERTICAL);
    pGuiderSettingsPanel->SetSizer(pGuidingTabSizer);
    m_pNotebook->AddPage(pGuiderSettingsPanel, _("Guiding"));

    // and populate it
    m_pGuiderPane = pFrame->pGuider->GetConfigDialogPane(pGuiderSettingsPanel);
    pGuidingTabSizer->Add(m_pGuiderPane, sizer_flags);

    // Build the camera tab
    AddCameraPage();

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
}

AdvancedDialog::~AdvancedDialog()
{
}

void AdvancedDialog::AddCameraPage(void)
{
    wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL).Border(wxALL,2).Expand();

    wxPanel *pCameraSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pCameraTabSizer = new wxBoxSizer(wxVERTICAL);
    pCameraSettingsPanel->SetSizer(pCameraTabSizer);
    m_pNotebook->InsertPage(CAMERA_PAGE, pCameraSettingsPanel, _("Camera"));

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
        m_pCameraPane = NULL;
        wxStaticBoxSizer *pBox = new wxStaticBoxSizer(new wxStaticBox(pCameraSettingsPanel, wxID_ANY, _("Camera Settings")), wxVERTICAL);
        wxStaticText *pText = new wxStaticText(pCameraSettingsPanel, wxID_ANY, _("No Camera Selected"),wxPoint(-1,-1),wxSize(-1,-1));
        pBox->Add(pText);
        pCameraTabSizer->Add(pBox, sizer_flags);
    }
}

void AdvancedDialog::AddMountPage(void)
{
    wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL).Border(wxALL,2).Expand();

    wxPanel *pScopeSettingsPanel = new wxPanel(m_pNotebook);
    wxBoxSizer *pScopeTabSizer = new wxBoxSizer(wxVERTICAL);
    pScopeSettingsPanel->SetSizer(pScopeTabSizer);
    m_pNotebook->InsertPage(MOUNT_PAGE, pScopeSettingsPanel, _("Mount"));

    Mount *mount = NULL;
    if (pSecondaryMount)
        mount = pSecondaryMount;
    else if (pMount && !pMount->IsStepGuider())
        mount = pMount;

    m_pMountPane = NULL;

    if (mount)
    {
        m_pMountPane = mount->GetConfigDialogPane(pScopeSettingsPanel);
        pScopeTabSizer->Add(m_pMountPane, sizer_flags);
    }
    else
    {
        // Add a text box to the Mount tab informing the user there is no Mount
        wxStaticBoxSizer *pBox = new wxStaticBoxSizer(new wxStaticBox(pScopeSettingsPanel, wxID_ANY, _("Mount Settings")), wxVERTICAL);
        wxStaticText *pText = new wxStaticText(pScopeSettingsPanel, wxID_ANY, _("No Mount Selected"),wxPoint(-1,-1),wxSize(-1,-1));
        pBox->Add(pText);
        pScopeTabSizer->Add(pBox, sizer_flags);
    }
}

void AdvancedDialog::AddAoPage(void)
{
    wxASSERT(!m_aoPage);

    if (pMount && pMount->IsStepGuider())
    {
        // We have an AO selected

        wxPanel *pAoSettingsPanel = new wxPanel(m_pNotebook);
        wxBoxSizer *pAoTabSizer = new wxBoxSizer(wxVERTICAL);
        pAoSettingsPanel->SetSizer(pAoTabSizer);
        m_pNotebook->InsertPage(AO_PAGE, pAoSettingsPanel, _("AO"));

        m_pAoPane = pMount->GetConfigDialogPane(pAoSettingsPanel);

        // and the primary mount config goes on the Adaptive Optics tab
        wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP | wxALIGN_CENTER_HORIZONTAL).Border(wxALL, 2).Expand();
        pAoTabSizer->Add(m_pAoPane, sizer_flags);

        m_aoPage = pAoSettingsPanel;
    }
    else
    {
        m_pAoPane = NULL;
    }
}

void AdvancedDialog::AddRotatorPage(void)
{
    wxASSERT(!m_rotatorPage);

    if (pRotator)
    {
        // We have a rotator selected

        wxPanel *rotatorPanel = new wxPanel(m_pNotebook);
        wxBoxSizer *rotatorTabSizer = new wxBoxSizer(wxVERTICAL);
        rotatorPanel->SetSizer(rotatorTabSizer);
        int idx = ROTATOR_PAGE;
        if (!m_aoPage)
            --idx;
        m_pNotebook->InsertPage(idx, rotatorPanel, _("Rotator"));

        m_rotatorPane = pRotator->GetConfigDialogPane(rotatorPanel);

        // and the primary mount config goes on the Adaptive Optics tab
        wxSizerFlags sizer_flags = wxSizerFlags(0).Align(wxALIGN_TOP | wxALIGN_CENTER_HORIZONTAL).Border(wxALL, 2).Expand();
        rotatorTabSizer->Add(m_rotatorPane, sizer_flags);

        m_rotatorPage = rotatorPanel;
    }
    else
    {
        m_rotatorPane = 0;
    }
}

void AdvancedDialog::UpdateCameraPage(void)
{
    AddCameraPage();
    m_pNotebook->DeletePage(CAMERA_PAGE + 1);
    m_pNotebook->GetPage(CAMERA_PAGE)->Layout();
    GetSizer()->Fit(this);
}

void AdvancedDialog::UpdateMountPage(void)
{
    AddMountPage();
    m_pNotebook->DeletePage(MOUNT_PAGE + 1);
    m_pNotebook->GetPage(MOUNT_PAGE)->Layout();
    GetSizer()->Fit(this);
}

void AdvancedDialog::UpdateAoPage(void)
{
    if (m_aoPage)
    {
        int idx = m_pNotebook->FindPage(m_aoPage);
        wxASSERT(idx != wxNOT_FOUND);
        m_pNotebook->DeletePage(idx);
        m_aoPage = 0;
    }
    AddAoPage();
    if (m_aoPage)
        m_aoPage->Layout();
    GetSizer()->Fit(this);
}

void AdvancedDialog::UpdateRotatorPage(void)
{
    if (m_rotatorPage)
    {
        int idx = m_pNotebook->FindPage(m_rotatorPage);
        wxASSERT(idx != wxNOT_FOUND);
        m_pNotebook->DeletePage(idx);
        m_rotatorPage = 0;
    }
    AddRotatorPage();
    if (m_rotatorPage)
        m_rotatorPage->Layout();
    GetSizer()->Fit(this);
}

void AdvancedDialog::LoadValues(void)
{
    ConfigDialogPane *const panes[] =
        { m_pGlobalPane, m_pGuiderPane, m_pCameraPane, m_pMountPane, m_pAoPane, m_rotatorPane };

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
        { m_pGlobalPane, m_pGuiderPane, m_pCameraPane, m_pMountPane, m_pAoPane, m_rotatorPane };

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
        { m_pGlobalPane, m_pGuiderPane, m_pCameraPane, m_pMountPane, m_pAoPane, m_rotatorPane };

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
    return m_pGlobalPane->GetFocalLength();
}

void AdvancedDialog::SetFocalLength(int val)
{
    m_pGlobalPane->SetFocalLength(val);
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
