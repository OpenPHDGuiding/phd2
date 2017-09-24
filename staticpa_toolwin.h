/*
 *  staticpa_toolwin.h
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
#ifndef STATICPA_TOOLWIN_H
#define STATICPA_TOOLWIN_H

#include "phd.h"

#include <wx/gbsizer.h>
#include <wx/valnum.h>
#include <wx/textwrapper.h>

//==================================
struct StaticPaToolWin : public wxFrame
{
	StaticPaToolWin();
	~StaticPaToolWin();

	wxStaticText *m_instructions;
	wxTextCtrl *m_camScale; // Text box for camera pixel scale
	wxTextCtrl *m_camRot;   // Text box for camera rotation
	wxCheckBox *m_manual;   // Checkbox for auto/manual slewing
	wxTextCtrl *m_calPt[4][2];  // Text boxes for each point plus the CoR
	wxButton *m_star1;      // Button for manual get of point 1
	wxButton *m_star2;      // Button for manual get of point 2
	wxButton *m_star3;      // Button for manual get of point 3

	wxStaticText *m_notesLabel;
	wxTextCtrl *m_notes;
	wxButton *m_adjust;     // Button to calculate CoR
	wxButton *m_close;      // Close button
	wxStatusBar *m_statusBar;
	wxChoice *refStarChoice;  // Listbox for reference stars
	wxChoice *hemiChoice;     // Listbox for manual hemisphere choice 

	bool m_can_slew;
	wxString *pinstructions;
	wxString *pautoInstr;
	wxString *pmanualInstr;

	class PolePanel : public wxPanel
	{
	public:
		PolePanel(StaticPaToolWin* parent);
		StaticPaToolWin *paParent;
		void OnPaint(wxPaintEvent &evt);
		void Paint();
		void Render(wxDC &dc);
		DECLARE_EVENT_TABLE()
	};
	PolePanel *pole; // Panel for drawing of pole stars 

	class Star
	{
	public:
		std::string name;
		double ra, dec, mag;
		Star(const char* a, const double b, const double c, const double d) :name(a), ra(b), dec(c), mag(d) {};
	};
	std::vector<Star> SthStars, NthStars;
	std::vector<Star> *poleStars;

	enum StaticPaCtrlIds
	{
		ID_SLEW = 10001,
		ID_STAR1,
		ID_STAR2,
		ID_STAR3,
		ID_ROTATE,
		ID_CALCULATE,
		ID_CLOSE,
		ID_MANUAL,
		ID_REFSTAR,
		ID_HEMI
	};
	double m_pxScale;  // Camera pixel scale
	double m_dCamRot;  // Camera rotation
	double m_camXpx;   // Camera width
	int m_refStar;     // Selected reference star
	bool bauto;        // Auto slewing
	int s_hemi;        // Hemisphere

	bool aligning = false; // Collecting points
	double m_devpx;

	unsigned int state;
	int m_numPos;          // Number of alignment points
	double m_rotdg;        // Amount of rotation required
	int m_nstep;           //Number of steps needed
	double tottheta = 0.0; // Rotation so far
	int nstep;             // Number of steps taken so far

	double m_ra_rot[3];    // RA readings at each point
	PHD_Point m_Pospx[3];  // Alignment points - in pixels
	PHD_Point m_CoRpx;     // Centre of Rotation in pixels
	double m_Radius;       // Radius of centre of rotation to reference star

	double m_dispSz[2];    // Display size
	PHD_Point m_AzCorr, m_AltCorr;
	PHD_Point m_ConeCorr, m_DecCorr;

	void UpdateRefStar();
	void SetButtons();

	void OnHemi(wxCommandEvent& evt);
	void OnRefStar(wxCommandEvent& evt);
	void OnManual(wxCommandEvent& evt);
	void OnRotate(wxCommandEvent& evt);
	void OnStar2(wxCommandEvent& evt);
	void OnStar3(wxCommandEvent& evt);
	void OnCalculate(wxCommandEvent& evt);
	void OnCloseBtn(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
		
	void CreateStarTemplate(wxDC &dc);
	bool IsAligning(){ return aligning; };
	bool RotateMount();
	bool SetParams(double newoffset);
	void MoveWestBy(double thetadeg); //, double exp);
	bool SetStar(int idx);
	bool IsAligned(){ return bauto ? state == (3 << 1) : state == (7 << 1); }
	void CalcRotationCentre(void);
	void PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale);
	PHD_Point Radec2Px(PHD_Point radec);

	DECLARE_EVENT_TABLE()
};

#endif

