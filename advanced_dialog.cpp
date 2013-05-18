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
AdvancedDialog::AdvancedDialog():
//#if defined (__WINDOWS__)
//wxDialog(pFrame, wxID_ANY, _("Advanced setup"), wxPoint(-1,-1), wxSize(210,350), wxCAPTION | wxCLOSE_BOX)
//#else
//wxDialog(pFrame, wxID_ANY, _("Advanced setup"), wxPoint(-1,-1), wxSize(250,350), wxCAPTION | wxCLOSE_BOX)
//#endif
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

    wxSizerFlags sizer_flags = wxSizerFlags(0).Center().Border(wxALL,2).Expand();

    // build all the empty sizer
    wxBoxSizer *pConfigSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pLeftSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *pRightSizer = new wxBoxSizer(wxVERTICAL);

    pConfigSizer->Add(pLeftSizer, sizer_flags);
    pConfigSizer->Add(pRightSizer, sizer_flags);

    wxBoxSizer *pTopLevelSizer = new wxBoxSizer(wxVERTICAL);
    pTopLevelSizer->Add(pConfigSizer, wxSizerFlags(0).Expand().Border(wxALL, 5));
    pTopLevelSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags(0).Expand().Border(wxALL, 5));

    // Build the left column of panes

    m_pFramePane = pFrame->GetConfigDialogPane(this);
    pLeftSizer->Add(m_pFramePane, sizer_flags);

    m_pGuiderPane = pFrame->pGuider->GetConfigDialogPane(this);
    pLeftSizer->Add(m_pGuiderPane, sizer_flags);

    if (pCamera)
    {
        m_pCameraPane = pCamera->GetConfigDialogPane(this);
        if (m_pCameraPane != NULL)
            pLeftSizer->Add(m_pCameraPane, sizer_flags);
    }
    else
    {
        m_pCameraPane=NULL;
        wxStaticBoxSizer *pBox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Camera Settings")), wxVERTICAL);
        wxStaticText *pText = new wxStaticText(this, wxID_ANY, _("No Camera Connected"),wxPoint(-1,-1),wxSize(-1,-1));
        pBox->Add(pText);
        pLeftSizer->Add(pBox, sizer_flags);
    }

    // Build the right column of panes

    if (pSecondaryMount)
    {
        // if there are two mounts, the mount config goes on the left and
        // the secondary goes on the right
        m_pMountPane = pMount->GetConfigDialogPane(this);
        pLeftSizer->Add(m_pMountPane, sizer_flags);

        m_pSecondaryMountPane = pSecondaryMount->GetConfigDialogPane(this);
        pRightSizer->Add(m_pSecondaryMountPane, sizer_flags);
    }
    else
    {
        // otherwise there is no secondary mount, so we put up an empty AO box
        m_pSecondaryMountPane = NULL;
        wxStaticBoxSizer *pBox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("AO Settings")), wxVERTICAL);
        wxStaticText *pText = new wxStaticText(this, wxID_ANY, _("No AO Connected"),wxPoint(-1,-1),wxSize(-1,-1));
        pBox->Add(pText);
        pLeftSizer->Add(pBox, sizer_flags);

        // and the mount goes on the right
        m_pMountPane = pMount->GetConfigDialogPane(this);
        pRightSizer->Add(m_pMountPane, sizer_flags);

    }


    SetSizerAndFit(pTopLevelSizer);
}


void AdvancedDialog::LoadValues(void)
{
    m_pFramePane->LoadValues();
    m_pMountPane->LoadValues();
    if (m_pSecondaryMountPane)
    {
        m_pSecondaryMountPane->LoadValues();
    }
    m_pGuiderPane->LoadValues();

    if (m_pCameraPane != NULL)
    {
        m_pCameraPane->LoadValues();
    }
}

void AdvancedDialog::UnloadValues(void)
{
    m_pFramePane->UnloadValues();
    m_pMountPane->UnloadValues();
    if (m_pSecondaryMountPane)
    {
        m_pSecondaryMountPane->UnloadValues();
    }
    m_pGuiderPane->UnloadValues();

    if (m_pCameraPane != NULL)
    {
        m_pCameraPane->UnloadValues();
    }
}

BEGIN_EVENT_TABLE(AdvancedDialog, wxDialog)
    EVT_BUTTON(wxID_PROPERTIES,AdvancedDialog::OnSetupCamera)
END_EVENT_TABLE()

void AdvancedDialog::OnSetupCamera(wxCommandEvent& WXUNUSED(event)) {
    // Prior to this we check to make sure the current camera is a WDM camera (main dialog) but...

    if (pFrame->CaptureActive || !pCamera || !pCamera->Connected || !pCamera->HasPropertyDialog) return;  // One more safety check
    /*if (pCamera == &Camera_WDM)
        Camera_WDM.ShowPropertyDialog();
    else if (pCamera == &Camera_VFW)
        Camera_VFW.ShowPropertyDialog();*/
    pCamera->ShowPropertyDialog();
}

