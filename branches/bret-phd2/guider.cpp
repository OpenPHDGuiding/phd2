/*
 *  guider.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
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

BEGIN_EVENT_TABLE(Guider, wxWindow)
    EVT_PAINT(Guider::OnPaint)
    EVT_CLOSE(Guider::OnClose)
	EVT_ERASE_BACKGROUND(Guider::OnErase)
END_EVENT_TABLE()

Guider::Guider(wxWindow *parent, int xSize, int ySize) :
    wxWindow(parent, wxID_ANY, wxPoint(0,0), wxSize(xSize, xSize))
{
    m_state = STATE_UNINITIALIZED;
    m_scaleFactor = 1.0;
	m_displayedImage = new wxImage(XWinSize,YWinSize,true);
    m_pRaGuideAlgorithm = new GuideAlgorithm();
    m_pDecGuideAlgorithm = new GuideAlgorithm();
    m_paused = false;
    m_guidingEnabled = true;
    m_timeLapse   = pConfig->GetInt("/guider/TimeLapse", 0);
    m_minMotion    = pConfig->GetDouble("/guider/MinMotion", 0.2);
    m_searchRegion = pConfig->GetInt("/guider/SearchRegion", 15);

	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	SetBackgroundColour(wxColour((unsigned char) 30, (unsigned char) 30,(unsigned char) 30));
}

Guider::~Guider(void)
{
    pConfig->SetInt("/guider/TimeLapse", m_timeLapse);
    pConfig->SetDouble("/guider/MinMotion", m_minMotion);
    pConfig->SetInt("/guider/SearchRegion", m_searchRegion);

	delete m_displayedImage;
    delete m_pRaGuideAlgorithm;
    delete m_pDecGuideAlgorithm;
}

bool Guider::IsPaused(void)
{
    return m_paused;
}

bool Guider::Pause(void)
{
    bool bReturn = m_paused;
    m_paused = true;
    return bReturn;
}

bool Guider::Unpause(void)
{
    bool bReturn = m_paused;
    m_paused = false;
    return bReturn;
}

bool Guider::DisableGuiding(void)
{
    bool bReturn = m_guidingEnabled;
    m_guidingEnabled = false;
    return bReturn;
}

bool Guider::EnableGuiding(void)
{
    bool bReturn = m_guidingEnabled;
    m_guidingEnabled = true;
    return bReturn;
}

Point &Guider::LockPosition()
{
    return m_lockPosition;
}

double Guider::CurrentError(void)
{
    return m_lockPosition.Distance(CurrentPosition());
}


E_GUIDER_STATES Guider::GetState(void)
{
    return m_state;
}

bool Guider::SetState(E_GUIDER_STATES newState)
{
    bool bError = false;

    m_state = newState;

    return bError;
}

void Guider::SetRaGuideAlgorithm(GuideAlgorithm *pAlgorithm)
{
    delete m_pRaGuideAlgorithm;
    m_pRaGuideAlgorithm = pAlgorithm;

}

void Guider::SetDecGuideAlgorithm(GuideAlgorithm *pAlgorithm)
{
    delete m_pDecGuideAlgorithm;
    m_pDecGuideAlgorithm = pAlgorithm;
}

void Guider::OnErase(wxEraseEvent &evt)
{
	evt.Skip();
}

void Guider::OnClose(wxCloseEvent& evt)
{
    Destroy();
}

bool Guider::PaintHelper(wxAutoBufferedPaintDC &dc, wxMemoryDC &memDC)
{
	wxBitmap* DisplayedBitmap = NULL;
    bool bError = false;

    try
    {
        m_scaleFactor = 1.0;

        // see if we need to scale the image
        if ((m_displayedImage->GetWidth() == XWinSize) && (m_displayedImage->GetHeight() == YWinSize)) 
        {
            // No scaling required
            DisplayedBitmap = new wxBitmap(*m_displayedImage);
            memDC.SelectObject(*DisplayedBitmap);
        }
        else
        {
            DisplayedBitmap = new wxBitmap(m_displayedImage->Size(wxSize(XWinSize,YWinSize),wxPoint(0,0)));
            
            memDC.SelectObject(*DisplayedBitmap);
        }

        try 
        {
            dc.Blit(0, 0, DisplayedBitmap->GetWidth(),DisplayedBitmap->GetHeight(), & memDC, 0, 0, wxCOPY, false);
        }
        catch (...) 
        {
            throw ERROR_INFO("dc.Blit() failed");
        }

        if (OverlayMode) 
        {
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
                //TODO: Bret fix this
#if 0
                dc.SetPen(wxPen(frame->GraphLog->RA_Color,2,wxPENSTYLE_DOT));
                r=15.0;
                dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                    ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));
                dc.SetPen(wxPen(frame->GraphLog->DEC_Color,2,wxPENSTYLE_DOT));
                cos_angle = cos(pScope->DecAngle());
                sin_angle = sin(pScope->DecAngle());
                dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                    ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));
#endif

                wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
                gc->SetPen(wxPen(frame->GraphLog->RA_Color,1,wxPENSTYLE_DOT ));
                wxGraphicsPath path = gc->CreatePath();
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
    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);
        bError = true;
    }

    delete DisplayedBitmap;

    return bError;
}

void Guider::UpdateImageDisplay(usImage *pImage) {
	int blevel, wlevel;

	pImage->CalcStats();
	blevel = pImage->Min;
	wlevel = pImage->FiltMax;
    
	if (pImage->Size.GetWidth() >= 1280) {
		pImage->BinnedCopyToImage(&m_displayedImage,blevel,wlevel,frame->Stretch_gamma);
	}
	else {
		pImage->CopyToImage(&m_displayedImage,blevel,wlevel,frame->Stretch_gamma);
	}

    Refresh();
    Update();
}


