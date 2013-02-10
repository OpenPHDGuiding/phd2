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
    m_paused = false;
    m_pCurrentImage = new usImage(); // so we always have one

    SetOverlayMode(DefaultOverlayMode);

    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetBackgroundColour(wxColour((unsigned char) 30, (unsigned char) 30,(unsigned char) 30));
}

Guider::~Guider(void)
{
    delete m_displayedImage;
    delete m_pCurrentImage;
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
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
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
        GUIDER_STATE state = GetState();
        int imageWidth   = m_displayedImage->GetWidth();
        int imageHeight  = m_displayedImage->GetHeight();
        wxImage newImage(*m_displayedImage);

        // scale the image if necessary

        if (imageWidth != XWinSize || imageHeight != YWinSize)
        {
            // The image is not the exact right size -- figure out what to do.
            double xScaleFactor = imageWidth/(double)XWinSize;
            double yScaleFactor = imageHeight/(double)YWinSize;
            int newWidth = imageWidth;
            int newHeight = imageHeight;

           double newScaleFactor = (xScaleFactor > yScaleFactor) ?
                                    xScaleFactor :
                                    yScaleFactor;

            if (xScaleFactor > 1.0 || yScaleFactor > 1.0 ||
                xScaleFactor < 0.5 || yScaleFactor < 0.5)
            {
                // The image is either too big, or so small that at least
                // one dimension is less than half the width of the window
                // so we are going to rescale it.
                newWidth /= newScaleFactor;
                newHeight /= newScaleFactor;

                if (newScaleFactor > 1.0)
                {
                    newScaleFactor = 1.0/newScaleFactor;
                }
                m_scaleFactor = newScaleFactor;

                m_displayedImage->Rescale(newWidth, newHeight);
            }

            newImage.Resize(wxSize(XWinSize,YWinSize),wxPoint(0,0));
        }

        DisplayedBitmap = new wxBitmap(m_displayedImage->Size(wxSize(XWinSize,YWinSize),wxPoint(0,0)));
        memDC.SelectObject(*DisplayedBitmap);

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
                    double cos_angle = cos(pMount->RaAngle());
                    double sin_angle = sin(pMount->RaAngle());
                    double StarX = pFrame->pGuider->CurrentPosition().X;
                    double StarY = pFrame->pGuider->CurrentPosition().Y;

                    dc.SetPen(wxPen(pFrame->GraphLog->RA_Color,2,wxPENSTYLE_DOT));
                    r=15.0;
                    dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                        ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));
                    dc.SetPen(wxPen(pFrame->GraphLog->DEC_Color,2,wxPENSTYLE_DOT));
                    cos_angle = cos(pMount->DecAngle());
                    sin_angle = sin(pMount->DecAngle());
                    dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                        ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));

                    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
                    gc->SetPen(wxPen(pFrame->GraphLog->RA_Color,1,wxPENSTYLE_DOT ));
                    wxGraphicsPath path = gc->CreatePath();
                    int i;
                    double step = (double) YWinSize / 10.0;

                    double MidX = (double) XWinSize / 2.0;
                    double MidY = (double) YWinSize / 2.0;
                    gc->Rotate(pMount->RaAngle());
                    gc->GetTransform().TransformPoint(&MidX, &MidY);
                    gc->Rotate(-pMount->RaAngle());
                    gc->Translate((double) XWinSize / 2.0 - MidX, (double) YWinSize / 2.0 - MidY);
                    gc->Rotate(pMount->RaAngle());
                    for (i=-2; i<12; i++) {
                        gc->StrokeLine(0.0,step * (double) i,
                            (double) XWinSize, step * (double) i);
                    }

                    MidX = (double) XWinSize / 2.0;
                    MidY = (double) YWinSize / 2.0;
                    gc->Rotate(-pMount->RaAngle());
                    gc->Rotate(pMount->DecAngle());
                    gc->GetTransform().TransformPoint(&MidX, &MidY);
                    gc->Rotate(-pMount->DecAngle());
                    gc->Translate((double) XWinSize / 2.0 - MidX, (double) YWinSize / 2.0 - MidY);
                    gc->Rotate(pMount->DecAngle());
                    gc->SetPen(wxPen(pFrame->GraphLog->DEC_Color,1,wxPENSTYLE_DOT ));
                    for (i=-2; i<12; i++) {
                        gc->StrokeLine(0.0,step * (double) i,
                            (double) XWinSize, step * (double) i);
                    }
                    delete gc;
                    break;
                }
            }
        }

        // draw the lockpoint of there is one
        if (state > STATE_SELECTED)
        {
            double LockX = LockPosition().X;
            double LockY = LockPosition().Y;

            switch(state)
            {
                case STATE_CALIBRATING_PRIMARY:
                case STATE_CALIBRATING_SECONDARY:
                    dc.SetPen(wxPen(wxColor(255,255,0),1,wxDOT));
                    break;
                case STATE_CALIBRATED:
                case STATE_GUIDING:
                    dc.SetPen(wxPen(wxColor(0,255,0)));
                    break;
            }

            dc.DrawLine(0, LockY*m_scaleFactor, XWinSize, LockY*m_scaleFactor);
            dc.DrawLine(LockX*m_scaleFactor, 0, LockX*m_scaleFactor, YWinSize);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    delete DisplayedBitmap;

    return bError;
}

void Guider::UpdateImageDisplay(usImage *pImage) {
    int blevel, wlevel;

    if (pImage==NULL)
    {
        pImage = m_pCurrentImage;
    }

    pImage->CalcStats();
    blevel = pImage->Min;
    wlevel = pImage->FiltMax;

#if 0
    if (pImage->Size.GetWidth() >= 1280) {
        pImage->BinnedCopyToImage(&m_displayedImage,blevel,wlevel,pFrame->Stretch_gamma);
        m_scaleFactor = 0.5;
    }
    else
    {
        pImage->CopyToImage(&m_displayedImage,blevel,wlevel,pFrame->Stretch_gamma);
        m_scaleFactor = 1.0;
    }
#else
    pImage->CopyToImage(&m_displayedImage, blevel, wlevel, pFrame->Stretch_gamma);
#endif

    Refresh();
    Update();
}

bool Guider::SaveCurrentImage(const wxString& fileName)
{
    return m_pCurrentImage->Save(fileName);
}

void Guider::InvalidateLockPosition(void)
{
    m_lockPosition.Invalidate();
}

void Guider::UpdateLockPosition(void)
{
    SetLockPosition(CurrentPosition(), true);
}

bool Guider::SetLockPosition(const Point& position, bool bExact)
{
    bool bError = false;

    try
    {
        if (!position.IsValid())
        {
            throw ERROR_INFO("Point is not valid");
        }

        double x=position.X;
        double y=position.Y;
        Debug.AddLine(wxString::Format("setting lock position to (%lf, %lf)", x, y));

        if ((x <= 0) || (x >= m_pCurrentImage->Size.x))
        {
            throw ERROR_INFO("invalid y value");
        }

        if ((y <= 0) || (y >= m_pCurrentImage->Size.x))
        {
            throw ERROR_INFO("invalid x value");
        }

        if (bExact)
        {
            m_lockPosition.SetXY(x,y);
        }
        else
        {
            SetCurrentPosition(m_pCurrentImage, Point(x, y));

            if (CurrentPosition().IsValid())
            {
                SetLockPosition(CurrentPosition());
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Guider::SetState(GUIDER_STATE newState)
{
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
                case STATE_CALIBRATING_PRIMARY:
                    // because we have done some moving here, we need to just
                    // start over...
                    newState = STATE_UNINITIALIZED;
                    break;
                case STATE_CALIBRATING_SECONDARY:
                    // because we have done some moving here, we need to just
                    // start over...
                    newState = STATE_CALIBRATING_PRIMARY;
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
                InvalidateLockPosition();
                InvalidateCurrentPosition();
                newState = STATE_SELECTING;
                break;
            case STATE_SELECTED:
                pMount->ClearHistory();
                break;
            case STATE_CALIBRATING_PRIMARY:
                if (!pMount->IsCalibrated() &&
                     pMount->BeginCalibration(CurrentPosition()))
                {
                    newState = STATE_UNINITIALIZED;
                    Debug.Write(ERROR_INFO("pMount->BeginCalibration failed"));
                }
                // else we move to STATE_CALIBRATING_PRIMARY as requested
                break;
            case STATE_CALIBRATING_SECONDARY:
                if (!pSecondaryMount)
                {
                    newState = STATE_CALIBRATED;
                }
                else if (!pSecondaryMount->IsCalibrated() &&
                          pSecondaryMount->BeginCalibration(CurrentPosition()))
                {
                    newState = STATE_UNINITIALIZED;
                    Debug.Write(ERROR_INFO("pSecondaryMount->BeginCalibration failed"));
                }
                // else we move to STATE_CALIBRATING_SECONDARY as requested
                break;
            case STATE_GUIDING:
                //TODO: Deal with manual lock position
                m_lockPosition = CurrentPosition();
                break;
        }

        m_state = newState;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

usImage *Guider::CurrentImage(void)
{
    return m_pCurrentImage;
}

double Guider::ScaleFactor(void)
{
    return m_scaleFactor;
}

void Guider::StartGuiding(void)
{
    // we set the state to calibrating.  The state machine will
    // automatically move from calibrating->calibrated->guiding
    // when it can
    SetState(STATE_CALIBRATING_PRIMARY);
}

void Guider::Reset(void)
{
    SetState(STATE_UNINITIALIZED);
}

/*************  A new image is ready ************************/

void Guider::UpdateGuideState(usImage *pImage, bool bStopping)
{
    bool updateStatus = true;
    wxString statusMessage;

    assert(!pMount->IsBusy());

    try
    {
        Debug.Write(wxString::Format("UpdateGuideState(): m_state=%d\n", m_state));

        // switch in the new image

        usImage *pPrevImage = m_pCurrentImage;
        m_pCurrentImage = pImage;
        delete pPrevImage;

        if (bStopping)
        {
            SetState(STATE_STOP);
            statusMessage = _("Stopped Guiding");
            throw THROW_INFO("Stopped Guiding");
        }

        if (IsPaused())
        {
            statusMessage = _("Paused");
            throw THROW_INFO("Skipping frame - guider is paused");
        }

        if (UpdateCurrentPosition(pImage, statusMessage))
        {
            if (m_state == STATE_GUIDING)
            {
                wxColor prevColor = GetBackgroundColour();
                SetBackgroundColour(wxColour(64,0,0));
                ClearBackground();
                wxBell();
                wxMilliSleep(100);
                SetBackgroundColour(prevColor);
            }

            throw THROW_INFO("unable to update current position");
        }

        switch(m_state)
        {
            case STATE_SELECTING:
                if (CurrentPosition().IsValid())
                {
                    m_lockPosition = CurrentPosition();
                    Debug.AddLine("CurrentPosition() valid, moving to STATE_SELECTED");
                    SetState(STATE_SELECTED);
                }
                break;
            case STATE_SELECTED:
                if (!CurrentPosition().IsValid())
                {
                    // we had a current position and lost it
                     SetState(STATE_UNINITIALIZED);
                }
                break;
            case STATE_CALIBRATING_PRIMARY:
                if (pMount->IsCalibrated())
                {
                    SetState(STATE_CALIBRATING_SECONDARY);
                }
                else if (pMount->UpdateCalibrationState(CurrentPosition()))
                {
                    SetState(STATE_UNINITIALIZED);
                    throw ERROR_INFO("Calibration failed");
                }
                break;
            case STATE_CALIBRATING_SECONDARY:
                if (!pSecondaryMount || pSecondaryMount->IsCalibrated())
                {
                    SetState(STATE_CALIBRATED);
                }
                else if (pSecondaryMount->UpdateCalibrationState(CurrentPosition()))
                {
                    SetState(STATE_UNINITIALIZED);
                    throw ERROR_INFO("Calibration failed");
                }
                break;
            case STATE_CALIBRATED:
                SetState(STATE_GUIDING);
                break;
            case STATE_GUIDING:
                pFrame->ScheduleMovePrimary(pMount, CurrentPosition() - LockPosition());
                break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    // during calibration, the mount is responsible for updating the status message
    if (m_state != STATE_CALIBRATING_PRIMARY)
    {
        pFrame->SetStatusText(statusMessage);
    }

    pFrame->UpdateButtonsStatus();

    Debug.Write("UpdateGuideState exits:" + statusMessage + "\n");

    UpdateImageDisplay(pImage);
}


Guider::GuiderConfigDialogPane::GuiderConfigDialogPane(wxWindow *pParent, Guider *pGuider)
    : ConfigDialogPane(_("Guider Settings"), pParent)
{
}

Guider::GuiderConfigDialogPane::~GuiderConfigDialogPane(void)
{
}

void Guider::GuiderConfigDialogPane::LoadValues(void)
{
}

void Guider::GuiderConfigDialogPane::UnloadValues(void)
{

}
