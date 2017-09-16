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
	wxTextCtrl *m_raCurrent;
	wxTextCtrl *m_decCurrent;
	wxTextCtrl *m_siteLat;
	wxTextCtrl *m_siteLon;
	wxTextCtrl *m_camRot;
	wxTextCtrl *m_calPt[5][2];

	wxStaticText *m_notesLabel;
	wxTextCtrl *m_notes;
	wxButton *m_rotate;
	wxButton *m_adjust;
	wxButton *m_close;
	wxButton *m_phaseBtn;
	wxStatusBar *m_statusBar;
	wxTimer *m_timer;
// Added from testguide
	wxChoice *alignStarChoice;

	enum StaticPaMode
	{
		MODE_IDLE,
		MODE_ROTATE,
		MODE_ADJUST,
	};

	enum StaticPaCtrlIds
	{
		ID_SLEW = 10001,
		ID_ROTATE,
		ID_ADJUST,
		ID_CLOSE,
		ID_PULSEDURATION = 330001,
		ID_ALIGNSTAR,
	};
	StaticPaMode m_mode;
	bool m_rotating;

	PHD_Point m_siteLatLong;
	double m_pxScale;
	double m_dCamRot;
	int m_alignStar;

	PHD_Point m_Pospx[3];
	int m_numPos;
	PHD_Point m_CoRpx;
	double m_dispSz[2];
	double m_Radius;
	int m_nstar;
	double m_starpx[10][3];
	PHD_Point m_AzCorr, m_AltCorr;
	PHD_Point m_ConeCorr, m_DecCorr;
	double m_rotdg, m_rotpx, m_prevtheta;
	int nstep, m_nstep;
	double m_offsetpx, m_devpx, offsetdeg, m_guidedur;
	bool m_can_slew;
	bool m_slewing;
	bool aligning = false;
	bool aligned = false;

	double tottheta = 0.0;

	void UpdateModeState();
	void UpdateAlignStar();

	//void OnSlew(wxCommandEvent& evt);
	void OnRotate(wxCommandEvent& evt);
	void OnAdjust(wxCommandEvent& evt);
	void OnAlignStar(wxCommandEvent& evt);
	//void OnAppStateNotify(wxCommandEvent& evt);
	void OnCloseBtn(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);
	void CalcRotationCentre(void);
	PHD_Point Radec2Px(PHD_Point radec);
	bool IsAligning(){ return aligning; };
	bool IsAligned(){ return aligned; }
	void PaintHelper(wxAutoBufferedPaintDCBase& dc, double scale);
	bool UpdateAlignmentState(const PHD_Point& position);
	bool setStar(int idx);
	bool setparams(double newoffset);
	bool rotateMount(double theta, int npulse);
	void movewestby(double thetadeg); //, double exp);

	DECLARE_EVENT_TABLE()
};

#endif

