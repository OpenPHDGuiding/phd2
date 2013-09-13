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

#include <wx/spinctrl.h>
#include <wx/textfile.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <fitsio.h>
#include <memory>

void MyFrame::OnExposureDurationSelected(wxCommandEvent& WXUNUSED(evt))
{
    wxString sel = Dur_Choice->GetValue();
    m_exposureDuration = ExposureDurationFromSelection(sel);

    pConfig->Profile.SetString("/ExposureDuration", sel);

    if (pCamera)
    {
        // select the best matching dark frame
        pCamera->SelectDark(m_exposureDuration);

        if (pCamera->CurrentDarkFrame)
        {
            if (pCamera->CurrentDarkFrame->ImgExpDur != m_exposureDuration)
            {
                Dark_Button->SetBackgroundColour(wxColor(255,0,0));
                Dark_Button->SetForegroundColour(wxColour(0,0,0));
            }
            else
            {
                Dark_Button->SetBackgroundColour(wxNullColour);
            }
        }
    }
}

int MyFrame::RequestedExposureDuration()
{
    if (!pCamera || !pCamera->Connected)
        return 0.0;

    return m_exposureDuration;
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    Close(false);
}

void MyFrame::OnInstructions(wxCommandEvent& WXUNUSED(event)) {
    if (CaptureActive) return;  // Looping an exposure already
    wxMessageBox(wxString::Format(_("Welcome to PHD (Push Here Dummy) Guiding\n\n \
Operation is quite simple (hence the 'PHD')\n\n \
  1) Press the Camera Button and select your camera\n \
  2) Select your scope interface in the Mount menu if not\n \
     already selected.  Then, press the Telescope Button \n \
     to connect to your scope\n \
  3) Pick an exposure duration from the drop-down list\n \
  4) Hit the Loop Button, adjust your focus\n \
  5) Click on a star away from the edge\n \
  6) Press the PHD (archery target) icon\n\n \
PHD will then calibrate itself and begin guiding.  That's it!\n\n \
To stop guiding, simply press the Stop Button. If you need to \n \
tweak any options, click on the Brain Button to bring up the\n \
Advanced panel.  ")),_("Instructions"));

}

void MyFrame::OnHelp(wxCommandEvent& WXUNUSED(event)) {
    help->Display(_("Introduction"));
}
void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
    if (CaptureActive) return;  // Looping an exposure already
    AboutDialog dlg;
    dlg.ShowModal();
}

void MyFrame::OnOverlay(wxCommandEvent &evt) {
    pGuider->SetOverlayMode(evt.GetId() - MENU_XHAIR0);
}

void MyFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
    if (CaptureActive)
    {
        return;  // Looping an exposure already
    }

    wxString fname = wxFileSelector( _("Save FITS Image"), (const wxChar *)NULL,
                          (const wxChar *)NULL,
                           wxT("fit"), wxT("FITS files (*.fit)|*.fit"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
                           this);

    if (fname.IsEmpty())
        return;  // Check for canceled dialog

    if (wxFileExists(fname))
        fname = _T("!") + fname;

    if (pGuider->SaveCurrentImage(fname))
    {
        (void) wxMessageBox(_("Error"),_("Your data were not saved"),wxOK | wxICON_ERROR);
    }
}

static bool save_multi(const ExposureImgMap& darks, const wxString& fname)
{
    bool bError = false;

    try
    {
        fitsfile *fptr;  // FITS file pointer
        int status = 0;  // CFITSIO status value MUST be initialized to zero!
        fits_create_file(&fptr, (const char*) fname.mb_str(wxConvUTF8), &status);

        for (ExposureImgMap::const_iterator it = darks.begin(); it != darks.end(); ++it)
        {
            const usImage *const img = it->second;
            long fpixel[3] = {1,1,1};
            long fsize[] = {
                (long) img->Size.GetWidth(),
                (long) img->Size.GetHeight(),
            };
            if (!status) fits_create_img(fptr, USHORT_IMG, 2, fsize, &status);

            float exposure = (float) img->ImgExpDur / 1000.0;
            char keyname[] = "EXPOSURE";
            char comment[] = "Exposure time in seconds";
            if (!status) fits_write_key(fptr, TFLOAT, keyname, &exposure, comment, &status);
            if (!status) fits_write_pix(fptr, TUSHORT, fpixel, img->NPixels, img->ImageData, &status);
            Debug.AddLine("saving dark frame exposure = %d", img->ImgExpDur);
        }

        fits_close_file(fptr, &status);
        bError = status ? true : false;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

static bool load_multi(GuideCamera *camera, const wxString& fname)
{
    bool bError = false;
    fitsfile *fptr = 0;
    int status = 0;  // CFITSIO status value MUST be initialized to zero!

    try
    {
        if (!wxFileExists(fname))
        {
            throw ERROR_INFO("File does not exist");
        }

        if (fits_open_diskfile(&fptr, (const char*) fname.c_str(), READONLY, &status) == 0)
        {
            int nhdus = 0;
            fits_get_num_hdus(fptr, &nhdus, &status);

            while (true)
            {
                int hdutype;
                fits_get_hdu_type(fptr, &hdutype, &status);
                if (hdutype != IMAGE_HDU)
                {
                    (void) wxMessageBox(wxT("FITS file is not of an image"), _("Error"),wxOK | wxICON_ERROR);
                    throw ERROR_INFO("Fits file is not an image");
                }

                int naxis;
                fits_get_img_dim(fptr, &naxis, &status);
                if (naxis != 2)
                {
                    (void) wxMessageBox( _T("Unsupported type or read error loading FITS file") ,_("Error"),wxOK | wxICON_ERROR);
                    throw ERROR_INFO("unsupported type");
                }

                long fsize[2];
                fits_get_img_size(fptr, 2, fsize, &status);

                std::auto_ptr<usImage> img(new usImage());

                if (img->Init((int) fsize[0], (int) fsize[1]))
                {
                    wxMessageBox(_T("Memory allocation error"),_("Error"),wxOK | wxICON_ERROR);
                    throw ERROR_INFO("Memory Allocation failure");
                }

                long fpixel[] = { 1, 1, 1 };
                if (fits_read_pix(fptr, TUSHORT, fpixel, fsize[0] * fsize[1], NULL, img->ImageData, NULL, &status))
                {
                    (void) wxMessageBox(_T("Error reading data"), _("Error"),wxOK | wxICON_ERROR);
                    throw ERROR_INFO("Error reading");
                }

                char keyname[] = "EXPOSURE";
                float exposure;
                if (fits_read_key(fptr, TFLOAT, keyname, &exposure, NULL, &status))
                {
                    exposure = (float) pFrame->RequestedExposureDuration() / 1000.0;
                    Debug.AddLine("missing EXPOSURE value, assume %.3f", exposure);
                    status = 0;
                }
                img->ImgExpDur = (int) (exposure * 1000.0);

                Debug.AddLine("loaded dark frame exposure = %d", img->ImgExpDur);
                camera->AddDark(img.release());

                // if this is the last hdu, we are done
                int hdunr = 0;
                fits_get_hdu_num(fptr, &hdunr);
                if (status || hdunr >= nhdus)
                    break;

                // move to the next hdu
                fits_movrel_hdu(fptr, +1, NULL, &status);
            }
        }
        else
        {
            wxMessageBox(_T("Error opening FITS file"));
            throw ERROR_INFO("error opening file");
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    if (fptr)
    {
        fits_close_file(fptr, &status);
    }

    return bError;
}

void MyFrame::OnLoadSaveDark(wxCommandEvent &evt)
{
    wxString fname;

    if (evt.GetId() == MENU_SAVEDARK)
    {
        if (!pCamera || pCamera->Darks.empty())
        {
            wxMessageBox(_("You haven't captured any dark frames - nothing to save"));
            return;
        }
        wxString default_path = pConfig->Global.GetString("/darkFilePath", wxEmptyString);
        fname = wxFileSelector( _("Save darks (FITS Image)"), default_path,
                                wxEmptyString, wxT("fit"),
                                wxT("FITS files (*.fit)|*.fit"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
                                this);
        if (fname.IsEmpty())
        {
            // dialog canceled
            return;
        }

        pConfig->Global.SetString("/darkFilePath", wxFileName(fname).GetPath());
        if (!fname.EndsWith(_T(".fit")))
        {
            fname.Append(_T(".fit"));
        }

        if (wxFileExists(fname))
        {
            fname = _T("!") + fname;
        }

        if (save_multi(pCamera->Darks, fname))
        {
            wxMessageBox (_("Error saving FITS file"));
        }

        pConfig->Profile.SetString("/camera/DarksFile", fname);
    }
    else if (evt.GetId() == MENU_LOADDARK)
    {
        if (!pCamera || !pCamera->Connected)
        {
            wxMessageBox(_("You must connect a camera before loading dark frames"));
            return;
        }
        wxString default_path = pConfig->Global.GetString("/darkFilePath", wxEmptyString);
        fname = wxFileSelector( _("Load darks (FITS Image)"), default_path,
                               wxEmptyString,
                               wxT("fit"), wxT("FITS files (*.fit)|*.fit"), wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                               this);
        if (fname.IsEmpty())
        {
            // dialog canceled
            return;
        }
        pConfig->Global.SetString("/darkFilePath", wxFileName(fname).GetPath());

        LoadDarkFrames(fname);
    }
}

void MyFrame::LoadDarkFrames(const wxString& filename)
{
    if (load_multi(pCamera, filename))
    {
        Debug.AddLine(wxString::Format("failed to load dark frames from %s", filename));
        SetStatusText(_("Darks not loaded"));
    }
    else
    {
        pCamera->SelectDark(m_exposureDuration);
        tools_menu->FindItem(MENU_CLEARDARK)->Enable(true);
        Dark_Button->SetLabel(_("Redo Dark"));
        SetStatusText(_("Darks loaded"));
        pConfig->Profile.SetString("/camera/DarksFile", filename);
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

        if (CaptureActive)
        {
            throw ERROR_INFO("cannot start looping when capture active");
        }

        m_frameCounter = 0;

        pFrame->StartCapturing();

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
            pGuider->Reset();
            CaptureActive = m_continueCapturing;
            UpdateButtonsStatus();
            SetStatusText(_("Stopped."), 1);

            Debug.Write("OnExposureComplete(): Capture Error reported\n");

            throw ERROR_INFO("Error reported capturing image");
        }

        ++m_frameCounter;

        pGuider->UpdateGuideState(pNewFrame, !m_continueCapturing);
        pNewFrame = NULL; // the guider owns it now

#ifdef BRET_DODO
        if (RandomMotionMode && pGuider->GetState() < STATE_CALIBRATING_PRIMARY)
        {
            GUIDE_DIRECTION dir;

            if (rand() % 2)
                dir = EAST;
            else
                dir = WEST;
            int dur = rand() % 1000;
            ScheduleGuide(dir, dur, wxString::Format(_T("Random motion: %d %d"),dir,dur));

            if ((rand() % 5) == 0) {  // Occasional Dec
                if (rand() % 2)
                    dir = NORTH;
                else
                    dir = SOUTH;
                dur = rand() % 1000;
                pMount->Guide(dir,dur);
                ScheduleGuide(dir, dur, wxString::Format(_T("Random motion: %d %d"),dir,dur));
            }
        }
#endif

        Debug.AddLine(wxString::Format("OnExposeCompete: CaptureActive=%d m_continueCapturing=%d", CaptureActive, m_continueCapturing));
#ifdef ZESLY_DODO
        double dx = (double)rand() / RAND_MAX * 2 - 1;
        double dy = (double)rand() / RAND_MAX * 2 - 1;
        double ra = (double)rand() / RAND_MAX * 2 - 1;
        double dec = (double)rand() / RAND_MAX * 2 - 1;
        pGraphLog->AppendData(dx,dy,ra,dec);
        pTarget->AppendData(ra,dec);
#endif // ZESLY_DODO

        CaptureActive = m_continueCapturing;

        if (CaptureActive)
        {
            ScheduleExposure(RequestedExposureDuration(), pGuider->GetBoundingBox());
        }
        else
        {
            UpdateButtonsStatus();
            SetStatusText(_("Stopped."), 1);
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

        if (!pThisMount->IsBusy())
        {
            UpdateButtonsStatus();
        }

        if (event.GetInt())
        {
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
    UpdateButtonsStatus();
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
    int ExpDur = RequestedExposureDuration();
    if (pGuider->GetState() > STATE_SELECTED) return;
    if (!pCamera || !pCamera->Connected) {
        wxMessageBox(_("Please connect to a camera first"),_("Info"));
        return;
    }
    if (CaptureActive) return;  // Looping an exposure already
    Dark_Button->SetForegroundColour(wxColour(200,0,0));
    int NDarks = 5;

    SetStatusText(_("Capturing dark"));
    if (pCamera->HasShutter)
        pCamera->ShutterState=true; // dark
    else
        wxMessageBox(_("Cover guide scope"));
    pCamera->InitCapture();
    std::auto_ptr<usImage> darkFrame(new usImage());
    darkFrame->ImgExpDur = ExpDur;
    if (pCamera->Capture(ExpDur, *darkFrame, false))
    {
        wxMessageBox(_("Error capturing dark frame"));
        SetStatusText(wxString::Format(_T("%.1f s dark FAILED"), (double) ExpDur / 1000.0));
        Dark_Button->SetLabel(_T("Take Dark"));
        pCamera->ShutterState = false;
    }
    else
    {
        SetStatusText(wxString::Format(_T("%.1f s dark #1 captured"), (double) ExpDur / 1000.0));
        int *avgimg = new int[darkFrame->NPixels];
        int i, j;
        int *iptr = avgimg;
        unsigned short *usptr = darkFrame->ImageData;
        for (i = 0; i < darkFrame->NPixels; i++, iptr++, usptr++)
            *iptr = (int) *usptr;
        for (j = 1; j < NDarks; j++) {
            pCamera->Capture(ExpDur, *darkFrame, false);
            iptr = avgimg;
            usptr = darkFrame->ImageData;
            for (i = 0; i < darkFrame->NPixels; i++, iptr++, usptr++)
                *iptr = *iptr + (int) *usptr;
            SetStatusText(wxString::Format(_T("%.1f s dark #%d captured"), (double) ExpDur / 1000.0, j + 1));
        }
        iptr = avgimg;
        usptr = darkFrame->ImageData;
        for (i = 0; i < darkFrame->NPixels; i++, iptr++, usptr++)
            *usptr = (unsigned short) (*iptr / NDarks);

        delete[] avgimg;

        Dark_Button->SetLabel(_("Redo Dark"));
        Dark_Button->SetBackgroundColour(wxNullColour);

        pCamera->AddDark(darkFrame.release());
        pCamera->SelectDark(ExpDur);
        assert(pCamera->CurrentDarkFrame->ImgExpDur == ExpDur);
    }
    SetStatusText(_("Darks done"));
    if (pCamera->HasShutter)
        pCamera->ShutterState = false; // Lights
    else
        wxMessageBox(_("Uncover guide scope"));
    tools_menu->FindItem(MENU_CLEARDARK)->Enable(pCamera->CurrentDarkFrame ? true : false);
}

void MyFrame::OnClearDark(wxCommandEvent& WXUNUSED(evt))
{
    if (!pCamera->CurrentDarkFrame)
    {
        return;
    }
    pCamera->ClearDarks();
    UpdateDarksButton();
}

void MyFrame::UpdateDarksButton(void)
{
    Dark_Button->SetLabel(_("Take Dark"));
    Dark_Button->SetForegroundColour(wxColour(0,0,0));
    tools_menu->FindItem(MENU_CLEARDARK)->Enable(false);
}

void MyFrame::OnToolBar(wxCommandEvent &evt)
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

void MyFrame::OnGraph(wxCommandEvent &evt) {
    if (evt.IsChecked())
    {
        m_mgr.GetPane(_T("GraphLog")).Show().Bottom().Position(0).MinSize(-1, 220);
    }
    else
    {
        m_mgr.GetPane(_T("GraphLog")).Hide();
    }
    this->pGraphLog->SetState(evt.IsChecked());
    m_mgr.Update();
}

void MyFrame::OnAoGraph(wxCommandEvent &evt) {
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

void MyFrame::OnStarProfile(wxCommandEvent &evt) {
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

void MyFrame::OnTarget(wxCommandEvent &evt) {
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


void MyFrame::OnLog(wxCommandEvent &evt) {
    if (evt.GetId() == MENU_LOG) {
        if (evt.IsChecked()) {  // enable it
            GuideLog.EnableLogging();
#ifdef PHD1_LOGGING // deprecated
            Log_Data = true;
            if (!LogFile->IsOpened()) {
                if (LogFile->Exists()) LogFile->Open();
                else LogFile->Create();
            }
            wxDateTime now = wxDateTime::Now();
            LogFile->AddLine(_T("Logging manually enabled"));
            LogFile->AddLine(wxString::Format(_T("PHD Guide %s  -- "), FULLVER) + now.FormatDate()  + _T(" ") + now.FormatTime());
            LogFile->Write();
#endif
            SetTitle(wxString::Format(_T("%s %s (Log active)"), APPNAME, FULLVER));
        }
        else {
#ifdef PHD1_LOGGING // deprecated
            if (LogFile->IsOpened()) {
                LogFile->AddLine(_T("Logging manually disabled"));
                LogFile->Write();
                LogFile->Close();
            }
            Log_Data = false;
#endif
            GuideLog.DisableLogging();
            this->SetTitle(wxString::Format(_T("%s %s"), APPNAME, FULLVER));
        }
    } else if (evt.GetId() == MENU_LOGIMAGES) {
        if (wxGetKeyState(WXK_SHIFT)) {
//          wxMessageBox("arg");
#ifdef __WINDOWS__
//          tools_menu->FindItem(MENU_LOGIMAGES)->SetTextColour(wxColour(200,10,10));
#endif
            tools_menu->FindItem(MENU_LOGIMAGES)->SetItemLabel(_("Enable Raw Star logging"));
            if (evt.IsChecked()) {
                GuideLog.EnableImageLogging(LIF_RAW_FITS);
            }
            else {
                GuideLog.DisableImageLogging();
            }
        }
        else {
#ifdef __WINDOWS__
//          tools_menu->FindItem(MENU_LOGIMAGES)->SetTextColour(*wxBLACK);
#endif
            tools_menu->FindItem(MENU_LOGIMAGES)->SetText(_("Enable Star Image logging"));
            if (evt.IsChecked()) {
                GuideLog.EnableImageLogging(LIF_LOW_Q_JPEG);
            }
            else {
                GuideLog.DisableImageLogging();
            }
        }
        Menubar->Refresh();
    } else if (evt.GetId() == MENU_DEBUG)
    {
        bool enable = evt.IsChecked();
        pConfig->Global.SetBoolean("/EnableDebugLog", enable);
        Debug.SetState(enable);
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

void MyFrame::OnAutoStar(wxCommandEvent& WXUNUSED(evt)) {
    pFrame->pGuider->AutoSelect();
}

#ifndef __WXGTK__
void MyFrame::OnDonateMenu(wxCommandEvent &evt) {

    switch (evt.GetId()) {
        case DONATE1:
            wxLaunchDefaultBrowser(_T("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=11353812"));
            break;
        case DONATE2:
            wxLaunchDefaultBrowser(_T("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=6NAB6S65UNHP4"));
            break;
        case DONATE3:
            wxLaunchDefaultBrowser(_T("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=VHJPKAQZVF9GN"));
            break;
        case DONATE4:
            wxLaunchDefaultBrowser(_T("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=CGUQHJLE9GR8A"));
            break;
    }

}
#endif

void MyFrame::OnSetupCamera(wxCommandEvent& WXUNUSED(event)) {
    if (!pCamera || !pCamera->Connected || !pCamera->HasPropertyDialog) return;  // One more safety check

    pCamera->ShowPropertyDialog();

}

void MyFrame::OnAdvanced(wxCommandEvent& WXUNUSED(event)) {

    AdvancedDialog* dlog = new AdvancedDialog();

    dlog->LoadValues();

    if (dlog->ShowModal() == wxID_OK)
    {
        Debug.AddLine("User exited setup dialog with 'ok'");    // Make things more clear in the debug log
        dlog->UnloadValues();
        pGraphLog->UpdateControls();

    }
    else                // Cancel event may require non-trivial undos
    {
        Debug.AddLine("User exited setup dialog with 'cancel'");
        dlog->Undo();

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

        pGuider->StartGuiding();

        StartCapturing();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        pGuider->Reset();
    }
    return;
}

void MyFrame::OnTestGuide(wxCommandEvent& WXUNUSED(evt)) {

    if (pFrame->pGuider->GetState() > STATE_SELECTED)
    {
        wxMessageBox(_("Cannot Manual Guide when Calibrating or Guiding"),_("Info"));
    }

    if (!pMount || !pMount->IsConnected())
    {
        wxMessageBox(_("Cannot Manual Guide without a mount connected"),_("Info"));
    }

    TestGuideDialog *pDialog = new TestGuideDialog();
    pDialog->Show();
    // we leak pDialog here
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
        UpdateButtonsStatus();
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

void MyFrame::OnCharHook(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == 'B' && !evt.HasModifiers())
    {
        pGuider->ToggleShowBookmarks();
    }
    else
    {
        evt.Skip();
    }
}
