/*
 *  canvas.cpp
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
#include "image_math.h"
#include <wx/bitmap.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#if defined (__APPLE__)
#include "../cfitsio/fitsio.h"
#else
#include <fitsio.h>
#endif

#define SCALE_UP_SMALL  // Currently problematic as the box for the star is drawn in the wrong spot.
#if ((wxMAJOR_VERSION < 3) && (wxMINOR_VERSION < 9))
#define wxPENSTYLE_DOT wxDOT
#endif

BEGIN_EVENT_TABLE(MyCanvas, wxWindow)
   EVT_PAINT(MyCanvas::OnPaint)
	//EVT_MOTION(MyCanvas::OnMouse)
 	EVT_LEFT_DOWN(MyCanvas::OnLClick)
	//EVT_RIGHT_DOWN(MyCanvas::OnRClick)
	//EVT_KEY_DOWN(MyCanvas::CheckAbort)
	EVT_ERASE_BACKGROUND(MyCanvas::OnErase)
END_EVENT_TABLE()


// Define a constructor for my canvas
MyCanvas::MyCanvas(wxWindow *parent):
 wxWindow(parent, wxID_ANY,wxPoint(0,0),wxSize(XWinSize,YWinSize))  {

//	Origin_x = Origin_y = Targ_x = Targ_y = 0;
	State = STATE_NONE;
	ScaleFactor = 1.0;
	binned = false;
	Displayed_Image = new wxImage(XWinSize,YWinSize,true);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	SetBackgroundColour(wxColour((unsigned char) 30, (unsigned char) 30,(unsigned char) 30));
}

MyCanvas::~MyCanvas() {
	delete Displayed_Image;
}

void MyCanvas::OnErase(wxEraseEvent &evt) {
	//if (CaptureActive) return;
	evt.Skip();
 }
void MyCanvas::OnLClick(wxMouseEvent &mevent) {
	if (State > STATE_SELECTED) {
		mevent.Skip();
		return;
	}
	if (mevent.m_shiftDown) {  // clear them out
//		Origin_x = Origin_y = Targ_x = Targ_y = 0;
		StarX = StarY = LockX = LockY = 0.0;
		State = STATE_NONE;
	}
	else if ((mevent.m_x <= SearchRegion) || (mevent.m_x >= (XWinSize+SearchRegion)) || (mevent.m_y <= SearchRegion) || (mevent.m_y >= (XWinSize+SearchRegion))) {
		mevent.Skip();
		return;
	}
	else if (!CurrentFullFrame.NPixels){
		mevent.Skip();
		return;
	}

	else {
		StarX = (double) mevent.m_x / ScaleFactor;
		StarY = (double) mevent.m_y / ScaleFactor;
		State = STATE_SELECTED;
		//Abort = true;  // If looping now, stop
		dX = dY = 0.0;
		FindStar(CurrentFullFrame);
		//LockX = StarX;
		//LockY = StarY;
		frame->SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"),StarMass,StarSNR));
		//Targ_x = (int) StarX;
		//Targ_y = (int) StarY;
		//Origin_x = Targ_x;
		//Origin_y = Targ_y;
	}
	Refresh();

}

/*
void MyCanvas::OnMouse(wxMouseEvent &mevent) {

	CalcUnscrolledPosition(mevent.m_x,mevent.m_y,&mouse_x,&mouse_y);
	mouse_x = (int) ((double) mouse_x / Disp_ZoomFactor);
	mouse_y = (int) ((double) mouse_y / Disp_ZoomFactor);
	if (mouse_x >= (int) CurrImage.Size[0]) mouse_x = (int) CurrImage.Size[0]-1;
	if (mouse_y >= (int) CurrImage.Size[1]) mouse_y = (int) CurrImage.Size[1]-1;
	if (CurrImage.RawPixels) {
		frame->SetStatusText(wxString::Format("%d,%d = %.1f",mouse_x,mouse_y,*(CurrImage.RawPixels + mouse_x + mouse_y*CurrImage.Size[0])),2);
		if (frame->Info_Dialog->IsShown())
			ShowLocalStats();
	}
	else
		frame->SetStatusText(wxString::Format("%d,%d",mouse_x,mouse_y),2);
 }



 void MyCanvas::OnRClick(wxMouseEvent &mevent) {
	int click_x, click_y;
	wxString info_string;


	if (mevent.m_shiftDown) {  // clear them out
		n_targ = 0;
	}
	else {
		CalcUnscrolledPosition(mevent.m_x,mevent.m_y,&click_x,&click_y);
		click_x = (int) ((double) click_x / Disp_ZoomFactor);
		click_y = (int) ((double) click_y  / Disp_ZoomFactor);
		if (click_x > CurrImage.Size[0]) click_x = CurrImage.Size[0];
		if (click_y > CurrImage.Size[1]) click_y = CurrImage.Size[1];

		n_targ = (n_targ % 3) + 1;
		targ_x[n_targ-1] = click_x;
		targ_y[n_targ-1] = click_y;
	}
	Refresh();

}


void MyCanvas::CheckAbort(wxKeyEvent& event) {

	long keycode = event.GetKeyCode();
	if (keycode == WXK_ESCAPE)
		Capture_Abort = true;
	else
		event.Skip();
}
*/

void MyCanvas::FullFrameToDisplay() {
	int blevel, wlevel;

	CurrentFullFrame.CalcStats();
	blevel = CurrentFullFrame.Min; //CurrentFullFrame.Min;
	wlevel = CurrentFullFrame.FiltMax ;  //CurrentFullFrame.Max / 2;
//	blevel = CurrentFullFrame.Min;
//	wlevel = CurrentFullFrame.Max / 2;
    

	if (CurrentFullFrame.Size.GetWidth() >= 1280) {
		CurrentFullFrame.BinnedCopyToImage(&Displayed_Image,blevel,wlevel,frame->Stretch_gamma);
		binned = true;
//		ScaleFactor = 0.5;
	}
	else {
		CurrentFullFrame.CopyToImage(&Displayed_Image,blevel,wlevel,frame->Stretch_gamma);
		binned = false;
//		ScaleFactor = 1.0;
	}
//	frame->SetStatusText(wxString::Format("%d, %d: %d, %d",CurrentFullFrame.Min,CurrentFullFrame.Max,CurrentFullFrame.FiltMin,CurrentFullFrame.FiltMax),1);
	Refresh();
}


// Define the repainting behaviour
void MyCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxAutoBufferedPaintDC dc(this);
	wxBitmap* DisplayedBitmap = NULL;
	wxMemoryDC memDC;

	if (binned) ScaleFactor = 0.5;
	else ScaleFactor = 1.0;
	if ( (Displayed_Image->Ok()) && (Displayed_Image->GetWidth()) ) {
		if ((Displayed_Image->GetWidth() != XWinSize) || (Displayed_Image->GetHeight() != YWinSize)) {
#if defined  SCALE_UP_SMALL
			if (Displayed_Image->GetWidth() != XWinSize) { // must get x-size to 640 -- have room in the 512 ysize for 640x480 - 640x512 already
				ScaleFactor = ScaleFactor * (double) XWinSize / (double) Displayed_Image->GetWidth();
				int orig_size = Displayed_Image->GetHeight();
				if (binned) orig_size *= 2;
				int new_size = (int) ((float) orig_size * ScaleFactor);
				DisplayedBitmap = new wxBitmap(Displayed_Image->Scale(XWinSize,new_size));
			}
			else  // x-dim OK, just pad / crop y (should just pad)
#endif
				DisplayedBitmap = new wxBitmap(Displayed_Image->Size(wxSize(XWinSize,YWinSize),wxPoint(0,0)));
			
			memDC.SelectObject(*DisplayedBitmap);
//			frame->SetStatusText(wxString::Format("scaled %d %d %.2f",Displayed_Image->GetWidth(),Displayed_Image->GetHeight(),ScaleFactor),1);
		}
		else {  // No scaling required
			DisplayedBitmap = new wxBitmap(*Displayed_Image);
			memDC.SelectObject(*DisplayedBitmap);
			//frame->SetStatusText(wxString::Format("%.2f",ScaleFactor),1);
		}
		if (!DisplayedBitmap) {
			wxMessageBox(wxString::Format(_T("hmmm %d %d %f"),Displayed_Image->GetWidth(),Displayed_Image->GetHeight(),ScaleFactor));
			return;
		}
		try {
			dc.Blit(0, 0, DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight(), & memDC, 0, 0, wxCOPY, false);
//			dc.Blit(0, 0, XWinSize,YWinSize, & memDC, 0, 0, wxCOPY, false);
		}
		catch (...) {
			wxMessageBox(wxString::Format(_T("hmmm2 %d %d %f"),Displayed_Image->GetWidth(),Displayed_Image->GetHeight(),ScaleFactor));
			return;


		}
		if (State == STATE_SELECTED) {
//			int xorig = ROUND(StarX);
//			int yorig = ROUND(StarY);
			if (FoundStar)
//				dc.SetPen(wxPen(wxColour(32,196,32),1,wxSOLID ));  // Draw the box around the star
				dc.SetPen(wxPen(wxColour(100,255,90),1,wxSOLID ));  // Draw the box around the star
			else
				dc.SetPen(wxPen(wxColour(230,130,30),1,wxDOT ));
			//dc.SetPen(* wxGREEN_PEN);
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*ScaleFactor)-SearchRegion,ROUND(StarY*ScaleFactor)-SearchRegion,SearchRegion*2+1,SearchRegion*2+1);
			//dc.DrawLine(xorig-12,yorig,xorig-6,yorig);
			//dc.DrawLine(xorig+11,yorig,xorig+6,yorig);
			//dc.DrawLine(xorig,yorig-12,xorig,yorig-6);
			//dc.DrawLine(xorig,yorig+11,xorig,yorig+6);
		//	frame->SetStatusText(wxString::Format("%.1f %.1f %d %d",StarX,StarY,xorig,yorig));
		}
		else if (State == STATE_CALIBRATING) {  // in the cal process
			dc.SetPen(wxPen(wxColour(32,196,32),1,wxSOLID ));  // Draw the box around the star
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*ScaleFactor)-SearchRegion,ROUND(StarY*ScaleFactor)-SearchRegion,SearchRegion*2+1,SearchRegion*2+1);
			dc.SetPen(wxPen(wxColor(255,255,0),1,wxDOT));
			//dc.CrossHair(ROUND(LockX*ScaleFactor),ROUND(LockY*ScaleFactor));  // Draw the cross-hair on the origin
			dc.DrawLine(0, LockY*ScaleFactor, XWinSize, LockY*ScaleFactor);
			dc.DrawLine(LockX*ScaleFactor, 0, LockX*ScaleFactor, YWinSize);

		}
		else if (State == STATE_GUIDING_LOCKED) { // locked and guiding
			if (FoundStar)
				dc.SetPen(wxPen(wxColour(32,196,32),1,wxSOLID ));  // Draw the box around the star
			else
				dc.SetPen(wxPen(wxColour(230,130,30),1,wxDOT ));
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*ScaleFactor)-SearchRegion,ROUND(StarY*ScaleFactor)-SearchRegion,SearchRegion*2+1,SearchRegion*2+1);
			dc.SetPen(wxPen(wxColor(0,255,0)));
			//dc.CrossHair(ROUND(LockX*ScaleFactor),ROUND(LockY*ScaleFactor));  // Draw the cross-hair on the origin
			dc.DrawLine(0, LockY*ScaleFactor, XWinSize, LockY*ScaleFactor);
			dc.DrawLine(LockX*ScaleFactor, 0, LockX*ScaleFactor, YWinSize);

		}
		if (OverlayMode) {
			dc.SetPen(wxPen(wxColor(200,50,50)));
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			if (OverlayMode == 1) {
				int cx = XWinSize / 2;
				int cy = YWinSize / 2;
				dc.DrawCircle(cx,cy,25);
				dc.DrawCircle(cx,cy,50);
				dc.DrawCircle(cx,cy,100);
				dc.DrawLine(0, cy, XWinSize, cy);
				dc.DrawLine(cx, 0, cx, YWinSize);
				//dc.CrossHair(cx,cy);
			}
			else if ((OverlayMode == 2) || (OverlayMode == 3)){
				int i;
				int size = (OverlayMode - 1) * 20;
				for (i=size; i<XWinSize; i+=size)
					dc.DrawLine(i,0,i,YWinSize);
				for (i=size; i<YWinSize; i+=size)
					dc.DrawLine(0,i,XWinSize,i);

			}
			else if (OverlayMode == 4) { // RA and Dec
				double r=30.0;
				double cos_angle = cos(pScope->RaAngle());
				double sin_angle = sin(pScope->RaAngle());
				dc.SetPen(wxPen(frame->GraphLog->RA_Color,2,wxPENSTYLE_DOT));
			/*	dc.DrawLine(ROUND(LockX*ScaleFactor+r*cos_angle),ROUND(LockY*ScaleFactor+r*sin_angle),
					ROUND(LockX*ScaleFactor-r*cos_angle),ROUND(LockY*ScaleFactor-r*sin_angle));
				dc.SetPen(wxPen(frame->GraphLog->RA_Color,1,wxPENSTYLE_SOLID ));*/
				r=15.0;
				dc.DrawLine(ROUND(StarX*ScaleFactor+r*cos_angle),ROUND(StarY*ScaleFactor+r*sin_angle),
					ROUND(StarX*ScaleFactor-r*cos_angle),ROUND(StarY*ScaleFactor-r*sin_angle));
				dc.SetPen(wxPen(frame->GraphLog->DEC_Color,2,wxPENSTYLE_DOT));
				cos_angle = cos(pScope->DecAngle());
				sin_angle = sin(pScope->DecAngle());
				dc.DrawLine(ROUND(StarX*ScaleFactor+r*cos_angle),ROUND(StarY*ScaleFactor+r*sin_angle),
					ROUND(StarX*ScaleFactor-r*cos_angle),ROUND(StarY*ScaleFactor-r*sin_angle));

				wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
				gc->SetPen(wxPen(frame->GraphLog->RA_Color,1,wxPENSTYLE_DOT ));
				wxGraphicsPath path = gc->CreatePath();
//				wxGraphicsMatrix mat = gc->CreateMatrix();
				int i;
				double step = (double) YWinSize / 10.0;

				double MidX = (double) XWinSize / 2.0;
				double MidY = (double) YWinSize / 2.0;
				gc->Rotate(pScope->RaAngle());
				gc->GetTransform().TransformPoint(&MidX, &MidY);
				gc->Rotate(-pScope->RaAngle());
				gc->Translate((double) XWinSize / 2.0 - MidX, (double) YWinSize / 2.0 - MidY);
				gc->Rotate(pScope->RaAngle());
				for (i=-2; i<12; i++) {
//					gc->StrokeLine((double) XWinSize / (double) (i-1),0.0,
//						(double) XWinSize / (double) (i-1), (double) YWinSize);
					gc->StrokeLine(0.0,step * (double) i,
						(double) XWinSize, step * (double) i);
				}

				MidX = (double) XWinSize / 2.0;
				MidY = (double) YWinSize / 2.0;
				gc->Rotate(-pScope->RaAngle());
				gc->Rotate(pScope->DecAngle());
				gc->GetTransform().TransformPoint(&MidX, &MidY);
				gc->Rotate(-pScope->DecAngle());
				gc->Translate((double) XWinSize / 2.0 - MidX, (double) YWinSize / 2.0 - MidY);
				gc->Rotate(pScope->DecAngle());
				gc->SetPen(wxPen(frame->GraphLog->DEC_Color,1,wxPENSTYLE_DOT ));
				for (i=-2; i<12; i++) {
					gc->StrokeLine(0.0,step * (double) i,
						(double) XWinSize, step * (double) i);
				}


				delete gc;

			}
		}
		if ((Log_Images==1) && (State >= STATE_SELECTED)) {  // Save star image as a JPEG
			wxBitmap SubBmp(60,60,-1);
			wxMemoryDC tmpMdc;
			tmpMdc.SelectObject(SubBmp);
			memDC.SetPen(wxPen(wxColor(0,255,0),1,wxDOT));
			//memDC.CrossHair(ROUND(LockX*ScaleFactor),ROUND(LockY*ScaleFactor));  // Draw the cross-hair on the origin
			memDC.DrawLine(0, LockY*ScaleFactor, XWinSize, LockY*ScaleFactor);
			memDC.DrawLine(LockX*ScaleFactor, 0, LockX*ScaleFactor, YWinSize);
	#ifdef __APPLEX__
			tmpMdc.Blit(0,0,60,60,&memDC,ROUND(StarX*ScaleFactor)-30,Displayed_Image->GetHeight() - ROUND(StarY*ScaleFactor)-30,wxCOPY,false);
	#else
			tmpMdc.Blit(0,0,60,60,&memDC,ROUND(StarX*ScaleFactor)-30,ROUND(StarY*ScaleFactor)-30,wxCOPY,false);
	#endif

			//			tmpMdc.Blit(0,0,200,200,&Cdc,0,0,wxCOPY);
			wxString fname = LogFile->GetName();
			wxDateTime CapTime;
			CapTime=wxDateTime::Now();
			//full_fname = base_name + CapTime.Format("_%j_%H%M%S.fit");
			fname = fname.BeforeLast('.') + CapTime.Format(_T("_%j_%H%M%S")) + _T(".jpg");
			SubBmp.SaveFile(fname,wxBITMAP_TYPE_JPEG);
			tmpMdc.SelectObject(wxNullBitmap);
		}
		else if ((Log_Images==2) && (State >= STATE_SELECTED)) { // Save star image as a FITS
			SaveStarFITS();
		}
		


/*		if (Log_Images && (State >= STATE_SELECTED) ) {  //GUIDING_LOCKED
			//wxBitmap SubBmp = DisplayedBitmap->GetSubBitmap(wxRect(ROUND(LockX*ScaleFactor)-20,ROUND(LockY*ScaleFactor)-20,40,40));
			wxBitmap SubBmp(60,60);
			wxMemoryDC tmpMdc;
			tmpMdc.SelectObject(SubBmp);
//			tmpMdc.Blit(0,0,60,60,&dc,ROUND(LockX*ScaleFactor)-30,ROUND(LockY*ScaleFactor)-30);
//			tmpMdc.Blit(0,0,60,60,&dc,ROUND(StarX*ScaleFactor)-30,ROUND(StarY*ScaleFactor)-30,wxCOPY);
			tmpMdc.Blit(0,0,60,60,&dc,0,0,wxCOPY);
			wxString fname = LogFile->GetName();
			fname = fname.BeforeLast('.') + wxString::Format("_Star_%d.jpg",FrameTime);
			SubBmp.SaveFile(fname,wxBITMAP_TYPE_JPEG);
			tmpMdc.SelectObject(wxNullBitmap);
		}*/
		memDC.SelectObject(wxNullBitmap);
	}
	delete DisplayedBitmap;

}



void MyCanvas::SaveStarFITS() {
	usImage tmpimg;
	tmpimg.Init(60,60);
	int start_x = ROUND(StarX)-30;
	int start_y = ROUND(StarY)-30;
	if ((start_x + 60) > CurrentFullFrame.Size.GetWidth())
		start_x = CurrentFullFrame.Size.GetWidth() - 60;
	if ((start_y + 60) > CurrentFullFrame.Size.GetHeight())
		start_y = CurrentFullFrame.Size.GetHeight() - 60;
	int x,y, width;
	width = CurrentFullFrame.Size.GetWidth();
	unsigned short *usptr = tmpimg.ImageData;
	for (y=0; y<60; y++)
		for (x=0; x<60; x++, usptr++)
			*usptr = *(CurrentFullFrame.ImageData + (y+start_y)*width + (x+start_x));
	wxString fname = LogFile->GetName();
	wxDateTime CapTime;
	CapTime=wxDateTime::Now();
	fname = fname.BeforeLast('.') + CapTime.Format(_T("_%j_%H%M%S")) + _T(".fit");

	fitsfile *fptr;  // FITS file pointer
	int status = 0;  // CFITSIO status value MUST be initialized to zero!
	long fpixel[3] = {1,1,1};
	long fsize[3];
	char keyname[9]; // was 9
	char keycomment[100];
	char keystring[100];
	int output_format=USHORT_IMG;

	fsize[0] = 60;
	fsize[1] = 60;
	fsize[2] = 0;
	fits_create_file(&fptr,(const char*) fname.mb_str(wxConvUTF8),&status);
	if (!status) {
		fits_create_img(fptr,output_format, 2, fsize, &status);

		time_t now;
		struct tm *timestruct;
		time(&now);
		timestruct=gmtime(&now);
		sprintf(keyname,"DATE");
		sprintf(keycomment,"UTC date that FITS file was created");
		sprintf(keystring,"%.4d-%.2d-%.2d %.2d:%.2d:%.2d",timestruct->tm_year+1900,timestruct->tm_mon+1,timestruct->tm_mday,timestruct->tm_hour,timestruct->tm_min,timestruct->tm_sec);
		if (!status) fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);

		sprintf(keyname,"DATE-OBS");
		sprintf(keycomment,"YYYY-MM-DDThh:mm:ss observation start, UT");
		sprintf(keystring,"%s",(const char*) CurrentFullFrame.ImgStartDate.c_str());
		if (!status) fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);

		sprintf(keyname,"EXPOSURE");
		sprintf(keycomment,"Exposure time [s]");
		float dur = (float) CurrentFullFrame.ImgExpDur / 1000.0;
		if (!status) fits_write_key(fptr, TFLOAT, keyname, &dur, keycomment, &status);

		unsigned int tmp = 1;
		sprintf(keyname,"XBINNING");
		sprintf(keycomment,"Camera binning mode");
		fits_write_key(fptr, TUINT, keyname, &tmp, keycomment, &status);
		sprintf(keyname,"YBINNING");
		sprintf(keycomment,"Camera binning mode");
		fits_write_key(fptr, TUINT, keyname, &tmp, keycomment, &status);
		
		sprintf(keyname,"XORGSUB");
		sprintf(keycomment,"Subframe x position in binned pixels");
		tmp = start_x;
		fits_write_key(fptr, TINT, keyname, &tmp, keycomment, &status);
		sprintf(keyname,"YORGSUB");
		sprintf(keycomment,"Subframe y position in binned pixels");
		tmp = start_y;
		fits_write_key(fptr, TINT, keyname, &tmp, keycomment, &status);
		
		
		if (!status) fits_write_pix(fptr,TUSHORT,fpixel,tmpimg.NPixels,tmpimg.ImageData,&status);

	}
	fits_close_file(fptr,&status);

}
