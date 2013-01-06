/*
 *  guider_onestar.cpp
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

static const double DefaultMassChangeThreshold = 0.5;
static const int DefaultSearchRegion = 15;
static const GUIDE_ALGORITHM DefaultRaGuideAlgorithm = GUIDE_ALGORITHM_HYSTERESIS;
static const GUIDE_ALGORITHM DefaultDecGuideAlgorithm = GUIDE_ALGORITHM_RESIST_SWITCH;

BEGIN_EVENT_TABLE(GuiderOneStar, Guider)
    EVT_PAINT(GuiderOneStar::OnPaint)
 	EVT_LEFT_DOWN(GuiderOneStar::OnLClick)
END_EVENT_TABLE()

// Define a constructor for the guide canvas
GuiderOneStar::GuiderOneStar(wxWindow *parent):
    Guider(parent, XWinSize, YWinSize)
{
    double massChangeThreshold  = pConfig->GetDouble("/guider/onestar/MassChangeThreshold", 
            DefaultMassChangeThreshold);
    SetMassChangeThreshold(massChangeThreshold);

    int searchRegion = pConfig->GetInt("/guider/onestar/SearchRegion", DefaultSearchRegion);
    SetSearchRegion(searchRegion);

    int raGuideAlgorithm = pConfig->GetInt("/guider/onestar/RaGuideAlgorithm", DefaultRaGuideAlgorithm);
    SetRaGuideAlgorithm(raGuideAlgorithm);

    int decGuideAlgorithm = pConfig->GetInt("/guider/onestar/DecGuideAlgorithm", DefaultDecGuideAlgorithm);
    SetDecGuideAlgorithm(decGuideAlgorithm);

    SetState(STATE_UNINITIALIZED);
}

GuiderOneStar::~GuiderOneStar() 
{
}

bool GuiderOneStar::SetDecGuideAlgorithm(int decGuideAlgorithm)
{

    bool bError = Guider::SetDecGuideAlgorithm(decGuideAlgorithm);

    if (bError)
    {
        Guider::SetDecGuideAlgorithm(DefaultDecGuideAlgorithm);
    }
    pConfig->SetInt("/guider/onestar/DecGuideAlgorithm", GetDecGuideAlgorithm());

    return bError;
}

bool GuiderOneStar::SetRaGuideAlgorithm(int raGuideAlgorithm)
{

    bool bError = Guider::SetRaGuideAlgorithm(raGuideAlgorithm);

    if (bError)
    {
        Guider::SetRaGuideAlgorithm(DefaultRaGuideAlgorithm);
    }
    pConfig->SetInt("/guider/onestar/RaGuideAlgorithm", GetRaGuideAlgorithm());

    return bError;
}

double GuiderOneStar::GetMassChangeThreshold(void)
{
    return m_massChangeThreshold;
}

bool GuiderOneStar::SetMassChangeThreshold(double massChangeThreshold)
{
    bool bError = false;

    try
    {
        if (massChangeThreshold < 0)
        {
            throw ERROR_INFO("massChangeThreshold < 0");
        }
        
        m_massChangeThreshold = massChangeThreshold;
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);

        bError = true;
        m_massChangeThreshold = DefaultMassChangeThreshold;
    }

    m_badMassCount = 0;
    pConfig->SetDouble("/guider/onestar/MassChangeThreshold", m_massChangeThreshold);

    return bError;
}

int GuiderOneStar::GetSearchRegion(void)
{
    return m_searchRegion;
}

bool GuiderOneStar::SetSearchRegion(int searchRegion)
{
    bool bError = false;

    try
    {
        if (searchRegion <= 0)
        {
            throw ERROR_INFO("searchRegion <= 0");
        }
        m_searchRegion = searchRegion;
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
        m_searchRegion = DefaultSearchRegion;
    }

    pConfig->SetInt("/guider/onestar/SearchRegion", m_searchRegion);

    return bError;
}

bool GuiderOneStar::SetLockPosition(double x, double y, bool bExact)
{
    bool bError = false;

    try
    {
        if ((x <= 0) || (x >= pCurrentFullFrame->Size.x))
        {
            throw ERROR_INFO("invalid y value");
        }

        if ((y <= 0) || (y >= pCurrentFullFrame->Size.x))
        {
            throw ERROR_INFO("invalid x value");
        }

        if (SetState(STATE_SELECTING))
        {
            throw ERROR_INFO("unable to set state to STATE_SELECTING");
        }

        if (bExact)
        {
            m_lockPosition = Point(x,y);
        }
        else
        {
            m_star.Find(pCurrentFullFrame, m_searchRegion, x, y);
            // if the find was successful, the next state machine update will 
            // move our state to STATE_SELECTED
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
    }

    return bError;
}

bool GuiderOneStar::AutoSelect(usImage *pImage)
{
    bool bError = false;

    try
    {
        Star newStar;

        if (!newStar.AutoFind(pImage))
        {
            throw ERROR_INFO("Uable to AutoFind");
        }

        if (!SetLockPosition(newStar.X, newStar.Y))
        {
            throw ERROR_INFO("Uable to set Lock Position");
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
    }

    return bError;
}

bool GuiderOneStar::IsLocked(void)
{
    return m_star.WasFound();
}

Point &GuiderOneStar::CurrentPosition(void)
{
    return m_star;
}

bool GuiderOneStar::SetState(GUIDER_STATE newState)
{
    bool bError = false;

    try
    {
        Debug.Write(wxString::Format("Changing from state %d to %d\n", m_state, newState));

        if (newState == STATE_STOP)
        {
            // we are going to stop looping exposures.  We should put 
            // ourselves into a good state to restart looping later
            switch(m_state)
            {
                case STATE_UNINITIALIZED:
                case STATE_SELECTING:
                case STATE_SELECTED:
                    break;
                case STATE_CALIBRATING:
                    // because we have done some moving here, we need to just 
                    // start over...
                    newState = STATE_UNINITIALIZED;
                    break;
                case STATE_CALIBRATED:
                case STATE_GUIDING:
                    newState = STATE_SELECTED;
                    break;
            }
        }

        if (newState > m_state + 1)
        {
            throw ERROR_INFO("Illegal state transition");
        }

        switch(newState)
        {
            case STATE_UNINITIALIZED:
                m_lockPosition.Invalidate();
                m_star.Invalidate();
                m_autoSelectTries = 0;
                newState = STATE_SELECTING;
                break;
            case STATE_CALIBRATING:
                if (pScope->IsCalibrated())
                {
                    newState = STATE_CALIBRATED;
                }
                else
                {
                    if (pScope->BeginCalibration(m_star))
                    {
                        newState = STATE_UNINITIALIZED;
                        Debug.Write(ERROR_INFO("pScope->BeginCalibration failed"));
                    }
                }
                break;
            case STATE_GUIDING:
                m_lockPosition = m_star;
                break;
        }

        if (Guider::SetState(newState))
        {
            throw ERROR_INFO("Guider::SetState() failed");
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
    }

    return bError;
}

/*************  A new image is ready ************************/

bool GuiderOneStar::UpdateGuideState(usImage *pImage, bool bStopping)
{
    bool bError = false;
    bool updateStatus = true;
    wxString statusMessage;

    try
    {
        Debug.Write(wxString::Format("UpdateGuideState(): m_state=%d\n", m_state));

        if (bStopping)
        {
            SetState(STATE_STOP);
            statusMessage = _T("Stopped Exposing");
            throw ERROR_INFO("Stopped Exposing");
        }

        if (IsPaused())
        {
            statusMessage = _T("Paused");
            throw ERROR_INFO("Skipping frame - guider is paused");
        }

        if (m_state == STATE_SELECTING && m_autoSelectTries++ == 0)
        {
            Debug.Write("UpdateGuideState(): Autoselecting\n");
            if (AutoSelect(pImage))
            {
                pFrame->SetStatusText(wxString::Format(_T("Auto Selected star at (%.1f, %.1f)"),m_star.X, m_star.Y), 1);
            }
            else
            {
                statusMessage = _T("No Star selected");
                throw ERROR_INFO("No Star selected");
            }
        }

        Star newStar(m_star);
        
        if (!newStar.Find(pImage, m_searchRegion))
        {
            Debug.Write("UpdateGuideState():newStar not found\n");
            statusMessage = _T("No Star found");
        }

        if (m_massChangeThreshold < 0.99 &&
            m_star.Mass > 0.0 &&  
            newStar.Mass > 0.0 && 
            m_badMassCount++ < 2)
        {
            // check to see if it seems like the star we just found was the
            // same as the orignial star.  We do this by comparing the 
            // mass
            double massRatio;

            if (newStar.Mass > m_star.Mass)
            {
                massRatio = m_star.Mass/newStar.Mass;
            }
            else
            {
                massRatio = newStar.Mass/m_star.Mass;
            }

            massRatio = 1.0 - massRatio;

            assert(massRatio >= 0 && massRatio < 1.0);

            if (massRatio > m_massChangeThreshold)
            {
                m_star.SetError(Star::STAR_MASSCHANGE);
                pFrame->SetStatusText(wxString::Format(_T("Mass: %.0f vs %.0f"), newStar.Mass, m_star.Mass), 1);
                Debug.Write(wxString::Format("UpdateGuideState(): star mass ratio=%.1f, thresh=%.1f new=%.1f, old=%.1f\n", massRatio, m_massChangeThreshold, newStar.Mass, m_star.Mass));
                throw ERROR_INFO("massChangeThreshold error");
            }
        }

        // update the star position, mass, etc.
        m_star = newStar;
        m_badMassCount = 0;

        statusMessage.Printf(_T("m=%.0f SNR=%.1f"), m_star.Mass, m_star.SNR);

        switch(m_state)
        {
            case STATE_SELECTING:
                if (m_star.IsValid())
                {
                    m_lockPosition = m_star;
                    SetState(STATE_SELECTED);
                }
                break;
            case STATE_SELECTED:
                if (!m_star.IsValid())
                {
                     SetState(STATE_UNINITIALIZED);
                }
                break;
            case STATE_CALIBRATING:
                updateStatus = false;

                if (pScope->IsCalibrated())
                {
                    SetState(STATE_CALIBRATED);
                }
                else if (pScope->UpdateCalibrationState(m_star))
                {
                    SetState(STATE_UNINITIALIZED);
                    throw ERROR_INFO("Calibration failed");
                }
                break;
            case STATE_CALIBRATED:
                SetState(STATE_GUIDING);
                break;
            case STATE_GUIDING:
                if (DoGuide())
                {
                    Debug.Write("UpdateGuideState(): DoGuide returned an error\n");

                    wxColor prevColor = GetBackgroundColour();
                    SetBackgroundColour(wxColour(64,0,0));
                    ClearBackground();
                    wxBell();
                    wxMilliSleep(100);
                    SetBackgroundColour(prevColor);

                    throw ERROR_INFO("DoGuide returned an error");
                }
                break;
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
    }

    if (updateStatus)
    {
        pFrame->SetStatusText(statusMessage);
    }

    pFrame->UpdateButtonsStatus();

    Debug.Write("UpdateGuideState exits:" + statusMessage + "\n");

    UpdateImageDisplay(pImage);

    return bError;
}

void GuiderOneStar::ResetGuideState(void)
{
    SetState(STATE_UNINITIALIZED);
}

void GuiderOneStar::StartGuiding(void)
{
    // we set the state to calibrating.  The state machine will
    // automatically move from calibrating->calibrated->guiding 
    SetState(STATE_CALIBRATING);
}

void GuiderOneStar::OnLClick(wxMouseEvent &mevent) 
{
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
            if ((mevent.m_x <= m_searchRegion) || (mevent.m_x >= (XWinSize+m_searchRegion)) || (mevent.m_y <= m_searchRegion) || (mevent.m_y >= (XWinSize+m_searchRegion))) 
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

            if (SetLockPosition(StarX, StarY))
            {
                pFrame->SetStatusText(wxString::Format(_T("No star found")));
            }
            else
            {
                pFrame->SetStatusText(wxString::Format(_T("Selected star at (%.1f, %.1f)"),m_star.X, m_star.Y), 1);
                pFrame->SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"),m_star.Mass,m_star.SNR));
            }

            Refresh();
            Update();
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
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
        
		if (m_state == STATE_SELECTED || IsPaused()) {

			if (FoundStar)
				dc.SetPen(wxPen(wxColour(100,255,90),1,wxSOLID ));  // Draw the box around the star
			else
				dc.SetPen(wxPen(wxColour(230,130,30),1,wxDOT ));
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*m_scaleFactor)-m_searchRegion,ROUND(StarY*m_scaleFactor)-m_searchRegion,m_searchRegion*2+1,m_searchRegion*2+1);
		}
		else if (m_state == STATE_CALIBRATING) {  // in the cal process
			dc.SetPen(wxPen(wxColour(32,196,32),1,wxSOLID ));  // Draw the box around the star
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*m_scaleFactor)-m_searchRegion,ROUND(StarY*m_scaleFactor)-m_searchRegion,m_searchRegion*2+1,m_searchRegion*2+1);
			dc.SetPen(wxPen(wxColor(255,255,0),1,wxDOT));
			dc.DrawLine(0, LockY*m_scaleFactor, XWinSize, LockY*m_scaleFactor);
			dc.DrawLine(LockX*m_scaleFactor, 0, LockX*m_scaleFactor, YWinSize);

		}
		else if (m_state == STATE_GUIDING) { // locked and guiding
			if (FoundStar)
				dc.SetPen(wxPen(wxColour(32,196,32),1,wxSOLID ));  // Draw the box around the star
			else
				dc.SetPen(wxPen(wxColour(230,130,30),1,wxDOT ));
			dc.SetBrush(* wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(ROUND(StarX*m_scaleFactor)-m_searchRegion,ROUND(StarY*m_scaleFactor)-m_searchRegion,m_searchRegion*2+1,m_searchRegion*2+1);
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

ConfigDialogPane *GuiderOneStar::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuiderOneStarConfigDialogPane(pParent, this);
}

GuiderOneStar::GuiderOneStarConfigDialogPane::GuiderOneStarConfigDialogPane(wxWindow *pParent, GuiderOneStar *pGuider)
    : GuiderConfigDialogPane(pParent, pGuider)
{
    int width;

    m_pGuiderOneStar = pGuider;

    width = StringWidth(_T("0000"));
	m_pSearchRegion = new wxSpinCtrl(pParent, wxID_ANY, _T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 10, 50, 15, _T("Search"));
    DoAdd(_T("Search region (pixels)"), m_pSearchRegion,
	      _T("How many pixels (up/down/left/right) do we examine to find the star? Default = 15"));

    width = StringWidth(_T("0000"));
	m_pMassChangeThreshold = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.1, 100.0, 0.0, 1.0,_T("MassChangeThreshold"));
    m_pMassChangeThreshold->SetDigits(1);
	DoAdd(_T("Star mass tolerance"), m_pMassChangeThreshold,
	      _T("Tolerance for change in star mass b/n frames. Default = 0.3 (0.1-1.0)"));
}

GuiderOneStar::GuiderOneStarConfigDialogPane::~GuiderOneStarConfigDialogPane(void)
{
}

void GuiderOneStar::GuiderOneStarConfigDialogPane::LoadValues(void)
{
    GuiderConfigDialogPane::LoadValues();
    m_pMassChangeThreshold->SetValue(100.0*m_pGuiderOneStar->GetMassChangeThreshold());
    m_pSearchRegion->SetValue(m_pGuiderOneStar->GetSearchRegion());
}

void GuiderOneStar::GuiderOneStarConfigDialogPane::UnloadValues(void)
{
    m_pGuiderOneStar->SetMassChangeThreshold(m_pMassChangeThreshold->GetValue()/100.0);

    m_pGuiderOneStar->SetSearchRegion(m_pSearchRegion->GetValue());
    GuiderConfigDialogPane::UnloadValues();
}
