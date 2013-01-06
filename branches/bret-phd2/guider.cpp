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

static const int DefaultOverlayMode  = OVERLAY_NONE;

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
    m_guidingEnabled = true;
    m_pDecGuideAlgorithm = NULL;
    m_pRaGuideAlgorithm = NULL;
    m_paused = false;

    SetOverlayMode(DefaultOverlayMode);

	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	SetBackgroundColour(wxColour((unsigned char) 30, (unsigned char) 30,(unsigned char) 30));
}

Guider::~Guider(void)
{
	delete m_displayedImage;
    delete m_pRaGuideAlgorithm;
    delete m_pDecGuideAlgorithm;
}

bool Guider::IsPaused()
{
    return m_paused;
}

bool Guider::SetPaused(bool state)
{
    bool bReturn = m_paused;

    m_paused = state;

    return bReturn;
}

bool Guider::GetGuidingEnabled(void)
{
    return m_guidingEnabled;
}

bool Guider::SetGuidingEnabled(bool guidingEnabled)
{
    bool bError = false;

    m_guidingEnabled = guidingEnabled;

    return bError;
}

OVERLAY_MODE Guider::GetOverlayMode(void)
{
    return m_overlayMode;
}

bool Guider::SetOverlayMode(int overlayMode)
{
    bool bError = false;

    try
    {
        switch(overlayMode)
        {
            case OVERLAY_NONE:
            case OVERLAY_BULLSEYE:
            case OVERLAY_GRID_FINE:
            case OVERLAY_GRID_COARSE:
            case OVERLAY_RADEC:
                break;
            default:
                throw ERROR_INFO("invalid overlayMode");
        }

        m_overlayMode = (OVERLAY_MODE)overlayMode;
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        m_overlayMode = OVERLAY_NONE;
        bError = true;
    }

    Refresh();
    Update();

    return bError;
}

Point &Guider::LockPosition()
{
    return m_lockPosition;
}

double Guider::CurrentError(void)
{
    return m_lockPosition.Distance(CurrentPosition());
}

GUIDER_STATE Guider::GetState(void)
{
    return m_state;
}

bool Guider::SetState(GUIDER_STATE newState)
{
    bool bError = false;

    m_state = newState;

    return bError;
}

GUIDE_ALGORITHM Guider::GetGuideAlgorithm(GuideAlgorithm *pAlgorithm)
{
    GUIDE_ALGORITHM ret = GUIDE_ALGORITHM_NONE;

    if (pAlgorithm)
    {
        ret = pAlgorithm->Algorithm();
    }
    return ret;
}

bool Guider::SetGuideAlgorithm(int guideAlgorithm, GuideAlgorithm** ppAlgorithm)
{
    bool bError = false;

    try
    {
        switch (guideAlgorithm)
        {
            case GUIDE_ALGORITHM_IDENTITY:
            case GUIDE_ALGORITHM_HYSTERESIS:
            case GUIDE_ALGORITHM_LOWPASS:
            case GUIDE_ALGORITHM_LOWPASS2:
            case GUIDE_ALGORITHM_RESIST_SWITCH:
                break;
            case GUIDE_ALGORITHM_NONE:
            default:
                throw ERROR_INFO("invalid guideAlgorithm");
                break;
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
        guideAlgorithm = GUIDE_ALGORITHM_IDENTITY;
    }

    switch (guideAlgorithm)
    {
        case GUIDE_ALGORITHM_IDENTITY:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmIdentity();
            break;
        case GUIDE_ALGORITHM_HYSTERESIS:
            *ppAlgorithm = (GuideAlgorithm *) new GuideAlgorithmHysteresis();
            break;
        case GUIDE_ALGORITHM_LOWPASS:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass();
            break;
        case GUIDE_ALGORITHM_LOWPASS2:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmLowpass2();
            break;
        case GUIDE_ALGORITHM_RESIST_SWITCH:
            *ppAlgorithm = (GuideAlgorithm *)new GuideAlgorithmResistSwitch();
            break;
        case GUIDE_ALGORITHM_NONE:
        default:
            assert(false);
            break;
    }

    return bError;
}

GUIDE_ALGORITHM Guider::GetRaGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pRaGuideAlgorithm);
}

bool Guider::SetRaGuideAlgorithm(int raGuideAlgorithm)
{
    delete m_pRaGuideAlgorithm;

    return SetGuideAlgorithm(raGuideAlgorithm, &m_pRaGuideAlgorithm);
}

GUIDE_ALGORITHM Guider::GetDecGuideAlgorithm(void)
{
    return GetGuideAlgorithm(m_pDecGuideAlgorithm);
}

bool Guider::SetDecGuideAlgorithm(int decGuideAlgorithm)
{
    delete m_pDecGuideAlgorithm;

    return SetGuideAlgorithm(decGuideAlgorithm, &m_pDecGuideAlgorithm);
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

        if (m_overlayMode) 
        {
            dc.SetPen(wxPen(wxColor(200,50,50)));
            dc.SetBrush(* wxTRANSPARENT_BRUSH);

            switch(m_overlayMode)
            {
                case OVERLAY_BULLSEYE:
                {
                    int cx = XWinSize / 2;
                    int cy = YWinSize / 2;
                    dc.DrawCircle(cx,cy,25);
                    dc.DrawCircle(cx,cy,50);
                    dc.DrawCircle(cx,cy,100);
                    dc.DrawLine(0, cy, XWinSize, cy);
                    dc.DrawLine(cx, 0, cx, YWinSize);
                    break;
                }
                case OVERLAY_GRID_FINE:
                case OVERLAY_GRID_COARSE:
                {
                    int i;
                    int size = (m_overlayMode - 1) * 20;
                    for (i=size; i<XWinSize; i+=size)
                        dc.DrawLine(i,0,i,YWinSize);
                    for (i=size; i<YWinSize; i+=size)
                        dc.DrawLine(0,i,XWinSize,i);
                    break;
                }
                case OVERLAY_RADEC:
                {
                    double r=30.0;
                    double cos_angle = cos(pScope->RaAngle());
                    double sin_angle = sin(pScope->RaAngle());
                    double StarX = frame->pGuider->CurrentPosition().X;
                    double StarY = frame->pGuider->CurrentPosition().Y;

                    dc.SetPen(wxPen(frame->GraphLog->RA_Color,2,wxPENSTYLE_DOT));
                    r=15.0;
                    dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                        ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));
                    dc.SetPen(wxPen(frame->GraphLog->DEC_Color,2,wxPENSTYLE_DOT));
                    cos_angle = cos(pScope->DecAngle());
                    sin_angle = sin(pScope->DecAngle());
                    dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                        ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));

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
                    break;
                }
            }
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
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


/*************  Do the actual guiding ***********************/
bool Guider::DoGuide(void)
{
    bool bError = false;

    try
    {
        if (!LockPosition().IsValid())
        {
            throw ERROR_INFO("invalid LockPosition");
        }

        if (!CurrentPosition().IsValid())
        {
            throw ERROR_INFO("invalid CurrentPosition");
        }

        double theta = LockPosition().Angle(CurrentPosition());
        double hyp   = LockPosition().Distance(CurrentPosition());

        // Convert theta and hyp into RA and DEC

        double raDistance  = cos(pScope->RaAngle() - theta) * hyp;
        double decDistance = cos(pScope->DecAngle() - theta) * hyp;

        frame->GraphLog->AppendData(LockPosition().dX(CurrentPosition()), LockPosition().dY(CurrentPosition()),
                raDistance, decDistance);

        // Feed the raw distances to the guide algorithms
        
        raDistance = m_pRaGuideAlgorithm->result(raDistance);
        decDistance = m_pDecGuideAlgorithm->result(decDistance);

        // Figure out the guide directions based on the (possibly) updated distances
        GUIDE_DIRECTION raDirection = raDistance > 0 ? EAST : WEST;
        GUIDE_DIRECTION decDirection = decDistance > 0 ? SOUTH : NORTH;

        // Compute the required guide durations
        double raDuration = fabs(raDistance/pScope->RaRate());
        double decDuration = fabs(decDistance/pScope->DecRate());

        if (m_guidingEnabled)
        {
            raDuration = pScope->LimitGuide(raDirection, raDuration);
            decDuration = pScope->LimitGuide(decDirection, decDuration);
        }
        else
        {
            raDuration = 0.0;
            decDuration = 0.0;
        }

        frame->SetStatusText("",1);

        // We are now ready to actuallly guide
        assert(raDuration >= 0);
        if (raDuration > 0.0)
        {
            wxString msg;
            msg.Printf("%c dur=%.1f dist=%.2f", (raDirection==EAST)?'E':'W', raDuration, raDistance);
            frame->ScheduleGuide(raDirection, raDuration, msg);
        }

        assert(decDuration >= 0);
        if (decDuration > 0.0)
        {
            wxString msg;
            msg.Printf("%c dur=%.1f dist=%.2f", (decDirection==SOUTH)?'S':'N', decDuration, decDistance);
            frame->ScheduleGuide(decDirection, decDuration, msg);
        }
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;
    }

    return bError;
}

Guider::GuiderConfigDialogPane::GuiderConfigDialogPane(wxWindow *pParent, Guider *pGuider)
    : ConfigDialogPane(_T("Guider Settings"), pParent)
{
    m_pGuider = pGuider;
    int width;

    m_pEnableGuide = new wxCheckBox(pParent, wxID_ANY,_T("Enable Guide Output"), wxPoint(-1,-1), wxSize(75,-1));
    DoAdd(m_pEnableGuide, _T("Should mount guide commands be issued"));

	wxString raAlgorithms[] = {
		_T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
	};

    width = StringArrayWidth(raAlgorithms, WXSIZEOF(raAlgorithms));
	m_pRaGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1), 
                                    wxSize(width+35, -1), WXSIZEOF(raAlgorithms), raAlgorithms);
    DoAdd(_T("RA Algorithm"), m_pRaGuideAlgorithm, 
	      _T("Which Guide Algorithm to use for Right Ascention"));

    m_pRaGuideAlgorithmConfigDialogPane  = m_pGuider->m_pRaGuideAlgorithm->GetConfigDialogPane(pParent);
    DoAdd(m_pRaGuideAlgorithmConfigDialogPane);

	wxString decAlgorithms[] = {
		_T("Identity"),_T("Hysteresis"),_T("Lowpass"),_T("Lowpass2"), _T("Resist Switch")
	};

    width = StringArrayWidth(decAlgorithms, WXSIZEOF(decAlgorithms));
	m_pDecGuideAlgorithm = new wxChoice(pParent, wxID_ANY, wxPoint(-1,-1), 
                                    wxSize(width+35, -1), WXSIZEOF(decAlgorithms), decAlgorithms);
    DoAdd(_T("Declination Algorithm"), m_pDecGuideAlgorithm, 
	      _T("Which Guide Algorithm to use for Declination"));

    m_pDecGuideAlgorithmConfigDialogPane  = pGuider->m_pDecGuideAlgorithm->GetConfigDialogPane(pParent);
    DoAdd(m_pDecGuideAlgorithmConfigDialogPane);
}

Guider::GuiderConfigDialogPane::~GuiderConfigDialogPane(void)
{
}

void Guider::GuiderConfigDialogPane::LoadValues(void)
{
	m_pRaGuideAlgorithm->SetSelection(m_pGuider->GetRaGuideAlgorithm());
	m_pDecGuideAlgorithm->SetSelection(m_pGuider->GetDecGuideAlgorithm());
    m_pEnableGuide->SetValue(m_pGuider->GetGuidingEnabled());

    m_pRaGuideAlgorithmConfigDialogPane->LoadValues();
    m_pDecGuideAlgorithmConfigDialogPane->LoadValues();
}

void Guider::GuiderConfigDialogPane::UnloadValues(void)
{
    m_pGuider->SetGuidingEnabled(m_pEnableGuide->GetValue());

    // note these two have to be before the SetXxxAlgorithm calls, because if we
    // changed the algorithm, the current one will get freed, and if we make
    // these two calls after that, bad things happen
    m_pRaGuideAlgorithmConfigDialogPane->UnloadValues();
    m_pDecGuideAlgorithmConfigDialogPane->UnloadValues();

    m_pGuider->SetRaGuideAlgorithm(m_pRaGuideAlgorithm->GetSelection());
    m_pGuider->SetDecGuideAlgorithm(m_pDecGuideAlgorithm->GetSelection());
}
