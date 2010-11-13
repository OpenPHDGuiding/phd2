/*
 *  frame_events.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
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
//#include "ascom.h"
#include "scope.h"
#include "camera.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

//#ifndef __WXGTK__
#define RAWSAVE
//#endif

#if defined RAWSAVE
 #if defined (__APPLE__)
  #include "../cfitsio/fitsio.h"
 #else
  #include <fitsio.h>
 #endif
#endif

// Some specific camera includes
#if defined (__WINDOWS__) && defined (LE_PARALLEL_CAMERA)
#include "cam_LEwebcam.h"
extern Camera_LEwebcamClass Camera_LEwebcamParallel;
extern Camera_LEwebcamClass Camera_LEwebcamLXUSB;
#endif


void MyFrame::SetExpDuration() { // Sets the global duration variable based on pull-down
	wxString durtext;
	double tmpval;
//	if (CaptureActive) return;  // Looping an exposure already

	durtext = frame->Dur_Choice->GetStringSelection();
	durtext = durtext.BeforeFirst(' '); // remove the " s" bit
#if wxUSE_XLOCALE
	durtext.ToCDouble(&tmpval);
#else
	durtext.ToDouble(&tmpval);
#endif
	ExpDur = (int) (tmpval * 1000);
	if (HaveDark) {
		if (DarkDur != ExpDur)
			Dark_Button->SetBackgroundColour(wxColor(255,0,0));
		else
			Dark_Button->SetBackgroundColour(wxNullColour);
	}

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
	wxMessageBox(wxString::Format(_T("PHD Guiding for Orion v%s\n\nCopyright 2006-2009 Craig Stark, Stark Labs"),VERSION),_T("About PHD Guiding"), wxOK);
#else
	wxMessageBox(wxString::Format(_T("PHD Guiding v%s\n\nwww.stark-labs.com\n\nCopyright 2006-2009 Craig Stark\n\nSpecial Thanks to:\n  Sean Prange"),VERSION),_T("About PHD Guiding"), wxOK);
#endif
}

void MyFrame::OnOverlay(wxCommandEvent &evt) {
	OverlayMode = evt.GetId() - MENU_XHAIR0;
	canvas->Refresh();
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
	fsize[0] = (long) CurrentFullFrame.Size.GetWidth();
	fsize[1] = (long) CurrentFullFrame.Size.GetHeight();
	fsize[2] = 0;
	fits_create_file(&fptr,(const char*) fname.mb_str(wxConvUTF8),&status);
	if (!status) fits_create_img(fptr,output_format, 2, fsize, &status);
	if (!status) fits_write_pix(fptr,TUSHORT,fpixel,CurrentFullFrame.NPixels,CurrentFullFrame.ImageData,&status);
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
	if ( (canvas->Displayed_Image->Ok()) && (canvas->Displayed_Image->GetWidth()) ) {
		wxString fname = wxFileSelector( wxT("Save BMP Image"), (const wxChar *)NULL,
                          (const wxChar *)NULL,
                           wxT("bmp"), wxT("BMP files (*.bmp)|*.bmp"),wxSAVE | wxOVERWRITE_PROMPT);
		if (fname.IsEmpty()) return;  // Check for canceled dialog
//		wxBitmap* DisplayedBitmap = new wxBitmap(canvas->Displayed_Image,24);
		bool retval = canvas->Displayed_Image->SaveFile(fname, wxBITMAP_TYPE_BMP);
		if (!retval)
			(void) wxMessageBox(_T("Error"),wxT("Your data were not saved"),wxOK | wxICON_ERROR);
		else
			SetStatusText(fname + _T(" saved"));
		//delete DisplayedBitmap;
	}
#endif
}

void MyFrame::OnIdle(wxIdleEvent& WXUNUSED(event)) {
/*	if (ASCOM_IsMoving())
		SetStatusText(_T("Moving"),2);
	else
		SetStatusText(_T("Still"),2);*/
}

void MyFrame::OnLoopExposure(wxCommandEvent& WXUNUSED(event)) {
	if (canvas->State > STATE_SELECTED) return;
	if (!GuideCameraConnected) {
		wxMessageBox(_T("Please connect to a camera first"),_T("Info"));
		return;
	}
	if (CaptureActive) return;  // Looping an exposure already
	wxStandardPathsBase& stdpath = wxStandardPaths::Get();
//	wxFileOutputStream debugstr (wxString(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_Debug_log") + _T(".txt")));
	wxFFileOutputStream debugstr (wxString(stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_Debug_log") + _T(".txt")), _T("a+t"));
	wxTextOutputStream debug (debugstr);
//	wxString debug_fname = 	stdpath.GetDocumentsDir() + PATHSEPSTR + _T("PHD_Debug_log") + _T(".txt");

	Abort = 0;
	CaptureActive = true;
	int i=0;
	SetStatusText (_T("Capturing"),0);
	SetExpDuration();
	CurrentGuideCamera->InitCapture();
	Loop_Button->Enable(false);
	Guide_Button->Enable(false);
	Cam_Button->Enable(false);
	Scope_Button->Enable(false);
	Brain_Button->Enable(false);
	Dark_Button->Enable(false);

	bool debuglog = this->Menubar->IsChecked(MENU_DEBUG);
	if (debuglog) {
		wxDateTime now = wxDateTime::Now();
//		debugfile->AddLine(wxString::Format("DEBUG PHD Guide %s  -- ",VERSION) + now.FormatDate() + now.FormatTime());
		debug << _T("\n\nDEBUG PHD Guide ") << VERSION << _T(" ") <<  now.FormatDate() << _T(" ") <<  now.FormatTime() << endl;
		debug << _T("Machine: ") << wxGetOsDescription() << _T(" ") << wxGetUserName() << endl;
		debug << _T("Camera: ") << CurrentGuideCamera->Name << endl;
		debug << _T("Dur: ") << ExpDur << _T(" NR: ") << NR_mode << _T(" Dark: ") << HaveDark << endl;
		debug << _T("Looping entered\n");
		debugstr.Sync();
	}

//	wxStatusBar *StatBar = GetStatusBar();
	//wxColor DefaultColor = GetBackgroundColour();

	while (!Abort) {
		i++;
		SetExpDuration();
	//	SetStatusText(wxString::Format("Frame %d %dms",i,ExpDur));
		while (Paused) {
			wxMilliSleep(250);
			wxTheApp->Yield();
		}
		if (debuglog) { debug << _T("Capturing - "); debugstr.Sync(); }
		try {
			if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentFullFrame)) {
				Abort = 1;
				break;
			}
		}
		catch (...) {
			wxMessageBox(_T("Exception thrown during image capture - bailing"));
			if (debuglog) { debug << _T("Camera threw an exception during capture\n"); debugstr.Sync(); }
			Abort = 1;
			break;
		}
		if (debuglog) { debug << _T("Done\n"); debugstr.Sync(); }
		if (debuglog && NR_mode) debug << _T("Calling NR - ");
		if (NR_mode == NR_2x2MEAN)
			QuickLRecon(CurrentFullFrame);
		else if (NR_mode == NR_3x3MEDIAN)
			Median3(CurrentFullFrame);
		if (debuglog && NR_mode) { debug << _T("Done\n"); debugstr.Sync(); }

		if (canvas->State == STATE_SELECTED) {  // May take this out
			if (debuglog) { debug << _T("Finding star - "); debugstr.Sync(); }
			FindStar(CurrentFullFrame); // track it
			if (debuglog) { debug << _T("Done (") << FoundStar << _T(")\n"); debugstr.Sync(); }
			if (FoundStar)
				SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"),StarMass,StarSNR));
			else {
				SetStatusText(_T("Star lost"));
			/*	SetBackgroundColour(wxColour(255,0,0));
				Refresh();
				wxTheApp->Yield();
				wxMilliSleep(100);
				SetBackgroundColour(DefaultColor);
				Refresh();*/
			}
			this->Profile->UpdateData(CurrentFullFrame,StarX,StarY);
			Guide_Button->Enable(FoundStar && (ScopeConnected > 0));
		}
//		wxTheApp->Yield();
		if (debuglog) { debug << _T("Calling display - "); debugstr.Sync(); }
		canvas->FullFrameToDisplay();
		if (debuglog) { debug << _T("Done\n"); debugstr.Sync(); }
		wxTheApp->Yield(true);
		if (RandomMotionMode) {
			int dir = (rand() % 2) + 2;  // RA
			int dur = rand() % 1000;
			SetStatusText(wxString::Format(_T("Random motion: %d %d"),dir,dur),1);
			GuideScope(dir,dur);
			if ((rand() % 5) == 0) {  // Occasional Dec
				dir = (rand() % 2);
				dur = rand() % 1000;
				SetStatusText(wxString::Format(_T("Random motion: %d %d"),dir,dur),1);
				GuideScope(dir,dur);
			}
		}
	}
	if (debuglog) { debug << _T("Looping exited\n"); debugstr.Sync(); }
	Loop_Button->Enable(true);
	Guide_Button->Enable(ScopeConnected > 0);
	Cam_Button->Enable(true);
	Scope_Button->Enable(true);
	Brain_Button->Enable(true);
	Dark_Button->Enable(true);
	CaptureActive = false;
	SetStatusText(_T(""));
	if (Abort == 2) {
		Abort = 0;
		wxCommandEvent *evt = new wxCommandEvent(BUTTON_GUIDE, 100);
		//wxPostEvent(wxTheApp,event);
		OnGuide(*evt);
	}
	else Abort = 0;
}

void MyFrame::OnButtonStop(wxCommandEvent& WXUNUSED(event)) {
	Abort = 1;

}

void MyFrame::OnGammaSlider(wxScrollEvent& WXUNUSED(event)) {
	Stretch_gamma = (double) Gamma_Slider->GetValue() / 100.0;
	canvas->FullFrameToDisplay();
}

void MyFrame::OnDark(wxCommandEvent& WXUNUSED(event)) {
	if (canvas->State > STATE_SELECTED) return;
	if (!GuideCameraConnected) {
		wxMessageBox(_T("Please connect to a camera first"),_T("Info"));
		return;
	}
	if (CaptureActive) return;  // Looping an exposure already
	Dark_Button->SetForegroundColour(wxColour(200,0,0));
	int NDarks = 5;

	SetStatusText(_T("Capturing dark"));
	wxMessageBox(_T("Cover guide scope"));
	SetExpDuration();
	CurrentGuideCamera->InitCapture();
  	if (CurrentGuideCamera->CaptureFull(ExpDur, CurrentDarkFrame, false)) {
		wxMessageBox(_T("Error capturing dark frame"));
		HaveDark = false;
		SetStatusText(wxString::Format(_T("%.1f s dark FAILED"),(float) ExpDur / 1000.0));
		Dark_Button->SetLabel(_T("Take Dark"));
	}
	else {
		SetStatusText(wxString::Format(_T("%.1f s dark #1 captured"),(float) ExpDur / 1000.0));
		int *avgimg = new int[CurrentDarkFrame.NPixels];
		int i, j;
		int *iptr = avgimg;
		unsigned short *usptr = CurrentDarkFrame.ImageData;
		for (i=0; i<CurrentDarkFrame.NPixels; i++, iptr++, usptr++)
			*iptr = (int) *usptr;
		for (j=1; j<NDarks; j++) {
			CurrentGuideCamera->CaptureFull(ExpDur, CurrentDarkFrame, false);
			iptr = avgimg;
			usptr = CurrentDarkFrame.ImageData;
			for (i=0; i<CurrentDarkFrame.NPixels; i++, iptr++, usptr++)
				*iptr = *iptr + (int) *usptr;
			SetStatusText(wxString::Format(_T("%.1f s dark #%d captured"),(float) ExpDur / 1000.0,j+1));
		}
		iptr = avgimg;
		usptr = CurrentDarkFrame.ImageData;
		for (i=0; i<CurrentDarkFrame.NPixels; i++, iptr++, usptr++)
			*usptr = (unsigned short) (*iptr / NDarks);


		Dark_Button->SetLabel(_T("Redo Dark"));
		HaveDark = true;
		DarkDur = ExpDur;
	}
	SetStatusText(_T("Darks done"));
	wxMessageBox(_T("Uncover guide scope"));
	tools_menu->FindItem(MENU_CLEARDARK)->Enable(HaveDark);
}

void MyFrame::OnClearDark(wxCommandEvent& WXUNUSED(evt)) {
	if (!HaveDark) return;
	Dark_Button->SetLabel(_T("Take Dark"));
	Dark_Button->SetForegroundColour(wxColour(0,0,0));
	HaveDark = false;
	tools_menu->FindItem(MENU_CLEARDARK)->Enable(HaveDark);
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
		Log_Images = evt.IsChecked();
	}
}

void MyFrame::OnAutoStar(wxCommandEvent& WXUNUSED(evt)) {
	int x,y;

	if (!CurrentFullFrame.NPixels) // Need to have an image
		return;
	if ((canvas->State == STATE_CALIBRATING) || (canvas->State == STATE_GUIDING_LOCKED))
		return;
	Paused = true;
	AutoFindStar(CurrentFullFrame,x,y);
	Paused = false;
	if ((x*y) == 0) // if it failed to find a star x=y=0
		return;
	StarX=x;
	StarY=y;
	dX = dY = 0.0;
	canvas->State=STATE_SELECTED;
	FindStar(CurrentFullFrame);
	LockX = StarX;
	LockY = StarY;
	SetStatusText(wxString::Format(_T("Star %.2f %.2f"),StarX,StarY));
	canvas->Refresh();
}

class AdvancedDialog: public wxDialog {
public:
	wxSpinCtrl *RA_Aggr_Ctrl;
	wxSpinCtrl *RA_Hyst_Ctrl;
//	wxSpinCtrl *Dec_Aggr_Ctrl;
	wxChoice	*Dec_Mode;
	wxChoice	*Dec_AlgoCtrl;
	wxTextCtrl *DecSlopeWeight_Ctrl;
	wxCheckBox *Cal_Box;
	wxCheckBox *Subframe_Box;
//	wxSpinCtrl	*Dec_Backlash_Ctrl;
	wxSpinCtrl	*Cal_Dur_Ctrl;
	wxSpinCtrl *Time_Lapse_Ctrl;
	wxSpinCtrl *Gain_Ctrl;
	wxSpinCtrl *SearchRegion_Ctrl;
	wxTextCtrl *MinMotion_Ctrl;
	wxTextCtrl *MassDelta_Ctrl;
	wxSpinCtrl *MaxDecDur_Ctrl;
	wxSpinCtrl *MaxRADur_Ctrl;
	wxChoice	*NR_Ctrl;
//	wxButton	*Setup_Button;
	wxCheckBox *Log_Box;
	wxCheckBox *Disable_Box;
	wxCheckBox *RADither_Box;
//	wxButton	*OK_Button;
//	wxButton	*Cancel_Button;
	wxSpinCtrl	*Delay_Ctrl;
	wxChoice	*Port_Choice;
	wxStaticText *Delay_Text, *Port_Text;

	AdvancedDialog();
	~AdvancedDialog(void) {};
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
	if (AdvDlg_fontsize > 0)  // From Open-PHD -- not sure the point of this line
		SetFont(wxFont(AdvDlg_fontsize,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL));
	wxFlexGridSizer *sizer = new wxFlexGridSizer(4);

	wxStaticText *RAA_Text = new wxStaticText(this,wxID_ANY,_T("RA Aggressiveness"),wxPoint(-1,-1),wxSize(-1,-1));
	RA_Aggr_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,120,100,_T("RA_Aggr"));
	RA_Aggr_Ctrl->SetToolTip(_T("What percent of the measured error should be applied? Default = 100%, adjust if responding too much or too slowly?"));
	sizer->Add(RAA_Text,wxSizerFlags().Expand().Proportion(2).Border(wxALL,3));
	sizer->Add(RA_Aggr_Ctrl,wxSizerFlags().Border(wxALL,3));

	wxStaticText *DM_Text = new wxStaticText(this,wxID_ANY,_T("Dec guide mode"),wxPoint(-1,-1),wxSize(-1,-1));
	wxString dec_choices[] = {
		_T("Off"),_T("Auto"),_T("North"),_T("South")
	};
	Dec_Mode= new wxChoice(this,wxID_ANY,wxPoint(-1,-1),wxSize(75,-1),WXSIZEOF(dec_choices), dec_choices );
	Dec_Mode->SetToolTip(_T("Guide in declination as well?"));
	sizer->Add(DM_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(Dec_Mode,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *RAH_Text =new wxStaticText(this,wxID_ANY,_T("RA Hysteresis"),wxPoint(-1,-1),wxSize(-1,-1));
	RA_Hyst_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,50,10,_T("RA_Hyst"));
	RA_Hyst_Ctrl->SetToolTip(_T("How much history of previous guide pulses should be applied\nDefault = 10%, increase to smooth out guiding commands"));
	sizer->Add(RAH_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(RA_Hyst_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *DAlgo_Text = new wxStaticText(this,wxID_ANY,_T("Dec Algorithm"),wxPoint(-1,-1),wxSize(-1,-1));
	wxString decalgo_choices[] = {
		_T("Lowpass filter"),_T("Resist switching")
	};	// ,_T("Lowpass-2")
	Dec_AlgoCtrl= new wxChoice(this,wxID_ANY,wxPoint(-1,-1),wxSize(75,-1),WXSIZEOF(decalgo_choices), decalgo_choices );
	Dec_AlgoCtrl->SetToolTip(_T("Declination guide algorithm"));
	sizer->Add(DAlgo_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(Dec_AlgoCtrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *MRAD_Text =new wxStaticText(this,wxID_ANY,_T("Max RA duration (ms)"),wxPoint(-1,-1),wxSize(-1,-1));
	MaxRADur_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,2000,1000,_T("MaxRA_Dur"));
	MaxRADur_Ctrl->SetToolTip(_T("Longest length of pulse to send in RA\nDefault = 1000 ms. "));
	sizer->Add(MRAD_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(MaxRADur_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *DSR_Text = new wxStaticText(this,wxID_ANY,_T("Dec slope weight"));
	DecSlopeWeight_Ctrl = new wxTextCtrl(this,wxID_ANY,wxString::Format(_T("%.2f"),Dec_slopeweight),wxPoint(-1,-1),wxSize(75,-1));
	DecSlopeWeight_Ctrl->SetToolTip(_T("Weighting of slope parameter in lowpass auto-dec"));
	sizer->Add(DSR_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(DecSlopeWeight_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *SR_Text = new wxStaticText(this,wxID_ANY,_T("Search region (pixels)"));
	SearchRegion_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo2"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,10,50,15,_T("Search"));
	SearchRegion_Ctrl->SetToolTip(_T("How many pixels (up/down/left/right) do we examine to find the star? Default = 15"));
	sizer->Add(SR_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(SearchRegion_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *MDD_Text =new wxStaticText(this,wxID_ANY,_T("Max Dec duration (ms)"),wxPoint(-1,-1),wxSize(-1,-1));
	MaxDecDur_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,2000,150,_T("MaxDec_Dur"));
	MaxDecDur_Ctrl->SetToolTip(_T("Longest length of pulse to send in declination\nDefault = 100 ms.  Increase if drift is fast."));
	sizer->Add(MDD_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(MaxDecDur_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *MM_Text = new wxStaticText(this,wxID_ANY,_T("Min. motion (pixels)"));
	MinMotion_Ctrl = new wxTextCtrl(this,wxID_ANY,wxString::Format(_T("%.2f"),MinMotion),wxPoint(-1,-1),wxSize(75,-1));
	MinMotion_Ctrl->SetToolTip(_T("How many pixels (fractional pixels) must the star move to trigger a guide pulse? Default = 0.15"));
	sizer->Add(MM_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(MinMotion_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *MDelta_Text = new wxStaticText(this,wxID_ANY,_T("Star mass tolerance"));
	MassDelta_Ctrl = new wxTextCtrl(this,wxID_ANY,wxString::Format(_T("%.2f"),StarMassChangeRejectThreshold),wxPoint(-1,-1),wxSize(75,-1));
	MassDelta_Ctrl->SetToolTip(_T("Tolerance for change in star mass b/n frames. Default = 0.3 (0.1-1.0)"));
	sizer->Add(MDelta_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(MassDelta_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *CS_Text = new wxStaticText(this,wxID_ANY,_T("Calibration step (ms)"));
	Cal_Dur_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo2"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,10000,1000,_T("Cal_Dur"));
	Cal_Dur_Ctrl->SetToolTip(_T("How long a guide pulse should be used during calibration? Default = 750ms, increase for short f/l scopes and decrease for longer f/l scopes"));
	sizer->Add(CS_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(Cal_Dur_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *NR_Text = new wxStaticText(this,wxID_ANY,_T("Noise Reduction"));
	wxString nralgo_choices[] = {
		_T("None"),_T("2x2 mean"),_T("3x3 median")
	};
	NR_Ctrl= new wxChoice(this,wxID_ANY,wxPoint(-1,-1),wxSize(75,-1),WXSIZEOF(nralgo_choices), nralgo_choices );
	NR_Ctrl->SetToolTip(_T("Technique to reduce noise in images"));
	sizer->Add(NR_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(NR_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *TL_Text = new wxStaticText(this,wxID_ANY,_T("Time lapse (ms)"));
	Time_Lapse_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo2"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,10000,0,_T("Time_lapse"));
	Time_Lapse_Ctrl->SetToolTip(_T("How long should PHD wait between guide frames? Default = 0ms, useful when using very short exposures (e.g., using a video camera) but wanting to send guide commands less frequently"));
	sizer->Add(TL_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(Time_Lapse_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxStaticText *CG_Text = new wxStaticText(this,wxID_ANY,_T("Camera gain (%)"));
	Gain_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo2"),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,100,100,_T("Cam_Gain"));
	Gain_Ctrl->SetToolTip(_T("Camera gain boost? Default = 95%, lower if you experience noise or wish to guide on a very bright star). Not available on all cameras."));
	Gain_Ctrl->Enable(false);
	sizer->Add(CG_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(Gain_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));


//	Setup_Button = new wxButton(this,wxID_PROPERTIES,_T("Camera Setup"),wxPoint(100,217),wxSize(-1,-1));
//	new wxStaticText(this,wxID_ANY,_T("Dec Aggressiveness"),wxPoint(10,102),wxSize(-1,-1));
//	Dec_Aggr_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo2"),wxPoint(120,100),wxSize(60,-1),wxSP_ARROW_KEYS,0,200,100,_T("Dec_Aggr"));

	//new wxStaticText(this,wxID_ANY,_T("Dec Backlash"),wxPoint(10,102),wxSize(-1,-1));
	//Dec_Backlash_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T("foo3"),wxPoint(120,100),wxSize(60,-1),wxSP_ARROW_KEYS,0,3000,0,_T("Dec_Backlash"));

	wxString port_choices[] = {
		_T("Port 378"),_T("Port 3BC"),_T("Port 278"),_T("COM1"),_T("COM2"),_T("COM3"),_T("COM4")
	};
	Port_Choice= new wxChoice(this,wxID_ANY,wxPoint(-1,-1),wxSize(75,-1),WXSIZEOF(port_choices), port_choices );
	Port_Choice->SetToolTip(_T("Port number for long-exposure control"));
	Port_Choice->SetSelection(0);
	//Port_Choice->Enable(false);
	Port_Text = new wxStaticText(this,wxID_ANY,_T("LE Port"),wxPoint(-1,1),wxSize(-1,-1));
	sizer->Add(Port_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(Port_Choice,wxSizerFlags().Proportion(1).Border(wxALL,3));


	Delay_Ctrl = new wxSpinCtrl(this,wxID_ANY,_T(""),wxPoint(-1,-1),wxSize(75,-1),wxSP_ARROW_KEYS,0,50,0,_T("Delay"));
	Delay_Ctrl->SetToolTip(_T("Adjust if you get dropped frames"));
	Delay_Ctrl->Enable(false);
	Delay_Text = new wxStaticText(this,wxID_ANY,_T("LE Read Delay"),wxPoint(-1,-1),wxSize(-1,-1));
	sizer->Add(Delay_Text,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(Delay_Ctrl,wxSizerFlags().Proportion(1).Border(wxALL,3));

	Cal_Box = new wxCheckBox(this,wxID_ANY,_T("Force calibration"),wxPoint(-1,-1),wxSize(75,-1));
	Cal_Box->SetToolTip(_T("Check to clear any previous calibration and force PHD to recalibrate"));
	wxStaticText *TmpText = new wxStaticText(this,wxID_ANY,_T(""));
	sizer->Add(Cal_Box,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(TmpText,wxSizerFlags().Proportion(1).Border(wxALL,3));

	Subframe_Box = new wxCheckBox(this,wxID_ANY,_T("Use subframes"),wxPoint(-1,-1),wxSize(75,-1));
	Subframe_Box->SetToolTip(_T("Check to only download subframes (ROIs) if your camera supports it"));
	wxStaticText *TmpText4 = new wxStaticText(this,wxID_ANY,_T(""));
	sizer->Add(Subframe_Box,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(TmpText4,wxSizerFlags().Proportion(1).Border(wxALL,3));

	Log_Box = new wxCheckBox(this,wxID_ANY,_T("Log info"),wxPoint(-1,-1),wxSize(-1,-1));
	Log_Box->SetToolTip(_T("Save guide commands and info to a file?"));
	Log_Box->Enable(true);
	wxStaticText *TmpText2 = new wxStaticText(this,wxID_ANY,_T(""));
	sizer->Add(Log_Box,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(TmpText2,wxSizerFlags().Proportion(1).Border(wxALL,3));

	RADither_Box = new wxCheckBox(this,wxID_ANY,_T("RA-only dither"),wxPoint(-1,-1),wxSize(-1,-1));
	RADither_Box->SetToolTip(_T("Constrain dither to RA only?"));
	RADither_Box->Enable(true);
	//wxStaticText *TmpText4 = new wxStaticText(this,wxID_ANY,_T(""));
	sizer->Add(RADither_Box,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->AddStretchSpacer(); //(TmpText2,wxSizerFlags().Proportion(1).Border(wxALL,3));

	Disable_Box = new wxCheckBox(this,wxID_ANY,_T("Disable guide output"),wxPoint(-1,-1),wxSize(-1,-1));
	Disable_Box->SetToolTip(_T("Don't actually send guide commands, just log"));
	Disable_Box->Enable(true);
	wxStaticText *TmpText3 = new wxStaticText(this,wxID_ANY,_T(""));
	sizer->Add(Disable_Box,wxSizerFlags().Proportion(2).Expand().Border(wxALL,3));
	sizer->Add(TmpText3,wxSizerFlags().Proportion(1).Border(wxALL,3));

	wxBoxSizer *sizer2 = new wxBoxSizer(wxVERTICAL);
	sizer2->Add(sizer);
	wxSizer *button_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	sizer2->Add(button_sizer,wxSizerFlags().Center().Border(wxALL,8));

/*	OK_Button = new wxButton(this, wxID_OK, _T("&Done"),wxPoint(15,290),wxSize(-1,-1));
	Cancel_Button = new wxButton(this, wxID_CANCEL, _T("&Cancel"),wxPoint(110,290),wxSize(-1,-1));

	sizer->Add(OK_Button,wxSizerFlags().Expand().Border(wxALL,6));
	sizer->AddStretchSpacer(1);
	sizer->Add(Cancel_Button,wxSizerFlags().Border(wxALL,6));
*/
	SetSizer(sizer2);
	sizer2->SetSizeHints(this);
 }

BEGIN_EVENT_TABLE(AdvancedDialog, wxDialog)
	EVT_BUTTON(wxID_PROPERTIES,AdvancedDialog::OnSetupCamera)
END_EVENT_TABLE()

void AdvancedDialog::OnSetupCamera(wxCommandEvent& WXUNUSED(event)) {
	// Prior to this we check to make sure the current camera is a WDM camera (main dialog) but...

	if (CaptureActive || !GuideCameraConnected || !CurrentGuideCamera->HasPropertyDialog) return;  // One more safety check
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

	dlog->RA_Aggr_Ctrl->SetValue((int) (RA_aggr * 100.0));
//	dlog->Dec_Aggr_Ctrl->SetValue((int) (Dec_aggr * 100.0));
	dlog->RA_Hyst_Ctrl->SetValue((int) (RA_hysteresis * 100.0));
	//dlog->UseDec_Box->SetValue(Dec_guide);
	dlog->Cal_Dur_Ctrl->SetValue(Cal_duration);
	dlog->MinMotion_Ctrl->SetValue(wxString::Format(_T("%.2f"),MinMotion));
	dlog->MassDelta_Ctrl->SetValue(wxString::Format(_T("%.2f"),StarMassChangeRejectThreshold));
	dlog->DecSlopeWeight_Ctrl->SetValue(wxString::Format(_T("%.2f"),Dec_slopeweight));
	dlog->SearchRegion_Ctrl->SetValue(SearchRegion);
	dlog->Time_Lapse_Ctrl->SetValue(Time_lapse);
	dlog->Gain_Ctrl->SetValue(GuideCameraGain);
	dlog->Log_Box->SetValue(Log_Data);
	dlog->RADither_Box->SetValue(DitherRAOnly);
	dlog->Disable_Box->SetValue(DisableGuideOutput);
	dlog->Dec_Mode->SetSelection(Dec_guide);
	dlog->MaxDecDur_Ctrl->SetValue(Max_Dec_Dur);
	dlog->MaxRADur_Ctrl->SetValue(Max_RA_Dur);
	dlog->Dec_AlgoCtrl->SetSelection(Dec_algo);
	dlog->NR_Ctrl->SetSelection(NR_mode);
	if (Calibrated) dlog->Cal_Box->SetValue(false);
	else dlog->Cal_Box->SetValue(true);
	dlog->Subframe_Box->SetValue(UseSubframes);

	// Turn off things that vary by camera by default
	dlog->Gain_Ctrl->Enable(false);
//	dlog->Setup_Button->Enable(false);

//	if ((CurrentGuideCamera == &Camera_WDM) || (CurrentGuideCamera == &Camera_VFW)){
	if (GuideCameraConnected) { // Take care of specifics based on current camera
		if (CurrentGuideCamera->HasGainControl)
			dlog->Gain_Ctrl->Enable(true);
//		if (CurrentGuideCamera->HasPropertyDialog)
//			dlog->Setup_Button->Enable(true);

		if (CurrentGuideCamera->HasDelayParam) {
			dlog->Delay_Ctrl->Enable(true);
			dlog->Delay_Ctrl->SetValue(CurrentGuideCamera->Delay);
	//		dlog->Delay_Text->Enable(true);
		}
	//	if (CurrentGuideCamera->HasPortNum) {
#if defined (LE_PARALLEL_CAMERA)
			switch (Camera_LEwebcamParallel.Port) {
				case 0x3BC:
					dlog->Port_Choice->SetSelection(1);
					break;
				case 0x278:
					dlog->Port_Choice->SetSelection(2);
					break;
				case 1:  // COM1
					dlog->Port_Choice->SetSelection(3);
					break;
				case 2:  // COM2
					dlog->Port_Choice->SetSelection(4);
					break;
				case 3:  // COM3
					dlog->Port_Choice->SetSelection(5);
					break;
				case 4:  // COM4
					dlog->Port_Choice->SetSelection(6);
					break;
				default:
					dlog->Port_Choice->SetSelection(0);
					break;
			}
#endif
		//	dlog->Port_Choice->Enable(true);
	//	}
	}
//	dlog->Dec_Backlash_Ctrl->SetValue((int) Dec_backlash);

//	dlog->UseDec_Box->Enable(false);
//	dlog->Dec_Aggr_Ctrl->Enable(false);
//	dlog->Dec_Backlash_Ctrl->Enable(false);

	if (dlog->ShowModal() != wxID_OK)  // Decided to cancel
		return;
	if (dlog->Cal_Box->GetValue()) Calibrated=false; // clear calibration
	if (!Dec_guide && dlog->Dec_Mode->GetSelection()) Calibrated = false; // added dec guiding -- recal
	if (!Calibrated) SetStatusText(_T("No cal"),5);

	RA_aggr = (double) dlog->RA_Aggr_Ctrl->GetValue() / 100.0;
//	Dec_aggr = (double) dlog->Dec_Aggr_Ctrl->GetValue() / 100.0;
	RA_hysteresis = (double) dlog->RA_Hyst_Ctrl->GetValue() / 100.0;
	Cal_duration = dlog->Cal_Dur_Ctrl->GetValue();
	SearchRegion = dlog->SearchRegion_Ctrl->GetValue();
	dlog->MinMotion_Ctrl->GetValue().ToDouble(&MinMotion);
	if (MinMotion < 0.001) MinMotion = 0.0;
	dlog->MassDelta_Ctrl->GetValue().ToDouble(&StarMassChangeRejectThreshold);
	if (StarMassChangeRejectThreshold < 0.1) StarMassChangeRejectThreshold = 0.1;
	else if (StarMassChangeRejectThreshold > 1.0) StarMassChangeRejectThreshold = 1.0;
	Dec_guide = dlog->Dec_Mode->GetSelection();
	Dec_algo = dlog->Dec_AlgoCtrl->GetSelection();
	dlog->DecSlopeWeight_Ctrl->GetValue().ToDouble(&Dec_slopeweight);
	Max_Dec_Dur = dlog->MaxDecDur_Ctrl->GetValue();
	Max_RA_Dur = dlog->MaxRADur_Ctrl->GetValue();
	Time_lapse = dlog->Time_Lapse_Ctrl->GetValue();
	GuideCameraGain = dlog->Gain_Ctrl->GetValue();
	NR_mode = dlog->NR_Ctrl->GetSelection();
	Log_Data = dlog->Log_Box->GetValue();
	DitherRAOnly = dlog->RADither_Box->GetValue();
	DisableGuideOutput = dlog->Disable_Box->GetValue();
	UseSubframes = dlog->Subframe_Box->GetValue();
	if (CurrentGuideCamera && CurrentGuideCamera->HasPortNum) {
		switch (dlog->Port_Choice->GetSelection()) {
			case 0:
				CurrentGuideCamera->Port = 0x378; break;
			case 1:
				CurrentGuideCamera->Port = 0x3BC; break;
			case 2:
				CurrentGuideCamera->Port = 0x278; break;
			case 3: CurrentGuideCamera->Port = 1; break;
			case 4: CurrentGuideCamera->Port = 2; break;
			case 5: CurrentGuideCamera->Port = 3; break;
			case 6: CurrentGuideCamera->Port = 4; break;
		}
	}
	if (CurrentGuideCamera && CurrentGuideCamera->HasDelayParam) {
		CurrentGuideCamera->Delay = dlog->Delay_Ctrl->GetValue();
	}

	frame->GraphLog->RAA_Ctrl->SetValue((int) (RA_aggr * 100));
	frame->GraphLog->RAH_Ctrl->SetValue((int) (RA_hysteresis * 100));
//Geoff	frame->GraphLog->MM_Ctrl->SetValue(MinMotion);
	frame->GraphLog->MDD_Ctrl->SetValue(Max_Dec_Dur);
	frame->GraphLog->DM_Ctrl->SetSelection(Dec_guide);
	
	//Dec_backlash = (double) dlog->Dec_Backlash_Ctrl->GetValue();
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
//	if ((frame->canvas->State > STATE_SELECTED) || !(ScopeConnected)) return;
	if (!(ScopeConnected)) return;
	switch (evt.GetId()) {
		case MGUIDE_N:
			GuideScope(NORTH,Cal_duration);
			break;
		case MGUIDE_S:
			GuideScope(SOUTH,Cal_duration);
			break;
		case MGUIDE_E:
			GuideScope(EAST,Cal_duration);
			break;
		case MGUIDE_W:
			GuideScope(WEST,Cal_duration);
			break;
	}

}

void MyFrame::OnTestGuide(wxCommandEvent& WXUNUSED(evt)) {
	if ((frame->canvas->State > STATE_SELECTED) || !(ScopeConnected)) return;
	TestGuideDialog* dlog = new TestGuideDialog();
	dlog->Show();
}
