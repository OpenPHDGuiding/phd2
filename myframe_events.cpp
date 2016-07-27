/*
 *  frame_events.cpp
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
#include "about_dialog.h"
#include "image_math.h"
#include "darks_dialog.h"
#include "Refine_DefMap.h"
#include "camcal_import_dialog.h"
#include "aui_controls.h"

#include <wx/spinctrl.h>
#include <wx/textfile.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <memory>

wxDEFINE_EVENT(APPSTATE_NOTIFY_EVENT, wxCommandEvent);

void MyFrame::OnExposureDurationSelected(wxCommandEvent& WXUNUSED(evt))
{
    wxString sel = Dur_Choice->GetValue();
    int duration = ExposureDurationFromSelection(sel);
    if (duration > 0)
    {
        Debug.Write(wxString::Format("OnExposureDurationSelected: duration = %d\n", duration));

        m_exposureDuration = duration;
        m_autoExp.enabled = false;

        if (pCamera)
        {
            // select the best matching dark frame
            pCamera->SelectDark(m_exposureDuration);
        }
    }
    else
    {
        // Auto-exposure
        if (!m_autoExp.enabled)
            Debug.AddLine("AutoExp: enabled");
        m_autoExp.enabled = true;
    }

    GuideLog.SetGuidingParam("Exposure", ExposureDurationSummary());

    pConfig->Profile.SetString("/ExposureDuration", sel);
}

int MyFrame::RequestedExposureDuration()
{
    if (!pCamera || !pCamera->Connected)
        return 0;

    return m_exposureDuration;
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(false);
}

void MyFrame::OnInstructions(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format(_("Welcome to PHD2 (Push Here Dummy, Gen2) Guiding\n\n \
Basic operation is quite simple (hence the 'PHD')\n\n \
  1) Press the green 'USB' button, select your camera and mount, click on 'Connect All'\n \
  2) Pick an exposure duration from the drop-down list\n \
  3) Hit the 'Loop' button, adjust your focus if necessary\n \
  4) Click on a star away from the edge or use Alt-S to auto-select a star\n \
  5) Press the guide (green target) icon\n\n \
PHD2 will then calibrate itself and begin guiding.  That's it!\n\n \
To stop guiding, simply press the 'Loop' or 'Stop' buttons. If you need to \n \
tweak any options, click on the 'Brain' button to bring up the 'Advanced' \n \
panel. Use the 'View' menu to watch your guiding performance. If you have\n \
problems, read the help files! ")),_("Instructions"));

}

void MyFrame::OnHelp(wxCommandEvent& WXUNUSED(event))
{
    help->Display(_("Introduction"));
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    AboutDialog dlg;
    dlg.ShowModal();
}

void MyFrame::OnOverlay(wxCommandEvent& evt)
{
    pGuider->SetOverlayMode(evt.GetId() - MENU_XHAIR0);
}

struct SlitPropertiesDlg : public wxDialog
{
    wxSpinCtrl *m_x;
    wxSpinCtrl *m_y;
    wxSpinCtrl *m_width;
    wxSpinCtrl *m_height;
    wxSpinCtrl *m_angle;

    SlitPropertiesDlg(wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& title = _("Spectrograph Slit Overlay"),
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(390, 181), long style = wxDEFAULT_DIALOG_STYLE);
};

SlitPropertiesDlg::SlitPropertiesDlg(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    wxBoxSizer *vSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer *szPosition = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Position (Center)")), wxVERTICAL);
    wxStaticBoxSizer *szSlitSize = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Size")), wxVERTICAL);
    // Position controls
    wxBoxSizer *hXSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *xLabel = new wxStaticText(this, wxID_ANY, _("X"), wxDefaultPosition, wxDefaultSize, 0);
    hXSizer->Add(xLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_x = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 8000, 0);
    hXSizer->Add(m_x, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szPosition->Add(hXSizer, 0, wxEXPAND, 5);

    wxBoxSizer *hYSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* yLabel = new wxStaticText(this, wxID_ANY, _("Y"), wxDefaultPosition, wxDefaultSize, 0);
    hYSizer->Add(yLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_y = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 8000, 0);
    hYSizer->Add(m_y, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szPosition->Add(hYSizer, 1, wxEXPAND, 5);
    hSizer->Add(szPosition, 1, 0, 5);

    // Size controls
    wxBoxSizer* hWidthSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* widthLabel = new wxStaticText(this, wxID_ANY, _("Width"), wxDefaultPosition, wxSize(40, -1), 0);
    hWidthSizer->Add(widthLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_width = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 2, 1000, 2);
    hWidthSizer->Add(m_width, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szSlitSize->Add(hWidthSizer, 1, wxEXPAND, 5);

    wxBoxSizer* hHeightSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* heightLabel = new wxStaticText(this, wxID_ANY, _("Height"), wxDefaultPosition, wxSize(40, -1), 0);
    hHeightSizer->Add(heightLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_height = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 2, 1000, 2);
    hHeightSizer->Add(m_height, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    szSlitSize->Add(hHeightSizer, 1, wxEXPAND, 5);
    hSizer->Add(szSlitSize, 1, 0, 5);

    vSizer->Add(hSizer, 0, wxEXPAND, 5);
    // Angle controls
    wxBoxSizer* hAngleSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* staticText1 = new wxStaticText(this, wxID_ANY, _("Angle (degrees)"), wxDefaultPosition, wxDefaultSize, 0);
    //staticText1->Wrap(-1);
    hAngleSizer->Add(staticText1, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_angle = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, -90, 90, 0);
    hAngleSizer->Add(m_angle, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    vSizer->Add(hAngleSizer, 0, wxEXPAND, 5);

    // ok/cancel buttons
    vSizer->Add(
        CreateButtonSizer(wxOK | wxCANCEL),
        wxSizerFlags(0).Expand().Border(wxALL, 10));

    SetSizerAndFit(vSizer);
}

struct SlitPosCtx : public wxObject
{
    SlitPropertiesDlg *dlg;
    Guider *guider;
    SlitPosCtx(SlitPropertiesDlg *d, Guider *g) : dlg(d), guider(g) { }
};

static void UpdateSlitPos(wxSpinEvent& event)
{
    SlitPosCtx *ctx = static_cast<SlitPosCtx *>(event.GetEventUserData());
    wxPoint center(ctx->dlg->m_x->GetValue(), ctx->dlg->m_y->GetValue());
    wxSize size(ctx->dlg->m_width->GetValue(), ctx->dlg->m_height->GetValue());
    int angle = ctx->dlg->m_angle->GetValue();
    ctx->guider->SetOverlaySlitCoords(center, size, angle);
}

void MyFrame::OnOverlaySlitCoords(wxCommandEvent& evt)
{
    wxPoint center;
    wxSize size;
    int angle;
    pGuider->GetOverlaySlitCoords(&center, &size, &angle);

    SlitPropertiesDlg dlg(this);

    dlg.m_x->SetValue(center.x);
    dlg.m_y->SetValue(center.y);
    dlg.m_width->SetValue(size.GetWidth());
    dlg.m_height->SetValue(size.GetHeight());
    dlg.m_angle->SetValue(angle);

    dlg.Bind(wxEVT_SPINCTRL, &UpdateSlitPos, wxID_ANY, wxID_ANY, new SlitPosCtx(&dlg, pGuider));

    if (dlg.ShowModal() != wxID_OK)
    {
        // revert to original values
        pGuider->SetOverlaySlitCoords(center, size, angle);
    }
}

void MyFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
    if (!pGuider->CurrentImage()->ImageData)
        return;

    wxString fname = wxFileSelector( _("Save FITS Image"), (const wxChar *)NULL,
                          (const wxChar *)NULL,
                           wxT("fit"), wxT("FITS files (*.fit)|*.fit"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
                           this);

    if (fname.IsEmpty())
        return;  // Check for canceled dialog

    if (pGuider->SaveCurrentImage(fname))
    {
        Alert(wxString::Format(_("The image could not be saved to %s"), fname));
    }
    else
    {
        pFrame->StatusMsg(wxString::Format(_("%s saved"), wxFileName(fname).GetFullName()));
    }
}

void MyFrame::OnIdle(wxIdleEvent& WXUNUSED(event))
{
}

void MyFrame::OnLoopExposure(wxCommandEvent& WXUNUSED(event))
{
    try
    {
        if (!pCamera || !pCamera->Connected)
        {
            wxMessageBox(_("Please connect to a camera first"),_("Info"));
            throw ERROR_INFO("Camera not connected");
        }

        if (CaptureActive && !pGuider->IsCalibratingOrGuiding())
        {
            throw ERROR_INFO("cannot start looping when capture active");
        }

        StartLooping();
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

void MyFrame::FinishStop(void)
{
    assert(!CaptureActive);
    EvtServer.NotifyLoopingStopped();
    // when looping resumes, start with at least one full frame. This enables applications
    // controlling PHD to auto-select a new star if the star is lost while looping was stopped.
    pGuider->ForceFullFrame();
    ResetAutoExposure();
    UpdateButtonsStatus();
    StatusMsg(_("Stopped."));
    PhdController::AbortController("Stopped capturing");
}

static wxString RawModeWarningKey(void)
{
    return wxString::Format("/Confirm/%d/RawModeWarningEnabled", pConfig->GetCurrentProfileId());
}

static void SuppressRawModeWarning(long)
{
    pConfig->Global.SetBoolean(RawModeWarningKey(), false);
}

static void WarnRawImageMode(void)
{
    if (pCamera->FullSize != pCamera->DarkFrameSize())
    {
        pFrame->SuppressableAlert(RawModeWarningKey(), _("For refining the Bad-pixel Map PHD2 is now displaying raw camera data frames, which are a different size from ordinary guide frames for this camera."),
            SuppressRawModeWarning, 0);
    }
}

/*
 * OnExposeComplete is the dispatch routine that is called when an image has been taken
 * by the background thread.
 *
 * It:
 * - causes the image to be redrawn by calling pGuider->UpateImageDisplay()
 * - calls the routine to update the guider state (which may do nothing)
 * - calls any other appropriate state update routine depending upon the current state
 * - updates button state based on appropriate state variables
 * - schedules another exposure if CaptureActive is stil true
 *
 */
void MyFrame::OnExposeComplete(usImage *pNewFrame, bool err)
{
    try
    {
        Debug.Write("OnExposeComplete: enter\n");

        m_exposurePending = false;

        if (pGuider->GetPauseType() == PAUSE_FULL)
        {
            delete pNewFrame;
            Debug.Write("guider is paused, ignoring frame, not scheduling exposure\n");
            return;
        }

        if (err)
        {
            Debug.Write("OnExposeComplete: Capture Error reported\n");

            delete pNewFrame;

            bool stopping = !m_continueCapturing;
            StopCapturing();
            if (pGuider->IsCalibratingOrGuiding())
            {
                pGuider->StopGuiding();
                pGuider->UpdateImageDisplay();
            }
            EvtServer.NotifyLoopingStopped();
            pGuider->Reset(false);
            CaptureActive = m_continueCapturing;
            UpdateButtonsStatus();
            PhdController::AbortController(stopping ? "Image capture stopped" : "Error reported capturing image");
            StatusMsg(_("Stopped."));

            // some camera drivers disconnect the camera on error
            if (!pCamera->Connected)
                m_statusbar->UpdateStates();

            throw ERROR_INFO("Error reported capturing image");
        }
        ++m_frameCounter;

        if (m_rawImageMode && !m_rawImageModeWarningDone)
        {
            WarnRawImageMode();
            m_rawImageModeWarningDone = true;
        }

        // check for dark frame compatibility in case the frame size changed (binning changed)
        if (pCamera->DarkFrameSize() != m_prevDarkFrameSize)
        {
            CheckDarkFrameGeometry();
        }

        pGuider->UpdateGuideState(pNewFrame, !m_continueCapturing);
        pNewFrame = NULL; // the guider owns it now

        PhdController::UpdateControllerState();

        Debug.Write(wxString::Format("OnExposeComplete: CaptureActive=%d m_continueCapturing=%d\n",
            CaptureActive, m_continueCapturing));

        CaptureActive = m_continueCapturing;

        if (CaptureActive)
        {
            ScheduleExposure();
        }
        else
        {
            FinishStop();
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        UpdateButtonsStatus();
    }
}

void MyFrame::OnExposeComplete(wxThreadEvent& event)
{
    usImage *image = event.GetPayload<usImage *>();
    bool err = event.GetInt() != 0;
    OnExposeComplete(image, err);
}

void MyFrame::OnMoveComplete(wxThreadEvent& event)
{
    try
    {
        Mount *mount = event.GetPayload<Mount *>();
        assert(mount->IsBusy());
        mount->DecrementRequestCount();

        Mount::MOVE_RESULT moveResult = static_cast<Mount::MOVE_RESULT>(event.GetInt());

        mount->LogGuideStepInfo();

        // deliver the outstanding GuidingStopped notification if this is a late-arriving
        // move completion event
        if (!pGuider->IsCalibratingOrGuiding() &&
            (!pMount || !pMount->IsBusy()) &&
            (!pSecondaryMount || !pSecondaryMount->IsBusy()))
        {
            pFrame->NotifyGuidingStopped();
        }

        if (moveResult != Mount::MOVE_OK)
        {
            mount->IncrementErrorCount();

            if (moveResult == Mount::MOVE_STOP_GUIDING)
            {
                Debug.Write("mount move error indicates guiding should stop\n");
                pGuider->StopGuiding();
            }

            throw ERROR_INFO("Error reported moving");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

void MyFrame::OnButtonStop(wxCommandEvent& WXUNUSED(event))
{
    Debug.Write("Stop button clicked\n");
    StopCapturing();
}

void MyFrame::OnGammaSlider(wxScrollEvent& WXUNUSED(event))
{
    int val = Gamma_Slider->GetValue();
    pConfig->Profile.SetInt("/Gamma", val);
    Stretch_gamma = (double) val / 100.0;
    pGuider->UpdateImageDisplay();
}

void MyFrame::OnDark(wxCommandEvent& WXUNUSED(event))
{
    if (!pCamera || !pCamera->Connected)
    {
        wxMessageBox(_("Please connect to a camera first"), _("Info"));
        return;
    }

    DarksDialog dlg(this, true);
    dlg.ShowModal();

    pCamera->SelectDark(RequestedExposureDuration());       // Might be req'd if user cancelled in midstream
}

// Outside event handler because loading a dark library will automatically unload a defect map
bool MyFrame::LoadDarkHandler(bool checkIt)
{
    if (!pCamera || !pCamera->Connected)
    {
        Alert(_("You must connect a camera before loading a dark library"));
        m_useDarksMenuItem->Check(false);
        return false;
    }
    pConfig->Profile.SetBoolean("/camera/AutoLoadDarks", checkIt);
    if (checkIt)  // enable it
    {
        m_useDarksMenuItem->Check(true);
        if (pCamera->CurrentDefectMap)
            LoadDefectMapHandler(false);
        if (LoadDarkLibrary())
            return true;
        else
        {
            m_useDarksMenuItem->Check(false);
            return false;
        }
    }
    else
    {
        if (!pCamera->CurrentDarkFrame)
        {
            m_useDarksMenuItem->Check(false);      // shouldn't have gotten here
            return false;
        }
        pCamera->ClearDarks();
        m_useDarksMenuItem->Check(false);
        StatusMsg(_("Dark library unloaded"));
        return true;
    }
}

void MyFrame::OnLoadDark(wxCommandEvent& evt)
{
    LoadDarkHandler(evt.IsChecked());
    pFrame->UpdateStateLabels();
}

// Outside event handler because loading a defect map will automatically unload a dark library
void MyFrame::LoadDefectMapHandler(bool checkIt)
{
    if (!pCamera || !pCamera->Connected)
    {
        Alert(_("You must connect a camera before loading a bad-pixel map"));
        darks_menu->FindItem(MENU_LOADDEFECTMAP)->Check(false);
        return;
    }
    pConfig->Profile.SetBoolean("/camera/AutoLoadDefectMap", checkIt);
    if (checkIt)
    {
        DefectMap *defectMap = DefectMap::LoadDefectMap(pConfig->GetCurrentProfileId());
        if (defectMap)
        {
            if (pCamera->CurrentDarkFrame)
                LoadDarkHandler(false);
            pCamera->SetDefectMap(defectMap);
            m_useDarksMenuItem->Check(false);
            m_useDefectMapMenuItem->Check(true);
            StatusMsg(_("Defect map loaded"));
        }
        else
        {
            StatusMsg(_("Defect map not loaded"));
        }
    }
    else
    {
        if (!pCamera->CurrentDefectMap)
        {
            m_useDefectMapMenuItem->Check(false);  // Shouldn't have gotten here
            return;
        }
        pCamera->ClearDefectMap();
        m_useDefectMapMenuItem->Check(false);
        StatusMsg(_("Bad-pixel map unloaded"));
    }
}

void MyFrame::OnLoadDefectMap(wxCommandEvent& evt)
{
    LoadDefectMapHandler(evt.IsChecked());
    pFrame->UpdateStateLabels();
}

void MyFrame::OnRefineDefMap(wxCommandEvent& evt)
{
    if (!pCamera || !pCamera->Connected)
    {
        wxMessageBox(_("Please connect to a camera first"), _("Info"));
        return;
    }

    if (!pRefineDefMap)
        pRefineDefMap = new RefineDefMap(this);

    if (pRefineDefMap->InitUI())                    // Required data present, UI built and ready to go
    {
        pRefineDefMap->Show();

        // Don't let the user build a new defect map while we're trying to refine one; and it almost certainly makes sense
        // to have a defect map loaded if the user wants to refine it
        m_takeDarksMenuItem->Enable(false);             // Dialog restores it when its window is closed
        LoadDefectMapHandler(true);
    }
    else
        pRefineDefMap->Destroy();                       // user cancelled out before starting the process
}

void MyFrame::OnImportCamCal(wxCommandEvent& evt)
{
    if (!pCamera)
    {
        wxMessageBox(_("Please connect a camera first."));
        return;
    }

    CamCalImportDialog dlg(this);

    dlg.ShowModal();
}

void MyFrame::OnToolBar(wxCommandEvent& evt)
{
    if (evt.IsChecked())
    {
        //wxSize toolBarSize = MainToolbar->GetSize();
        m_mgr.GetPane(_T("MainToolBar")).Show().Bottom()/*.MinSize(toolBarSize)*/;
    }
    else
    {
        m_mgr.GetPane(_T("MainToolBar")).Hide();
    }
    m_mgr.Update();
}

void MyFrame::OnGraph(wxCommandEvent& evt)
{
    if (evt.IsChecked())
    {
        m_mgr.GetPane(_T("GraphLog")).Show().Bottom().Position(0).MinSize(-1, 240);
    }
    else
    {
        m_mgr.GetPane(_T("GraphLog")).Hide();
    }
    pGraphLog->SetState(evt.IsChecked());
    m_mgr.Update();
}

void MyFrame::OnStats(wxCommandEvent& evt)
{
    if (evt.IsChecked())
    {
        m_mgr.GetPane(_T("Stats")).Show().Bottom().Position(0).MinSize(-1, 240);
    }
    else
    {
        m_mgr.GetPane(_T("Stats")).Hide();
    }
    pStatsWin->SetState(evt.IsChecked());
    m_mgr.Update();
}

void MyFrame::OnAoGraph(wxCommandEvent& evt)
{
    if (pStepGuiderGraph->SetState(evt.IsChecked()))
    {
        m_mgr.GetPane(_T("AOPosition")).Show().Right().Position(1).MinSize(293,208);
    }
    else
    {
        m_mgr.GetPane(_T("AOPosition")).Hide();
    }
    m_mgr.Update();
}

void MyFrame::OnStarProfile(wxCommandEvent& evt)
{
    if (evt.IsChecked())
    {
#if defined (__APPLE__)
        m_mgr.GetPane(_T("Profile")).Show().Float().MinSize(110,72);
#else
        m_mgr.GetPane(_T("Profile")).Show().Right().Position(0).MinSize(115,85);
        //m_mgr.GetPane(_T("Profile")).Show().Bottom().Layer(1).Position(2).MinSize(115,85);
#endif
    }
    else
    {
        m_mgr.GetPane(_T("Profile")).Hide();
    }
    pProfile->SetState(evt.IsChecked());
    m_mgr.Update();
}

void MyFrame::OnTarget(wxCommandEvent& evt)
{
    if (evt.IsChecked())
    {
        m_mgr.GetPane(_T("Target")).Show().Right().Position(2).MinSize(293,208);
    }
    else
    {
        m_mgr.GetPane(_T("Target")).Hide();
    }
    pTarget->SetState(evt.IsChecked());
    m_mgr.Update();
}

// Redock windows and restore main window to size/position where everything should be readily accessible
void MyFrame::OnRestoreWindows(wxCommandEvent& evt)
{
    wxAuiPaneInfoArray& panes = m_mgr.GetAllPanes();

    // Start by restoring the main window although it doesn't seem like this could be much of a problem
    pFrame->SetSize(wxSize(800, 600));
    pFrame->SetPosition(wxPoint(20, 20));           // Should work on any screen size
    // Now re-dock all the windows that are being managed by wxAuiManager
    int lim = panes.GetCount();
    for (int i = 0; i < lim; i++)
    {
        panes.Item(i).Dock();                       // Already docked, shown or not, doesn't matter
        if (panes.Item(i).name == _("Guider"))
            panes.Item(i).Show(true);
    }
    m_mgr.Update();

    if (pCometTool)
        pCometTool->Center();
    if (pDriftTool)
        pDriftTool->Center();
    if (pGuidingAssistant)
        pGuidingAssistant->Center();
    if (pNudgeLock)
        pNudgeLock->Center();
}

void MyFrame::OnLog(wxCommandEvent& evt)
{
    if (evt.GetId() == MENU_LOGIMAGES)
    {
        pFrame->EnableImageLogging(evt.IsChecked());
    }
}

bool MyFrame::FlipRACal()
{
    bool bError = false;
    Mount *scope = TheScope();

    if (scope)
    {
        bError = scope->FlipCalibration();

        if (!bError)
        {
            EvtServer.NotifyCalibrationDataFlipped(scope);
        }
    }

    return bError;
}

void MyFrame::OnAutoStar(wxCommandEvent& WXUNUSED(evt))
{
    pFrame->pGuider->AutoSelect();
}

void MyFrame::OnSetupCamera(wxCommandEvent& WXUNUSED(event))
{
    if (pCamera &&
        (((pCamera->PropertyDialogType & PROPDLG_WHEN_CONNECTED) != 0 && pCamera->Connected) ||
         ((pCamera->PropertyDialogType & PROPDLG_WHEN_DISCONNECTED) != 0 && !pCamera->Connected)))
    {
        pCamera->ShowPropertyDialog();
    }
}

void MyFrame::OnAdvanced(wxCommandEvent& WXUNUSED(event))
{
    pAdvancedDialog->LoadValues();

    if (pAdvancedDialog->ShowModal() == wxID_OK)
    {
        Debug.Write("User exited setup dialog with 'ok'\n");
        pAdvancedDialog->UnloadValues();
        pGraphLog->UpdateControls();
        TestGuide::ManualGuideUpdateControls();
    }
    else
    {
        // Cancel event may require non-trivial undos
        Debug.Write("User exited setup dialog with 'cancel'\n");
        pAdvancedDialog->Undo();
    }
}

static wxString DarksWarningEnabledKey()
{
    // we want the key to be under "/Confirm" so ConfirmDialog::ResetAllDontAskAgain() resets it, but we also want the setting to be per-profile
    return wxString::Format("/Confirm/%d/DarksWarningEnabled", pConfig->GetCurrentProfileId());
}

static void SuppressDarksAlert(long)
{
    pConfig->Global.SetBoolean(DarksWarningEnabledKey(), false);
}

static void ValidateDarksLoaded(void)
{
    if (!pCamera->CurrentDarkFrame && !pCamera->CurrentDefectMap)
    {
        pFrame->SuppressableAlert(DarksWarningEnabledKey(),
            _("For best results, use a Dark Library or a Bad-pixel Map "
            "while guiding. This will help prevent PHD from locking on to a hot pixel. "
            "Use the Darks menu to build a Dark Library or Bad-pixel Map."), SuppressDarksAlert, 0);
    }
}

void MyFrame::GuideButtonClick(bool interactive)
{
    try
    {
        if (pMount == NULL)
        {
            // no mount selected -- should never happen
            throw ERROR_INFO("pMount == NULL");
        }

        if (!pMount->IsConnected())
        {
            throw ERROR_INFO("Unable to guide with no scope Connected");
        }

        if (!pCamera || !pCamera->Connected)
        {
            throw ERROR_INFO("Unable to guide with no camera Connected");
        }

        if (pGuider->GetState() < STATE_SELECTED)
        {
            wxMessageBox(_T("Please select a guide star before attempting to guide"));
            throw ERROR_INFO("Unable to guide with state < STATE_SELECTED");
        }

        ValidateDarksLoaded();

        if (wxGetKeyState(WXK_SHIFT))
        {
            bool recalibrate = true;
            if (pMount->IsCalibrated() || (pSecondaryMount && pSecondaryMount->IsCalibrated()))
            {
                recalibrate = ConfirmDialog::Confirm(_("Are you sure you want force recalibration?"),
                    "/force_recalibration_ok", _("Force Recalibration"));
            }
            if (recalibrate)
            {
                pMount->ClearCalibration();
                if (pSecondaryMount)
                    pSecondaryMount->ClearCalibration();
            }
        }

        if (interactive && pPointingSource && pPointingSource->IsConnected() && pPointingSource->CanReportPosition())
        {
            bool proceed = true;
            bool error = pPointingSource->PreparePositionInteractive();

            if (!error && fabs(pPointingSource->GetDeclination()) > Scope::DEC_COMP_LIMIT && !TheScope()->IsCalibrated() )
            {
                proceed = ConfirmDialog::Confirm(
                    _("Calibration this far from the celestial equator will be error-prone.  For best results, calibrate at a declination of -20 to +20."),
                    "/highdec_calibration_ok", _("Confirm Calibration at Large Declination")
                    );
            }
            if (error || !proceed)
                return;
        }

        StartGuiding();
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        pGuider->Reset(false);
    }
}

void MyFrame::OnGuide(wxCommandEvent& WXUNUSED(event))
{
    GuideButtonClick(true);
}

void MyFrame::OnTestGuide(wxCommandEvent& WXUNUSED(evt))
{
    if (!pMount || !pMount->IsConnected())
    {
        if (!pSecondaryMount || !pSecondaryMount->IsConnected())
        {
            wxMessageBox(_("Please connect a mount first."), _("Manual Guide"));
            return;
        }
    }

    if (!pManualGuide)
        pManualGuide = TestGuide::CreateManualGuideWindow();

    pManualGuide->Show();
}

void MyFrame::OnPanelClose(wxAuiManagerEvent& evt)
{
    wxAuiPaneInfo *p = evt.GetPane();
    if (p->name == _T("MainToolBar"))
    {
        Menubar->Check(MENU_TOOLBAR, false);
    }
    if (p->name == _T("GraphLog"))
    {
        Menubar->Check(MENU_GRAPH, false);
        pGraphLog->SetState(false);
    }
    if (p->name == _T("Stats"))
    {
        Menubar->Check(MENU_STATS, false);
        pStatsWin->SetState(false);
    }
    if (p->name == _T("Profile"))
    {
        Menubar->Check(MENU_STARPROFILE, false);
        pProfile->SetState(false);
    }
    if (p->name == _T("AOPosition"))
    {
        Menubar->Check(MENU_AO_GRAPH, false);
        pStepGuiderGraph->SetState(false);
    }
    if (p->name == _T("Target"))
    {
        Menubar->Check(MENU_TARGET, false);
        pTarget->SetState(false);
    }
}

void MyFrame::OnSelectGear(wxCommandEvent& evt)
{
    try
    {
        if (CaptureActive)
        {
            throw ERROR_INFO("OnSelectGear called while CaptureActive");
        }

        if (pConfig->NumProfiles() == 1 && pGearDialog->IsEmptyProfile())
        {
            if (ConfirmDialog::Confirm(
                _("It looks like this is a first-time connection to your camera and mount. The Setup Wizard can help\n"
                  "you with that and will also establish baseline guiding parameters for your new configuration.\n"
                  "Would you like to use the Setup Wizard now?"),
                  "/use_new_profile_wizard", _("Yes"), _("No"), _("Setup Wizard Recommendation")))
            {
                pGearDialog->ShowProfileWizard(evt);
                return;
            }
        }

        pGearDialog->ShowGearDialog(wxGetKeyState(WXK_SHIFT));
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

void MyFrame::OnBookmarksShow(wxCommandEvent& evt)
{
    pGuider->SetBookmarksShown(evt.IsChecked());
}

void MyFrame::OnBookmarksSetAtLockPos(wxCommandEvent& evt)
{
    pGuider->BookmarkLockPosition();
}

void MyFrame::OnBookmarksSetAtCurPos(wxCommandEvent& evt)
{
    pGuider->BookmarkCurPosition();
}

void MyFrame::OnBookmarksClearAll(wxCommandEvent& evt)
{
    pGuider->DeleteAllBookmarks();
}

void MyFrame::OnTextControlSetFocus(wxFocusEvent& evt)
{
    m_showBookmarksMenuItem->SetAccel(0);
    m_bookmarkLockPosMenuItem->SetAccel(0);
    evt.Skip();
}

void MyFrame::OnTextControlKillFocus(wxFocusEvent& evt)
{
    m_showBookmarksMenuItem->SetAccel(m_showBookmarksAccel);
    m_bookmarkLockPosMenuItem->SetAccel(m_bookmarkLockPosAccel);
    evt.Skip();
}

void MyFrame::OnCharHook(wxKeyEvent& evt)
{
    bool handled = false;

    // This never gets called on OSX (since we moved to wxWidgets-3.0.0), so we
    // rely on the menu accelerators on the MyFrame to provide the keyboard
    // responses. For Windows and Linux, we keep this here so the keystrokes
    // work when other windows like the Drift Tool window have focus.

    if (evt.GetKeyCode() == 'B')
    {
        if (!evt.GetEventObject()->IsKindOf(wxCLASSINFO(wxTextCtrl)))
        {
            int modifiers;
#ifdef __WXOSX__
            modifiers = 0;
            if (wxGetKeyState(WXK_ALT))
                modifiers |= wxMOD_ALT;
            if (wxGetKeyState(WXK_CONTROL))
                modifiers |= wxMOD_CONTROL;
            if (wxGetKeyState(WXK_SHIFT))
                modifiers |= wxMOD_SHIFT;
            if (wxGetKeyState(WXK_RAW_CONTROL))
                modifiers |= wxMOD_RAW_CONTROL;
#else
            modifiers = evt.GetModifiers();
#endif
            if (!modifiers)
            {
                pGuider->ToggleShowBookmarks();
                bookmarks_menu->Check(MENU_BOOKMARKS_SHOW, pGuider->GetBookmarksShown());
                handled = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                pGuider->DeleteAllBookmarks();
                handled = true;
            }
            else if (modifiers == wxMOD_SHIFT)
            {
                pGuider->BookmarkLockPosition();
                handled = true;
            }
        }
    }

    if (!handled)
    {
        evt.Skip();
    }
}
