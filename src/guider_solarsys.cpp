/*
 *  guider_solarsys.cpp
 *  PHD Guiding

 *  Original guider_onestar Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  guider_onestar completely refactored by Bret McKee
 *  Copyright (c) 2012 Bret McKee
 *  All rights reserved.
 *
 *  guider_solarSys adaptation created by Bruce Waddington
 *  to integrate earlier work by Leo Schatz
 *
 *  Copyright (c) 2025 Bruce Waddington
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
#include "solarsys.h"
#include "solarsys_tool.h"

#include <wx/dir.h>
#include <algorithm>

#if ((wxMAJOR_VERSION < 3) && (wxMINOR_VERSION < 9))
# define wxPENSTYLE_DOT wxDOT
#endif

// clang-format off
wxBEGIN_EVENT_TABLE(GuiderSolarSys, Guider)
    EVT_PAINT(GuiderSolarSys::OnPaint)
    EVT_LEFT_DOWN(GuiderSolarSys::OnLClick)
wxEND_EVENT_TABLE();
// clang-format on

// Define a constructor for the guide canvas
GuiderSolarSys::GuiderSolarSys(wxWindow *parent) : Guider(parent, XWinSize, YWinSize), m_lockPositionMoved(false)
{
    SetState(STATE_UNINITIALIZED);
}

void GuiderSolarSys::LoadProfileSettings()
{
    Guider::LoadProfileSettings();
    if (m_SolarSystemObject != nullptr)
    {
        m_SolarSystemObject->RestoreDetectionParams();
        // Profile changed in solar mode, tool window already displayed
        if (pFrame && pFrame->pSolarSysTool != nullptr)
        {
            PlanetTool::RestoreProfileSettings();
        }
    }
}

bool GuiderSolarSys::SetTolerateJumps(bool enable, double threshold)
{
    return false;
}

bool GuiderSolarSys::SetCurrentPosition(const usImage *pImage, const PHD_Point& position)
{
    bool bError = true;

    try
    {
        if (!position.IsValid())
        {
            throw ERROR_INFO("position is invalid");
        }

        double x = position.X;
        double y = position.Y;

        Debug.Write(wxString::Format("SetCurrentPosition(%.2f,%.2f)\n", x, y));

        if ((x <= 0) || (x >= pImage->Size.x))
        {
            throw ERROR_INFO("invalid x value");
        }

        if ((y <= 0) || (y >= pImage->Size.y))
        {
            throw ERROR_INFO("invalid y value");
        }

        bError = !m_SolarSystemObject->FindDisk(pImage, false, &m_primaryStar);
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    return bError;
}

static wxString StarStatusStr(const Star& star)
{
    if (!star.IsValid())
        return _("No target selected");

    switch (star.GetError())
    {
    case Star::STAR_LOWSNR:
        return _("Star lost - low SNR");
    case Star::STAR_LOWMASS:
        return _("Star lost - low mass");
    case Star::STAR_LOWHFD:
        return _("Star lost - low HFD");
    case Star::STAR_TOO_NEAR_EDGE:
        return _("Star too near edge");
    case Star::STAR_MASSCHANGE:
        return _("Star lost - mass changed");
    default:
        return _("No star found");
    }
}

static wxString StarStatus(const Star& star)
{
    wxString status = wxString::Format(_("m=%.0f SNR=%.1f"), star.Mass, star.SNR);

    if (star.GetError() == Star::STAR_SATURATED)
        status += _T(" ") + _("Saturated");

    int exp;
    bool auto_exp;
    pFrame->GetExposureInfo(&exp, &auto_exp);

    if (auto_exp)
    {
        status += _T(" ");
        if (exp >= 1)
            status += wxString::Format(_("Exp=%0.1f s"), (double) exp / 1000.);
        else
            status += wxString::Format(_("Exp=%d ms"), exp);
    }

    return status;
}

bool GuiderSolarSys::AutoSelect(const wxRect& roi)
{
    Debug.Write("GuiderSolarSys::AutoSelect enter\n");

    bool error = false;

    usImage *image = CurrentImage();

    try
    {
        if (!image || !image->ImageData)
        {
            throw ERROR_INFO("No Current Image");
        }

        // If mount is not calibrated, we need to choose a star a bit farther
        // from the egde to allow for the motion of the star during
        // calibration
        //
        int edgeAllowance = 0;
        if (pMount && pMount->IsConnected() && !pMount->IsCalibrated())
            edgeAllowance = wxMax(edgeAllowance, pMount->CalibrationTotDistance());
        if (pSecondaryMount && pSecondaryMount->IsConnected() && !pSecondaryMount->IsCalibrated())
            edgeAllowance = wxMax(edgeAllowance, pSecondaryMount->CalibrationTotDistance());

        Star newDisk;
        if (!m_SolarSystemObject->AutoFindDisk(*image, &newDisk))
        {
            throw ERROR_INFO("Unable to AutoFind");
        }

        if (!m_SolarSystemObject->FindDisk(image, false, &newDisk))
        {
            throw ERROR_INFO("Unabled to find");
        }
        m_primaryStar = newDisk;

        if (SetLockPosition(m_primaryStar))
        {
            throw ERROR_INFO("Unable to set Lock Position");
        }

        if (GetState() == STATE_SELECTING)
        {
            // immediately advance the state machine now, rather than waiting for
            // the next exposure to complete. Socket server clients are going to
            // try to start guiding after selecting the star, but guiding will fail
            // to start if state is still STATE_SELECTING
            Debug.Write(wxString::Format("AutoSelect: state = %d, call UpdateGuideState\n", GetState()));
            UpdateGuideState(NULL, false);
        }

        UpdateImageDisplay();

        pFrame->StatusMsg(wxString::Format(_("Auto-selected disk at (%.1f, %.1f)"), m_primaryStar.X, m_primaryStar.Y));
        pFrame->UpdateStatusBarStarInfo(m_primaryStar.SNR, m_primaryStar.GetError() == Star::STAR_SATURATED);
        pFrame->pProfile->UpdateData(image, m_primaryStar.X, m_primaryStar.Y);
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
    }

    if (image && image->ImageData)
    {
        if (error)
            Debug.Write("GuiderSolarSys::AutoSelect failed.\n");

        ImageLogger::LogAutoSelectImage(image, !error);
    }

    return error;
}

inline static wxRect SubframeRect(const PHD_Point& pos, int halfwidth)
{
    return wxRect(ROUND(pos.X) - halfwidth, ROUND(pos.Y) - halfwidth, 2 * halfwidth + 1, 2 * halfwidth + 1);
}

wxRect GuiderSolarSys::GetBoundingBox() const
{
    enum
    {
        SUBFRAME_BOUNDARY_PX = 0
    };

    GUIDER_STATE state = GetState();

    bool subframe;
    PHD_Point pos;

    switch (state)
    {
    case STATE_SELECTED:
    case STATE_CALIBRATING_PRIMARY:
    case STATE_CALIBRATING_SECONDARY:
        subframe = m_primaryStar.WasFound();
        pos = CurrentPosition();
        break;
    case STATE_GUIDING:
    {
        subframe = m_primaryStar.WasFound(); // true;
        // As long as the star is close to the lock position, keep the subframe
        // at the lock position. Otherwise, follow the star.
        double dist = CurrentPosition().Distance(LockPosition());
        if ((int) dist > m_searchRegion / 3)
            pos = CurrentPosition();
        else
            pos = LockPosition();
        break;
    }
    default:
        subframe = false;
    }

    if (m_forceFullFrame)
    {
        subframe = false;
    }

    if (subframe)
    {
        wxRect box(SubframeRect(pos, m_searchRegion + SUBFRAME_BOUNDARY_PX));
        box.Intersect(wxRect(pCamera->FrameSize));
        return box;
    }
    else
    {
        return wxRect(0, 0, 0, 0);
    }
}

void GuiderSolarSys::InvalidateCurrentPosition(bool fullReset)
{
    m_primaryStar.Invalidate();

    if (fullReset)
    {
        m_primaryStar.X = m_primaryStar.Y = 0.0;
    }
}

wxString GuiderSolarSys::GetStarCount() const
{
    return _("Disk");
}

bool GuiderSolarSys::UpdateCurrentPosition(const usImage *pImage, GuiderOffset *ofs, FrameDroppedInfo *errorInfo)
{
    if (!m_primaryStar.IsValid() && m_primaryStar.X == 0.0 && m_primaryStar.Y == 0.0)
    {
        Debug.Write("UpdateCurrentPosition: no target selected\n");
        errorInfo->starError = Star::STAR_ERROR;
        errorInfo->starMass = 0.0;
        errorInfo->starSNR = 0.0;
        errorInfo->starHFD = 0.0;
        errorInfo->status = _("No target selected");
        ImageLogger::LogImageStarDeselected(pImage);
        return true;
    }

    bool bError = false;

    try
    {
        Star newStar(m_primaryStar);

        if (!m_SolarSystemObject->FindDisk(pImage, false, &newStar))
        {
            errorInfo->starError = newStar.GetError();
            errorInfo->starMass = 0.0;
            errorInfo->starSNR = 0.0;
            errorInfo->starHFD = 0.0;
            errorInfo->status = m_SolarSystemObject->m_statusMsg;
            m_primaryStar.SetError(newStar.GetError());

            ImageLogger::LogImage(pImage, *errorInfo);

            throw ERROR_INFO("UpdateCurrentPosition():newStar not found");
        }

        const PHD_Point& lockPos = LockPosition();
        double distance;
        bool raOnly = MyFrame::GuidingRAOnly();
        if (lockPos.IsValid())
        {
            if (raOnly)
                distance = fabs(newStar.X - lockPos.X);
            else
                distance = newStar.Distance(lockPos);
        }
        else
            distance = 0.;

        // double tolerance = m_tolerateJumpsEnabled ? m_tolerateJumpsThreshold : 9e99;

        ImageLogger::LogImage(pImage, distance);

        // update the star position, mass, etc.
        m_primaryStar = newStar;

        if (lockPos.IsValid())
        {
            ofs->cameraOfs = m_primaryStar - lockPos;
            m_starsUsed = 1;

            if (pMount && pMount->IsCalibrated())
                pMount->TransformCameraCoordinatesToMountCoordinates(ofs->cameraOfs, ofs->mountOfs, true);
            double distanceRA = ofs->mountOfs.IsValid() ? fabs(ofs->mountOfs.X) : 0.;
            UpdateCurrentDistance(distance, distanceRA);
        }

        pFrame->pProfile->UpdateData(pImage, m_primaryStar.X, m_primaryStar.Y);

        pFrame->AdjustAutoExposure(m_primaryStar.SNR);
        pFrame->UpdateStatusBarStarInfo(m_primaryStar.SNR, m_primaryStar.GetError() == Star::STAR_SATURATED);
        errorInfo->status = StarStatus(m_primaryStar);

        // Show sun/moon/planet position after successful detection
        if (GetState() != STATE_GUIDING)
        {
            wxString statusMsg;
            m_SolarSystemObject->GetDetectionStatus(statusMsg);
            pFrame->StatusMsg(statusMsg);
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        pFrame->ResetAutoExposure(); // use max exposure duration
    }

    return bError;
}

bool GuiderSolarSys::SetLockPosition(const PHD_Point& position)
{
    if (!Guider::SetLockPosition(position))
    {
        return false;
    }
    else
        return true;
}

bool GuiderSolarSys::IsValidLockPosition(const PHD_Point& pt)
{
    const usImage *pImage = CurrentImage();
    if (!pImage)
        return false;
    // this is a bit ugly as it is tightly coupled to Star::Find
    return pt.X >= 1 + m_searchRegion && pt.X + 1 + m_searchRegion < pImage->Size.GetX() && pt.Y >= 1 + m_searchRegion &&
        pt.Y + 1 + m_searchRegion < pImage->Size.GetY();
}

bool GuiderSolarSys::IsValidSecondaryStarPosition(const PHD_Point& pt)
{
    const usImage *pImage = CurrentImage();
    if (!pImage)
        return false;
    // As above, tightly coupled to Star::Find but with somewhat relaxed constraints. Find handles cases where search region is
    // only partly within image
    return pt.X >= 5 && pt.X + 5 < pImage->Size.GetX() && pt.Y >= 5 && pt.Y + 5 < pImage->Size.GetY();
}

void GuiderSolarSys::OnLClick(wxMouseEvent& mevent)
{
    try
    {
        if (mevent.GetModifiers() == wxMOD_CONTROL)
        {
            double const scaleFactor = ScaleFactor();
            wxRealPoint pt((double) mevent.m_x / scaleFactor, (double) mevent.m_y / scaleFactor);
            ToggleBookmark(pt);
            m_showBookmarks = true;
            pFrame->bookmarks_menu->Check(MENU_BOOKMARKS_SHOW, GetBookmarksShown());
            Refresh();
            Update();
            return;
        }

        if (GetState() > STATE_SELECTED)
        {
            mevent.Skip();
            throw THROW_INFO("Skipping event because state > STATE_SELECTED");
        }

        if (mevent.GetModifiers() == wxMOD_SHIFT)
        {
            // Deselect guide star
            Debug.Write(wxS("manual deselect\n"));
            InvalidateCurrentPosition(true);
        }
        else
        {
            if ((mevent.m_x <= m_searchRegion) || (mevent.m_x + m_searchRegion >= XWinSize) || (mevent.m_y <= m_searchRegion) ||
                (mevent.m_y + m_searchRegion >= YWinSize))
            {
                mevent.Skip();
                throw THROW_INFO("Skipping event because click outside of search region");
            }

            usImage *pImage = CurrentImage();

            if (pImage->NPixels == 0)
            {
                mevent.Skip();
                throw ERROR_INFO("Skipping event m_pCurrentImage->NPixels == 0");
            }

            double scaleFactor = ScaleFactor();
            double StarX = (double) mevent.m_x / scaleFactor;
            double StarY = (double) mevent.m_y / scaleFactor;

            m_SolarSystemObject->m_clicked_x = wxMin(StarX, pImage->Size.GetWidth() - 1);
            m_SolarSystemObject->m_clicked_y = wxMin(StarY, pImage->Size.GetHeight() - 1);
            m_SolarSystemObject->m_userLClick = true;
            m_SolarSystemObject->m_detectionCounter = 0;

            SetCurrentPosition(pImage, PHD_Point(StarX, StarY));

            if (!m_primaryStar.IsValid())
            {
                pFrame->StatusMsg(wxString::Format(_("No star found")));
            }
            else
            {
                SetLockPosition(m_primaryStar);
                Debug.Write("Solar system: target forced by user star selection\n");
                pFrame->StatusMsg(
                    wxString::Format(_("Selected %s at (%.1f, %.1f)"), _("Disc"), m_primaryStar.X, m_primaryStar.Y));
                pFrame->UpdateStatusBarStarInfo(m_primaryStar.SNR, m_primaryStar.GetError() == Star::STAR_SATURATED);
                EvtServer.NotifyStarSelected(CurrentPosition());
                SetState(STATE_SELECTED);
                pFrame->UpdateButtonsStatus();
                pFrame->pProfile->UpdateData(pImage, m_primaryStar.X, m_primaryStar.Y);
            }

            if (pFrame->pSolarSysTool)
                PlanetTool::ShowDiameters(true); // If user has click somewhere, show him the current min/max diameters
            Refresh();
            Update();
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

inline static void DrawBox(SolarSystemObject *ssoHelper, wxDC& dc, const PHD_Point& star, int halfW, double scale)
{
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    halfW = 10;
    double w = ROUND((halfW * 2 + 1) * scale);
    int xpos = int((star.X - halfW) * scale);
    int ypos = int((star.Y - halfW) * scale);

    // Clip drawing region to displayed image frame
    wxImage *pImg = pFrame->pGuider->DisplayedImage();
    if (pImg)
        dc.SetClippingRegion(wxRect(0, 0, pImg->GetWidth(), pImg->GetHeight()));

    if (ssoHelper->m_detected)
    {
        int x = int(star.X * scale + 0.5);
        int y = int(star.Y * scale + 0.5);
        int r = int(ssoHelper->m_radius * scale + 0.5);
        dc.DrawCircle(x, y, r);
        dc.SetPen(wxPen(dc.GetPen().GetColour(), 1, dc.GetPen().GetStyle()));
        dc.DrawRectangle(xpos, ypos, w, w);
    }

    // Replaces visual bell for paused detection while guiding
    if (ssoHelper->GetDetectionPausedState())
    {
        static int dash = 0;
        static wxDash dashPattern[4][4] = {
            /* d  g  d  g */
            { 4, 2, 4, 2 },
            { 4, 3, 4, 3 },
            { 4, 4, 4, 4 },
            { 4, 3, 4, 3 },
        };

        // Create a pen with the custom dash pattern
        dash = (dash + 1) % 4;
        wxPen pen(wxColour(230, 30, 30), 4, wxPENSTYLE_USER_DASH);
        pen.SetDashes(4, dashPattern[dash]);
        dc.SetPen(pen);

        int x = int(star.X * scale + 0.5);
        int y = int(star.Y * scale + 0.5);
        int r = int(ssoHelper->m_radius * scale + 0.5);
        dc.DrawCircle(x, y, r);
    }

    // Show active processing region (ROI)
    if (ssoHelper->m_roiActive && pFrame->CaptureActive)
    {
        dc.SetPen(wxPen(wxColour(200, 200, 200), 2, wxPENSTYLE_SHORT_DASH));
        dc.DrawRectangle(ssoHelper->m_roiRect.x * scale, ssoHelper->m_roiRect.y * scale, ssoHelper->m_roiRect.width * scale,
                         ssoHelper->m_roiRect.height * scale);
    }

    dc.DestroyClippingRegion();
}

void GuiderSolarSys::SetImageDisplayWindow(wxWindow *dispWindow)
{
    m_ImgDisplayWindow = dispWindow;
}

bool GuiderSolarSys::PaintHelper(wxAutoBufferedPaintDCBase& dc, wxMemoryDC& memDC)
{
    bool bError = false;

    try
    {
        GUIDER_STATE state = GetState();
        m_ImgDisplayWindow->GetSize(&XWinSize, &YWinSize);

        if (m_pCurrentImage->ImageData)
        {
            int blevel = m_pCurrentImage->FiltMin;
            int wlevel = m_pCurrentImage->FiltMax;
            m_pCurrentImage->CopyToImage(&m_displayedImage, blevel, wlevel, pFrame->Stretch_gamma);
        }

        int imageWidth = m_displayedImage->GetWidth();
        int imageHeight = m_displayedImage->GetHeight();

        // scale the image if necessary

        if (imageWidth != XWinSize || imageHeight != YWinSize)
        {
            // The image is not the exact right size -- figure out what to do.
            double xScaleFactor = imageWidth / (double) XWinSize;
            double yScaleFactor = imageHeight / (double) YWinSize;
            int newWidth = imageWidth;
            int newHeight = imageHeight;

            double newScaleFactor = (xScaleFactor > yScaleFactor) ? xScaleFactor : yScaleFactor;

            //            Debug.Write(wxString::Format("xScaleFactor=%.2f, yScaleFactor=%.2f, newScaleFactor=%.2f\n",
            //            xScaleFactor,
            //                    yScaleFactor, newScaleFactor));

            // we rescale the image if:
            // - The image is either too big
            // - The image is so small that at least one dimension is less
            //   than half the width of the window or
            // - The user has requested rescaling

            if (xScaleFactor > 1.0 || yScaleFactor > 1.0 || xScaleFactor < 0.45 || yScaleFactor < 0.45 || m_scaleImage)
            {

                newWidth /= newScaleFactor;
                newHeight /= newScaleFactor;

                newScaleFactor = 1.0 / newScaleFactor;

                m_scaleFactor = newScaleFactor;

                if (imageWidth != newWidth || imageHeight != newHeight)
                {
                    // Debug.Write(wxString::Format("Resizing image to %d,%d\n", newWidth, newHeight));

                    if (newWidth > 0 && newHeight > 0)
                    {
                        m_displayedImage->Rescale(newWidth, newHeight, wxIMAGE_QUALITY_NORMAL);
                    }
                }
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
            dc.SetPen(wxPen(wxColor(200, 50, 50)));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);

            switch (m_overlayMode)
            {
            case OVERLAY_BULLSEYE:
            {
                int cx = XImgSize / 2;
                int cy = YImgSize / 2;
                dc.DrawCircle(cx, cy, 25);
                dc.DrawCircle(cx, cy, 50);
                dc.DrawCircle(cx, cy, 100);
                dc.DrawLine(0, cy, XImgSize, cy);
                dc.DrawLine(cx, 0, cx, YImgSize);
                break;
            }
            case OVERLAY_GRID_FINE:
            case OVERLAY_GRID_COARSE:
            {
                int i;
                int size = (m_overlayMode - 1) * 20;
                for (i = size; i < XImgSize; i += size)
                    dc.DrawLine(i, 0, i, YImgSize);
                for (i = size; i < YImgSize; i += size)
                    dc.DrawLine(0, i, XImgSize, i);
                break;
            }

            case OVERLAY_RADEC:
            {
                Mount *mount = TheScope();
                if (mount)
                {
                    double StarX = CurrentPosition().X;
                    double StarY = CurrentPosition().Y;

                    double r = 15.0;
                    double rlabel = r + 9.0;

                    double wAngle = mount->IsCalibrated() ? mount->xAngle() : 0.0;
                    double eAngle = wAngle + M_PI;
                    GuideParity raParity = mount->RAParity();
                    if (raParity == GUIDE_PARITY_ODD)
                    {
                        // odd parity => West calibration pulses move scope East
                        //   => star moves West
                        //   => East vector is opposite direction from X calibration vector (West calibration direction)
                        eAngle += M_PI;
                    }
                    double cos_eangle = cos(eAngle);
                    double sin_eangle = sin(eAngle);
                    dc.SetPen(wxPen(pFrame->pGraphLog->GetRaOrDxColor(), 2, wxPENSTYLE_DOT));
                    dc.DrawLine(ROUND(StarX * m_scaleFactor + r * cos_eangle), ROUND(StarY * m_scaleFactor + r * sin_eangle),
                                ROUND(StarX * m_scaleFactor - r * cos_eangle), ROUND(StarY * m_scaleFactor - r * sin_eangle));
                    if (raParity != GUIDE_PARITY_UNKNOWN)
                    {
                        dc.SetTextForeground(pFrame->pGraphLog->GetRaOrDxColor());
                        dc.DrawText(_("E"), ROUND(StarX * m_scaleFactor + rlabel * cos_eangle) - 4,
                                    ROUND(StarY * m_scaleFactor + rlabel * sin_eangle) - 6);
                    }

                    double nAngle = mount->IsCalibrated() ? mount->yAngle() : M_PI / 2.0;
                    GuideParity decParity = mount->DecParity();
                    if (decParity == GUIDE_PARITY_EVEN)
                    {
                        // even parity => North calibration pulses move scope North
                        //   => star moves South
                        //   => North vector is opposite direction from Y calibration vector (North calibration direction)
                        nAngle += M_PI;
                    }
                    double cos_nangle = cos(nAngle);
                    double sin_nangle = sin(nAngle);
                    dc.SetPen(wxPen(pFrame->pGraphLog->GetDecOrDyColor(), 2, wxPENSTYLE_DOT));
                    dc.DrawLine(ROUND(StarX * m_scaleFactor + r * cos_nangle), ROUND(StarY * m_scaleFactor + r * sin_nangle),
                                ROUND(StarX * m_scaleFactor - r * cos_nangle), ROUND(StarY * m_scaleFactor - r * sin_nangle));
                    if (decParity != GUIDE_PARITY_UNKNOWN)
                    {
                        dc.SetTextForeground(pFrame->pGraphLog->GetDecOrDyColor());
                        dc.DrawText(_("N"), ROUND(StarX * m_scaleFactor + rlabel * cos_nangle) - 4,
                                    ROUND(StarY * m_scaleFactor + rlabel * sin_nangle) - 6);
                    }

                    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
                    gc->SetPen(wxPen(pFrame->pGraphLog->GetRaOrDxColor(), 1, wxPENSTYLE_DOT));
                    double step = (double) YImgSize / 10.0;

                    double MidX = (double) XImgSize / 2.0;
                    double MidY = (double) YImgSize / 2.0;
                    gc->Rotate(eAngle);
                    gc->GetTransform().TransformPoint(&MidX, &MidY);
                    gc->Rotate(-eAngle);
                    gc->Translate((double) XImgSize / 2.0 - MidX, (double) YImgSize / 2.0 - MidY);
                    gc->Rotate(eAngle);
                    for (int i = -2; i < 12; i++)
                    {
                        gc->StrokeLine(0.0, step * (double) i, (double) XImgSize, step * (double) i);
                    }

                    MidX = (double) XImgSize / 2.0;
                    MidY = (double) YImgSize / 2.0;
                    gc->Rotate(-eAngle);
                    gc->Rotate(nAngle);
                    gc->GetTransform().TransformPoint(&MidX, &MidY);
                    gc->Rotate(-nAngle);
                    gc->Translate((double) XImgSize / 2.0 - MidX, (double) YImgSize / 2.0 - MidY);
                    gc->Rotate(nAngle);
                    gc->SetPen(wxPen(pFrame->pGraphLog->GetDecOrDyColor(), 1, wxPENSTYLE_DOT));
                    for (int i = -2; i < 12; i++)
                    {
                        gc->StrokeLine(0.0, step * (double) i, (double) XImgSize, step * (double) i);
                    }
                    delete gc;
                }
                break;
            }

            case OVERLAY_SLIT:
                break;

            case OVERLAY_NONE:
                break;
            }
        }

        // draw the lockpoint if there is one
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
                dc.SetPen(wxPen(wxColor(255, 255, 0), 1, wxPENSTYLE_DOT));
                break;
            case STATE_CALIBRATED:
            case STATE_GUIDING:
                dc.SetPen(wxPen(wxColor(0, 255, 0)));
                break;
            }

            dc.DrawLine(0, int(LockY * m_scaleFactor), XImgSize, int(LockY * m_scaleFactor));
            dc.DrawLine(int(LockX * m_scaleFactor), 0, int(LockX * m_scaleFactor), YImgSize);
        }

        if (IsPaused())
        {
            dc.SetTextForeground(*wxYELLOW);
            dc.DrawText(_("PAUSED"), 10, YWinSize - 20);
        }
        else if (pMount && !pMount->GetGuidingEnabled())
        {
            dc.SetTextForeground(*wxYELLOW);
            dc.DrawText(_("Guide output DISABLED"), 10, YWinSize - 20);
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

// Define the repainting behaviour
void GuiderSolarSys::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    wxMemoryDC memDC;

    try
    {
        int w, h;
        m_ImgDisplayWindow->GetSize(&w, &h);
        if (PaintHelper(dc, memDC))
        {
            throw ERROR_INFO("PaintHelper failed");
        }
        // PaintHelper drew the image and any overlays
        // now decorate the image to show the selection

        // display bookmarks
        if (m_showBookmarks && m_bookmarks.size() > 0)
        {
            dc.SetPen(wxPen(wxColour(0, 255, 255), 1, wxPENSTYLE_SOLID));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);

            for (std::vector<wxRealPoint>::const_iterator it = m_bookmarks.begin(); it != m_bookmarks.end(); ++it)
            {
                wxPoint p((int) (it->x * m_scaleFactor), (int) (it->y * m_scaleFactor));
                dc.DrawCircle(p, 3);
                dc.DrawCircle(p, 6);
                dc.DrawCircle(p, 12);
            }
        }

        GUIDER_STATE state = GetState();
        bool FoundStar = m_primaryStar.WasFound();

        int thickness = 4;
        if (state == STATE_SELECTED)
        {
            if (FoundStar)
                dc.SetPen(wxPen(wxColour(100, 255, 90), thickness, wxPENSTYLE_SOLID)); // Draw the box around the star
            else
                dc.SetPen(wxPen(wxColour(230, 130, 30), thickness, wxPENSTYLE_DOT));
            DrawBox(m_SolarSystemObject, dc, m_primaryStar, m_searchRegion, m_scaleFactor);
        }
        else if (state == STATE_CALIBRATING_PRIMARY || state == STATE_CALIBRATING_SECONDARY)
        {
            // in the calibration process
            dc.SetPen(wxPen(wxColour(32, 196, 32), thickness, wxPENSTYLE_SOLID)); // Draw the box around the star
            DrawBox(m_SolarSystemObject, dc, m_primaryStar, m_searchRegion, m_scaleFactor);
        }
        else if (state == STATE_CALIBRATED || state == STATE_GUIDING)
        {
            // locked and guiding
            if (FoundStar)
                dc.SetPen(wxPen(wxColour(32, 196, 32), thickness, wxPENSTYLE_SOLID)); // Draw the box around the star
            else
                dc.SetPen(wxPen(wxColour(230, 130, 30), thickness, wxPENSTYLE_DOT));
            DrawBox(m_SolarSystemObject, dc, m_primaryStar, m_searchRegion, m_scaleFactor);
        }

        // Display visual elements to assist with tuning the solar and planetary detection parameters
        m_SolarSystemObject->VisualHelper(dc, m_primaryStar, m_scaleFactor);
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

void GuiderSolarSys::SaveStarFITS()
{
    double StarX = m_primaryStar.X;
    double StarY = m_primaryStar.Y;
    usImage *pImage = CurrentImage();
    usImage tmpimg;
    wxString imgLogDirectory;

    tmpimg.Init(60, 60);
    int start_x = ROUND(StarX) - 30;
    int start_y = ROUND(StarY) - 30;
    if ((start_x + 60) > pImage->Size.GetWidth())
        start_x = pImage->Size.GetWidth() - 60;
    if ((start_y + 60) > pImage->Size.GetHeight())
        start_y = pImage->Size.GetHeight() - 60;
    int x, y, width;
    width = pImage->Size.GetWidth();
    unsigned short *usptr = tmpimg.ImageData;
    for (y = 0; y < 60; y++)
    {
        for (x = 0; x < 60; x++, usptr++)
            *usptr = *(pImage->ImageData + (y + start_y) * width + (x + start_x));
    }

    imgLogDirectory = Debug.GetLogDir() + PATHSEPSTR + "PHD2_Stars";
    if (!wxDirExists(imgLogDirectory))
        wxFileName::Mkdir(imgLogDirectory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxString fname = imgLogDirectory + PATHSEPSTR + "PHD_GuideStar" + wxDateTime::Now().Format(_T("_%j_%H%M%S")) + ".fit";

    fitsfile *fptr; // FITS file pointer
    int status = 0; // CFITSIO status value MUST be initialized to zero!

    PHD_fits_create_file(&fptr, fname, false, &status);

    if (!status)
    {
        long fsize[] = { 60, 60 };
        fits_create_img(fptr, USHORT_IMG, 2, fsize, &status);

        FITSHdrWriter hdr(fptr, &status);

        hdr.write("DATE", wxDateTime::UNow(), wxDateTime::UTC, "file creation time, UTC");
        hdr.write("DATE-OBS", pImage->ImgStartTime, wxDateTime::UTC, "image capture start time, UTC");
        hdr.write("EXPOSURE", (float) pImage->ImgExpDur / 1000.0f, "Exposure time [s]");
        hdr.write("XBINNING", (unsigned int) pCamera->Binning, "Camera X binning");
        hdr.write("YBINNING", (unsigned int) pCamera->Binning, "Camera Y binning");
        hdr.write("XORGSUB", start_x, "Subframe x position in binned pixels");
        hdr.write("YORGSUB", start_y, "Subframe y position in binned pixels");

        if (!status)
        {
            long fpixel[] = { 1, 1, 1 };
            fits_write_pix(fptr, TUSHORT, fpixel, tmpimg.NPixels, tmpimg.ImageData, &status);
        }
    }

    PHD_fits_close_file(fptr);
}

wxString GuiderSolarSys::GetSettingsSummary() const
{
    return _("Solar system mode");
}

Guider::GuiderConfigDialogPane *GuiderSolarSys::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuiderSolarSysConfigDialogPane(pParent, this);
}

GuiderSolarSys::GuiderSolarSysConfigDialogPane::GuiderSolarSysConfigDialogPane(wxWindow *pParent, GuiderSolarSys *pGuider)
    : GuiderConfigDialogPane(pParent, pGuider)
{
}

void GuiderSolarSys::GuiderSolarSysConfigDialogPane::LayoutControls(Guider *pGuider, BrainCtrlIdMap& CtrlMap)
{
    GuiderConfigDialogPane::LayoutControls(pGuider, CtrlMap);
}

GuiderConfigDialogCtrlSet *GuiderSolarSys::GetConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider,
                                                                  AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap)
{
    return new GuiderSolarSysConfigDialogCtrlSet(pParent, pGuider, pAdvancedDialog, CtrlMap);
}

GuiderSolarSysConfigDialogCtrlSet::GuiderSolarSysConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider,
                                                                     AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap)
    : GuiderConfigDialogCtrlSet(pParent, pGuider, pAdvancedDialog, CtrlMap)
{
    // The following minimal control is required by the Advanced Settings dialog in order to allow switching back and forth
    // between stellar and solar guiding
    wxStaticText *pLabel = new wxStaticText(GetParentWindow(AD_szStarTracking), wxID_ANY,
                                            _("In solar system mode, tracking parameters are handled in the Tool window"));
    wxBoxSizer *pLabelSizer = new wxBoxSizer(wxHORIZONTAL);
    pLabelSizer->Add(pLabel, wxSizerFlags().Align(wxALIGN_CENTER_HORIZONTAL));
    wxFlexGridSizer *pTrackingParams = new wxFlexGridSizer(3, 2, 8, 15);
    pTrackingParams->Add(pLabelSizer, wxSizerFlags(0).Border(wxTOP, 12));

    AddGroup(CtrlMap, AD_szStarTracking, pTrackingParams);
}

GuiderSolarSysConfigDialogCtrlSet::~GuiderSolarSysConfigDialogCtrlSet() { }

void GuiderSolarSysConfigDialogCtrlSet::LoadValues()
{
    GuiderConfigDialogCtrlSet::LoadValues();
}

void GuiderSolarSysConfigDialogCtrlSet::UnloadValues()
{
    GuiderConfigDialogCtrlSet::UnloadValues();
}
