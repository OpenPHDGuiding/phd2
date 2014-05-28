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
        Debug.AddLine("OnExposureDurationSelected: duration = %d", duration);

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
Operation is quite simple (hence the 'PHD')\n\n \
  1) Press the 'Camera' button, select your camera and mount, click on 'Connect All'\n \
  2) Pick an exposure duration from the drop-down list\n \
  3) Hit the 'Loop' button, adjust your focus if necessary\n \
  4) Click on a star away from the edge or use Alt-S to auto-select a star\n \
  5) Press the PHD (archery target) icon\n\n \
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
        Alert(_("The image could not be saved to ") + fname);
    }
}

void MyFrame::OnIdle(wxIdleEvent& WXUNUSED(event))
{
/*  if (ASCOM_IsMoving())
        SetStatusText(_T("Moving"),2);
    else
        SetStatusText(_T("Still"),2);*/
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
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
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
void MyFrame::OnExposeComplete(wxThreadEvent& event)
{
    try
    {
        Debug.AddLine("Processing an image");

        usImage *pNewFrame = event.GetPayload<usImage *>();

        if (event.GetInt())
        {
            delete pNewFrame;

            StopCapturing();
            pGuider->Reset(false);
            CaptureActive = m_continueCapturing;
            UpdateButtonsStatus();
            PhdController::AbortController("Error reported capturing image");
            SetStatusText(_("Stopped."), 1);

            Debug.Write("OnExposeComplete(): Capture Error reported\n");

            throw ERROR_INFO("Error reported capturing image");
        }
        ++m_frameCounter;

        pGuider->UpdateGuideState(pNewFrame, !m_continueCapturing);
        pNewFrame = NULL; // the guider owns it now

        PhdController::UpdateControllerState();

        Debug.AddLine(wxString::Format("OnExposeCompete: CaptureActive=%d m_continueCapturing=%d",
            CaptureActive, m_continueCapturing));

        CaptureActive = m_continueCapturing;

        if (CaptureActive)
        {
            ScheduleExposure(RequestedExposureDuration(), pGuider->GetBoundingBox());
        }
        else
        {
            // when looping resumes, start with at least one full frame. This enables applications
            // controlling PHD to auto-select a new star if the star is lost while looping was stopped.
            pGuider->ForceFullFrame();
            ResetAutoExposure();
            UpdateButtonsStatus();
            SetStatusText(_("Stopped."), 1);
            PhdController::AbortController("Stopped capturing");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        UpdateButtonsStatus();
    }
}

void MyFrame::OnMoveComplete(wxThreadEvent& event)
{
    try
    {
        Mount *pThisMount = event.GetPayload<Mount *>();
        assert(pThisMount->IsBusy());
        pThisMount->DecrementRequestCount();

        Mount::MOVE_RESULT moveResult = static_cast<Mount::MOVE_RESULT>(event.GetInt());
        if (moveResult != Mount::MOVE_OK)
        {
            if (moveResult == Mount::MOVE_STOP_GUIDING)
            {
                Debug.AddLine("mount move error indicates guiding should stop");
                pGuider->StopGuiding();
            }

            throw ERROR_INFO("Error reported moving");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

void MyFrame::OnButtonStop(wxCommandEvent& WXUNUSED(event))
{
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
void MyFrame::LoadDarkHandler(bool checkIt)
{
    if (!pCamera || !pCamera->Connected)
    {
        Alert(_("You must connect a camera before loading a dark library"));
        darks_menu->FindItem(MENU_LOADDARK)->Check(false);
        return;
    }
    pConfig->Profile.SetBoolean("/camera/AutoLoadDarks", checkIt);
    if (checkIt)  // enable it
    {
        darks_menu->FindItem(MENU_LOADDARK)->Check(true);
        if (pCamera->CurrentDefectMap)
            LoadDefectMapHandler(false);
        LoadDarkLibrary();
        SetStatusText(_("Dark library loaded"));
    }
    else
    {
        if (!pCamera->CurrentDarkFrame)
        {
            darks_menu->FindItem(MENU_LOADDARK)->Check(false);      // shouldn't have gotten here
            return;
        }
        pCamera->ClearDarks();
        darks_menu->FindItem(MENU_LOADDARK)->Check(false);
        SetStatusText(_("Dark library unloaded"));
    }
}

void MyFrame::OnLoadDark(wxCommandEvent& evt)
{
    LoadDarkHandler(evt.IsChecked());
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
        if (pCamera->CurrentDarkFrame)
            LoadDarkHandler(false);
        LoadDefectMap();            // Status msg generated internally
        if (pCamera->CurrentDefectMap)
        {
            darks_menu->FindItem(MENU_LOADDARK)->Check(false);
            darks_menu->FindItem(MENU_LOADDEFECTMAP)->Check(true);
        }
    }
    else
    {
        if (!pCamera->CurrentDefectMap)
        {
            darks_menu->FindItem(MENU_LOADDEFECTMAP)->Check(false);  // Shouldn't have gotten here
            return;
        }
        pCamera->ClearDefectMap();
        darks_menu->FindItem(MENU_LOADDEFECTMAP)->Check(false);
        SetStatusText(_("Bad-pixel map unloaded"));
    }
}

void MyFrame::OnLoadDefectMap(wxCommandEvent& evt)
{
    LoadDefectMapHandler(evt.IsChecked());
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

    pRefineDefMap->InitUI();
    pRefineDefMap->Show();

    // Don't let the user build a new defect map while we're trying to refine one; and it almost certainly makes sense
    // to have a defect map loaded if the user wants to refine it
    m_takeDarksMenuItem->Enable(false);  // Dialog restores it when its window is closed
    LoadDefectMapHandler(true);
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
    this->pGraphLog->SetState(evt.IsChecked());
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
    this->pGraphLog->SetState(evt.IsChecked());
    m_mgr.Update();
}

void MyFrame::OnAoGraph(wxCommandEvent& evt)
{
    if (this->pStepGuiderGraph->SetState(evt.IsChecked()))
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
    this->pProfile->SetState(evt.IsChecked());
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
    this->pTarget->SetState(evt.IsChecked());
    m_mgr.Update();
}

void MyFrame::OnLog(wxCommandEvent& evt)
{
    if (evt.GetId() == MENU_LOG)
    {
        if (evt.IsChecked())  // enable it
        {
            GuideLog.EnableLogging();
        }
        else
        {
            GuideLog.DisableLogging();
        }
        UpdateTitle();
    }
    else if (evt.GetId() == MENU_LOGIMAGES)
    {
        pFrame->EnableImageLogging(evt.IsChecked());
    }
    else if (evt.GetId() == MENU_DEBUG)
    {
        bool enable = evt.IsChecked();
        pConfig->Global.SetBoolean("/EnableDebugLog", enable);
        Debug.Enable(enable);
    }
}

bool MyFrame::FlipRACal()
{
    bool bError = false;
    Mount *mount = pSecondaryMount ? pSecondaryMount : pMount;

    if (mount)
    {
        bError = mount->FlipCalibration();

        if (!bError)
        {
            EvtServer.NotifyCalibrationDataFlipped(mount);
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
        Debug.AddLine("User exited setup dialog with 'ok'");
        pAdvancedDialog->UnloadValues();
        pGraphLog->UpdateControls();
        TestGuide::ManualGuideUpdateControls();
    }
    else
    {
        // Cancel event may require non-trivial undos
        Debug.AddLine("User exited setup dialog with 'cancel'");
        pAdvancedDialog->Undo();
    }
}

void MyFrame::OnGuide(wxCommandEvent& WXUNUSED(event))
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

        StartGuiding();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        pGuider->Reset(false);
    }
    return;
}

void MyFrame::OnTestGuide(wxCommandEvent& WXUNUSED(evt))
{
    if (!pMount || !pMount->IsConnected())
    {
        wxMessageBox(_("Please connect a mount first."), _("Manual Guide"));
        return;
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
        this->pGraphLog->SetState(false);
    }
    if (p->name == _T("GraphLog"))
    {
        Menubar->Check(MENU_GRAPH, false);
        this->pGraphLog->SetState(false);
    }
    if (p->name == _T("Profile"))
    {
        Menubar->Check(MENU_STARPROFILE, false);
        this->pProfile->SetState(false);
    }
    if (p->name == _T("AOPosition"))
    {
        Menubar->Check(MENU_AO_GRAPH, false);
        this->pStepGuiderGraph->SetState(false);
    }
    if (p->name == _T("Target"))
    {
        Menubar->Check(MENU_TARGET, false);
        this->pTarget->SetState(false);
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
        pFrame->pGearDialog->ShowModal(wxGetKeyState(WXK_SHIFT));
    }
    catch (wxString Msg)
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
