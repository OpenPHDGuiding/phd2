/*
 *  graph.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006, 2007, 2008, 2009, 2010 Craig Stark.
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
#include "graph.h"
#include <wx/dcbuffer.h>
#include <wx/utils.h>
#include <wx/colordlg.h>

// ADD SOMETHING ON THE GRAPH TO SHOW THE DEC DIRECTION YOU COULD SET TO.
// IF DRIFT IS POSITIVE, GUIDE IN SOUTH.  IF DRIFT IS NEGATIVE, GUIDE IN NORTH
// PERHAPS DO A LINEAR REGRESSION ON THE DEC DIST

BEGIN_EVENT_TABLE(GraphLogWindow, wxMiniFrame)
EVT_PAINT(GraphLogWindow::OnPaint)
EVT_BUTTON(BUTTON_GRAPH_HIDE,GraphLogWindow::OnButtonHide)
EVT_BUTTON(BUTTON_GRAPH_MODE,GraphLogWindow::OnButtonMode)
EVT_BUTTON(BUTTON_GRAPH_LENGTH,GraphLogWindow::OnButtonLength)
EVT_BUTTON(BUTTON_GRAPH_CLEAR,GraphLogWindow::OnButtonClear)
EVT_SPINCTRL(GRAPH_RAA,GraphLogWindow::OnUpdateSpinGuideParams)
EVT_SPINCTRL(GRAPH_RAH,GraphLogWindow::OnUpdateSpinGuideParams)

#if (wxMAJOR_VERSION > 2) || ((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION > 8))
EVT_SPINCTRLDOUBLE(GRAPH_MM,GraphLogWindow::OnUpdateSpinDGuideParams)
#endif

EVT_SPINCTRL(GRAPH_MRAD,GraphLogWindow::OnUpdateSpinGuideParams)
EVT_SPINCTRL(GRAPH_MDD,GraphLogWindow::OnUpdateSpinGuideParams)
EVT_CHOICE(GRAPH_DM,GraphLogWindow::OnUpdateCommandGuideParams)
END_EVENT_TABLE()

GraphLogWindow::GraphLogWindow(wxWindow *parent):
wxMiniFrame(parent,wxID_ANY,_T("History"),wxDefaultPosition,wxSize(610,252),wxCAPTION & ~wxSTAY_ON_TOP) {  // was 230

	this->visible = false;
	this->n_items = 0;
	this->mode = 0; // RA/Dec
	this->length = 100;
//	this->maxdx = this->maxdy = this->maxra = this->maxdec = 0.0;
//	this->mindx = this->mindy = this->minra = this->mindec = 0.0;
//	bmp = new wxBitmap(520,200,24);
	this->SetBackgroundStyle(wxBG_STYLE_CUSTOM);

	this->LengthButton = new wxButton(this,BUTTON_GRAPH_LENGTH,_T("100"),wxPoint(10,10),wxSize(-1,-1));
	this->LengthButton->SetToolTip(_T("# of frames of history to display"));
	this->ModeButton = new wxButton(this,BUTTON_GRAPH_MODE,_T("RA/Dec"),wxPoint(10,40),wxSize(-1,-1));
	this->ModeButton->SetToolTip(_T("Toggle RA/Dec vs dx/dy.  Shift-click to change RA/dx color.  Ctrl-click to change Dec/dy color"));
	this->HideButton = new wxButton(this,BUTTON_GRAPH_HIDE,_T("Hide"),wxPoint(10,70),wxSize(-1,-1));
	this->HideButton->SetToolTip(_T("Hide graph"));
	this->ClearButton = new wxButton(this,BUTTON_GRAPH_CLEAR,_T("Clear"),wxPoint(10,100),wxSize(-1,-1));
	this->ClearButton->SetToolTip(_T("Clear graph data"));
	
	RA_Color=wxColour(100,100,255);
	DEC_Color = wxColour(255,0,0);
	
	//SetForegroundColour(* wxWHITE);
#ifdef __WINDOWS__
	int ctl_size = 45;
	int extra_offset = -5;
#else
	int ctl_size = 60;
	int extra_offset = 0;
#endif
	wxStaticText *raa_label = new wxStaticText(this,wxID_ANY,_T("RA agr"),wxPoint(10,210),wxSize(60,-1));
	raa_label->SetOwnForegroundColour(* wxWHITE);
#ifdef __WINDOWS__
	raa_label->SetOwnBackgroundColour(* wxBLACK);
#endif
	this->RAA_Ctrl = new wxSpinCtrl(this,GRAPH_RAA,wxString::Format(_T("%d"),(int) (RA_aggr * 100)),
									wxPoint(50,205),wxSize(ctl_size,-1),wxSP_ARROW_KEYS,
									0,120,(int) (RA_aggr * 100));
	wxStaticText *rah_label = new wxStaticText(this,wxID_ANY,_T("RA hys"),wxPoint(110,210),wxSize(60,-1));
	rah_label->SetOwnForegroundColour(* wxWHITE);
#ifdef __WINDOWS__
	rah_label->SetOwnBackgroundColour(* wxBLACK);
#endif
	this->RAH_Ctrl = new wxSpinCtrl(this,GRAPH_RAH,wxString::Format(_T("%d"),(int) (RA_hysteresis * 100)),
									wxPoint(150,205),wxSize(ctl_size,-1),wxSP_ARROW_KEYS,
									0,50,(int) (RA_hysteresis * 100));
	wxStaticText *mm_label = new wxStaticText(this,wxID_ANY,_T("Mn mo"),wxPoint(210,210),wxSize(60,-1));
	mm_label->SetOwnForegroundColour(* wxWHITE);
#ifdef __WINDOWS__
	mm_label->SetOwnBackgroundColour(* wxBLACK);
#endif
#if (wxMAJOR_VERSION > 2) || ((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION > 8))
	this->MM_Ctrl = new wxSpinCtrlDouble(this,GRAPH_MM,wxString::Format(_T("%.2f"),MinMotion),
									wxPoint(255,210+extra_offset),wxSize(ctl_size,-1),wxSP_ARROW_KEYS,
									0,5,MinMotion,0.05);
#else
	this->MM_Ctrl = new wxTextCtrl(this,GRAPH_MM,wxString::Format(_T("%.2f"),MinMotion),
									wxPoint(255,210+extra_offset),wxSize(ctl_size,-1));
#endif
//	wxStaticText *DM_Text = new wxStaticText(this,wxID_ANY,_T("Dec guide mode"),wxPoint(400,210),wxSize(-1,-1));
	wxStaticText *mrad_label = new wxStaticText(this,wxID_ANY,_T("Mx RA"),wxPoint(315,210),wxSize(ctl_size+10,-1));
	mrad_label->SetOwnForegroundColour(* wxWHITE);
#ifdef __WINDOWS__
	mrad_label->SetOwnBackgroundColour(* wxBLACK);
#endif
	this->MRAD_Ctrl = new wxSpinCtrl(this,GRAPH_MRAD,wxString::Format(_T("%d"),Max_RA_Dur),
									wxPoint(360,205),wxSize(ctl_size+10,-1),wxSP_ARROW_KEYS,
									0,2000,Max_RA_Dur);
	wxStaticText *mdd_label = new wxStaticText(this,wxID_ANY,_T("Mx dec"),wxPoint(425,210),wxSize(ctl_size+10,-1));
	mdd_label->SetOwnForegroundColour(* wxWHITE);
#ifdef __WINDOWS__
	mdd_label->SetOwnBackgroundColour(* wxBLACK);
#endif
	this->MDD_Ctrl = new wxSpinCtrl(this,GRAPH_MDD,wxString::Format(_T("%d"),Max_Dec_Dur),
									wxPoint(470,205),wxSize(ctl_size+10,-1),wxSP_ARROW_KEYS,
									0,2000,Max_Dec_Dur);
	wxString dec_choices[] = {
		_T("Off"),_T("Auto"),_T("North"),_T("South")
	};
	this->DM_Ctrl= new wxChoice(this,GRAPH_DM,wxPoint(535,210+extra_offset),wxSize(ctl_size+15,-1),WXSIZEOF(dec_choices), dec_choices );
	DM_Ctrl->SetSelection(Dec_guide);

}

GraphLogWindow::~GraphLogWindow() {

}
void GraphLogWindow::OnUpdateSpinGuideParams(wxSpinEvent& WXUNUSED(evt)) {
	RA_aggr = (float) this->RAA_Ctrl->GetValue() / 100.0;
	RA_hysteresis = (float) this->RAH_Ctrl->GetValue() / 100.0;
	Max_Dec_Dur = this->MDD_Ctrl->GetValue();
	Max_RA_Dur = this->MRAD_Ctrl->GetValue();
#if (wxMAJOR_VERSION > 2) || ((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION > 8))
	MinMotion = this->MM_Ctrl->GetValue();
#else
	this->MM_Ctrl->GetValue().ToDouble(&MinMotion);
#endif
}
void GraphLogWindow::OnUpdateCommandGuideParams(wxCommandEvent& WXUNUSED(evt)) {
	Dec_guide = this->DM_Ctrl->GetSelection();	
}

#if (wxMAJOR_VERSION > 2) || ((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION > 8))
void GraphLogWindow::OnUpdateSpinDGuideParams(wxSpinDoubleEvent& WXUNUSED(evt)) {
	MinMotion = this->MM_Ctrl->GetValue();
}
#endif

void GraphLogWindow::OnButtonHide(wxCommandEvent& WXUNUSED(evt)) {
	this->visible = false;
	frame->Menubar->Check(MENU_GRAPH,false);
	this->Show(false);
}

void GraphLogWindow::OnButtonMode(wxCommandEvent& WXUNUSED(evt)) {
//	bool foo1 = wxGetMouseState::ShiftDown();
//	bool foo2 = wxGetKeyState(WXK_SHIFT);
	wxMouseState mstate = wxGetMouseState();
//	bool foo1 = mstate.ShiftDown();
	
	
	if (wxGetKeyState(WXK_SHIFT)) {
		wxColourData cdata;
		cdata.SetColour(RA_Color);
		wxColourDialog cdialog(this, &cdata);
		if (cdialog.ShowModal() == wxID_OK) {
			cdata = cdialog.GetColourData();
			RA_Color = cdata.GetColour();
		}
	}
		
		
	this->mode = 1 - this->mode;
	if (this->mode)
		this->ModeButton->SetLabel(_T("dx/dy"));
	else
		this->ModeButton->SetLabel(_T("RA/Dec"));
	this->Refresh();
}

void GraphLogWindow::OnButtonLength(wxCommandEvent& WXUNUSED(evt)) {
	switch (length) {
		case 100:
			length = 250;
			break;
		case 250:
			length = 500;
			break;
		case 500:
			length = 50;
			break;
		case 50:
			length = 100;
			break;
		default:
			length = 100;
	}
	this->LengthButton->SetLabel(wxString::Format(_T("%d"),length));
	this->Refresh();
}
/*void GraphLogWindow::OnCloseWindow(wxCloseEvent& WXUNUSED(evt)) {

}
*/

void GraphLogWindow::SetState(bool is_active) {
	this->visible = is_active;
	this->Show(is_active);
	if (is_active) {
		this->RAA_Ctrl->SetValue((int) (RA_aggr * 100));  
		this->RAH_Ctrl->SetValue((int) (RA_hysteresis * 100));
//Geoff		this->MM_Ctrl->SetValue(MinMotion);
		this->MDD_Ctrl->SetValue(Max_Dec_Dur);
		this->MRAD_Ctrl->SetValue(Max_RA_Dur);
		this->DM_Ctrl->SetSelection(Dec_guide);
		Refresh();
	}
}

void GraphLogWindow::AppendData(float dx, float dy, float RA, float Dec) {
	if (this->n_items < 500) {
		hdx[n_items]=dx;
		hdy[n_items]=dy;
		hra[n_items]=RA;
		hdec[n_items]=Dec;
		n_items++;
	}
	else {
		int i;
		for (i=0; i<499; i++) {
			hdx[i]=hdx[i+1];
			hdy[i]=hdy[i+1];
			hra[i]=hra[i+1];
			hdec[i]=hdec[i+1];
		}
		hdx[499]=dx;
		hdy[499]=dy;
		hra[499]=RA;
		hdec[499]=Dec;
	}
	if (this->visible)
		Refresh();
}

void GraphLogWindow::OnButtonClear(wxCommandEvent& WXUNUSED(evt)) {
	this->n_items = 0;
}

void GraphLogWindow::OnPaint(wxPaintEvent& WXUNUSED(evt)) {
	wxAutoBufferedPaintDC dc(this);

	const int xorig = 100;
	const int yorig = 102;
	int i,j;
	const int xmag = 500 / this->length;
	const int ymag = 25;

	dc.SetBackground(* wxBLACK_BRUSH);
	dc.SetBackground(wxColour(10,0,0));
	dc.Clear();
	wxPen GreyDashPen, BluePen, RedPen;
	GreyDashPen = wxPen(wxColour(200,200,200),1, wxDOT);
//	BluePen = wxPen(wxColour(0,0,255));
	BluePen = wxPen(RA_Color);
	RedPen = wxPen(DEC_Color);

	// Draw axes
	dc.SetPen(* wxGREY_PEN);
	dc.DrawLine(xorig,yorig,xorig+500,yorig);
	dc.DrawLine(xorig,yorig-100,xorig,yorig+100);

	// Draw horiz rule (scale is 1 pixel error per 25 pixels)
	dc.SetPen(GreyDashPen);
	dc.DrawLine(xorig,yorig+25,xorig+500,yorig+25);
	dc.DrawLine(xorig,yorig+50,xorig+500,yorig+50);
	dc.DrawLine(xorig,yorig+75,xorig+500,yorig+75);
	dc.DrawLine(xorig,yorig+100,xorig+500,yorig+100);
	dc.DrawLine(xorig,yorig-25,xorig+500,yorig-25);
	dc.DrawLine(xorig,yorig-50,xorig+500,yorig-50);
	dc.DrawLine(xorig,yorig-75,xorig+500,yorig-75);
	dc.DrawLine(xorig,yorig-100,xorig+500,yorig-100);

	// Draw vertical rule
	for (i=25; i<this->length; i+=25)
		dc.DrawLine(xorig+(i*xmag),yorig-100,xorig+(i*xmag),yorig+100);

	if (this->mode) {
		dc.SetTextForeground(RA_Color);
		dc.DrawText(_T("dx"),10,125);
		dc.SetTextForeground(DEC_Color);
		dc.DrawText(_T("dy"),60,125);
	}
	else {
		dc.SetTextForeground(RA_Color);
		dc.DrawText(_T("RA"),10,125);
		dc.SetTextForeground(DEC_Color);
		dc.DrawText(_T("Dec"),60,125);
	}


	// Draw data
	wxPoint Line1[500];
	wxPoint Line2[500];
	if (this->n_items) {
		int start_item = 0;
		if (this->n_items > this->length)
			start_item = this->n_items - this->length;
		j=0;
		if (mode) {
			for (i=start_item; i<this->n_items; i++,j++) {
				Line1[j]=wxPoint(xorig+(j*xmag),yorig + (int) (hdx[i] * (float) ymag));
				Line2[j]=wxPoint(xorig+(j*xmag),yorig + (int) (hdy[i] * (float) ymag));
			}
		}
		else {
			for (i=start_item; i<this->n_items; i++,j++) {
				Line1[j]=wxPoint(xorig+(j*xmag),yorig + (int) (hra[i] * (float) ymag));
				Line2[j]=wxPoint(xorig+(j*xmag),yorig + (int) (hdec[i] * (float) ymag));
			}
		}
		int plot_length = this->length;
		if (this->length > this->n_items)
			plot_length = this->n_items;
		dc.SetPen(BluePen);
		dc.DrawLines(plot_length,Line1);
		dc.SetPen(RedPen);
		dc.DrawLines(plot_length,Line2);

		// Figure oscillation score
		int same_sides = 0;
		float mean = 0.0;
		for (i=(start_item+1); i < this->n_items; i++) {
			if ( (hra[i] * hra[i-1]) > 0.0) same_sides++;
			mean = mean + hra[i];
		}
		if (n_items != start_item)
			mean = mean / (float) (n_items - start_item);
		else
			mean = 0.0;
		double RMS = 0.0;
		for (i=(start_item+1); i < this->n_items; i++) {
			RMS = RMS + (hra[i]-mean)*(hra[i]-mean);
		}
		if (n_items != start_item)
			RMS = sqrt(RMS/(float) (n_items - start_item));
		else
			RMS = 0.0;

		dc.SetTextForeground(*wxLIGHT_GREY);
		dc.DrawText(_T("Osc-Index"),10,145);
		dc.DrawText(wxString::Format(_T("RMS: %.2f"),RMS),10,180);
//		dc.SetTextForeground(wxColour(0,0,255));
		float osc_index = 0.0;
		if (n_items != start_item)
			osc_index= 1.0 - (float) same_sides / (float) (this->n_items - (start_item));
		if ((osc_index > 0.6) || (osc_index < 0.15))
			dc.SetTextForeground(wxColour(185,20,0));
		dc.DrawText(wxString::Format(_T("%.2f"),osc_index),10,160);
	}

}


BEGIN_EVENT_TABLE(ProfileWindow, wxMiniFrame)
EVT_PAINT(ProfileWindow::OnPaint)
EVT_LEFT_DOWN(ProfileWindow::OnLClick)
END_EVENT_TABLE()

ProfileWindow::ProfileWindow(wxWindow *parent):
#if defined (__APPLE__)
wxMiniFrame(parent,wxID_ANY,_T("Profile"),wxDefaultPosition,wxSize(50,77),wxCAPTION & ~wxSTAY_ON_TOP) {
#else
wxMiniFrame(parent,wxID_ANY,_T("Profile"),wxDefaultPosition,wxSize(55,90),wxCAPTION & ~wxSTAY_ON_TOP) {
#endif

	this->visible = false;
	this->mode = 0; // 2D profile
	this->data = NULL;
	this->SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	this->data = new unsigned short[441];  // 21x21 subframe

}

ProfileWindow::~ProfileWindow() {
	if (this->data) {
		delete [] this->data;
		this->data = NULL;
	}
}

void ProfileWindow::OnLClick(wxMouseEvent& WXUNUSED(mevent)) {
	this->mode = this->mode + 1;
	if (this->mode > 2) this->mode = 0;
	Refresh();
}

void ProfileWindow::SetState(bool is_active) {
	this->visible = is_active;
	this->Show(is_active);
	if (is_active)
		Refresh();
}

void ProfileWindow::UpdateData(usImage& img, float xpos, float ypos) {
	if (this->data == NULL) return;
	int xstart = ROUND(xpos) - 10;
	int ystart = ROUND(ypos) - 10;
	if (xstart < 0) xstart = 0;
	else if (xstart > (img.Size.GetWidth() - 22))
		xstart = img.Size.GetWidth() - 22;
	if (ystart < 0) ystart = 0;
	else if (ystart > (img.Size.GetHeight() - 22))
	ystart = img.Size.GetHeight() - 22;

	int x,y;
	unsigned short *uptr = this->data;
	const int xrowsize = img.Size.GetWidth();
	for (x=0; x<21; x++)
		horiz_profile[x] = vert_profile[x] = midrow_profile[x] = 0;
	for (y=0; y<21; y++) {
		for (x=0; x<21; x++, uptr++) {
			*uptr = *(img.ImageData + xstart + x + (ystart + y) * xrowsize);
			horiz_profile[x] += (int) *uptr;
			vert_profile[y] += (int) *uptr;
		}
	}
	uptr = this->data + 210;
	for (x=0; x<21; x++, uptr++)
		midrow_profile[x] = (int) *uptr;
	if (this->visible)
		Refresh();

}

void ProfileWindow::OnPaint(wxPaintEvent& WXUNUSED(evt)) {
	wxAutoBufferedPaintDC dc(this);
	wxPoint Prof[21];

	dc.SetBackground(* wxBLACK_BRUSH);
	dc.SetBackground(wxColour(10,30,30));
	dc.Clear();
	if (frame->canvas->State == STATE_NONE) return;
	wxPen RedPen;
//	GreyDashPen = wxPen(wxColour(200,200,200),1, wxDOT);
//	BluePen = wxPen(wxColour(100,100,255));
	RedPen = wxPen(wxColour(255,0,0));

	int i;
	int *profptr;
	wxString label;
	switch (this->mode) {  // Figure which profile to use
		case 0: // mid-row
			profptr = midrow_profile;
			label = _T("Mid row");
			break;
		case 1: // avg row
			profptr = horiz_profile;
			label = _T("Avg row");
			break;
		case 2:
			profptr = vert_profile;
			label = _T("Avg col");
			break;
		default:
			profptr = midrow_profile;
			label = _T("Mid row");
			break;
	}

	// Figure max and min
	int Prof_Min, Prof_Max;
	Prof_Min = Prof_Max = *profptr;

	for (i=1; i<21; i++) {
		if (*(profptr + i) < Prof_Min)
			Prof_Min = *(profptr + i);
		else if (*(profptr + i) > Prof_Max)
			Prof_Max = *(profptr + i);
	}
	// Figure the actual points in the window
	int Prof_Range = (Prof_Max - Prof_Min) / 42;  //
	if (!Prof_Range) Prof_Range = 1;
	for (i=0; i<21; i++)
		Prof[i]=wxPoint(5+i*2,45-( (*(profptr + i) - Prof_Min) / Prof_Range ));

	// Draw it
	dc.SetPen(RedPen);
	dc.DrawLines(21,Prof);
	dc.SetTextForeground(wxColour(100,100,255));
#if defined (__APPLE__)
	dc.SetFont(*wxSMALL_FONT);
#else
	dc.SetFont(*wxSWISS_FONT);
#endif
	dc.DrawText(label,2,47);
}

