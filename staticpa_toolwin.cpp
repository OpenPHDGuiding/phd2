/*
 *  staticpa_toolwin.cpp
 *  PHD Guiding
 *
 *  Created by Ken Self
 *  Copyright (c) 2017 Ken Self
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
#include "staticpa_tool.h"
#include "staticpa_toolwin.h"

#include <wx/gbsizer.h>
#include <wx/valnum.h>
#include <wx/textwrapper.h>

static const std::string  SthStarName[] = {
"A: sigma Oct", "B: HD99828", "C: HD125371", "D: HD90105",
"E: BQ Oct", "F: HD99685", "G: TYC9518-405-1", "H: unnamed"
};
static const double SthStarRa[] = { 320.66, 164.22, 248.88, 122.36, 239.62, 130.32, 102.04, 136.63 };
static const double SthStarDec[] = { -88.89, -89.33, -89.38, -89.52, -89.83, -89.85, -89.88, -89.42 };
static const double SthStarMag[] = { 4.3, 7.5, 7.8,  7.2, 6.8, 7.8, 8.75, 8.0 };
static const int SthStarCount = 8;
static const std::string NthStarName[] = {
	"A: Polaris", "B: HD1687", "C: TYC4629-37-1", "D: TYC4661-2-1", "E: unnamed", "F: unnamed"
};
static const double NthStarRa[] = { 43.12, 12.14, 85.51, 297.95, 86.11, 358.33 };
static const double NthStarDec[] = { 89.34, 89.54, 89.65, 89.83, 89.43, 89.54 };
static const double NthStarMag[] = { 1.95, 8.1, 9.15, 9.65, 9.25, 9.35 };
static const int NthStarCount = 6;

//==================================
BEGIN_EVENT_TABLE(StaticPaToolWin, wxFrame)
//EVT_BUTTON(ID_SLEW, StaticPaToolWin::OnSlew)
EVT_BUTTON(ID_ROTATE, StaticPaToolWin::OnRotate)
EVT_BUTTON(ID_ADJUST, StaticPaToolWin::OnAdjust)
EVT_BUTTON(ID_CLOSE, StaticPaToolWin::OnCloseBtn)
EVT_CHOICE(ID_ALIGNSTAR, StaticPaToolWin::OnAlignStar)
EVT_CHOICE(ID_HEMI, StaticPaToolWin::OnHemi)
EVT_CHECKBOX(ID_MANUAL, StaticPaToolWin::OnManual)
EVT_BUTTON(ID_STAR2, StaticPaToolWin::OnStar2)
EVT_BUTTON(ID_STAR3, StaticPaToolWin::OnStar3)
EVT_CLOSE(StaticPaToolWin::OnClose)
END_EVENT_TABLE()

StaticPaToolWin::StaticPaToolWin()
: wxFrame(pFrame, wxID_ANY, _("Static Polar Alignment"), wxDefaultPosition, wxDefaultSize,
wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxTAB_TRAVERSAL | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR),
m_slewing(false), m_devpx(8)
{
	pFrame->pGuider->pStaticPaTool = this;
	m_numPos = 0;

/*	SthStars = {
		Star("A: sigma Oct", 320.66, -88.89, 4.3),
		Star("B: HD99828", 164.22, -89.33, 7.5),
		Star("C: HD125371", 248.88, -89.38, 7.8),
		Star("D: HD90105", 122.36, -89.52, 7.2),
		Star("E: BQ Oct", 239.62, -89.83, 6.8),
		Star("F: HD99685", 130.32, -89.85, 7.8),
		Star("G: TYC9518-405-1", 102.04, -89.88, 8.75),
		Star("H: unnamed", 136.63, -89.42, 8.0)
	};
	NthStars = {
		Star("A: Polaris", 43.12, 89.34, 1.95),
		Star("B: HD1687", 12.14, 89.54, 8.1),
		Star("C: TYC4629-37-1", 85.51, 89.65, 9.15),
		Star("D: TYC4661-2-1", 297.95, 89.83, 9.65),
		Star("E: unnamed", 86.11, 89.43, 9.25),
		Star("F: unnamed", 358.33, 89.54, 9.35)
	};
*/
	// get site lat/long from scope
	s_hemi = 1;
	if (pPointingSource)
	{
		double lat, lon;
		if (!pPointingSource->GetSiteLatLong(&lat, &lon))
		{
			s_hemi = lat >= 0 ? 1 : -1;
		}
	}
	poleStars = s_hemi >= 0 ? &NthStars : &SthStars;
	m_pxScale = pFrame->GetCameraPixelScale();
	wxCommandEvent dummy;
	if (!pFrame->CaptureActive)
	{
		// loop exposures
		SetStatusText(_("Start Looping..."));
		pFrame->OnLoopExposure(dummy);
	}
	// Drop down list box for alignment star
//	m_nstar = (s_hemi < 0) ? SthStarCount : NthStarCount;
	m_nstar = poleStars->size();

	SetBackgroundColour(wxColor(0xcccccc));
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	// a vertical sizer holding everything
	wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

	// a horizontal box sizer for the bitmap and the instructions
	wxBoxSizer *instrSizer = new wxBoxSizer(wxHORIZONTAL);

	m_instructions = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(400, 120), wxALIGN_LEFT | wxST_NO_AUTORESIZE);
#ifdef __WXOSX__
	m_instructions->SetFont(*wxSMALL_FONT);
#endif
	m_instructions->Wrap(-1);
	instrSizer->Add(m_instructions, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);
	// Put the graph and its legend on the left side
	//	wxBoxSizer* panelGraphVSizer = new wxBoxSizer(wxVERTICAL);
	//	panelHSizer->Add(panelGraphVSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	// Use a bitmap button so we don't have to fool with Paint events
	wxBitmap theGraph = CreateStarTemplate();
	wxBitmapButton* graphButton = new wxBitmapButton(this, wxID_ANY, theGraph, wxDefaultPosition, wxSize(320, 240), wxBU_AUTODRAW | wxBU_EXACTFIT);
	instrSizer->Add(graphButton, 0, wxALIGN_CENTER_HORIZONTAL | wxALL | wxFIXED_MINSIZE, 5);
	graphButton->SetBitmapDisabled(theGraph);
	graphButton->Enable(false);

	topSizer->Add(instrSizer);

	// static box sizer holding the scope pointing controls
	wxStaticBoxSizer *sbSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Scope Pointing")), wxVERTICAL);

	// a grid box sizer for laying out scope pointing the controls
	wxGridBagSizer *gbSizer = new wxGridBagSizer(0, 0);
	gbSizer->SetFlexibleDirection(wxBOTH);
	gbSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	wxStaticText *txt;

	txt = new wxStaticText(this, wxID_ANY, _("px Scale"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Camera Rot"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Hemisphere"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 5);

	// Alignment star specification
	txt = new wxStaticText(this, wxID_ANY, _("Alignment Star"));
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 3), wxGBSpan(1, 1), wxALL, 5);

/*	txt = new wxStaticText(this, wxID_ANY, _("Right Ascen (" DEGREES_SYMBOL ")"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Declination (" DEGREES_SYMBOL ")"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 5);
*/
/*	m_raCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_raCurrent, wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_decCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_decCurrent, wxGBPosition(1, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);
*/
/*	txt = new wxStaticText(this, wxID_ANY, _("Lon"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(2, 2), wxGBSpan(1, 1), wxALL, 5);
*/
	m_camScale = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_camScale, wxGBPosition(1, 0), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);
//	m_siteLat = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
//	gbSizer->Add(m_siteLat, wxGBPosition(3, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

//	m_siteLon = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
//	gbSizer->Add(m_siteLon, wxGBPosition(3, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_camRot = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_camRot, wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	wxArrayString hemi;
	hemi.Add(_("North"));
	hemi.Add(_("South"));
	hemiChoice = new wxChoice(this, ID_HEMI, wxDefaultPosition, wxDefaultSize, hemi);
	hemiChoice->SetToolTip(_("Select your hemisphere"));
	gbSizer->Add(hemiChoice, wxGBPosition(1, 2), wxGBSpan(1, 1), wxALL, 5);

	alignStarChoice = new wxChoice(this, ID_ALIGNSTAR, wxDefaultPosition, wxDefaultSize);
	alignStarChoice->SetToolTip(_("Select the star used for checking alignment."));
	gbSizer->Add(alignStarChoice, wxGBPosition(1, 3), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("X px"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(4, 1), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Y px"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(4, 2), wxGBSpan(1, 1), wxALL, 5);

	m_manual = new wxCheckBox(this, ID_MANUAL, _("Manual Slew"));
	gbSizer->Add(m_manual, wxGBPosition(4, 3), wxGBSpan(1, 1), wxALL, 5);
	m_manual->SetValue(false);
	m_manual->SetToolTip(_("Manually slew the mount to three alignment positions"));

	txt = new wxStaticText(this, wxID_ANY, _("Pt #1"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(5, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[0][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[0][0], wxGBPosition(5, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[0][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[0][1], wxGBPosition(5, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_star1 = new wxButton(this, ID_ROTATE, _("Rotate"), wxDefaultPosition, wxDefaultSize, 0);
	gbSizer->Add(m_star1, wxGBPosition(5, 3), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Pt #2"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(6, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[1][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[1][0], wxGBPosition(6, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[1][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[1][1], wxGBPosition(6, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_star2 = new wxButton(this, ID_STAR2, _("Get second position"), wxDefaultPosition, wxDefaultSize, 0);
	gbSizer->Add(m_star2, wxGBPosition(6, 3), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Pt #3"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(7, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[2][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[2][0], wxGBPosition(7, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[2][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[2][1], wxGBPosition(7, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_star3 = new wxButton(this, ID_STAR3, _("Get third position"), wxDefaultPosition, wxDefaultSize, 0);
	gbSizer->Add(m_star3, wxGBPosition(7, 3), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Centre"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(8, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[3][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[3][0], wxGBPosition(8, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[3][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[3][1], wxGBPosition(8, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_adjust = new wxButton(this, ID_ADJUST, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0);
	gbSizer->Add(m_adjust, wxGBPosition(8, 3), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	// add grid bag sizer to static sizer
	sbSizer->Add(gbSizer, 1, wxALIGN_CENTER, 5);

	// add static sizer to top-level sizer
	topSizer->Add(sbSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	// add some padding below the static sizer
	topSizer->Add(0, 3, 0, wxEXPAND, 3);

	m_notesLabel = new wxStaticText(this, wxID_ANY, _("Adjustment notes"), wxDefaultPosition, wxDefaultSize, 0);
	m_notesLabel->Wrap(-1);
	topSizer->Add(m_notesLabel, 0, wxEXPAND | wxTOP | wxLEFT, 8);

	m_notes = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 54), wxTE_MULTILINE);
	pFrame->RegisterTextCtrl(m_notes);
	topSizer->Add(m_notes, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
	//m_notes->Bind(wxEVT_COMMAND_TEXT_UPDATED, &StaticPaToolWin::OnNotesText, this);

	// horizontal sizer for the buttons
	wxBoxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);

	// proportional pad on left of Rotate button
	hSizer->Add(0, 0, 2, wxEXPAND, 5);

	// proportional pad on right of Rotate button
	hSizer->Add(0, 0, 1, wxEXPAND, 5);

	// proportional pad on right of Align button
	hSizer->Add(0, 0, 2, wxEXPAND, 5);

	m_close = new wxButton(this, ID_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0);
	hSizer->Add(m_close, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	// add button sizer to top level sizer
	topSizer->Add(hSizer, 1, wxEXPAND | wxALL, 5);

	SetSizer(topSizer);

	m_statusBar = CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);

	Layout();
	topSizer->Fit(this);

	int xpos = pConfig->Global.GetInt("/StaticPaTool/pos.x", -1);
	int ypos = pConfig->Global.GetInt("/StaticPaTool/pos.y", -1);
	MyFrame::PlaceWindowOnScreen(this, xpos, ypos);

	m_instructions->SetLabel(
		_("Slew to near the Celestial Pole.\n"
		"Choose an Alignment Star from the list.\n"
		"Select it as the guide star on the display.\n"
		"Press Rotate to calibrate the RA Axis.\n"
		"Wait for both calibration points to be measured.\n"
		"Adjust your mount's altitude and azimuth as displayed.\n"
		"Orange=Altitude; Green=Azimuth\n"
		));

	// can mount slew?
	bauto = true;
	bool m_can_slew = pPointingSource && pPointingSource->CanSlew();
	if (!m_can_slew){
		bauto = false;
		m_manual->Hide();
	}
	SetButtons();

	alignStarChoice->Select(pConfig->Profile.GetInt("/StaticPaTool/AlignStar", 4) - 1);
	UpdateAlignStar();

//	m_siteLat->SetValue(wxString::Format("%+.3f", lat));
//	m_siteLon->SetValue(wxString::Format("%+.3f", lon));

	double xangle=0.0, yangle;
	if (pMount && pMount->IsConnected() && pMount->IsCalibrated())
	{
		xangle = degrees(pMount->xAngle());
		yangle = degrees(pMount->yAngle());
	}
	m_camRot->SetValue(wxString::Format("%+.3f", xangle));
	m_dCamRot = xangle;

	m_mode = MODE_IDLE;
	UpdateModeState();
}

StaticPaToolWin::~StaticPaToolWin()
{
	pFrame->pGuider->pStaticPaTool = NULL;
	pFrame->pStaticPaTool = NULL;
}
void StaticPaToolWin::UpdateModeState()
{
	wxCommandEvent dummy;
	wxString idleStatus;

repeat:

		if (m_mode == MODE_ROTATE)
	{
		//m_rotate->Enable(false);
//		m_rotate->Enable(true);
		m_adjust->Enable(false);
//		EnableSlew(false);

//		if (!m_rotating)
		{
			if (!pCamera->Connected ||
				!pMount || !pMount->IsConnected())
			{
				idleStatus = _("Please connect a camera and a mount");
				m_mode = MODE_IDLE;
				goto repeat;
			}

			if (!pMount->IsCalibrated())
			{
				idleStatus = _("Please calibrate before starting static alignment");
				m_mode = MODE_IDLE;
				goto repeat;
			}

			if (!pFrame->CaptureActive)
			{
				// loop exposures
				SetStatusText(_("Start Looping..."));
				pFrame->OnLoopExposure(dummy);
				return;
			}
		}
	}
	else if (m_mode == MODE_ADJUST)
	{
//		m_rotate->Enable(true);
		m_adjust->Enable(true);
//		m_rotating = false;
//		EnableSlew(m_can_slew);
		SetStatusText(_("Adjust altitude and azimuth, click Rotate when done") );

		// use full frames for adjust phase
		pCamera->UseSubframes = false;

		if (pFrame->pGuider->IsGuiding())
		{
			// stop guiding but continue looping
			pFrame->OnLoopExposure(dummy);

			// Set the lock position to the where the star has rotateed to. This will be the center of the polar align circle.
			pFrame->pGuider->SetLockPosition(pFrame->pGuider->CurrentPosition());
			pFrame->pGraphLog->Refresh();  // polar align circle is updated in graph window"s OnPaint handler
		}
	}
	else // MODE_IDLE
	{
//		m_rotate->Enable(true);
		m_adjust->Enable(false);
//		m_rotating = false;
	}
}
void StaticPaToolWin::UpdateAlignStar()
{
	int i_alignStar = alignStarChoice->GetSelection();
	pConfig->Profile.SetInt("/StaticPaTool/AlignStar", alignStarChoice->GetSelection() + 1);

//	double ra_deg = s_hemi < 0 ? SthStarRa[i_alignStar] : NthStarRa[i_alignStar];
//	double dec_deg = s_hemi < 0 ? SthStarDec[i_alignStar] : NthStarDec[i_alignStar];

	m_alignStar = i_alignStar;
	m_alignStar = i_alignStar;

//	m_raCurrent->SetValue(wxString::Format("%+.2f", ra_deg));
//	m_decCurrent->SetValue(wxString::Format("%+.2f", dec_deg));

}

void StaticPaToolWin::OnHemi(wxCommandEvent& evt)
{
	int i_hemi = hemiChoice->GetSelection() <= 0 ? 1 : -1;
	if (i_hemi != s_hemi)
	{
		m_alignStar = 0;
		s_hemi = i_hemi;
//		m_nstar = s_hemi >= 0 ? NthStarCount : SthStarCount;
		m_nstar = poleStars->size();
	}
	SetButtons();
}
void StaticPaToolWin::OnManual(wxCommandEvent& evt)
{
	bauto = !m_manual->IsChecked();
	SetButtons();
}
void StaticPaToolWin::SetButtons()
{
	m_manual->SetValue(!bauto);

	m_star1->SetLabel(_("Rotate"));
	m_star2->Hide();
	m_star3->Hide();
	m_adjust->Hide();
	hemiChoice->Enable(false);
	hemiChoice->SetSelection(s_hemi > 0 ? 0 : 1);
	if (!bauto)
	{
		m_star1->SetLabel(_("Get first position"));
		m_star2->Show();
		m_star3->Show();
		m_adjust->Show();
		hemiChoice->Enable(true);
	}
//	wxArrayString choices;
	alignStarChoice->Clear();
	std::string starname;
	for (int is = 0; is < m_nstar; is++) {
//		starname = (s_hemi < 0) ? SthStarName[is] : NthStarName[is];
//		alignStarChoice->AppendString(starname);
		alignStarChoice->AppendString(poleStars->at(is).name);
		//		choices.Add(_(starname));
	}
	m_camScale->SetValue(wxString::Format("%+.3f", m_pxScale));
	m_camRot->SetValue(wxString::Format("%+.3f", m_dCamRot));
	Layout();
}

void StaticPaToolWin::OnStar2(wxCommandEvent& evt)
{
	m_numPos = 2;
	aligning = true;
	//	bauto = !m_manual->IsChecked();
}

void StaticPaToolWin::OnStar3(wxCommandEvent& evt)
{
	m_numPos = 3;
	aligning = true;
	//	bauto = !m_manual->IsChecked();
}

void StaticPaToolWin::OnRotate(wxCommandEvent& evt)
{
	UpdateModeState();
	aligning = false;
	m_numPos = 1;

	if (pFrame->pGuider->IsCalibratingOrGuiding() )
	{
		SetStatusText(_("Please wait till Calibration is done and/or stop guiding"));
		return;
	}
	if (!pFrame->pGuider->IsLocked())
	{
		SetStatusText(_("Please select a star"));
		return;
	}
	aligning = true;

	m_mode = MODE_ROTATE;
	UpdateModeState();
}

void StaticPaToolWin::OnAdjust(wxCommandEvent& evt)
{
	m_numPos = 3;
	CalcRotationCentre();
	m_calPt[3][0]->SetValue(wxString::Format("%+.f", m_CoRpx.X));
	m_calPt[3][1]->SetValue(wxString::Format("%+.f", m_CoRpx.Y));

	m_mode = MODE_ADJUST;
	UpdateModeState();
}

void StaticPaToolWin::OnAlignStar(wxCommandEvent& evt)
{
	UpdateAlignStar();
}

void StaticPaToolWin::OnCloseBtn(wxCommandEvent& evt)
{
	Debug.AddLine("Close StaticPaTool");
	if (IsAligning())
	{
		aligning = false;
	}
	Destroy();
}

void StaticPaToolWin::OnClose(wxCloseEvent& evt)
{
	Debug.AddLine("Close StaticPaTool");
	Destroy();
}

void StaticPaToolWin::CalcRotationCentre(void)
{
	double x1, y1, x2, y2, x3, y3;
	double cx, cy, cr;
	x1 = m_Pospx[0].X;
	y1 = m_Pospx[0].Y;
	x2 = m_Pospx[1].X;
	y2 = m_Pospx[1].Y;
	if (!bauto)
	{
		SetStatusText(wxString::Format("Manual CalcCoR: (%.1f,%.1f); (%.1f,%.1f)", x1, y1, x2, y2));
		Debug.AddLine(wxString::Format("Manual CalcCoR: (%.1f,%.1f); (%.1f,%.1f)", x1, y1, x2, y2));
		double a, b, c, e, f, g, i, j, k;
		double m11, m12, m13, m14;
		x3 = m_Pospx[2].X;
		y3 = m_Pospx[2].Y;
		// |A| = aei + bfg + cdh -ceg -bdi -afh
		// a b c
		// d e f
		// g h i
		// a= x1^2+y1^2; b=x1; c=y1; d=1
		// e= x2^2+y2^2; f=x2; g=y2; h=1
		// i= x3^2+y3^2; j=x3; k=y3; l=1
		//
		// x0 = 1/2.|M12|/|M11|
		// y0 = -1/2.|M13|/|M11|
		// r = x0^2 + y0^2 + |M14| / |M11|
		//
		a = x1 * x1 + y1 * y1;
		b = x1;
		c = y1;
		e = x2 * x2 + y2 * y2;
		f = x2;
		g = y2;
		i = x3 * x3 + y3 * y3;
		j = x3;
		k = y3;
		m11 = b * g + c * j + f * k - g * j - c * f - b * k;
		m12 = a * g + c * i + e * k - g * i - c * e - a * k;
		m13 = a * f + b * i + e * j - f * i - b * e - a * j;
		m14 = a * f * k + b * g * i + c * e * j - c * f * i - b * e * k - a * g * j;
		cx = (1. / 2.) * m12 / m11;
		cy = (-1. / 2.) * m13 / m11;
		cr = sqrt(cx * cx + cy *cy + m14 / m11);
	}
	else
	{
		SetStatusText(wxString::Format("SPA CalcCoR: (%.1f,%.1f); (%.1f,%.1f)", x1, y1, x2, y2));
		Debug.AddLine(wxString::Format("SPA CalcCoR: (%.1f,%.1f); (%.1f,%.1f)", x1, y1, x2, y2));
		// Alternative algorithm based on two poins and angle rotated
		double theta2;
//		theta2 = copysign(1.0, -m_siteLatLong.X) * radians(360.0 / 24.0 * (m_ra_rot[1] - m_ra_rot[0])) / 2.0;
		theta2 = -s_hemi * radians(360.0 / 24.0 * (m_ra_rot[1] - m_ra_rot[0])) / 2.0;
		double lenchord = sqrt(pow((x1 - x2), 2.0) + pow((y1 - y2), 2.0));
		cr = lenchord / 2.0 / sin(theta2);
		double lenbase = cr*cos(theta2);
		double slopebase = atan2(y1 - y2, x2 - x1) + M_PI / 2.0;
		cx = (x1 + x2) / 2.0 + lenbase * cos(slopebase);
		cy = (y1 + y2) / 2.0 - lenbase * sin(slopebase); // subtract for pixels
	}
	m_CoRpx.X = cx; // cx;
	m_CoRpx.Y = cy; // cy;
	m_Radius = cr; // cr;
	m_calPt[3][0]->SetValue(wxString::Format("%+.f", m_CoRpx.X));
	m_calPt[3][1]->SetValue(wxString::Format("%+.f", m_CoRpx.Y));

	usImage *pCurrImg = pFrame->pGuider->CurrentImage();
	wxImage *pDispImg = pFrame->pGuider->DisplayedImage();
	double scalefactor = pFrame->pGuider->ScaleFactor();
	int xpx = pDispImg->GetWidth() / scalefactor;
	int ypx = pDispImg->GetHeight() / scalefactor;
	m_dispSz[0] = xpx;
	m_dispSz[1] = ypx;

// Diatance and angle of CoR from centre of sensor
	double cor_r = sqrt(pow(xpx / 2 - cx, 2) + pow(ypx / 2 - cy, 2));
	double cor_a = degrees(atan2((ypx / 2 - cy), (xpx / 2 - cx)));
	double rarot = -m_dCamRot;
// Cone and Dec components of CoR vector
	double dec_r = cor_r * sin(radians(cor_a - rarot));
	m_DecCorr.X = -dec_r * sin(radians(rarot));
	m_DecCorr.Y = dec_r * cos(radians(rarot));
	double cone_r = cor_r * cos(radians(cor_a - rarot));
	m_ConeCorr.X = cone_r * cos(radians(rarot));
	m_ConeCorr.Y = cone_r * sin(radians(rarot));
	double decCorr_deg = dec_r * m_pxScale / 3600;

// Caclulate pixel values for the alignment stars
	PHD_Point starpx, stardeg;
	for (int is = 0; is < m_nstar; is++)
	{
//		stardeg = (m_siteLatLong.Y < 0) ? PHD_Point(SthStarRa[is], SthStarDec[is]) : PHD_Point(NthStarRa[is], NthStarDec[is]);
//		stardeg = (s_hemi < 0) ? PHD_Point(SthStarRa[is], SthStarDec[is]) : PHD_Point(NthStarRa[is], NthStarDec[is]);
		stardeg = PHD_Point(poleStars->at(is).ra, poleStars->at(is).dec);
		starpx = Radec2Px(stardeg);
		m_starpx[is][0] = starpx.X;
		m_starpx[is][1] = starpx.Y;
		m_starpx[is][2] = sqrt(pow(cx - starpx.X, 2) + pow(cy - starpx.Y, 2));
	}
	int idx = bauto ? 1 : 2;
	double xs = m_Pospx[idx].X;
	double ys = m_Pospx[idx].Y;
	double xt = m_starpx[m_alignStar][0];
	double yt = m_starpx[m_alignStar][1];

	//	// Calculate the camera rotation from the Azimuth axis
	//	// Alt angle aligns to HA=0, Azimuth (East) to HA = -90
	//	// In home position Az aligns with Dec
	//	// So at HA +/-90 (home pos) Alt rotation is 0 (HA+90)
	//	// At the meridian, HA=0 Alt aligns with dec so Rotation is +/-90
	//	// Let harot =  camera rotation from Alt axis
	//	// Alt axis is at HA+90
	//	// This is camera rotation from RA minus(?) LST angle 
	double	hcor_r = sqrt(pow(xt - xs, 2) + pow(yt - ys, 2));
	double	hcor_a = degrees(atan2((yt - ys), (xt - xs)));
	double ra_hrs, dec_deg, st_hrs;
	pPointingSource->GetCoordinates(&ra_hrs, &dec_deg, &st_hrs);
	double harot = rarot - (90 + (st_hrs - ra_hrs)*15.0);
	double hrot = hcor_a - harot;

	double az_r = hcor_r * sin(radians(hrot));
	double alt_r = hcor_r * cos(radians(hrot));

	m_AzCorr.X = -az_r * sin(radians(harot));
	m_AzCorr.Y = az_r * cos(radians(harot));
	m_AltCorr.X = alt_r * cos(radians(harot));
	m_AltCorr.Y = alt_r * sin(radians(harot));

}

PHD_Point StaticPaToolWin::Radec2Px( PHD_Point radec )
{
// Convert dec to pixel radius
	double r = (90.0 - fabs(radec.Y)) * 3600 / m_pxScale;

// Rotate by calibration angle and HA f object taking into account mount rotation (HA)
	double ra_hrs, dec_deg, ra_deg, st_hrs;
	pPointingSource->GetCoordinates(&ra_hrs, &dec_deg, &st_hrs);
	ra_deg = ra_hrs * 15.0;
// Target hour angle - or rather the rotation needed to correct. 
// HA = LST - RA
// In NH HA decreases clockwise; RA increases clockwise
// "Up" is HA=0
// Sensor "up" is 90deg counterclockwise from mount RA plus rotation
// Star rotation is RAstar - RAmount
	double a = m_dCamRot - copysign(radec.X - (ra_deg - 90.0), dec_deg);

	PHD_Point px(m_CoRpx.X + r * cos(radians(a)), m_CoRpx.Y - r * sin(radians(a)));
	return px;
}

wxWindow *StaticPaTool::CreateStaticPaToolWindow()
{
	if (!pCamera || !pCamera->Connected)
	{
		wxMessageBox(_("Please connect a camera first."));
		return 0;
	}

	// confirm that image scale is specified

	if (pFrame->GetCameraPixelScale() == 1.0)
	{
		bool confirmed = ConfirmDialog::Confirm(_(
			"The Static Align tool is most effective when PHD2 knows your guide\n"
			"scope focal length and camera pixel size.\n"
			"\n"
			"Enter your guide scope focal length on the Global tab in the Brain.\n"
			"Enter your camera pixel size on the Camera tab in the Brain.\n"
			"\n"
			"Would you like to run the tool anyway?"),
			"/rotate_tool_without_pixscale");

		if (!confirmed)
		{
			return 0;
		}
	}
	if (pFrame->pGuider->IsCalibratingOrGuiding() )
	{
		wxMessageBox(_("Please wait till Calibration is done and stop guiding"));
		return 0;
	}

	return new StaticPaToolWin();
}

void StaticPaToolWin::PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale)
{
	dc.SetPen(wxPen(wxColour(0, 255, 255), 1, wxSOLID));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);

	for (int i = 0; i < m_numPos; i++)
	{
		dc.DrawCircle(m_Pospx[i].X*scale, m_Pospx[i].Y*scale, 12 * scale);
	}
	if (m_numPos <= 3)
	{
		return;
	}
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	wxPenStyle penStyle = wxPENSTYLE_DOT;
	dc.SetPen(wxPen(wxColor(255, 0, 255), 1, penStyle));
	dc.DrawCircle(m_CoRpx.X*scale, m_CoRpx.Y*scale, m_Radius*scale);

	// draw the centre of the circle as a red cross
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
	int region = 5;
	dc.DrawLine((m_CoRpx.X - region)*scale, m_CoRpx.Y*scale, (m_CoRpx.X + region)*scale, m_CoRpx.Y*scale);
	dc.DrawLine(m_CoRpx.X*scale, (m_CoRpx.Y - region)*scale, m_CoRpx.X*scale, (m_CoRpx.Y + region)*scale);

	// Show the centre of the display wth a grey cross
	double xsc = m_dispSz[0] / 2;
	double ysc = m_dispSz[1] / 2;
	dc.SetPen(wxPen(wxColor(127, 127, 127), 1, wxPENSTYLE_SOLID));
	dc.DrawLine((xsc - region * 4)*scale, ysc*scale, (xsc + region * 4)*scale, ysc*scale);
	dc.DrawLine(xsc*scale, (ysc - region * 4)*scale, xsc*scale, (ysc + region * 4)*scale);

	// Draw orbits for each alignment star
	for (int is = 0; is < m_nstar; is++)
	{
		dc.SetPen(wxPen(wxColor(255, 255, 0), 1, wxPENSTYLE_SOLID));
		if (is == m_alignStar)
		{
			dc.SetPen(wxPen(wxColor(0, 255, 0), 1, wxPENSTYLE_SOLID));
		}
		dc.DrawCircle(m_CoRpx.X * scale, m_CoRpx.Y * scale, m_starpx[is][2] * scale);
		dc.DrawCircle(m_starpx[is][0] * scale, m_starpx[is][1] * scale, region*scale);
	}
	// Draw adjustment lines for centring the CoR on the display in blue (dec) and red (cone error)
	double xr = m_CoRpx.X * scale;
	double yr = m_CoRpx.Y * scale;
	dc.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xr, yr, xr + m_ConeCorr.X * scale, yr + m_ConeCorr.Y * scale);
	dc.SetPen(wxPen(wxColor(0, 0, 255), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xr + m_ConeCorr.X * scale, yr + m_ConeCorr.Y * scale,
		xr + m_DecCorr.X * scale + m_ConeCorr.X * scale,
		yr + m_DecCorr.Y * scale + m_ConeCorr.Y * scale);
	dc.SetPen(wxPen(wxColor(127, 127, 127), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xr, yr, xr + m_DecCorr.X * scale + m_ConeCorr.X * scale,
		yr + m_DecCorr.Y * scale + m_ConeCorr.Y * scale);

	// Draw adjustment lines for placing the guide star in its correct position relative to the CoR
	// Green (azimuth) and orange (altitude)
	int idx;
	idx = bauto ? 1 : 2;
	double xs = m_Pospx[idx].X * scale;
	double ys = m_Pospx[idx].Y * scale;
	dc.SetPen(wxPen(wxColor(255, 165, 0), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xs, ys, xs + m_AltCorr.X * scale, ys + m_AltCorr.Y * scale);
	dc.SetPen(wxPen(wxColor(0, 255, 0), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xs + m_AltCorr.X * scale, ys + m_AltCorr.Y * scale,
		xs + m_AltCorr.X * scale + m_AzCorr.X * scale, ys + m_AzCorr.Y * scale + m_AltCorr.Y * scale);
	dc.SetPen(wxPen(wxColor(127, 127, 127), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xs, ys, xs + m_AltCorr.X * scale + m_AzCorr.X * scale,
		ys + m_AltCorr.Y * scale + m_AzCorr.Y * scale);
}

bool StaticPaToolWin::RotateMount() //const PHD_Point& position)
{
	// Initially, assume an offset of 5.0 degrees of the camera from the CoR
	// Calculate how far to move in RA to get a detectable arc
	// Calculate the tangential ditance of that movement
	// Mark the starting position then rotate the mount
//	bool bauto = true;
	if (m_numPos == 1)
	{
		SetStatusText(_("Polar align: star #1"));
		Debug.AddLine("Polar align: star #1");
		//   Initially offset is 5 degrees;
		bool rc = SetParams(5.0); // m_rotdg, m_rotpx, m_nstep for assumed 5.0 degree PA error
		Debug.AddLine(wxString::Format("Polar align: star #1 rotdg=%.1f rotpx= %.0f nstep=%d", m_rotdg, m_rotpx, m_nstep));
		bool isset = SetStar(m_numPos);
		if (isset) m_numPos++;
		tottheta = 0.0;
		nstep = 0;
		Debug.AddLine(wxString::Format("Leave Polar align: star #1 rotdg=%.1f rotpx= %.0f nstep=%d", m_rotdg, m_rotpx, m_nstep));
		return isset;
	}
	if (m_numPos == 2)
	{
		double theta = m_rotdg - tottheta;
		SetStatusText(_("Polar align: star #2"));
		Debug.AddLine("Polar align: star #2");
		// Once the mount has rotated theta degrees (as indicated by prevtheta);
		if (!bauto)
		{
			// rotate mount manually;
			bool isset = SetStar(m_numPos);
			if (isset) m_numPos++;
			return isset;
		}
		SetStatusText(wxString::Format("Polar align: star #2 nstep=%d / %d theta=%.1f / %.1f", nstep, m_nstep, tottheta, m_rotdg));
		Debug.AddLine(wxString::Format("Polar align: star #2 nstep=%d / %d theta=%.1f / %.1f", nstep, m_nstep, tottheta, m_rotdg));
		if (tottheta < m_rotdg)
		{
			double newtheta = theta / (m_nstep - nstep);
			MoveWestBy(newtheta); // , exposure);
			tottheta += newtheta;
			// Calclate how far the mount moved compared to the expected movement;
			// This assumes that the mount was rotated through theta degrees;
			// So theta is the total rotation needed for the current offset;
			// And prevtheta is how we have already moved;
			// Recalculate the offset based on the actual movement;
		}
		else
		{
			// Recalculate the offset. CAUTION: This might end up in an endless loop. 
			bool isset = SetStar(m_numPos);

			double actpix = sqrt(pow(m_Pospx[1].X - m_Pospx[0].X, 2) + pow(m_Pospx[1].Y - m_Pospx[0].Y, 2));
			double actsec = actpix * m_pxScale;
			double actoffsetdeg = 90 - degrees(acos(actsec / 3600 / m_rotdg));
			Debug.AddLine(wxString::Format("Polar align: star #2 px=%.1f asec=%.1f pxscale=%.1f", actpix, actsec, m_pxScale));

			if (actoffsetdeg == 0)
			{
				Debug.AddLine(wxString::Format("Polar align: star #2 Mount did not move actual offset =%.1f", actoffsetdeg));
				SetStatusText(wxString::Format("Polar align: star #2 Mount did not move actual offset =%.1f", actoffsetdeg));
				//self.settings["msg"] = "Mount did not move - check settings";
				PHD_Point cor = m_Pospx[m_numPos - 1];
				return false;
			}
			double prev_rotdg = m_rotdg;
			int prev_nstep = m_nstep;
			// FIXME - alter setparams to take into account rotation so far
			bool rc = SetParams(actoffsetdeg); // m_rotdg, m_rotpx, m_nstep for measures PA error
			if (!rc)
			{
				aligning = false;
				return false;
			}
			if (m_rotdg <= prev_rotdg) // Moved far enough: show the adjustment chart
			{
				if (!isset)
				{
					aligning = false;
					return false;
				}
				m_numPos++;
				nstep = 0;
				tottheta = 0.0;
				CalcRotationCentre();
				m_calPt[3][0]->SetValue(wxString::Format("%+.f", m_CoRpx.X));
				m_calPt[3][1]->SetValue(wxString::Format("%+.f", m_CoRpx.Y));
				m_mode = MODE_ADJUST;
				UpdateModeState();
			}
			else if (m_rotdg > 45)
			{
				Debug.AddLine(wxString::Format("Polar align: star #2 Too close to CoR offset =%.1f Rot=%.1f", actoffsetdeg, m_rotdg));
				SetStatusText(wxString::Format("Polar align: star #2 Too close to CoR offset =%.1f Rot=%.1f", actoffsetdeg, m_rotdg));
				PHD_Point cor = m_Pospx[m_numPos - 1];
				aligning = false;
				return false;
			}
			else //if (m_nstep <= nstep)
			{
				nstep = int(m_nstep * tottheta/m_rotdg);
				Debug.AddLine(wxString::Format("Polar align: star #2 nstep=%d / %d theta=%.1f / %.1f", nstep, m_nstep, tottheta, m_rotdg));
			}
		}
		return true;
	}
	if (m_numPos == 3)
	{
//		double theta = m_rotdg - tottheta;
//		SetStatusText(wxString::Format("Polar align: star #3 nstep=%d / %d theta=%.1f / %.1f", nstep, m_nstep, tottheta, m_rotdg));
//		Debug.AddLine(wxString::Format("Polar align: star #3 nstep=%d / %d theta=%.1f / %.1f", nstep, m_nstep, tottheta, m_rotdg));
		if (!bauto)
		{
			// rotate mount manually;
			bool isset = SetStar(m_numPos);
			if (isset) m_numPos++;
			return isset;
		}
		m_numPos++;
		m_mode = MODE_ADJUST;
		UpdateModeState();
		return true;
	}
	m_rotpx = 0;
	return true;
}

bool StaticPaToolWin::SetStar(int npos)
{
	int idx = npos - 1;
	// Get X and Y coords from PHD2;
	// idx: 0=B, 1/2/3 = A[idx];
	double cur_dec, cur_st;
	if (pPointingSource->GetCoordinates(&m_ra_rot[idx], &cur_dec, &cur_st))
	{
		Debug.AddLine("SetStar: failed to get scope coordinates");
		return false;
	}
	m_Pospx[idx].X = -1;
	m_Pospx[idx].Y = -1;
	PHD_Point star = pFrame->pGuider->CurrentPosition();
	m_Pospx[idx] = star;
	m_calPt[idx][0]->SetValue(wxString::Format("%+.f", m_Pospx[idx].X));
	m_calPt[idx][1]->SetValue(wxString::Format("%+.f", m_Pospx[idx].Y));

	Debug.AddLine(wxString::Format("Setstar #%d %.0f, %.0f", npos, m_Pospx[idx].X, m_Pospx[idx].Y));
	SetStatusText(wxString::Format("Setstar #%d %.0f, %.0f", npos, m_Pospx[idx].X, m_Pospx[idx].Y));
	return star.IsValid();
}

bool StaticPaToolWin::SetParams(double newoffset)
{
	double offsetdeg = newoffset;
	m_offsetpx = offsetdeg * 3600 / m_pxScale;
	Debug.AddLine(wxString::Format("PA setparams(new=%.1f) px=%.1f offset=%.1f dev=%.1f", newoffset, m_pxScale, m_offsetpx, m_devpx));
	if (m_offsetpx < m_devpx)
	{
		Debug.AddLine(wxString::Format("PA setparams() Too close to CoR: offsetpx=%.1f devpx=%.1f", m_offsetpx, m_devpx));
		return false;
	}
	m_rotdg = degrees(acos(1 - m_devpx / m_offsetpx));
	m_rotpx = m_rotdg * 3600.0 / m_pxScale * sin(radians(offsetdeg));

	int region = pFrame->pGuider->GetSearchRegion();
	m_nstep = 1;
	if (m_rotpx > region)
	{
		m_nstep = int(ceil(m_rotpx / region));
	}
	Debug.AddLine(wxString::Format("PA setparams() rotdg=%.1f rotpx=%.1f nstep=%d region=%d", m_rotdg, m_rotpx, m_nstep, region));
	return true;
}
void StaticPaToolWin::MoveWestBy(double thetadeg)
{
	m_can_slew = pPointingSource && pPointingSource->CanSlew();
	double cur_ra, cur_dec, cur_st;
	if (pPointingSource->GetCoordinates(&cur_ra, &cur_dec, &cur_st))
	{
		Debug.AddLine("Rotate tool: slew failed to get scope coordinates");
		return;
	}
	double slew_ra = cur_ra - thetadeg * 24.0 / 360.0;

	if (slew_ra >= 24.0)
		slew_ra -= 24.0;
	else if (slew_ra < 0.0)
		slew_ra += 24.0;

	wxBusyCursor busy;
	m_slewing = true;
	if (pPointingSource->SlewToCoordinates(slew_ra, cur_dec))
	{
		m_slewing = false;
		Debug.AddLine("Rotate tool: slew failed");
	}
	nstep++;
	PHD_Point lockpos = pFrame->pGuider->CurrentPosition();
	bool error = pFrame->pGuider->SetLockPosToStarAtPosition(lockpos);
}

wxBitmap StaticPaToolWin::CreateStarTemplate()
{
	wxMemoryDC memDC;
	wxBitmap bmp(320, 240, -1);
	memDC.SelectObject(bmp);
	memDC.SetBackground(*wxBLACK_BRUSH);
	memDC.Clear();
	usImage *pCurrImg = pFrame->pGuider->CurrentImage();
	wxImage *pDispImg = pFrame->pGuider->DisplayedImage();
	double scalefactor = pFrame->pGuider->ScaleFactor();
	double xpx = pDispImg->GetWidth() / scalefactor;
	double ypx = pDispImg->GetHeight() / scalefactor;

	double scale = 320.0/xpx;
	int region = 5;
	double cx = xpx/2;
	double cy = ypx/2;
	m_CoRpx.X = cx;
	m_CoRpx.Y = cy;

	m_dCamRot = 0.0;
	if (pMount && pMount->IsConnected() && pMount->IsCalibrated())
	{
		double xangle = 0.0; // , yangle;
		m_dCamRot = degrees(pMount->xAngle());
//		yangle = degrees(pMount->yAngle());
//		m_dCamRot = xangle;
	}
	memDC.SetTextForeground(*wxLIGHT_GREY);
	const wxFont& SmallFont =
#if defined(__WXOSX__)
		*wxSMALL_FONT;
#else
		*wxSWISS_FONT;
#endif
	memDC.SetFont(SmallFont);
	const std::string alpha = "ABCDEFGHIJKL";
	PHD_Point starpx, stardeg;
	double starsz, starmag;
	// Draw orbits for each alignment star
	for (int is = 0; is < m_nstar; is++)
	{
//		stardeg = (s_hemi < 0) ? PHD_Point(SthStarRa[is], SthStarDec[is]) : PHD_Point(NthStarRa[is], NthStarDec[is]);
//		starmag = (s_hemi < 0) ? SthStarMag[is] : NthStarMag[is];
		stardeg = PHD_Point(poleStars->at(is).ra, poleStars->at(is).dec);
		starmag = poleStars->at(is).mag;

		starsz = 356.0*exp(-0.3*starmag) / m_pxScale;
		starpx = Radec2Px(stardeg);
		m_starpx[is][0] = starpx.X;
		m_starpx[is][1] = starpx.Y;
		m_starpx[is][2] = sqrt(pow(cx - starpx.X, 2) + pow(cy - starpx.Y, 2));
		memDC.SetPen(wxPen(wxColor(255, 255, 0), 1, wxPENSTYLE_SOLID));

		memDC.DrawCircle(m_starpx[is][0] * scale, m_starpx[is][1] * scale, starsz*scale);
		memDC.DrawText(wxString::Format("%c", alpha[is]), (m_starpx[is][0] + starsz) * scale, m_starpx[is][1] * scale);
	}
	// draw the centre of the circle as a red cross
	memDC.SetBrush(*wxTRANSPARENT_BRUSH);
	memDC.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
	memDC.DrawLine((cx - region)*scale, cy*scale, (cx + region)*scale, cy*scale);
	memDC.DrawLine(cx*scale, (cy - region)*scale, cx*scale, (cy + region)*scale);

	memDC.SelectObject(wxNullBitmap);
	return bmp;
}



