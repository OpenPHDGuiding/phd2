/*
 *  guide_onestar.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Greatly expanded by Bret McKee
 *  Copyright (c) 2012 Bret McKee
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

#if defined (__APPLE__)
#include "../cfitsio/fitsio.h"
#else
#include <fitsio.h>
#endif

#define SCALE_UP_SMALL  // Currently problematic as the box for the star is drawn in the wrong spot.
#if ((wxMAJOR_VERSION < 3) && (wxMINOR_VERSION < 9))
#define wxPENSTYLE_DOT wxDOT
#endif

BEGIN_EVENT_TABLE(GuiderOneStar, Guider)
    EVT_PAINT(GuiderOneStar::OnPaint)
 	EVT_LEFT_DOWN(GuiderOneStar::OnLClick)
END_EVENT_TABLE()

// Define a constructor for the guide canvas
GuiderOneStar::GuiderOneStar(wxWindow *parent):
    Guider(parent, XWinSize, YWinSize)
{
}

GuiderOneStar::~GuiderOneStar() 
{
}

Point &GuiderOneStar::LockPosition()
{
    return m_lockPosition;
}

Point &GuiderOneStar::CurrentPosition()
{
    return m_star;
}

bool GuiderOneStar::SetState(E_GUIDER_STATES newState)
{
    bool bError = false;

    try
    {
        if (newState > m_state + 1)
        {
            assert(false);
            throw ERROR_INFO("Illegal state transition");
        }

        switch(newState)
        {
            case STATE_UNINITIALIZED:
                m_lockPosition.Invalidate();
                m_star.Invalidate();

                // for onestar guiders, we fall immediately from STATE_UNINITIALIZED
                // to STATE_SELECTING

                newState = STATE_SELECTING;
                break;
            case STATE_SELECTED:
                m_lockPosition = m_star;
                break;
            case STATE_CALIBRATING:
                pScope->BeginCalibration(this);
                break;
            case STATE_CALIBRATED:
                m_lockPosition = m_star;
                newState = STATE_GUIDING_LOCKED;
                break;
        }

        bError = Guider::SetState(newState);

        frame->UpdateButtonsStatus();
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    return bError;
}

/*************  A new image is ready ************************/

void GuiderOneStar::UpdateGuideState(usImage *pImage, bool bUpdateStatus)
{
    switch (m_state)
    {
        case STATE_UNINITIALIZED:
        case STATE_SELECTING:
            if (bUpdateStatus)
            {
                frame->SetStatusText(_T("No Star found"));
            }
            break;
        default:
            m_star.Find(pImage);
            
            if (bUpdateStatus)
            {
                if (m_star.WasFound())
                {
                    frame->SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"), m_star.Mass, m_star.SNR));
                }
                else
                {
                    frame->SetStatusText(_T("No Star found"));
                }
            }
            break;
    }

    UpdateImageDisplay(pCurrentFullFrame);
}

void GuiderOneStar::OnLClick(wxMouseEvent &mevent) {
    try
    {
        if (m_state > STATE_SELECTED) 
        {
            mevent.Skip();
            throw ERROR_INFO("Skipping event because state > STATE_SELECTED");
        }

        if (mevent.m_shiftDown) 
        {  
            // clear them out
            SetState(STATE_UNINITIALIZED);
        }
        else
        {
            if ((mevent.m_x <= SearchRegion) || (mevent.m_x >= (XWinSize+SearchRegion)) || (mevent.m_y <= SearchRegion) || (mevent.m_y >= (XWinSize+SearchRegion))) 
            {
                mevent.Skip();
                throw ERROR_INFO("Skipping event because click outside of search region");
            }

            if (pCurrentFullFrame->NPixels == 0)
            {
                mevent.Skip();
                throw ERROR_INFO("Skipping event pCurrentFullFrame->NPixels == 0");
            }

            double StarX = (double) mevent.m_x / m_scaleFactor;
            double StarY = (double) mevent.m_y / m_scaleFactor;

            m_star.Find(pCurrentFullFrame, StarX, StarY);

#if 0
            SetState(STATE_SELECTED);
            frame->SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"),m_star.Mass,m_star.SNR));
#else
            if (m_star.WasFound())
            {
                SetState(STATE_SELECTED);
                frame->SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"),m_star.Mass,m_star.SNR));
            }
            else
            {
                m_star.Invalidate();
                frame->SetStatusText(wxString::Format(_T("No star found")));
            }
#endif

            Refresh();
        }
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
    }
}

// Define the repainting behaviour
void GuiderOneStar::OnPaint(wxPaintEvent& event) 
{
	wxAutoBufferedPaintDC dc(this);
	wxMemoryDC memDC;

	if (!PaintHelper(dc, memDC))
    {
        // PaintHelper drew the image and any overlays
        // now decorate the image to show the selection
        
        bool FoundStar = m_star.WasFound();
        double StarX = m_star.X;
        double StarY = m_star.Y;

        double LockX = m_lockPosition.X;
        double LockY = m_lockPosition.Y;
        
		if (m_state == STATE_SELECTED) {

			if (FoundStar)
				dc.SetPen(wxPen(wxColour(100,255,90),1,wxSOLID ));  // Draw the box around the star
			else
				dc.SetPen(wxPen(wxColour(230,130,30),1,wxDOT ));
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*m_scaleFactor)-SearchRegion,ROUND(StarY*m_scaleFactor)-SearchRegion,SearchRegion*2+1,SearchRegion*2+1);
		}
		else if (m_state == STATE_CALIBRATING) {  // in the cal process
			dc.SetPen(wxPen(wxColour(32,196,32),1,wxSOLID ));  // Draw the box around the star
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*m_scaleFactor)-SearchRegion,ROUND(StarY*m_scaleFactor)-SearchRegion,SearchRegion*2+1,SearchRegion*2+1);
			dc.SetPen(wxPen(wxColor(255,255,0),1,wxDOT));
			dc.DrawLine(0, LockY*m_scaleFactor, XWinSize, LockY*m_scaleFactor);
			dc.DrawLine(LockX*m_scaleFactor, 0, LockX*m_scaleFactor, YWinSize);

		}
		else if (m_state == STATE_GUIDING_LOCKED) { // locked and guiding
			if (FoundStar)
				dc.SetPen(wxPen(wxColour(32,196,32),1,wxSOLID ));  // Draw the box around the star
			else
				dc.SetPen(wxPen(wxColour(230,130,30),1,wxDOT ));
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*m_scaleFactor)-SearchRegion,ROUND(StarY*m_scaleFactor)-SearchRegion,SearchRegion*2+1,SearchRegion*2+1);
			dc.SetPen(wxPen(wxColor(0,255,0)));
			dc.DrawLine(0, LockY*m_scaleFactor, XWinSize, LockY*m_scaleFactor);
			dc.DrawLine(LockX*m_scaleFactor, 0, LockX*m_scaleFactor, YWinSize);

		}
		if ((Log_Images==1) && (m_state >= STATE_SELECTED)) {  // Save star image as a JPEG
			wxBitmap SubBmp(60,60,-1);
			wxMemoryDC tmpMdc;
			tmpMdc.SelectObject(SubBmp);
			memDC.SetPen(wxPen(wxColor(0,255,0),1,wxDOT));
			//memDC.CrossHair(ROUND(LockX*m_scaleFactor),ROUND(LockY*m_scaleFactor));  // Draw the cross-hair on the origin
			memDC.DrawLine(0, LockY*m_scaleFactor, XWinSize, LockY*m_scaleFactor);
			memDC.DrawLine(LockX*m_scaleFactor, 0, LockX*m_scaleFactor, YWinSize);
#ifdef __APPLEX__
			tmpMdc.Blit(0,0,60,60,&memDC,ROUND(StarX*m_scaleFactor)-30,Displayed_Image->GetHeight() - ROUND(StarY*m_scaleFactor)-30,wxCOPY,false);
#else
			tmpMdc.Blit(0,0,60,60,&memDC,ROUND(StarX*m_scaleFactor)-30,ROUND(StarY*m_scaleFactor)-30,wxCOPY,false);
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
		else if ((Log_Images==2) && (m_state >= STATE_SELECTED)) { // Save star image as a FITS
			SaveStarFITS();
		}
		memDC.SelectObject(wxNullBitmap);
	}
}

void GuiderOneStar::SaveStarFITS() {
    double StarX = m_star.X;
    double StarY = m_star.Y;

	usImage tmpimg;
	tmpimg.Init(60,60);
	int start_x = ROUND(StarX)-30;
	int start_y = ROUND(StarY)-30;
	if ((start_x + 60) > pCurrentFullFrame->Size.GetWidth())
		start_x = pCurrentFullFrame->Size.GetWidth() - 60;
	if ((start_y + 60) > pCurrentFullFrame->Size.GetHeight())
		start_y = pCurrentFullFrame->Size.GetHeight() - 60;
	int x,y, width;
	width = pCurrentFullFrame->Size.GetWidth();
	unsigned short *usptr = tmpimg.ImageData;
	for (y=0; y<60; y++)
		for (x=0; x<60; x++, usptr++)
			*usptr = *(pCurrentFullFrame->ImageData + (y+start_y)*width + (x+start_x));
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
		sprintf(keystring,"%s",(const char*) pCurrentFullFrame->ImgStartDate.c_str());
		if (!status) fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);

		sprintf(keyname,"EXPOSURE");
		sprintf(keycomment,"Exposure time [s]");
		float dur = (float) pCurrentFullFrame->ImgExpDur / 1000.0;
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
