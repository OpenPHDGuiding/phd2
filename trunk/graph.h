/*
 *  graph.h
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

#include <wx/minifram.h>
#include <wx/spinctrl.h>

#ifndef GRAPHCLASS
#define GRAPHCLASS

class GraphLogWindow : public wxMiniFrame {
public:
	GraphLogWindow(wxWindow *parent);
	~GraphLogWindow(void);
	void AppendData (float dx, float dy, float RA, float Dec);
	void SetState (bool is_active);
	void OnPaint(wxPaintEvent& evt);
	void OnButtonMode(wxCommandEvent& evt);
	void OnButtonLength(wxCommandEvent& evt);
	void OnButtonHide(wxCommandEvent& evt);
	void OnButtonClear(wxCommandEvent& evt);
	void OnUpdateSpinGuideParams(wxSpinEvent& evt);
#if (wxMAJOR_VERSION > 2) || ((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION > 8))
	void OnUpdateSpinDGuideParams(wxSpinDoubleEvent& evt);
#endif
	void OnUpdateCommandGuideParams(wxCommandEvent& evt);
	wxColour RA_Color, DEC_Color;
#if (wxMAJOR_VERSION > 2) || ((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION > 8))
	wxSpinCtrlDouble *MM_Ctrl, *DSW_Ctrl;
#else
	wxTextCtrl *MM_Ctrl, *DSW_Ctrl;
#endif
	wxSpinCtrl *RAA_Ctrl, *RAH_Ctrl, *MDD_Ctrl, *MRAD_Ctrl;
	wxChoice *DM_Ctrl;

private:
	wxButton *LengthButton;
	wxButton *ModeButton;
	wxButton *HideButton;
	wxButton *ClearButton;
//	wxBitmap *bmp;
	float hdx[500];	// History of dx
	float hdy[500];
	float hra[500];
	float hdec[500];
	int n_items;	// # items in the history
	bool visible;
	int mode;	// 0 = RA/Dec, 1=dx, dy
	int length;

/*	float maxdx;	// Max dx
	float maxdy;
	float maxra;
	float maxdec;
	float mindx;	// Min dx
	float mindy;
	float minra;
	float mindec;*/
	DECLARE_EVENT_TABLE()
};

class ProfileWindow : public wxMiniFrame {
public:
	ProfileWindow(wxWindow *parent);
	~ProfileWindow(void);
	void UpdateData(usImage& img, float xpos, float ypos);
	void OnPaint(wxPaintEvent& evt);
	void SetState(bool is_active);
	void OnLClick(wxMouseEvent& evt);
private:
	int mode; // 0= 2D profile of mid-row, 1=2D of avg_row, 2=2D of avg_col
	bool visible;
	unsigned short *data;
	int horiz_profile[21], vert_profile[21], midrow_profile[21];
	DECLARE_EVENT_TABLE()
};


#endif
