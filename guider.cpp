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
static const bool DefaultScaleImage  = false;

BEGIN_EVENT_TABLE(Guider, wxWindow)
    EVT_PAINT(Guider::OnPaint)
    EVT_CLOSE(Guider::OnClose)
    EVT_ERASE_BACKGROUND(Guider::OnErase)
END_EVENT_TABLE()

Guider::Guider(wxWindow *parent, int xSize, int ySize) :
    wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
    m_state = STATE_UNINITIALIZED;
    m_scaleFactor = 1.0;
    m_displayedImage = new wxImage(XWinSize,YWinSize,true);
    m_paused = false;
    m_lockPosIsSticky = false;
    m_pCurrentImage = new usImage(); // so we always have one

    SetOverlayMode(DefaultOverlayMode);

    m_polarAlignCircleRadius = 0;

    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetBackgroundColour(wxColour((unsigned char) 30, (unsigned char) 30,(unsigned char) 30));
}

Guider::~Guider(void)
{
    delete m_displayedImage;
    delete m_pCurrentImage;
}

void Guider::LoadProfileSettings(void)
{
    bool scaleImage = pConfig->Profile.GetBoolean("/guider/ScaleImage", DefaultScaleImage);
    SetScaleImage(scaleImage);
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

void Guider::SetPolarAlignCircle(const PHD_Point& pt, unsigned int radius)
{
    m_polarAlignCircleRadius = radius;
    m_polarAlignCircleCenter = pt;
}

bool Guider::SetScaleImage(bool newScaleValue)
{
    bool bError = false;

    try
    {
        m_scaleImage = newScaleValue;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    pConfig->Profile.SetBoolean("/guider/ScaleImage", m_scaleImage);

    return bError;
}

bool Guider::GetScaleImage(void)
{
    return m_scaleImage;
}

const PHD_Point& Guider::LockPosition()
{
    return m_lockPosition;
}

GUIDER_STATE Guider::GetState(void)
{
    return m_state;
}

bool Guider::IsCalibratingOrGuiding(void)
{
    return m_state >= STATE_CALIBRATING_PRIMARY && m_state <= STATE_GUIDING;
}

void Guider::OnErase(wxEraseEvent &evt)
{
    evt.Skip();
}

void Guider::OnClose(wxCloseEvent& evt)
{
    Destroy();
}

bool Guider::PaintHelper(wxClientDC &dc, wxMemoryDC &memDC)
{
    bool bError = false;

    try
    {
        GUIDER_STATE state = GetState();
        GetSize(&XWinSize, &YWinSize);

        if (m_pCurrentImage->ImageData != NULL)
        {
            int blevel, wlevel;

            blevel = m_pCurrentImage->FiltMin;
            wlevel = m_pCurrentImage->FiltMax;
            m_pCurrentImage->CopyToImage(&m_displayedImage, blevel, wlevel, pFrame->Stretch_gamma);
        }

        int imageWidth   = m_displayedImage->GetWidth();
        int imageHeight  = m_displayedImage->GetHeight();

        // scale the image if necessary

        if (imageWidth != XWinSize || imageHeight != YWinSize)
        {
            // The image is not the exact right size -- figure out what to do.
            double xScaleFactor = imageWidth / (double)XWinSize;
            double yScaleFactor = imageHeight / (double)YWinSize;
            int newWidth = imageWidth;
            int newHeight = imageHeight;

            double newScaleFactor = (xScaleFactor > yScaleFactor) ?
                                    xScaleFactor :
                                    yScaleFactor;

//            Debug.AddLine("xScaleFactor=%.2f, yScaleFactor=%.2f, newScaleFactor=%.2f", xScaleFactor,
//                    yScaleFactor, newScaleFactor);

            // we rescale the image if:
            // - The image is either too big
            // - The image is so small that at least one dimension is less
            //   than half the width of the window or
            // - The user has requsted rescaling

            if (xScaleFactor > 1.0 || yScaleFactor > 1.0 ||
                xScaleFactor < 0.5 || yScaleFactor < 0.5 || m_scaleImage)
            {

                newWidth /= newScaleFactor;
                newHeight /= newScaleFactor;

                newScaleFactor = 1.0/newScaleFactor;

                m_scaleFactor = newScaleFactor;

                Debug.AddLine("Resizing image to %d,%d", newWidth, newHeight);

                m_displayedImage->Rescale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH);
            }
            else
            {
                m_scaleFactor = 1.0;
            }
        }

        // important to provide explicit color for r,g,b, optional args to Size().
        // If default args are provided wxWidgets performs some expensive histogram
        // operations.
        wxBitmap DisplayedBitmap(m_displayedImage->Size(wxSize(XWinSize, YWinSize), wxPoint(0, 0), 0, 0, 0));
        memDC.SelectObject(DisplayedBitmap);

        dc.Blit(0, 0, DisplayedBitmap.GetWidth(), DisplayedBitmap.GetHeight(), &memDC, 0, 0, wxCOPY, false);

        int XImgSize = m_displayedImage->GetWidth();
        int YImgSize = m_displayedImage->GetHeight();

        if (m_overlayMode)
        {
            dc.SetPen(wxPen(wxColor(200,50,50)));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);

            switch (m_overlayMode)
            {
                case OVERLAY_BULLSEYE:
                {
                    int cx = XImgSize / 2;
                    int cy = YImgSize / 2;
                    dc.DrawCircle(cx,cy,25);
                    dc.DrawCircle(cx,cy,50);
                    dc.DrawCircle(cx,cy,100);
                    dc.DrawLine(0, cy, XImgSize, cy);
                    dc.DrawLine(cx, 0, cx, YImgSize);
                    break;
                }
                case OVERLAY_GRID_FINE:
                case OVERLAY_GRID_COARSE:
                {
                    int i;
                    int size = (m_overlayMode - 1) * 20;
                    for (i=size; i<XImgSize; i+=size)
                        dc.DrawLine(i,0,i,YImgSize);
                    for (i=size; i<YImgSize; i+=size)
                        dc.DrawLine(0,i,XImgSize,i);
                    break;
                }
                case OVERLAY_RADEC:
                {
                    if (!pMount)
                        Debug.AddLine("No mount specified for View/RA_Dec overlay");        // Soft error
                    else
                    {
                        double r=30.0;
                        double cos_angle = cos(pMount->xAngle());
                        double sin_angle = sin(pMount->xAngle());
                        double StarX = CurrentPosition().X;
                        double StarY = CurrentPosition().Y;

                        dc.SetPen(wxPen(pFrame->pGraphLog->GetRaOrDxColor(),2,wxPENSTYLE_DOT));
                        r=15.0;
                        dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                            ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));
                        dc.SetPen(wxPen(pFrame->pGraphLog->GetDecOrDyColor(),2,wxPENSTYLE_DOT));
                        cos_angle = cos(pMount->yAngle());
                        sin_angle = sin(pMount->yAngle());
                        dc.DrawLine(ROUND(StarX*m_scaleFactor+r*cos_angle),ROUND(StarY*m_scaleFactor+r*sin_angle),
                            ROUND(StarX*m_scaleFactor-r*cos_angle),ROUND(StarY*m_scaleFactor-r*sin_angle));

                        wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
                        gc->SetPen(wxPen(pFrame->pGraphLog->GetRaOrDxColor(),1,wxPENSTYLE_DOT ));
                        wxGraphicsPath path = gc->CreatePath();
                        int i;
                        double step = (double) YImgSize / 10.0;

                        double MidX = (double) XImgSize / 2.0;
                        double MidY = (double) YImgSize / 2.0;
                        gc->Rotate(pMount->xAngle());
                        gc->GetTransform().TransformPoint(&MidX, &MidY);
                        gc->Rotate(-pMount->xAngle());
                        gc->Translate((double) XImgSize / 2.0 - MidX, (double) YImgSize / 2.0 - MidY);
                        gc->Rotate(pMount->xAngle());
                        for (i=-2; i<12; i++) {
                            gc->StrokeLine(0.0,step * (double) i,
                                (double) XImgSize, step * (double) i);
                        }

                        MidX = (double) XImgSize / 2.0;
                        MidY = (double) YImgSize / 2.0;
                        gc->Rotate(-pMount->xAngle());
                        gc->Rotate(pMount->yAngle());
                        gc->GetTransform().TransformPoint(&MidX, &MidY);
                        gc->Rotate(-pMount->yAngle());
                        gc->Translate((double) XImgSize / 2.0 - MidX, (double) YImgSize / 2.0 - MidY);
                        gc->Rotate(pMount->yAngle());
                        gc->SetPen(wxPen(pFrame->pGraphLog->GetDecOrDyColor(),1,wxPENSTYLE_DOT ));
                        for (i=-2; i<12; i++) {
                            gc->StrokeLine(0.0,step * (double) i,
                                (double) XImgSize, step * (double) i);
                        }
                        delete gc;
                    }
                    break;
                }

            case OVERLAY_NONE:
                break;
            }
        }

        // draw the lockpoint of there is one
        if (state > STATE_SELECTED)
        {
            double LockX = LockPosition().X;
            double LockY = LockPosition().Y;

            switch (state)
            {
                case STATE_UNINITIALIZED:
                case STATE_SELECTING:
                case STATE_SELECTED:
                case STATE_STOP:
                    break;
                case STATE_CALIBRATING_PRIMARY:
                case STATE_CALIBRATING_SECONDARY:
                    dc.SetPen(wxPen(wxColor(255,255,0),1,wxDOT));
                    break;
                case STATE_CALIBRATED:
                case STATE_GUIDING:
                    dc.SetPen(wxPen(wxColor(0,255,0)));
                    break;
            }

            dc.DrawLine(0, LockY * m_scaleFactor, XImgSize, LockY * m_scaleFactor);
            dc.DrawLine(LockX * m_scaleFactor, 0, LockX * m_scaleFactor, YImgSize);
        }

        // draw a polar alignment circle
        if (m_polarAlignCircleRadius)
        {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(wxColor(255,0,255)));
            dc.DrawCircle(m_polarAlignCircleCenter.X * m_scaleFactor,
                m_polarAlignCircleCenter.Y * m_scaleFactor, m_polarAlignCircleRadius * m_scaleFactor);
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

void Guider::UpdateImageDisplay(usImage *pImage) {
    if (pImage==NULL)
    {
        pImage = m_pCurrentImage;
    }

    Debug.AddLine("UpdateImageDisplay begins");
    Debug.AddLine("Size=(%d,%d)", pImage->Size.x, pImage->Size.y);
    Debug.AddLine("min=%d, max=%d, FiltMin=%d, FiltMax=%d", pImage->Min, pImage->Max, pImage->FiltMin, pImage->FiltMax);

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
    EvtServer.NotifyLockPositionLost();
}

void Guider::UpdateLockPosition(void)
{
    SetLockPosition(CurrentPosition(), true);
}

bool Guider::SetLockPosition(const PHD_Point& position, bool bExact)
{
    bool bError = false;

    try
    {
        if (!position.IsValid())
        {
            throw ERROR_INFO("Point is not valid");
        }

        double x = position.X;
        double y = position.Y;
        Debug.AddLine(wxString::Format("setting lock position to (%.2f, %.2f)", x, y));

        if ((x < 0.0) || (x >= m_pCurrentImage->Size.x))
        {
            throw ERROR_INFO("invalid x value");
        }

        if ((y < 0.0) || (y >= m_pCurrentImage->Size.y))
        {
            throw ERROR_INFO("invalid y value");
        }

        if (bExact)
        {
            if (!m_lockPosition.IsValid() || position.X != m_lockPosition.X || position.Y != m_lockPosition.Y)
            {
                EvtServer.NotifySetLockPosition(position);
            }
            m_lockPosition.SetXY(x, y);
        }
        else
        {
            SetCurrentPosition(m_pCurrentImage, PHD_Point(x, y));

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

bool Guider::MoveLockPosition(const PHD_Point& mountDelta)
{
    bool bError = false;

    try
    {
        if (!mountDelta.IsValid())
        {
            throw ERROR_INFO("Point is not valid");
        }

        if (!pMount || !pMount->IsCalibrated())
        {
            throw ERROR_INFO("No mount");
        }

        PHD_Point cameraDelta;

        if (pMount->TransformMountCoordinatesToCameraCoordinates(mountDelta, cameraDelta))
        {
            throw ERROR_INFO("Transform failed");
        }

        PHD_Point newLockPosition = m_lockPosition + cameraDelta;

        if (SetLockPosition(newLockPosition, true))
        {
            throw ERROR_INFO("SetLockPosition failed");
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
            switch (m_state)
            {
                case STATE_UNINITIALIZED:
                case STATE_SELECTING:
                case STATE_SELECTED:
                    newState = m_state;
                    break;
                case STATE_CALIBRATING_PRIMARY:
                case STATE_CALIBRATING_SECONDARY:
                    // because we have done some moving here, we need to just
                    // start over...
                    newState = STATE_UNINITIALIZED;
                    break;
                case STATE_CALIBRATED:
                case STATE_GUIDING:
                    newState = STATE_SELECTED;
                    break;
                case STATE_STOP:
                    break;
            }

            if (pMount && pMount->GuidingCeases())
            {
                throw ERROR_INFO("GuidingCeases() failed");
            }
        }

        assert(newState != STATE_STOP);

        if (newState > m_state + 1)
        {
            Debug.AddLine("Cannot transition from %d to  newState=%d", m_state, newState);
            throw ERROR_INFO("Illegal state transition");
        }

        GUIDER_STATE requestedState = newState;

        switch (requestedState)
        {
            case STATE_UNINITIALIZED:
                InvalidateLockPosition();
                InvalidateCurrentPosition();
                newState = STATE_SELECTING;
                break;
            case STATE_SELECTED:
                if (pMount)
                {
                    pMount->ClearHistory();
                }
                break;
            case STATE_CALIBRATING_PRIMARY:
                if (!pMount->IsCalibrated())
                {
                    if (pMount->BeginCalibration(CurrentPosition()))
                    {
                        newState = STATE_UNINITIALIZED;
                        Debug.Write(ERROR_INFO("pMount->BeginCalibration failed"));
                    }
                    else
                    {
                        GuideLog.StartCalibration(pMount);
                        EvtServer.NotifyStartCalibration(pMount);
                    }
                }
                break;
            case STATE_CALIBRATING_SECONDARY:
                if (!pSecondaryMount)
                {
                    newState = STATE_CALIBRATED;
                }
                else if (!pSecondaryMount->IsCalibrated())
                {
                    if (pSecondaryMount->BeginCalibration(CurrentPosition()))
                    {
                        newState = STATE_UNINITIALIZED;
                        Debug.Write(ERROR_INFO("pSecondaryMount->BeginCalibration failed"));
                    }
                    else
                    {
                        GuideLog.StartCalibration(pSecondaryMount);
                        EvtServer.NotifyStartCalibration(pSecondaryMount);
                    }
                }
                break;
            case STATE_GUIDING:
                assert(pMount);

                pMount->AdjustCalibrationForScopePointing();
                if (pSecondaryMount)
                {
                    pSecondaryMount->AdjustCalibrationForScopePointing();
                }

                if (m_lockPosition.IsValid() && m_lockPosIsSticky)
                {
                    Debug.AddLine("keeping sticky lock position");
                }
                else
                {
                    SetLockPosition(CurrentPosition());
                }
                break;

            case STATE_SELECTING:
            case STATE_CALIBRATED:
            case STATE_STOP:
                break;
        }

        if (newState >= requestedState)
        {
            m_state = newState;
        }
        else
        {
            SetState(newState);
        }
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

wxImage *Guider::DisplayedImage(void)
{
    return m_displayedImage;
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

void Guider::StopGuiding(void)
{
    // first, send a notification that we stopped
    switch (m_state)
    {
        case STATE_UNINITIALIZED:
        case STATE_SELECTING:
        case STATE_SELECTED:
            EvtServer.NotifyLoopingStopped();
            break;
        case STATE_CALIBRATING_PRIMARY:
        case STATE_CALIBRATING_SECONDARY:
        case STATE_CALIBRATED:
            EvtServer.NotifyCalibrationFailed(m_state == STATE_CALIBRATING_SECONDARY ? pSecondaryMount : pMount,
                _("Calibration manually stopped"));
            break;
        case STATE_GUIDING:
            EvtServer.NotifyGuidingStopped();
            break;
        case STATE_STOP:
            break;
    }

    SetState(STATE_STOP);
}

void Guider::Reset(void)
{
    SetState(STATE_UNINITIALIZED);
}

/*************  A new image is ready ************************/

void Guider::UpdateGuideState(usImage *pImage, bool bStopping)
{
    wxString statusMessage;

    try
    {
        Debug.Write(wxString::Format("UpdateGuideState(): m_state=%d\n", m_state));

        // switch in the new image

        usImage *pPrevImage = m_pCurrentImage;
        m_pCurrentImage = pImage;
        delete pPrevImage;

        if (bStopping)
        {
            StopGuiding();
            statusMessage = _("Stopped Guiding");
            throw THROW_INFO("Stopped Guiding");
        }

        assert(!pMount || !pMount->IsBusy());

        if (IsPaused())
        {
            statusMessage = _("Paused");
            throw THROW_INFO("Skipping frame - guider is paused");
        }

        if (UpdateCurrentPosition(pImage, statusMessage))
        {
            switch (m_state)
            {
                case STATE_UNINITIALIZED:
                case STATE_SELECTING:
                    EvtServer.NotifyLooping(pFrame->m_frameCounter);
                    break;
                case STATE_SELECTED:
                    // we had a current position and lost it
                    SetState(STATE_UNINITIALIZED);
                    EvtServer.NotifyStarLost();
                    break;
                case STATE_CALIBRATING_PRIMARY:
                case STATE_CALIBRATING_SECONDARY:
                    Debug.AddLine("Star lost during calibration... blundering on");
                    EvtServer.NotifyStarLost();
                    pFrame->SetStatusText(_("star lost"), 1);
                    break;
                case STATE_GUIDING:
                {
                    EvtServer.NotifyStarLost();
                    wxColor prevColor = GetBackgroundColour();
                    SetBackgroundColour(wxColour(64,0,0));
                    ClearBackground();
                    wxBell();
                    wxMilliSleep(100);
                    SetBackgroundColour(prevColor);
                }
                    break;

                case STATE_CALIBRATED:
                case STATE_STOP:
                    break;
            }

            throw THROW_INFO("unable to update current position");
        }

        switch (m_state)
        {
            case STATE_UNINITIALIZED:
            case STATE_SELECTING:
            case STATE_SELECTED:
                EvtServer.NotifyLooping(pFrame->m_frameCounter);
                break;
            case STATE_CALIBRATING_PRIMARY:
            case STATE_CALIBRATING_SECONDARY:
            case STATE_CALIBRATED:
            case STATE_GUIDING:
            case STATE_STOP:
                break;
        }

        switch (m_state)
        {
            case STATE_SELECTING:
                assert(CurrentPosition().IsValid());

                SetLockPosition(CurrentPosition());
                Debug.AddLine("CurrentPosition() valid, moving to STATE_SELECTED");
                EvtServer.NotifyStarSelected(CurrentPosition());
                SetState(STATE_SELECTED);
                break;
            case STATE_SELECTED:
                // nothing to do but wait
                break;
            case STATE_CALIBRATING_PRIMARY:
                if (!pMount->IsCalibrated())
                {
                    if (pMount->UpdateCalibrationState(CurrentPosition()))
                    {
                        SetState(STATE_UNINITIALIZED);
                        throw ERROR_INFO("Calibration failed");
                    }

                    if (!pMount->IsCalibrated())
                    {
                        break;
                    }
                }

                SetState(STATE_CALIBRATING_SECONDARY);

                if (m_state == STATE_CALIBRATING_SECONDARY)
                {
                    // if we really have a secondary mount, and it isn't calibrated,
                    // we need to take another exposure before falling into the code
                    // below.  If we don't have one, or it is calibrated, we can fall
                    // through.  If we don't fall through, we end up displaying a frame
                    // which has the lockpoint in the wrong place, and while I thought I
                    // could live with it when I originally wrote the code, it bothered
                    // me so I did this.  Ick.
                    break;
                }

                // Fall through
            case STATE_CALIBRATING_SECONDARY:
                if (pSecondaryMount)
                {
                    if (!pSecondaryMount->IsCalibrated())
                    {
                        if (pSecondaryMount->UpdateCalibrationState(CurrentPosition()))
                        {
                            SetState(STATE_UNINITIALIZED);
                            throw ERROR_INFO("Calibration failed");
                        }
                    }

                    if (!pSecondaryMount->IsCalibrated())
                    {
                        break;
                    }
                }
                assert(!pSecondaryMount || pSecondaryMount->IsCalibrated());

                SetState(STATE_CALIBRATED);
                // fall through
            case STATE_CALIBRATED:
                assert(m_state == STATE_CALIBRATED);
                SetState(STATE_GUIDING);
                pFrame->SetStatusText(_("Guiding..."), 1);
                pFrame->m_guidingStarted = wxDateTime::UNow();
                pFrame->m_frameCounter = 0;
                GuideLog.StartGuiding();
                EvtServer.NotifyStartGuiding();
                break;
            case STATE_GUIDING:
                pFrame->SchedulePrimaryMove(pMount, CurrentPosition() - LockPosition());
                break;

            case STATE_UNINITIALIZED:
            case STATE_STOP:
                break;
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    // during calibration, the mount is responsible for updating the status message
    if (m_state != STATE_CALIBRATING_PRIMARY && m_state != STATE_CALIBRATING_SECONDARY)
    {
        pFrame->SetStatusText(statusMessage);
    }

    pFrame->UpdateButtonsStatus();

    UpdateImageDisplay(pImage);

    Debug.AddLine("UpdateGuideState exits:" + statusMessage);
}

wxString Guider::GetSettingsSummary()
{
    // return a loggable summary of current global configs managed by MyFrame
    return wxEmptyString;
}

ConfigDialogPane *Guider::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuiderConfigDialogPane(pParent, this);
}

Guider::GuiderConfigDialogPane::GuiderConfigDialogPane(wxWindow *pParent, Guider *pGuider)
    : ConfigDialogPane(_("Guider Settings"), pParent)
{
    m_pGuider = pGuider;
    m_pScaleImage = new wxCheckBox(pParent, wxID_ANY,_("Always Scale Images"));
    DoAdd(m_pScaleImage, _("Always scale images to fill window"));
}

Guider::GuiderConfigDialogPane::~GuiderConfigDialogPane(void)
{
}

void Guider::GuiderConfigDialogPane::LoadValues(void)
{
    m_pScaleImage->SetValue(m_pGuider->GetScaleImage());
}

void Guider::GuiderConfigDialogPane::UnloadValues(void)
{
    m_pGuider->SetScaleImage(m_pScaleImage->GetValue());
}

EXPOSED_STATE Guider::GetExposedState(void)
{
    EXPOSED_STATE rval;
    Guider *guider = pFrame->pGuider;

    if (!guider)
        rval = EXPOSED_STATE_NONE;

    else if (guider->IsPaused())
        rval = EXPOSED_STATE_PAUSED;

    else if (!pFrame->CaptureActive)
        rval = EXPOSED_STATE_NONE;

    else
    {
        // map the guider internal state into a server reported state

        switch (guider->GetState())
        {
            case STATE_UNINITIALIZED:
            case STATE_STOP:
            default:
                rval = EXPOSED_STATE_NONE;
                break;

            case STATE_SELECTING:
                rval = EXPOSED_STATE_LOOPING;
                break;

            case STATE_SELECTED:
            case STATE_CALIBRATED:
                rval = EXPOSED_STATE_SELECTED;
                break;

            case STATE_CALIBRATING_PRIMARY:
            case STATE_CALIBRATING_SECONDARY:
                rval = EXPOSED_STATE_CALIBRATING;
                break;

            case STATE_GUIDING:
                if (guider->IsLocked())
                    rval = EXPOSED_STATE_GUIDING_LOCKED;
                else
                    rval = EXPOSED_STATE_GUIDING_LOST;
        }

        Debug.AddLine(wxString::Format("case statement mapped state %d to %d", guider->GetState(), rval));
    }

    return rval;
}

void Guider::ToggleShowBookmarks()
{
    m_showBookmarks = !m_showBookmarks;
    if (m_bookmarks.size())
    {
        Update();
        Refresh();
    }
}

void Guider::DeleteAllBookmarks()
{
    if (m_bookmarks.size())
    {
        bool confirmed = ConfirmDialog::Confirm(_("Are you sure you want to delete all Bookmarks?"),
            "/delete_all_bookmarks_ok", _("Confirm"));
        if (confirmed)
        {
            m_bookmarks.clear();
            if (m_showBookmarks)
            {
                Update();
                Refresh();
            }
        }
    }
}

static bool IsClose(const wxRealPoint& p1, const wxRealPoint& p2, double tolerance)
{
    return fabs(p1.x - p2.x) <= tolerance &&
        fabs(p1.y - p2.y) <= tolerance;
}

static std::vector<wxRealPoint>::iterator FindBookmark(const wxRealPoint& pos, std::vector<wxRealPoint>& vec)
{
    static const double TOLERANCE = 6.0;
    std::vector<wxRealPoint>::iterator it;
    for (it = vec.begin(); it != vec.end(); ++it)
        if (IsClose(*it, pos, TOLERANCE))
            break;
    return it;
}

void Guider::ToggleBookmark(const wxRealPoint& pos)
{
    std::vector<wxRealPoint>::iterator it = FindBookmark(pos, m_bookmarks);
    if (it == m_bookmarks.end())
        m_bookmarks.push_back(pos);
    else
        m_bookmarks.erase(it);
}

void Guider::BookmarkLockPosition()
{
    if (m_lockPosition.IsValid())
    {
        wxRealPoint pt(m_lockPosition.X, m_lockPosition.Y);
        std::vector<wxRealPoint>::iterator it = FindBookmark(pt, m_bookmarks);
        if (it != m_bookmarks.end())
            m_bookmarks.erase(it);
        m_bookmarks.push_back(pt);
        if (m_showBookmarks)
        {
            Update();
            Refresh();
        }
    }
}
