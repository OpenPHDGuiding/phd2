/*
 *  geardialog.cpp
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

#include "phd.h"

BEGIN_EVENT_TABLE(GearDialog, wxDialog)
    EVT_BUTTON(GEAR_BUTTON_CONNECT_ALL, GearDialog::OnButtonConnectAll)
    EVT_BUTTON(GEAR_BUTTON_DISCONNECT_ALL, GearDialog::OnButtonDisconnectAll)

    EVT_CHOICE(GEAR_CHOICE_CAMERA, GearDialog::OnChoiceCamera)
    EVT_BUTTON(GEAR_BUTTON_CONNECT_CAMERA, GearDialog::OnButtonConnectCamera)
    EVT_BUTTON(GEAR_BUTTON_DISCONNECT_CAMERA, GearDialog::OnButtonDisconnectCamera)

    EVT_CHOICE(GEAR_CHOICE_SCOPE, GearDialog::OnChoiceScope)
    EVT_BUTTON(GEAR_BUTTON_CONNECT_SCOPE, GearDialog::OnButtonConnectScope)
    EVT_BUTTON(GEAR_BUTTON_DISCONNECT_SCOPE, GearDialog::OnButtonDisconnectScope)

    EVT_CHOICE(GEAR_CHOICE_STEPGUIDER, GearDialog::OnChoiceStepGuider)
    EVT_BUTTON(GEAR_BUTTON_CONNECT_STEPGUIDER, GearDialog::OnButtonConnectStepGuider)
    EVT_BUTTON(GEAR_BUTTON_DISCONNECT_STEPGUIDER, GearDialog::OnButtonDisconnectStepGuider)
END_EVENT_TABLE()

/*
 * The Gear Dialog allows the user to select and connec to their hardware.
 *
 * The dialog looks something like this:
 *
 * +--------------------------------------------------------------------------+
 * |                                                                          |
 * |                               Help text                                  |
 * |                                                                          |
 * +--------------------------------------------------------------------------+
 * |                                   |    +------------------------+        |
 * |  Camera Selection                 |    |Camera Connection Button|        |
 * |                                   |    +------------------------+        |
 * +--------------------------------------------------------------------------+
 * |                                   |    +-----------------------+         |
 * |  Mount Selection                  |    |Mount Connection Button|         |
 * |                                   |    +-----------------------+         |
 * +--------------------------------------------------------------------------+
 * |                                   |    +---------------------+           |
 * |  AO Selection                     |    | AO Connection Button|           |
 * |                                   |    +---------------------+           |
 * +--------------------------------------------------------------------------+
 * | +-------------------+   +-------------------+  +-------------------+     |
 * | |    Connect All    |   |  Disconnect All   |  |      Done         |     |
 * | +-------------------+   +-------------------+  +-------------------+     |
 * +--------------------------------------------------------------------------+
 */
GearDialog::GearDialog(wxWindow *pParent):
wxDialog(pParent, wxID_ANY, _("Gear setup"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
    m_pCamera              = NULL;
    m_pScope               = NULL;
    m_pStepGuider          = NULL;

    m_pCameras             = NULL;
    m_pScopes              = NULL;
    m_pStepGuiders         = NULL;

    m_pConnectAllButton         = NULL;
    m_pDisconnectAllButton      = NULL;
    m_pConnectCameraButton      = NULL;
    m_pConnectStepGuiderButton  = NULL;
    m_pConnectScopeButton       = NULL;

    Initialize();
}

GearDialog::~GearDialog(void)
{
    delete m_pCamera;
    delete m_pScope;
    delete m_pStepGuider;

    // prevent double frees
    pCamera         = NULL;
    pMount          = NULL;
    pSecondaryMount = NULL;

    delete m_pCameras;
    delete m_pScopes;
    delete m_pStepGuiders;

    delete m_pConnectAllButton;
    delete m_pDisconnectAllButton;
    delete m_pConnectCameraButton;
    delete m_pConnectStepGuiderButton;
    delete m_pConnectScopeButton;
}

void GearDialog::Initialize(void)
{
    wxSizerFlags sizerFlags       = wxSizerFlags().Align(wxALIGN_CENTER).Border(wxALL,2).Expand();
    wxSizerFlags sizerTextFlags   = wxSizerFlags().Align(wxALIGN_CENTER).Border(wxALL,2).Expand();
    wxSizerFlags sizerLabelFlags  = wxSizerFlags().Align(wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL).Border(wxALL,2).Expand();
    wxSizerFlags sizerButtonFlags = wxSizerFlags().Align(wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL).Border(wxALL,2).Expand();

    wxBoxSizer *pTopLevelSizer = new wxBoxSizer(wxVERTICAL);

    // text at the top.  I tried (really really hard) to get it to resize/Wrap()
    // with the rest of the sizer, but it just didn't want to work, and I needed
    // to get the rest of the dialog working.
    wxStaticText *pText = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    int width, height;
    pText->SetLabel(_("This the place where you select your equipment. I will type more and better instructions when I get around to doing it. For now this will have to do"));
    pText->GetTextExtent(_("MMMMMMMMMM"), &width, &height);
    pText->Wrap(4*width);
    pTopLevelSizer->Add(pText, sizerTextFlags.Align(wxALIGN_CENTER));

    // The Gear grid in the middle of the screen
    wxFlexGridSizer *pGearSizer = new wxFlexGridSizer(3);
    pTopLevelSizer->Add(pGearSizer, sizerFlags);

    // Camera
    pGearSizer->Add(new wxStaticText(this, wxID_ANY, "Camera", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL), sizerLabelFlags);
    m_pCameras = new wxChoice(this, GEAR_CHOICE_CAMERA, wxDefaultPosition, wxDefaultSize,
            GuideCamera::List(), 0, wxDefaultValidator, "Camera");
    pGearSizer->Add(m_pCameras, sizerButtonFlags);
    m_pConnectCameraButton = new wxButton( this, GEAR_BUTTON_CONNECT_CAMERA, "Disconnect");
    pGearSizer->Add(m_pConnectCameraButton, sizerButtonFlags);

    // mount
    pGearSizer->Add(new wxStaticText(this, wxID_ANY, "Mount", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), sizerLabelFlags);
    m_pScopes = new wxChoice(this, GEAR_CHOICE_SCOPE, wxDefaultPosition, wxDefaultSize,
            Scope::List(), 0, wxDefaultValidator, "Mounts");
    pGearSizer->Add(m_pScopes, sizerButtonFlags);
    m_pConnectScopeButton = new wxButton( this, GEAR_BUTTON_CONNECT_SCOPE, "Disconnect");
    pGearSizer->Add(m_pConnectScopeButton, sizerButtonFlags);

    // ao
    pGearSizer->Add(new wxStaticText(this, wxID_ANY, "AO", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), sizerLabelFlags);
    m_pStepGuiders = new wxChoice(this, GEAR_CHOICE_STEPGUIDER, wxDefaultPosition, wxDefaultSize,
            StepGuider::List(), 0, wxDefaultValidator, "Camera");
    pGearSizer->Add(m_pStepGuiders, sizerButtonFlags);
    m_pConnectStepGuiderButton = new wxButton( this, GEAR_BUTTON_CONNECT_STEPGUIDER, "Disconnect");
    pGearSizer->Add(m_pConnectStepGuiderButton, sizerButtonFlags);

    // Setup the bottom row of buttons

    wxBoxSizer *pButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pConnectAllButton = new wxButton( this, GEAR_BUTTON_CONNECT_ALL, "Connect All");
    pButtonSizer->Add(m_pConnectAllButton, sizerFlags);

    m_pDisconnectAllButton = new wxButton( this, GEAR_BUTTON_DISCONNECT_ALL, "Disconnect All");
    pButtonSizer->Add(m_pDisconnectAllButton, sizerFlags);

    pButtonSizer->Add(new wxButton( this, wxID_OK, "Done" ), sizerFlags);

    pTopLevelSizer->Add(pButtonSizer, wxSizerFlags().Align(wxALIGN_TOP|wxALIGN_CENTER_HORIZONTAL).Border(wxALL,2));

    // fit everything with the sizers
    pTopLevelSizer->SetSizeHints(this);
    SetSizerAndFit(pTopLevelSizer);

    // preselect the choices
    wxCommandEvent dummyEvent;
    wxString lastCamera = pConfig->GetString("/camera/LastMenuchoice", _T(""));
    m_pCameras->SetSelection(m_pCameras->FindString(lastCamera));
    OnChoiceCamera(dummyEvent);

    wxString lastScope = pConfig->GetString("/scope/LastMenuChoice", _T(""));
    m_pScopes->SetSelection(m_pScopes->FindString(lastScope));
    OnChoiceScope(dummyEvent);

    wxString lastStepGuider = pConfig->GetString("/stepguider/LastMenuChoice", _T(""));
    m_pStepGuiders->SetSelection(m_pStepGuiders->FindString(lastStepGuider));
    OnChoiceStepGuider(dummyEvent);
}

int GearDialog::ShowModal(bool autoConnect)
{
    int ret = wxID_OK;
    int callSuper = true;

    assert(pCamera == NULL || pCamera == m_pCamera);

    if (m_pStepGuider)
    {
        assert(pMount == NULL || pMount == m_pStepGuider);
        assert(pSecondaryMount == NULL || pSecondaryMount == m_pScope);
    }
    else
    {
        assert(pMount == NULL || pMount == m_pScope);
        assert(pSecondaryMount == NULL);
    }

    if (autoConnect)
    {
        wxCommandEvent dummyEvent;
        OnButtonConnectAll(dummyEvent);

        if (m_pCamera && m_pCamera->Connected &&
            m_pScope  && m_pScope->IsConnected() &&
            (!m_pStepGuider || m_pStepGuider->IsConnected()))
        {
            callSuper = false;
        }
    }

    if (callSuper)
    {
        UpdateButtonState();

        ret = wxDialog::ShowModal();
    }
    else
    {
        EndModal(ret);
    }

    return ret;
}

void GearDialog::EndModal(int retCode)
{

    assert(pCamera == m_pCamera);

    if (pCamera  && pCamera->HasPropertyDialog)
    {
        pFrame->MainToolbar->EnableTool(BUTTON_CAM_PROPERTIES, true);
    }
    else
    {
        pFrame->MainToolbar->EnableTool(BUTTON_CAM_PROPERTIES, false);
    }

    pFrame->SetSampling();

    if (pCamera && pCamera->Connected)
    {
        pFrame->SetStatusText(_T("Camera"),2);
    }
    else
    {
        pFrame->SetStatusText(_T("No cam"),2);
    }

    if (m_pStepGuider)
    {
        assert(pMount == m_pStepGuider);
        assert(pSecondaryMount == m_pScope);

        if (pMount && pMount->IsConnected())
        {
            pFrame->SetStatusText(_T("AO"),4);
        }
        else
        {
            pFrame->SetStatusText(_T(""),4);
        }

        if (pSecondaryMount && pSecondaryMount->IsConnected())
        {
            pFrame->SetStatusText(_("Scope"),3);
        }
        else
        {
            pFrame->SetStatusText(_(""),3);
        }
    }
    else
    {
        assert(pMount == m_pScope);
        assert(pSecondaryMount == NULL);

        if (pMount && pMount->IsConnected())
        {
            pFrame->SetStatusText(_("Scope"),3);
        }
        else
        {
            pFrame->SetStatusText(_(""),3);
        }
    }

    pFrame->UpdateButtonsStatus();
    pFrame->pGraphLog->UpdateControls();

    wxDialog::EndModal(retCode);
}

void GearDialog::UpdateCameraButtonState(void)
{
    // Now set up the buttons to match our current state
    if (!m_pCamera)
    {
        m_pConnectCameraButton->Enable(false);
        m_pConnectCameraButton->SetLabel("Connect");
        m_pConnectCameraButton->SetId(GEAR_BUTTON_CONNECT_CAMERA);
        m_pCameras->Enable(true);
    }
    else
    {
        m_pConnectCameraButton->Enable(true);

        if (m_pCamera->Connected)
        {
            m_pConnectCameraButton->SetLabel("Disconnect");
            m_pConnectCameraButton->SetId(GEAR_BUTTON_DISCONNECT_CAMERA);
            m_pCameras->Enable(false);
        }
        else
        {
            m_pConnectCameraButton->SetLabel("Connect");
            m_pConnectCameraButton->SetId(GEAR_BUTTON_CONNECT_CAMERA);
            m_pCameras->Enable(true);
        }
    }
}

void GearDialog::UpdateScopeButtonState(void)
{
    // Now set up the buttons to match our current state
    if (!m_pScope)
    {
        m_pConnectScopeButton->Enable(false);
        m_pConnectScopeButton->SetLabel("Connect");
        m_pConnectScopeButton->SetId(GEAR_BUTTON_CONNECT_SCOPE);
        m_pScopes->Enable(true);
    }
    else
    {
        m_pConnectScopeButton->Enable(true);

        if (m_pScope->IsConnected())
        {
            m_pConnectScopeButton->SetLabel("Disconnect");
            m_pConnectScopeButton->SetId(GEAR_BUTTON_DISCONNECT_SCOPE);
            m_pScopes->Enable(false);
        }
        else
        {
            m_pConnectScopeButton->SetLabel("Connect");
            m_pConnectScopeButton->SetId(GEAR_BUTTON_CONNECT_SCOPE);
            m_pScopes->Enable(true);

            if (m_pScope->RequiresCamera() && (!m_pCamera || !m_pCamera->ST4HasGuideOutput() || !m_pCamera->Connected))
            {
                m_pConnectScopeButton->Enable(false);
            }
            else if (m_pScope->RequiresStepGuider() && (!m_pStepGuider || !m_pStepGuider->ST4HasGuideOutput() || !m_pStepGuider->IsConnected()))
            {
                m_pConnectScopeButton->Enable(false);
            }
            else
            {
                m_pConnectScopeButton->Enable(true);
            }
        }
    }
}

void GearDialog::UpdateStepGuiderButtonState(void)
{
    // Now set up the buttons to match our current state
    if (!m_pStepGuider)
    {
        m_pConnectStepGuiderButton->Enable(false);
        m_pConnectStepGuiderButton->SetLabel("Connect");
        m_pConnectStepGuiderButton->SetId(GEAR_BUTTON_CONNECT_STEPGUIDER);
        m_pStepGuiders->Enable(true);
    }
    else
    {
        m_pConnectStepGuiderButton->Enable(true);

        if (m_pStepGuider->IsConnected())
        {
            m_pConnectStepGuiderButton->SetLabel("Disconnect");
            m_pConnectStepGuiderButton->SetId(GEAR_BUTTON_DISCONNECT_STEPGUIDER);
            m_pStepGuiders->Enable(false);
        }
        else
        {
            m_pConnectStepGuiderButton->SetLabel("Connect");
            m_pConnectStepGuiderButton->SetId(GEAR_BUTTON_CONNECT_STEPGUIDER);
            m_pStepGuiders->Enable(true);
        }
    }
}

void GearDialog::UpdateConnectAllButtonState(void)
{
    if ((m_pCamera     && !m_pCamera->Connected) ||
        (m_pScope      && !m_pScope->IsConnected()) ||
        (m_pStepGuider && !m_pStepGuider->IsConnected()))
    {
        m_pConnectAllButton->Enable(true);
    }
    else
    {
        m_pConnectAllButton->Enable(false);
    }
}

void GearDialog::UpdateDisconnectAllButtonState(void)
{
    if ((m_pCamera     && m_pCamera->Connected) ||
        (m_pScope      && m_pScope->IsConnected()) ||
        (m_pStepGuider && m_pStepGuider->IsConnected()))
    {
        m_pDisconnectAllButton->Enable(true);
    }
    else
    {
        m_pDisconnectAllButton->Enable(false);
    }
}

void GearDialog::UpdateButtonState(void)
{
    UpdateGearPointers();

    UpdateCameraButtonState();
    UpdateScopeButtonState();
    UpdateStepGuiderButtonState();
    UpdateConnectAllButtonState();
    UpdateDisconnectAllButtonState();
}

void GearDialog::OnButtonConnectAll(wxCommandEvent& event)
{
    OnButtonConnectCamera(event);
    OnButtonConnectStepGuider(event);
    OnButtonConnectScope(event);
}

void GearDialog::OnButtonDisconnectAll(wxCommandEvent& event)
{
    OnButtonDisconnectScope(event);
    OnButtonDisconnectCamera(event);
    OnButtonDisconnectStepGuider(event);
}

void GearDialog::OnChoiceCamera(wxCommandEvent& event)
{
    try
    {
        int idx         = m_pCameras->GetCurrentSelection();
        wxString choice = m_pCameras->GetString(idx);

        delete m_pCamera;
        m_pCamera = NULL;

        UpdateGearPointers();

        m_pCamera = GuideCamera::Factory(choice);

        Debug.AddLine("Created new camera of type %s = %p", choice, m_pCamera);

        if (!m_pCamera)
        {
            throw THROW_INFO("OnChoiceCamera: m_pScope == NULL");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    UpdateButtonState();
}

void GearDialog::OnButtonConnectCamera(wxCommandEvent& event)
{
    try
    {
        if (m_pCamera == NULL)
        {
            throw ERROR_INFO("OnButtonConnectCamera called with m_pCamera == NULL");
        }

        if (m_pCamera->Connected)
        {
            throw THROW_INFO("OnButtonConnectCamera: called when connected");
        }

        if (m_pCamera->Connect())
        {
            throw THROW_INFO("OnButtonConnectCamera: connect failed");
        }

        // Save the choice now that we have connected
        pConfig->SetString("/camera/LastMenuChoice", m_pCameras->GetString(m_pCameras->GetCurrentSelection()));

        pFrame->SetStatusText(_("Camera Connected"), 1);

        Debug.AddLine("Connected Camera:" + m_pCamera->Name);
        Debug.AddLine("FullSize=(%d,%d)", m_pCamera->FullSize.x, m_pCamera->FullSize.y);
        Debug.AddLine("HasGainControl=%d", m_pCamera->HasGainControl);
        if (m_pCamera->HasGainControl)
        {
            Debug.AddLine("GuideCameraGain=%d", m_pCamera->GuideCameraGain);
        }
        Debug.AddLine("HasShutter=%d", m_pCamera->HasShutter);
        Debug.AddLine("HasSubFrames=%d", m_pCamera->HasSubframes);
        Debug.AddLine("ST4HasGuideOutput=%d", m_pCamera->ST4HasGuideOutput());

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        pFrame->SetStatusText(_("Camera Connect Failed"));
    }

    UpdateButtonState();
}

void GearDialog::OnButtonDisconnectCamera(wxCommandEvent& event)
{
    try
    {
        if (m_pCamera == NULL)
        {
            throw ERROR_INFO("OnButtonDisconnectCamera called with m_pCamera == NULL");
        }

        if (!m_pCamera->Connected)
        {
            throw THROW_INFO("OnButtonDisconnectCamera: called when not connected");
        }

        m_pCamera->Disconnect();

        if (m_pScope && m_pScope->RequiresCamera() && m_pScope->IsConnected())
        {
            OnButtonDisconnectScope(event);
        }

        pFrame->SetStatusText(_("Camera Disconnected"), 1);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    UpdateButtonState();
}

void GearDialog::UpdateGearPointers(void)
{
    pCamera = m_pCamera;

    if (m_pStepGuider)
    {
        pMount = m_pStepGuider;
        pSecondaryMount = m_pScope;
    }
    else
    {
        pMount = m_pScope;
        pSecondaryMount = NULL;
    }
}

void GearDialog::OnChoiceScope(wxCommandEvent& event)
{
    try
    {
        int idx         = m_pScopes->GetCurrentSelection();
        wxString choice = m_pScopes->GetString(idx);

        delete m_pScope;
        m_pScope = NULL;
        UpdateGearPointers();

        m_pScope = Scope::Factory(choice);
        Debug.AddLine("Created new scope of type %s = %p", choice, m_pScope);

        if (!m_pScope)
        {
            throw THROW_INFO("OnChoiceScope: m_pScope == NULL");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    UpdateButtonState();
}

void GearDialog::OnButtonConnectScope(wxCommandEvent& event)
{
    try
    {
        if (m_pScope == NULL)
        {
            throw ERROR_INFO("OnButtonConnectScope called with m_pScope == NULL");
        }

        if (m_pScope->IsConnected())
        {
            throw THROW_INFO("OnButtonConnectScope: called when connected");
        }

        if (m_pScope->Connect())
        {
            throw THROW_INFO("OnButtonConnectScope: connect failed");
        }

        // Save the choice now that we have connected
        pConfig->SetString("/scope/LastMenuChoice", m_pScopes->GetString(m_pScopes->GetCurrentSelection()));
        pFrame->SetStatusText(_("Scope connected"));

        Debug.AddLine("Connected Scope:" + m_pScope->Name());
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        pFrame->SetStatusText(_("Scope Connect Failed"));
    }

    UpdateButtonState();
}

void GearDialog::OnButtonDisconnectScope(wxCommandEvent& event)
{
    try
    {
        if (m_pScope == NULL)
        {
            throw ERROR_INFO("OnButtonDisconnectScope called with m_pScope == NULL");
        }

        if (!m_pScope->IsConnected())
        {
            throw THROW_INFO("OnButtonDisconnectScope: called when not connected");
        }

        m_pScope->Disconnect();
        pFrame->SetStatusText(_("Scope Disconnected"));
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    UpdateButtonState();
}

void GearDialog::OnChoiceStepGuider(wxCommandEvent& event)
{
    try
    {
        int idx         = m_pStepGuiders->GetCurrentSelection();
        wxString choice = m_pStepGuiders->GetString(idx);

        delete m_pStepGuider;
        m_pStepGuider = NULL;
        UpdateGearPointers();

        m_pStepGuider = StepGuider::Factory(choice);
        Debug.AddLine("Created new scope of type %s = %p", choice, m_pStepGuider);

        if (!m_pStepGuider)
        {
            throw THROW_INFO("OnChoiceStepGuider: m_pStepGuider == NULL");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    UpdateButtonState();
}

void GearDialog::OnButtonConnectStepGuider(wxCommandEvent& event)
{
    try
    {
        if (m_pStepGuider == NULL)
        {
            throw ERROR_INFO("OnButtonConnectStepGuider called with m_pStepGuider == NULL");
        }

        if (m_pStepGuider->IsConnected())
        {
            throw THROW_INFO("OnButtonConnectStepGuider: called when connected");
        }

        if (m_pStepGuider->Connect())
        {
            throw THROW_INFO("OnButtonConnectStepGuider: connect failed");
        }

        // Save the choice now that we have connected
        pConfig->SetString("/stepguider/LastMenuChoice", m_pStepGuiders->GetString(m_pStepGuiders->GetCurrentSelection()));
        pFrame->SetStatusText(_("Adaptive Optics Connected"), 1);
        Debug.AddLine("Connected AO:" + m_pStepGuider->Name());
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        pFrame->SetStatusText(_("AO Connect Failed"));
    }

    UpdateButtonState();
}

void GearDialog::OnButtonDisconnectStepGuider(wxCommandEvent& event)
{
    try
    {
        if (m_pStepGuider == NULL)
        {
            throw ERROR_INFO("OnButtonDisconnectStepGuider called with m_pStepGuider == NULL");
        }

        if (!m_pStepGuider->IsConnected())
        {
            throw THROW_INFO("OnButtonDisconnectStepGuider: called when not connected");
        }

        m_pStepGuider->Disconnect();

        if (m_pScope && m_pScope->RequiresStepGuider() && m_pScope->IsConnected())
        {
            OnButtonDisconnectScope(event);
        }

        pFrame->SetStatusText(_("Adaptive Optics Disconnected"), 1);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    UpdateButtonState();
}

