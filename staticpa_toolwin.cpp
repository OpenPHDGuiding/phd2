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
"sigma Oct", "HD99828", "HD125371", "HD90105",
"BQ Oct", "HD99685", "TYC9518-405-1"
};
static const double SthStarRa[] = { 320.66, 164.22, 248.88, 122.36, 239.62, 130.32, 102.04 };
static const double SthStarDec[] = { -88.89, -89.33, -89.38, -89.52, -89.83, -89.85, -89.88 };
static const std::string NthStarName[] = {
	"Polaris", "HD1687", "TYC4629-37-1", "TYC4661-2-1"
};
static const double NthStarRa[] = { 43.12, 12.14, 85.51, 297.95 };
static const double NthStarDec[] = { 89.34, 89.54, 89.65, 89.83 };

//==================================
BEGIN_EVENT_TABLE(StaticPaToolWin, wxFrame)
//EVT_BUTTON(ID_SLEW, StaticPaToolWin::OnSlew)
EVT_BUTTON(ID_ROTATE, StaticPaToolWin::OnRotate)
EVT_BUTTON(ID_ADJUST, StaticPaToolWin::OnAdjust)
EVT_BUTTON(ID_CLOSE, StaticPaToolWin::OnCloseBtn)
EVT_CHOICE(ID_ALIGNSTAR, StaticPaToolWin::OnAlignStar)
//EVT_COMMAND(wxID_ANY, APPSTATE_NOTIFY_EVENT, StaticPaToolWin::OnAppStateNotify)
EVT_CLOSE(StaticPaToolWin::OnClose)
END_EVENT_TABLE()

StaticPaToolWin::StaticPaToolWin()
: wxFrame(pFrame, wxID_ANY, _("Static Polar Alignment"), wxDefaultPosition, wxDefaultSize,
wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxTAB_TRAVERSAL | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR),
m_slewing(false), m_devpx(8)
{
	pFrame->pGuider->pStaticPaTool = this;
	m_numPos = 0;
	
	SetBackgroundColour(wxColor(0xcccccc));

	SetSizeHints(wxDefaultSize, wxDefaultSize);

	// get site lat/long from scope
	double lat, lon;
	m_siteLatLong.Invalidate();
	if (pPointingSource && !pPointingSource->GetSiteLatLong(&lat, &lon))
	{
		m_siteLatLong.SetXY(lon, lat);
	}
	m_pxScale = pFrame->GetCameraPixelScale();

	wxCommandEvent dummy;
	if (!pFrame->CaptureActive)
	{
		// loop exposures
		SetStatusText(_("Start Looping..."));
		pFrame->OnLoopExposure(dummy);
	}

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

	topSizer->Add(instrSizer);

	// static box sizer holding the scope pointing controls
	wxStaticBoxSizer *sbSizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Scope Pointing")), wxVERTICAL);

	// a grid box sizer for laying out scope pointing the controls
	wxGridBagSizer *gbSizer = new wxGridBagSizer(0, 0);
	gbSizer->SetFlexibleDirection(wxBOTH);
	gbSizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	wxStaticText *txt;

// Alignment star specification
	txt = new wxStaticText(this, wxID_ANY, _("Alignment Star"));
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Right Ascen (" DEGREES_SYMBOL ")"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Declination (" DEGREES_SYMBOL ")"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 5);

	// Drop down list box for alignment star
	int nstar = (lat < 0) ? 7 : 4;
	m_nstar = nstar;
	wxArrayString choices;
	std::string starname;
	for (int is = 0; is < nstar; is++) {
		starname = (lat < 0) ? SthStarName[is] : NthStarName[is];
		choices.Add(_(starname));
	}
	alignStarChoice = new wxChoice(this, ID_ALIGNSTAR, wxDefaultPosition, wxDefaultSize, choices);
	alignStarChoice->SetToolTip(_("Select the star used for checking alignment."));
	gbSizer->Add(alignStarChoice, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL, 5);

	m_raCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_raCurrent, wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_decCurrent = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_decCurrent, wxGBPosition(1, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Site"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Lat"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(2, 1), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Lon"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(2, 2), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Camera Rot"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(2, 3), wxGBSpan(1, 1), wxALL, 5);

	m_siteLat = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_siteLat, wxGBPosition(3, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_siteLon = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_siteLon, wxGBPosition(3, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_camRot = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_camRot, wxGBPosition(3, 3), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("X px"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(4, 1), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Y px"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(4, 2), wxGBSpan(1, 1), wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Pt #1"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(5, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[0][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[0][0], wxGBPosition(5, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[0][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[0][1], wxGBPosition(5, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Pt #2"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(6, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[1][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[1][0], wxGBPosition(6, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[1][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[1][1], wxGBPosition(6, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Pt #3"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(7, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[2][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[2][0], wxGBPosition(7, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[2][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[2][1], wxGBPosition(7, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	txt = new wxStaticText(this, wxID_ANY, _("Centre"), wxDefaultPosition, wxDefaultSize, 0);
	txt->Wrap(-1);
	gbSizer->Add(txt, wxGBPosition(8, 0), wxGBSpan(1, 1), wxALL, 5);

	m_calPt[3][0] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[3][0], wxGBPosition(8, 1), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	m_calPt[3][1] = new wxTextCtrl(this, wxID_ANY, _T("--"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	gbSizer->Add(m_calPt[3][1], wxGBPosition(8, 2), wxGBSpan(1, 1), wxEXPAND | wxALL, 5);

	// add grid bag sizer to static sizer
	sbSizer->Add(gbSizer, 1, wxALIGN_CENTER, 5);

	// add static sizer to top-level sizer
	topSizer->Add(sbSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5);

	// add some padding below the static sizer
	topSizer->Add(0, 3, 0, wxEXPAND, 3);

	m_notesLabel = new wxStaticText(this, wxID_ANY, _("Altitude adjustment notes"), wxDefaultPosition, wxDefaultSize, 0);
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

	m_rotate = new wxButton(this, ID_ROTATE, _("Rotate"), wxDefaultPosition, wxDefaultSize, 0);
	hSizer->Add(m_rotate, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	// proportional pad on right of Rotate button
	hSizer->Add(0, 0, 1, wxEXPAND, 5);

	m_adjust = new wxButton(this, ID_ADJUST, _("Adjust"), wxDefaultPosition, wxDefaultSize, 0);
	hSizer->Add(m_adjust, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	// proportional pad on right of Align button
	hSizer->Add(0, 0, 2, wxEXPAND, 5);

	m_close = new wxButton(this, ID_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0);
	hSizer->Add(m_close, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	//m_phaseBtn = new wxButton(this, ID_PHASE, wxT("???"), wxDefaultPosition, wxDefaultSize, 0);
	//hSizer->Add(m_phaseBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

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
		"Wait for all three calibration points to be measured.\n"
		"Press Adjust to display the adjustments needed.\n"
		"Adjust your mount's altitude and azimuth as displayed."
		));

	// can mount slew?
	bool m_can_slew = pPointingSource && pPointingSource->CanSlew();
	alignStarChoice->Select(pConfig->Profile.GetInt("/StaticPaTool/AlignStar", 4) - 1);
	UpdateAlignStar();

	m_siteLat->SetValue(wxString::Format("%+.3f", lat));
	m_siteLon->SetValue(wxString::Format("%+.3f", lon));

	double xangle=0.0, yangle;
	if (pMount && pMount->IsConnected() && pMount->IsCalibrated())
	{
		xangle = degrees(pMount->xAngle());
		yangle = degrees(pMount->yAngle());
	}
	m_camRot->SetValue(wxString::Format("%+.3f", xangle));
	m_dCamRot = xangle;

	m_timer = NULL;

	//m_phase = PHASE_ADJUST_AZ;
	m_mode = MODE_IDLE;
	//m_location_prompt_done = false;
	//UpdatePhaseState();
	UpdateModeState();
}

StaticPaToolWin::~StaticPaToolWin()
{
	pFrame->pGuider->pStaticPaTool = NULL;
	pFrame->pStaticPaTool = NULL;
}
/*
void StaticPaToolWin::EnableSlew(bool enable)
{
}
*/
void StaticPaToolWin::UpdateModeState()
{
	wxCommandEvent dummy;
	wxString idleStatus;

repeat:

	if (m_mode == MODE_ROTATE)
	{
		//m_rotate->Enable(false);
		m_rotate->Enable(true);
		m_adjust->Enable(true);
//		EnableSlew(false);

		// restore subframes setting
		//pCamera->UseSubframes = m_save_use_subframes;

		if (!m_rotating)
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
			/*
			switch (pFrame->pGuider->GetState())
			{
			case STATE_UNINITIALIZED:
			case STATE_CALIBRATED:
			case STATE_SELECTING:
				if (!pFrame->pGuider->IsLocked())
				{
					SetStatusText(_("Auto-selecting a star"));
					pFrame->OnAutoStar(dummy);
				}
				return;
			case STATE_CALIBRATING_PRIMARY:
			case STATE_CALIBRATING_SECONDARY:
				if (!pMount->IsCalibrated())
					SetStatusText(_("Waiting for calibration to complete..."));
				return;
			case STATE_SELECTED:
			case STATE_GUIDING:
				SetStatusText(_("Start Looping..."));
				pFrame->OnLoopExposure(dummy);
				return;
				m_rotating = true;
				return;
			case STATE_STOP:
				return;
			}
			*/
		}
	}
	else if (m_mode == MODE_ADJUST)
	{
		m_rotate->Enable(true);
		m_adjust->Enable(false);
		m_rotating = false;
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
	}
}
void StaticPaToolWin::UpdateAlignStar()
{
	int i_alignStar = alignStarChoice->GetSelection();
	pConfig->Profile.SetInt("/StaticPaTool/AlignStar", alignStarChoice->GetSelection() + 1);

	double ra_deg = m_siteLatLong.Y < 0 ? SthStarRa[i_alignStar] : NthStarRa[i_alignStar];
	double dec_deg = m_siteLatLong.Y < 0 ? SthStarDec[i_alignStar] : NthStarDec[i_alignStar];

	m_alignStar = i_alignStar;
	m_alignStar = i_alignStar;

	m_raCurrent->SetValue(wxString::Format("%+.2f", ra_deg));
	m_decCurrent->SetValue(wxString::Format("%+.2f", dec_deg));

}

//void StaticPaToolWin::OnSlew(wxCommandEvent& evt)
//{
//	double cur_ra, cur_dec, cur_st;
//	if (pPointingSource->GetCoordinates(&cur_ra, &cur_dec, &cur_st))
//	{
//		Debug.AddLine("Rotate tool: slew failed to get scope coordinates");
//		return;
//	}
//
//	double slew_ra = 0; // cur_st + (raSlew * 24.0 / 360.0);
//	if (slew_ra >= 24.0)
//		slew_ra -= 24.0;
//	else if (slew_ra < 0.0)
//		slew_ra += 24.0;
//
//	//Debug.AddLine(wxString::Format("Rotate tool slew from ra %.2f, dec %.1f to ra %.2f, dec %.1f",
//	//	cur_ra, cur_dec, slew_ra, decSlew));
//
//	if (pPointingSource->CanSlewAsync())
//	{
//		struct SlewInBg : public RunInBg
//		{
//			double ra, dec;
//			SlewInBg(wxWindow *parent, double ra_, double dec_)
//				: RunInBg(parent, _("Slew"), _("Slewing...")), ra(ra_), dec(dec_)
//			{
//				SetPopupDelay(100);
//			}
//			bool Entry()
//			{
//				bool err = pPointingSource->SlewToCoordinatesAsync(ra, dec);
//				if (err)
//				{
//					SetErrorMsg(_("Slew failed!"));
//					return true;
//				}
//				while (pPointingSource->Slewing())
//				{
//					wxMilliSleep(500);
//					if (IsCanceled())
//					{
//						pPointingSource->AbortSlew();
//						SetErrorMsg(_("Slew was canceled"));
//						break;
//					}
//				}
//				return false;
//			}
//		};
//	}
//	else
//	{
//		wxBusyCursor busy;
//	}
//}

void StaticPaToolWin::OnRotate(wxCommandEvent& evt)
{
//	m_mode = MODE_ROTATE;
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
	/*
	double testCal[][2] = { { 293.11, 151.18 }, { 274.19, 189.42 }, { 288.95, 256.06 } };
	for (int i = 0; i < 3; i++)
	{
			m_Pospx[i].X = testCal[i][0];
			m_Pospx[i].Y = testCal[i][1];
			m_calPt[i][0]->SetValue(wxString::Format("%+.f", m_Pospx[i].X));
			m_calPt[i][1]->SetValue(wxString::Format("%+.f", m_Pospx[i].Y));
	}
	m_numPos = 3;
	CalcRotationCentre();
	m_calPt[3][0]->SetValue(wxString::Format("%+.f", m_CoRpx.X));
	m_calPt[3][1]->SetValue(wxString::Format("%+.f", m_CoRpx.Y));
	aligning = false;
	*/
	m_mode = MODE_ROTATE;
	UpdateModeState();
}

void StaticPaToolWin::OnAdjust(wxCommandEvent& evt)
{
	m_numPos = 3;
	CalcRotationCentre();
	m_calPt[3][0]->SetValue(wxString::Format("%+.f", m_CoRpx.X));
	m_calPt[3][1]->SetValue(wxString::Format("%+.f", m_CoRpx.Y));

	m_mode = MODE_ROTATE;
	m_mode = MODE_ADJUST;
	UpdateModeState();
}

void StaticPaToolWin::OnAlignStar(wxCommandEvent& evt)
{
	UpdateAlignStar();
}

//void StaticPaToolWin::OnAppStateNotify(wxCommandEvent& evt)
//{
//	UpdateModeState();
//}

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
	double a, b, c, e, f, g, i, j, k;
	double m11, m12, m13, m14;
	double cx, cy, cr;
	x1 = m_Pospx[0].X;
	y1 = m_Pospx[0].Y;
	x2 = m_Pospx[1].X;
	y2 = m_Pospx[1].Y;
	x3 = m_Pospx[2].X;
	y3 = m_Pospx[2].Y;
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

	m_CoRpx.X = cx;
	m_CoRpx.Y = cy;
	m_Radius = cr;

	usImage *pCurrImg = pFrame->pGuider->CurrentImage();
	wxImage *pDispImg = pFrame->pGuider->DisplayedImage();
	double scalefactor = pFrame->pGuider->ScaleFactor();
	int xpx = pDispImg->GetWidth();
	int ypx = pDispImg->GetHeight();
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
		stardeg = (m_siteLatLong.Y < 0) ? PHD_Point(SthStarRa[is], SthStarDec[is]) : PHD_Point(NthStarRa[is], NthStarDec[is]);
		starpx = Radec2Px(stardeg );
		m_starpx[is][0] = starpx.X;
		m_starpx[is][1] = starpx.Y;
		m_starpx[is][2] = sqrt(pow(cx - starpx.X, 2) + pow(cy - starpx.Y, 2));
	}

	double xs = m_Pospx[2].X;
	double ys = m_Pospx[2].Y;
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
	double ra_hrs, dec_deg, st_hrs;
	pPointingSource->GetCoordinates(&ra_hrs, &dec_deg, &st_hrs);

// Target hour angle - or rather the rotation needed to correct. 
// FIXME: check if dec-deg should be that of the mount or the target or the hemisphere
	double t_ha = st_hrs*15.0 + copysign(radec.X, radec.X);
	double a = t_ha + m_dCamRot + (st_hrs - ra_hrs)*15.0;

	PHD_Point px(m_CoRpx.X + r * cos(radians(a)), m_CoRpx.Y + r * sin(radians(a)));
	return px;
}

wxWindow *StaticPaTool::CreateStaticPaToolWindow()
{
	if (!pCamera)
	{
		wxMessageBox(_("Please connect a camera first."));
		return 0;
	}

	// confirm that image scale is specified

	if (pFrame->GetCameraPixelScale() == 1.0)
	{
		bool confirmed = ConfirmDialog::Confirm(_(
			"The Rotate Align tool is most effective when PHD2 knows your guide\n"
			"scope focal length and camera pixel size.\n"
			"\n"
			"Enter your guide scope focal length on the Global tab in the Brain.\n"
			"Enter your camera pixel size on the Camera tab in the Brain.\n"
			"\n"
			"Would you like to run the rotate tool anyway?"),
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
	if (m_numPos < 3)
	{
		return;
	}
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	//wxPenStyle penStyle = m_polarAlignCircleCorrection == 1.0 ? wxPENSTYLE_DOT : wxPENSTYLE_SOLID;
	wxPenStyle penStyle = wxPENSTYLE_DOT;
	dc.SetPen(wxPen(wxColor(255, 0, 255), 1, penStyle));
	//int radius = ROUND(m_polarAlignCircleRadius * m_polarAlignCircleCorrection * m_scaleFactor);
	dc.DrawCircle(m_CoRpx.X*scale, m_CoRpx.Y*scale, m_Radius*scale);
	// draw the centre of the circle as a red cross
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(wxColor(255, 0, 0), 1, wxPENSTYLE_SOLID));
	int region = 5;
	dc.DrawLine((m_CoRpx.X - region)*scale, m_CoRpx.Y*scale, (m_CoRpx.X + region)*scale, m_CoRpx.Y*scale);
	dc.DrawLine(m_CoRpx.X*scale, (m_CoRpx.Y - region)*scale, m_CoRpx.X*scale, (m_CoRpx.Y + region)*scale);
	// Show the centre of the display
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
	// Draw adjustment lines for centring the CoR on the display
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

	double xs = m_Pospx[2].X * scale;
	double ys = m_Pospx[2].Y * scale;
	dc.SetPen(wxPen(wxColor(255, 165, 0), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xs, ys, xs + m_AltCorr.X * scale, ys + m_AltCorr.Y * scale);
	dc.SetPen(wxPen(wxColor(0, 255, 0), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xs + m_AltCorr.X * scale, ys + m_AltCorr.Y * scale,
		xs + m_AltCorr.X * scale + m_AzCorr.X * scale, ys + m_AzCorr.Y * scale + m_AltCorr.Y * scale);
	dc.SetPen(wxPen(wxColor(127, 127, 127), 1, wxPENSTYLE_SOLID));
	dc.DrawLine(xs, ys, xs + m_AltCorr.X * scale + m_AzCorr.X * scale,
		ys + m_AltCorr.Y * scale + m_AzCorr.Y * scale);
}

bool StaticPaToolWin::UpdateAlignmentState(const PHD_Point& position) //(int npos, bool bauto)
{
	// Initially, assume an offset of 5.0 degrees of the camera from the CoR
	// Calculate how far to move in RA to get a detectable arc
	// Calculate the tangential ditance of that movement
	// Mark the starting position then rotate the mount
	bool bauto = true;
	if (m_numPos == 1)
	{
		SetStatusText(_("Polar align: star #1"));
		Debug.AddLine("Polar align: star #1");
		//   Initially offset is 5 degrees;
		bool rc = setparams(1.0); // m_rotdg, m_rotpx, m_nstep for assumed 5.0 degree PA error
		Debug.AddLine(wxString::Format("Polar align: star #1 rotdg=%.1frotpx= %d nstep=%.0f", m_rotdg, m_rotpx, m_nstep));
		bool isset = setStar(m_numPos);
		if (isset) m_numPos++;
		tottheta = 0.0;
		nstep = 0;
		Debug.AddLine(wxString::Format("Leave Polar align: star #1 rotdg=%.1frotpx= %.1f nstep=%d", m_rotdg, m_rotpx, m_nstep));
		return isset;
	}
	if (m_numPos == 2)
	{
		double theta = m_rotdg - tottheta;
		Debug.AddLine(wxString::Format("Enter Polar align: star #2 theta= %.1f rotdg=%.1f nstep=%d / %d", theta, m_rotdg, nstep, m_nstep));
		SetStatusText(_("Polar align: star #2"));
		Debug.AddLine("Polar align: star #2");
		// Once the mount has rotated theta degrees (as indicated by prevtheta);
		if (!bauto)
		{
			// rotate mount manually;
			bool isset = setStar(m_numPos);
			if (isset) m_numPos++;
			return isset;
		}
		Debug.AddLine(wxString::Format("Polar align: star #2 theta= %.1f rotdg=%.1f nstep=%d / %d", theta, m_rotdg, nstep, m_nstep));
		SetStatusText(wxString::Format("Polar align: star #2 theta= %.1f rotdg=%.1f nstep=%d / %d", theta, m_rotdg, nstep, m_nstep));
		if (tottheta < m_rotdg)
		{
			double newtheta = theta / (m_nstep - nstep);
			movewestby(newtheta); // , exposure);
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
			bool isset = setStar(m_numPos);

			double actpix = sqrt(pow(m_Pospx[1].X - m_Pospx[0].X, 2) + pow(m_Pospx[1].Y - m_Pospx[0].Y, 2));
			double actsec = actpix * m_pxScale;
			double actoffsetdeg = 90 - degrees(acos(actsec / 3600 / m_rotdg));

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
			bool rc = setparams(actoffsetdeg); // m_rotdg, m_rotpx, m_nstep for measures PA error
			if (!rc)
			{
				aligning = false;
				return false;
			}
			if (m_rotdg <= prev_rotdg) // Moved far enough
			{
				if (!isset)
				{
					aligning = false;
					return false;
				}
				m_numPos++;
				nstep = 0;
				tottheta = 0.0;
			}
			else if (m_rotdg > 45)
			{
				Debug.AddLine(wxString::Format("Polar align: star #2 Too close to CoR offset =%.1f Rot=%.1f", actoffsetdeg, m_rotdg));
				SetStatusText(wxString::Format("Polar align: star #2 Too close to CoR offset =%.1f Rot=%.1f", actoffsetdeg, m_rotdg));
				PHD_Point cor = m_Pospx[m_numPos - 1];
				aligning = false;
				return false;
			}
			else if (m_nstep <= nstep)
			{
				nstep = m_nstep - 1;
			}
		}
		return true;
	}
	if (m_numPos == 3)
	{
		SetStatusText(wxString::Format("Polar align: star #3 nstep=%d / %d theta=%.1f / %.1f", nstep, m_nstep, tottheta, m_rotdg));
		Debug.AddLine(wxString::Format("Polar align: star #3 nstep=%d / %d theta=%.1f / %.1f", nstep, m_nstep, tottheta, m_rotdg));
		if (!bauto)
		{
			// rotate mount manually;
			bool isset = setStar(m_numPos);
			if (isset) m_numPos++;
			return isset;
		}
		if (tottheta < m_rotdg)
		{
			double newtheta = m_rotdg / (m_nstep - nstep);
			movewestby(newtheta); // , exposure);
			tottheta += newtheta;
		}
		else
		{
			bool isset = setStar(m_numPos);
			if (isset)  m_numPos++;
			return isset;
		}
		return true;
	}
	m_rotpx = 0;
	return true;
}

bool StaticPaToolWin::setStar(int npos)
{
	int idx = npos - 1;
	// Get X and Y coords from PHD2;
	// idx: 0=B, 1/2/3 = A[idx];
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

bool StaticPaToolWin::setparams(double newoffset)
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
	//double gRate = sin(radians(m_rotdg))*self.settings["xRate"];
	//m_guidedur = m_rotpx / gRate;
	Debug.AddLine(wxString::Format("PA setparams() rotdg=%.1f rotpx=%.1f nstep=%.0f region=%d", m_rotdg, m_rotpx, m_nstep, region));
	return true;
}
/*
bool StaticPaToolWin::rotateMount(double theta, int npulse)
{
	double exposure = pFrame->RequestedExposureDuration() / 1000;
	double newtheta = theta / npulse;
	for (int n = 0; n < npulse; n++)
	{
		movewestby(newtheta); // , exposure);
		PHD_Point lockpos = pFrame->pGuider->CurrentPosition();
		bool error = pFrame->pGuider->SetLockPosToStarAtPosition(lockpos);
	}
	return true;
}
*/
void StaticPaToolWin::movewestby(double thetadeg) //, double exp)
{
	m_can_slew = pPointingSource && pPointingSource->CanSlew();
	// Rates in deg/sec;
	// Calculate duration for rate 0;
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

	//Debug.AddLine(wxString::Format("Rotate tool slew from ra %.2f, dec %.1f to ra %.2f, dec %.1f",
	//	cur_ra, cur_dec, slew_ra, decSlew));
	wxBusyCursor busy;
	m_slewing = true;
	//	GetStatusBar()->PushStatusText(_("Slewing ..."));
	if (pPointingSource->SlewToCoordinates(slew_ra, cur_dec))
	{
		//		GetStatusBar()->PopStatusText();
		m_slewing = false;
		Debug.AddLine("Rotate tool: slew failed");
	}
	nstep++;
	PHD_Point lockpos = pFrame->pGuider->CurrentPosition();
	bool error = pFrame->pGuider->SetLockPosToStarAtPosition(lockpos);
	//	wxMilliSleep(exp * 1000);
	//	pFrame->pGuider->UpdateImageDisplay();
}



