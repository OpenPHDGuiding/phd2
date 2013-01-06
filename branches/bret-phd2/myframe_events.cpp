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
#include <wx/spinctrl.h>
#include <wx/textfile.h>
#include "image_math.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

//#ifndef __WXGTK__
#define RAWSAVE
//#endif

#if defined RAWSAVE
  #include <fitsio.h>
#endif

// Some specific camera includes
#if defined (__WINDOWS__) && defined (LE_PARALLEL_CAMERA)
#include "cam_LEwebcam.h"
extern Camera_LEwebcamClass Camera_LEwebcamParallel;
extern Camera_LEwebcamClass Camera_LEwebcamLXUSB;
#endif

double MyFrame::RequestedExposureDuration() { // Sets the global duration variable based on pull-down
	wxString durtext;
	double dReturn;
//	if (CaptureActive) return;  // Looping an exposure already

	durtext = frame->Dur_Choice->GetStringSelection();
	durtext = durtext.BeforeFirst(' '); // remove the " s" bit
#if wxUSE_XLOCALE
	durtext.ToCDouble(&dReturn);
#else
	durtext.ToDouble(&dReturn);
#endif
    dReturn *= 1000;
	if (CurrentGuideCamera->HaveDark) {
		if (CurrentGuideCamera->DarkDur != dReturn)
			Dark_Button->SetBackgroundColour(wxColor(255,0,0));
		else
			Dark_Button->SetBackgroundColour(wxNullColour);
	}

    return dReturn;
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
	if (CaptureActive) return;  // Looping an exposure already
    Close(true);
}

void MyFrame::OnInstructions(wxCommandEvent& WXUNUSED(event)) {
	if (CaptureActive) return;  // Looping an exposure already
	wxMessageBox(wxString::Format(_T("Welcome to PHD (Push Here Dummy) Guiding\n\n \
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
Advanced panel.  ")),_T("Instructions"));

}

void MyFrame::OnHelp(wxCommandEvent& WXUNUSED(event)) {
	help->Display(_T("Introduction"));
}
void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	if (CaptureActive) return;  // Looping an exposure already
#ifdef ORION
	wxMessageBox(wxString::Format(_T("PHD Guiding for Orion v%s\n\nCopyright 2006-2012 Craig Stark, Stark Labs"),VERSION),_T("About PHD Guiding"), wxOK);
#else
	wxMessageBox(wxString::Format(_T("PHD Guiding v%s\n\nwww.stark-labs.com\n\nCopyright 2006-2011 Craig Stark\n\nSpecial Thanks to:\n  Sean Prange\n  Bret McKee\n  Jared Wellman"),VERSION),_T("About PHD Guiding"), wxOK);
#endif
}

void MyFrame::OnOverlay(wxCommandEvent &evt) {
	pGuider->SetOverlayMode(evt.GetId() - MENU_XHAIR0);
}

void MyFrame::OnSave(wxCommandEvent& WXUNUSED(event)) {
//	int retval;
	if (CaptureActive) return;  // Looping an exposure already
#if defined (RAWSAVE)
	wxString fname = wxFileSelector( wxT("Save FITS Image"), (const wxChar *)NULL,
                          (const wxChar *)NULL,
                           wxT("fit"), wxT("FITS files (*.fit)|*.fit"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fname.IsEmpty()) return;  // Check for canceled dialog
	if (wxFileExists(fname))
		fname = _T("!") + fname;
	fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
	long fpixel[3] = {1,1,1};
	long fsize[3];
	int output_format=USHORT_IMG;
	fsize[0] = (long) pCurrentFullFrame->Size.GetWidth();
	fsize[1] = (long) pCurrentFullFrame->Size.GetHeight();
	fsize[2] = 0;
	fits_create_file(&fptr,(const char*) fname.mb_str(wxConvUTF8),&status);
	if (!status) fits_create_img(fptr,output_format, 2, fsize, &status);
	if (!status) fits_write_pix(fptr,TUSHORT,fpixel,pCurrentFullFrame->NPixels,pCurrentFullFrame->ImageData,&status);
	fits_close_file(fptr,&status);
	if (status) wxMessageBox (_T("Error saving FITS file"));
/*	wxImage *Temp_Image;
	CurrentFullFrame.CopyToImage(&Temp_Image,0,255,1.0);
	wxString fname = wxFileSelector( wxT("Save BMP Image"), (const wxChar *)NULL,
                          (const wxChar *)NULL,
                           wxT("bmp"), wxT("BMP files (*.bmp)|*.bmp"),wxSAVE | wxOVERWRITE_PROMPT);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		wxBitmap* DisplayedBitmap = new wxBitmap(Temp_Image,24);
		retval = DisplayedBitmap->SaveFile(fname, wxBITMAP_TYPE_BMP);
		if (!retval)
			(void) wxMessageBox(_T("Error"),wxT("Your data were not saved"),wxOK | wxICON_ERROR);
		else
			SetStatusText(fname + _T(" saved"));
		delete DisplayedBitmap;*/
#else
	if ( (pGuider->Displayed_Image->Ok()) && (pGuider->Displayed_Image->GetWidth()) ) {
		wxString fname = wxFileSelector( wxT("Save BMP Image"), (const wxChar *)NULL,
                          (const wxChar *)NULL,
                           wxT("bmp"), wxT("BMP files (*.bmp)|*.bmp"),wxSAVE | wxOVERWRITE_PROMPT);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
//		wxBitmap* DisplayedBitmap = new wxBitmap(pGuider->Displayed_Image,24);
		bool retval = pGuider->Displayed_Image->SaveFile(fname, wxBITMAP_TYPE_BMP);
		if (!retval)
			(void) wxMessageBox(_T("Error"),wxT("Your data were not saved"),wxOK | wxICON_ERROR);
		else
			SetStatusText(fname + _T(" saved"));
		//delete DisplayedBitmap;
	}
#endif
}

void MyFrame::OnLoadSaveDark(wxCommandEvent &evt) {
	wxString fname;
	fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
	long fpixel[3] = {1,1,1};
	long fsize[3];
	int hdutype, naxis;
	int nhdus=0;

	int output_format=USHORT_IMG;

	if (evt.GetId() == MENU_SAVEDARK) {
		if (!CurrentGuideCamera->HaveDark) {
			wxMessageBox("You haven't captured a dark frame - nothing to save");
			return;
		}
		fname = wxFileSelector( wxT("Save dark (FITS Image)"), (const wxChar *)NULL,
										(const wxChar *)NULL,
										wxT("fit"), wxT("FITS files (*.fit)|*.fit"),wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		if (!fname.EndsWith(_T(".fit"))) fname.Append(_T(".fit"));
		if (wxFileExists(fname))
			fname = _T("!") + fname;
		fsize[0] = (long) CurrentGuideCamera->CurrentDarkFrame.Size.GetWidth();
		fsize[1] = (long) CurrentGuideCamera->CurrentDarkFrame.Size.GetHeight();
		fsize[2] = 0;
		fits_create_file(&fptr,(const char*) fname.mb_str(wxConvUTF8),&status);
		if (!status) fits_create_img(fptr,output_format, 2, fsize, &status);
		if (!status) fits_write_pix(fptr,TUSHORT,fpixel,CurrentGuideCamera->CurrentDarkFrame.NPixels,CurrentGuideCamera->CurrentDarkFrame.ImageData,&status);
		fits_close_file(fptr,&status);
		if (status) wxMessageBox (_T("Error saving FITS file"));
		else SetStatusText ("Dark loaded");
    }
	else if (evt.GetId() == MENU_LOADDARK) {
		fname = wxFileSelector( wxT("Load dark (FITS Image)"), (const wxChar *)NULL,
							   (const wxChar *)NULL,
							   wxT("fit"), wxT("FITS files (*.fit)|*.fit"), wxFD_OPEN | wxFD_CHANGE_DIR);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
		if (!wxFileExists(fname)) {
			wxMessageBox("File does not exist - cannot load");
			return;
		}
		if ( !fits_open_diskfile(&fptr, (const char*) fname.c_str(), READONLY, &status) ) {
			if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU) {
				(void) wxMessageBox(wxT("FITS file is not of an image"),wxT("Error"),wxOK | wxICON_ERROR);
				return;
			}
			
			// Get HDUs and size
			fits_get_img_dim(fptr, &naxis, &status);
			fits_get_img_size(fptr, 2, fsize, &status);
			fits_get_num_hdus(fptr,&nhdus,&status);
			if ((nhdus != 1) || (naxis != 2)) {
				(void) wxMessageBox(_T("Unsupported type or read error loading FITS file"),wxT("Error"),wxOK | wxICON_ERROR);
				return;
			}
			if (CurrentGuideCamera->CurrentDarkFrame.Init((int) fsize[0],(int) fsize[1])) {
				wxMessageBox(_T("Memory allocation error"),wxT("Error"),wxOK | wxICON_ERROR);
				return;
			}
			if (fits_read_pix(fptr, TUSHORT, fpixel, (int) fsize[0]*(int) fsize[1], NULL, CurrentGuideCamera->CurrentDarkFrame.ImageData, NULL, &status) ) { // Read image
				(void) wxMessageBox(_T("Error reading data"),wxT("Error"),wxOK | wxICON_ERROR);
				return;
			}
			fits_close_file(fptr,&status);
			CurrentGuideCamera->HaveDark = true;
			tools_menu->FindItem(MENU_CLEARDARK)->Enable(CurrentGuideCamera->HaveDark);
			Dark_Button->SetLabel(_T("Redo Dark"));
			SetStatusText("Dark loaded");
		}
		else {
			wxMessageBox("Error opening FITS file");
			return;
		}
	}
}
void MyFrame::OnIdle(wxIdleEvent& WXUNUSED(event)) {
/*	if (ASCOM_IsMoving())
		SetStatusText(_T("Moving"),2);
	else
		SetStatusText(_T("Still"),2);*/
}

void MyFrame::OnLoopExposure(wxCommandEvent& WXUNUSED(event)) 
{
    try
    {
        if (!GuideCameraConnected)
        {
            wxMessageBox(_T("Please connect to a camera first"),_T("Info"));
            throw ERROR_INFO("Camera not connected");
        }

        assert(!CaptureActive);

        frame->StartCapturing();

    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
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
        Debug.Write("Processing an image\n");

        usImage *pNextFullFrame = event.GetPayload<usImage *>();
        
        if (event.GetInt())
        {
            delete pNextFullFrame;

            StopCapturing();
            pGuider->ResetGuideState();

            Debug.Write("OnExposureComplete(): Capture Error reported\n");

            throw ERROR_INFO("Error reported capturing image");
        }

        // the capture was OK - switch in the new image
        delete pCurrentFullFrame;
        pCurrentFullFrame = pNextFullFrame;

        pGuider->UpdateGuideState(pCurrentFullFrame, !CaptureActive);
        
        this->Profile->UpdateData(pCurrentFullFrame, pGuider->CurrentPosition().X, pGuider->CurrentPosition().Y);

        if (RandomMotionMode && pGuider->GetState() < STATE_CALIBRATING)
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
				pScope->Guide(dir,dur);
                ScheduleGuide(dir, dur, wxString::Format(_T("Random motion: %d %d"),dir,dur));
			}
        }
        
        if (CaptureActive)
        {
            frame->ScheduleExposure(RequestedExposureDuration());
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
    }
}

void MyFrame::OnGuideComplete(wxThreadEvent& event)
{
    try
    {
        if (event.GetInt())
        {
            throw ERROR_INFO("Error reported guiding");
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
    }
}

void MyFrame::OnButtonStop(wxCommandEvent& WXUNUSED(event)) 
{
    StopCapturing();
}

void MyFrame::OnGammaSlider(wxScrollEvent& WXUNUSED(event)) {
	Stretch_gamma = (double) Gamma_Slider->GetValue() / 100.0;
	pGuider->UpdateImageDisplay(pCurrentFullFrame);
}

void MyFrame::OnDark(wxCommandEvent& WXUNUSED(event)) {
    double ExpDur = RequestedExposureDuration();
	if (pGuider->GetState() > STATE_SELECTED) return;
	if (!GuideCameraConnected) {
		wxMessageBox(_T("Please connect to a camera first"),_T("Info"));
		return;
	}
	if (CaptureActive) return;  // Looping an exposure already
	Dark_Button->SetForegroundColour(wxColour(200,0,0));
	int NDarks = 5;

	SetStatusText(_T("Capturing dark"));
	if (CurrentGuideCamera->HasShutter) 
		CurrentGuideCamera->ShutterState=true; // dark
	else
		wxMessageBox(_T("Cover guide scope"));
	CurrentGuideCamera->InitCapture();
  	if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentGuideCamera->CurrentDarkFrame, false)) {
		wxMessageBox(_T("Error capturing dark frame"));
		CurrentGuideCamera->HaveDark = false;
		SetStatusText(wxString::Format(_T("%.1f s dark FAILED"),(float) ExpDur / 1000.0));
		Dark_Button->SetLabel(_T("Take Dark"));
		CurrentGuideCamera->ShutterState=false;
	}
	else {
		SetStatusText(wxString::Format(_T("%.1f s dark #1 captured"),(float) ExpDur / 1000.0));
		int *avgimg = new int[CurrentGuideCamera->CurrentDarkFrame.NPixels];
		int i, j;
		int *iptr = avgimg;
		unsigned short *usptr = CurrentGuideCamera->CurrentDarkFrame.ImageData;
		for (i=0; i<CurrentGuideCamera->CurrentDarkFrame.NPixels; i++, iptr++, usptr++)
			*iptr = (int) *usptr;
		for (j=1; j<NDarks; j++) {
			CurrentGuideCamera->CaptureFull(ExpDur, CurrentGuideCamera->CurrentDarkFrame, false);
			iptr = avgimg;
			usptr = CurrentGuideCamera->CurrentDarkFrame.ImageData;
			for (i=0; i<CurrentGuideCamera->CurrentDarkFrame.NPixels; i++, iptr++, usptr++)
				*iptr = *iptr + (int) *usptr;
			SetStatusText(wxString::Format(_T("%.1f s dark #%d captured"),(float) ExpDur / 1000.0,j+1));
		}
		iptr = avgimg;
		usptr = CurrentGuideCamera->CurrentDarkFrame.ImageData;
		for (i=0; i<CurrentGuideCamera->CurrentDarkFrame.NPixels; i++, iptr++, usptr++)
			*usptr = (unsigned short) (*iptr / NDarks);


		Dark_Button->SetLabel(_T("Redo Dark"));
		CurrentGuideCamera->HaveDark = true;
		CurrentGuideCamera->DarkDur = ExpDur;
	}
	SetStatusText(_T("Darks done"));
	if (CurrentGuideCamera->HasShutter)
		CurrentGuideCamera->ShutterState=false; // Lights
	else 
		wxMessageBox(_T("Uncover guide scope"));
	tools_menu->FindItem(MENU_CLEARDARK)->Enable(CurrentGuideCamera->HaveDark);
}

void MyFrame::OnClearDark(wxCommandEvent& WXUNUSED(evt)) {
	if (!CurrentGuideCamera->HaveDark) return;
	Dark_Button->SetLabel(_T("Take Dark"));
	Dark_Button->SetForegroundColour(wxColour(0,0,0));
	CurrentGuideCamera->HaveDark = false;
	tools_menu->FindItem(MENU_CLEARDARK)->Enable(CurrentGuideCamera->HaveDark);
}

void MyFrame::OnGraph(wxCommandEvent &evt) {
	this->GraphLog->SetState(evt.IsChecked());
}

void MyFrame::OnStarProfile(wxCommandEvent &evt) {
	this->Profile->SetState(evt.IsChecked());
}

void MyFrame::OnLog(wxCommandEvent &evt) {
	if (evt.GetId() == MENU_LOG) {
		if (evt.IsChecked()) {  // enable it
			Log_Data = true;
			if (!LogFile->IsOpened()) {
				if (LogFile->Exists()) LogFile->Open();
				else LogFile->Create();
			}
			wxDateTime now = wxDateTime::Now();
			LogFile->AddLine(_T("Logging manually enabled"));
			LogFile->AddLine(wxString::Format(_T("PHD Guide %s  -- "),VERSION) + now.FormatDate()  + _T(" ") + now.FormatTime());
			LogFile->Write();
			this->SetTitle(wxString::Format(_T("PHD Guiding %s  -  www.stark-labs.com (Log active)"),VERSION));
		}
		else {
			if (LogFile->IsOpened()) {
				LogFile->AddLine(_T("Logging manually disabled"));
				LogFile->Write();
				LogFile->Close();
			}
			Log_Data = false;
			this->SetTitle(wxString::Format(_T("PHD Guiding %s  -  www.stark-labs.com"),VERSION));
		}
	}
	else if (evt.GetId() == MENU_LOGIMAGES) {
		if (wxGetKeyState(WXK_SHIFT)) {
//			wxMessageBox("arg");
#ifdef __WINDOWS__
//			tools_menu->FindItem(MENU_LOGIMAGES)->SetTextColour(wxColour(200,10,10));
#endif
			tools_menu->FindItem(MENU_LOGIMAGES)->SetItemLabel(_T("Enable Raw Star logging"));
			if (evt.IsChecked())
				Log_Images = 2;
			else
				Log_Images = 0;
		}
		else {
#ifdef __WINDOWS__
//			tools_menu->FindItem(MENU_LOGIMAGES)->SetTextColour(*wxBLACK);
#endif
			tools_menu->FindItem(MENU_LOGIMAGES)->SetText(_T("Enable Star Image logging"));
			if (evt.IsChecked())
				Log_Images = 1;
			else
				Log_Images = 0;
			
		}
		Menubar->Refresh();
	}
	else if (evt.GetId() == MENU_DEBUG)
    {
        Debug.SetState(evt.IsChecked()); 
    }
}

bool MyFrame::FlipRACal( wxCommandEvent& WXUNUSED(evt))
{
	if (!pScope->IsCalibrated())
	{
		SetStatusText(_T("No CAL"));
		return false;
	}
    double RaAngle  = pScope->RaAngle();

	double orig=pScope->RaAngle();
	RaAngle += 3.14;
	if (RaAngle > 3.14)
		RaAngle -= 6.28;
	pScope->SetCalibration(RaAngle, pScope->DecAngle(), pScope->RaRate(), pScope->DecRate());
	SetStatusText(wxString::Format(_T("CAL: %.2f -> %.2f"),orig,pScope->RaAngle()),0);
	return true;
}

void MyFrame::OnAutoStar(wxCommandEvent& WXUNUSED(evt)) {
    frame->pGuider->AutoSelect(pCurrentFullFrame);
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

class AdvancedDialog: public wxDialog {
    ConfigDialogPane *m_pFramePane;
    ConfigDialogPane *m_pMountPane;
    ConfigDialogPane *m_pCameraPane;
    ConfigDialogPane *m_pGuiderPane;
public:
	AdvancedDialog();
	~AdvancedDialog(void) {};

    void LoadValues(void);
    void UnloadValues(void);
private:
	void OnSetupCamera(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()
};

AdvancedDialog::AdvancedDialog():
#if defined (__WINDOWS__)
wxDialog(frame, wxID_ANY, _T("Advanced setup"), wxPoint(-1,-1), wxSize(210,350), wxCAPTION | wxCLOSE_BOX)
#else
wxDialog(frame, wxID_ANY, _T("Advanced setup"), wxPoint(-1,-1), wxSize(250,350), wxCAPTION | wxCLOSE_BOX)
#endif
{
    /*
     * the advanced dialog is made up of a number of "on the fly" generate slices that configure different things.
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

    // build all the empty sizer
    wxBoxSizer *pConfigSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *pLeftSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *pRightSizer = new wxBoxSizer(wxVERTICAL);

    pConfigSizer->Add(pLeftSizer, 0, wxALIGN_CENTER | wxGROW);
    pConfigSizer->Add(pRightSizer, 0, wxALIGN_CENTER | wxGROW);

    wxBoxSizer *pTopLevelSizer = new wxBoxSizer(wxVERTICAL);
    pTopLevelSizer->Add(pConfigSizer, 0, wxALIGN_CENTER | wxGROW);
    pTopLevelSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_CENTER | wxGROW);

    // Build the left column of panes

    m_pFramePane = frame->GetConfigDialogPane(this);
    pLeftSizer->Add(m_pFramePane, 0, wxALIGN_CENTER | wxGROW);

    m_pMountPane = pScope->GetConfigDialogPane(this);
    pLeftSizer->Add(m_pMountPane, 0, wxALIGN_CENTER | wxGROW);

    if (CurrentGuideCamera)
    {
        m_pCameraPane = CurrentGuideCamera->GetConfigDialogPane(this);
        pLeftSizer->Add(m_pCameraPane, 0, wxALIGN_CENTER | wxGROW);
    }
    else
    {
        m_pCameraPane=NULL;
        wxStaticBoxSizer *pBox = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Camera Settings")), wxVERTICAL);
        wxStaticText *pText = new wxStaticText(this, wxID_ANY, _T("No Camera Connected"),wxPoint(-1,-1),wxSize(-1,-1));
        pBox->Add(pText);
        pLeftSizer->Add(pBox, 0, wxALIGN_CENTER | wxGROW);
    }

    // Build the right column of panes

    m_pGuiderPane = frame->pGuider->GetConfigDialogPane(this);
    pRightSizer->Add(m_pGuiderPane, 0, wxALIGN_CENTER | wxGROW);

    SetSizerAndFit(pTopLevelSizer);
}


void AdvancedDialog::LoadValues(void)
{
    m_pFramePane->LoadValues();
    m_pMountPane->LoadValues();
    m_pGuiderPane->LoadValues();

    if (m_pCameraPane)
    {
        m_pCameraPane->LoadValues();
    }
}

void AdvancedDialog::UnloadValues(void)
{
    m_pFramePane->UnloadValues();
    m_pMountPane->UnloadValues();
    m_pGuiderPane->UnloadValues();

    if (m_pCameraPane)
    {
        m_pCameraPane->UnloadValues();
    }
}

BEGIN_EVENT_TABLE(AdvancedDialog, wxDialog)
	EVT_BUTTON(wxID_PROPERTIES,AdvancedDialog::OnSetupCamera)
END_EVENT_TABLE()

void AdvancedDialog::OnSetupCamera(wxCommandEvent& WXUNUSED(event)) {
	// Prior to this we check to make sure the current camera is a WDM camera (main dialog) but...

	if (frame->CaptureActive || !GuideCameraConnected || !CurrentGuideCamera->HasPropertyDialog) return;  // One more safety check
	/*if (CurrentGuideCamera == &Camera_WDM)
		Camera_WDM.ShowPropertyDialog();
	else if (CurrentGuideCamera == &Camera_VFW)
		Camera_VFW.ShowPropertyDialog();*/
	CurrentGuideCamera->ShowPropertyDialog();

}

void MyFrame::OnSetupCamera(wxCommandEvent& WXUNUSED(event)) {
	if (!GuideCameraConnected || !CurrentGuideCamera->HasPropertyDialog) return;  // One more safety check

	CurrentGuideCamera->ShowPropertyDialog();

}
void MyFrame::OnAdvanced(wxCommandEvent& WXUNUSED(event)) {

	if (CaptureActive) return;  // Looping an exposure already
	AdvancedDialog* dlog = new AdvancedDialog();

    dlog->LoadValues();

	if (dlog->ShowModal() == wxID_OK) 
    {
        dlog->UnloadValues();
#ifdef BRET_TODO
	frame->GraphLog->RAA_Ctrl->SetValue((int) (RA_aggr * 100));
	frame->GraphLog->RAH_Ctrl->SetValue((int) (RA_hysteresis * 100));
#if ((wxMAJOR_VERSION > 2) || (wxMINOR_VERSION > 8))
	frame->GraphLog->MM_Ctrl->SetValue(MinMotion);
#endif
	frame->GraphLog->MDD_Ctrl->SetValue(Max_Dec_Dur);
	frame->GraphLog->DM_Ctrl->SetSelection(Dec_guide);
#endif // BRET_TODO
    }
}


class TestGuideDialog: public wxDialog {
public:
	TestGuideDialog();
	~TestGuideDialog(void) {};
private:
		void OnButton(wxCommandEvent& evt);
	wxButton *NButton, *SButton, *EButton, *WButton;
	DECLARE_EVENT_TABLE()
};

TestGuideDialog::TestGuideDialog():
wxDialog(frame, wxID_ANY, _T("Manual Output"), wxPoint(-1,-1), wxSize(300,300)) {
	wxGridSizer *sizer = new wxGridSizer(3,3,0,0);

	NButton = new wxButton(this,MGUIDE_N,_T("North"),wxPoint(-1,-1),wxSize(-1,-1));
	SButton = new wxButton(this,MGUIDE_S,_T("South"),wxPoint(-1,-1),wxSize(-1,-1));
	EButton = new wxButton(this,MGUIDE_E,_T("East"),wxPoint(-1,-1),wxSize(-1,-1));
	WButton = new wxButton(this,MGUIDE_W,_T("West"),wxPoint(-1,-1),wxSize(-1,-1));
	sizer->AddStretchSpacer();
	sizer->Add(NButton,wxSizerFlags().Expand().Border(wxALL,6));
	sizer->AddStretchSpacer();
	sizer->Add(WButton,wxSizerFlags().Expand().Border(wxALL,6));
	sizer->AddStretchSpacer();
	sizer->Add(EButton,wxSizerFlags().Expand().Border(wxALL,6));
	sizer->AddStretchSpacer();
	sizer->Add(SButton,wxSizerFlags().Expand().Border(wxALL,6));

	SetSizer(sizer);
	sizer->SetSizeHints(this);
}

BEGIN_EVENT_TABLE(TestGuideDialog, wxDialog)
EVT_BUTTON(MGUIDE_N,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE_S,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE_E,TestGuideDialog::OnButton)
EVT_BUTTON(MGUIDE_W,TestGuideDialog::OnButton)
END_EVENT_TABLE()


void TestGuideDialog::OnButton(wxCommandEvent &evt) {
//	if ((frame->pGuider->GetState() > STATE_SELECTED) || !(pScope->IsConnected())) return;
	if (!(pScope->IsConnected())) return;
	switch (evt.GetId()) {
		case MGUIDE_N:
			pScope->Guide(NORTH, pScope->GetCalibrationDuration());
			break;
		case MGUIDE_S:
			pScope->Guide(SOUTH, pScope->GetCalibrationDuration());
			break;
		case MGUIDE_E:
			pScope->Guide(EAST, pScope->GetCalibrationDuration());
			break;
		case MGUIDE_W:
			pScope->Guide(WEST, pScope->GetCalibrationDuration());
			break;
	}
}

void MyFrame::OnTestGuide(wxCommandEvent& WXUNUSED(evt)) {
	if ((frame->pGuider->GetState() > STATE_SELECTED) || !(pScope->IsConnected())) return;
	TestGuideDialog* dlog = new TestGuideDialog();
	dlog->Show();
}
