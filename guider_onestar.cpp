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
#include <wx/dir.h>
#include <algorithm>

#define SCALE_UP_SMALL  // Currently problematic as the box for the star is drawn in the wrong spot.

#if ((wxMAJOR_VERSION < 3) && (wxMINOR_VERSION < 9))
#define wxPENSTYLE_DOT wxDOT
#endif

class MassChecker
{
    enum { DefaultTimeWindowMs = 15000 };

    struct Entry
    {
        wxLongLong_t time;
        double mass;
    };

    std::deque<Entry> m_data;
    unsigned long m_timeWindow;
    double *m_tmp;
    size_t m_tmpSize;
    int m_lastExposure;

public:

    MassChecker()
        : m_tmp(0),
          m_tmpSize(0),
          m_lastExposure(0)
    {
        SetTimeWindow(DefaultTimeWindowMs);
    }

    ~MassChecker()
    {
        delete[] m_tmp;
    }

    void SetTimeWindow(unsigned int milliseconds)
    {
        // an abrupt change in mass will affect the median after approx m_timeWindow/2
        m_timeWindow = milliseconds * 2;
    }

    void SetExposure(int exposure)
    {
        if (exposure != m_lastExposure)
        {
            m_lastExposure = exposure;
            Reset();
        }
    }

    void AppendData(double mass)
    {
        wxLongLong_t now = ::wxGetUTCTimeMillis().GetValue();
        wxLongLong_t oldest = now - m_timeWindow;

        while (m_data.size() > 0 && m_data.front().time < oldest)
            m_data.pop_front();

        Entry entry;
        entry.time = now;
        entry.mass = mass;
        m_data.push_back(entry);
    }

    bool CheckMass(double mass, double threshold, double limits[3])
    {
        if (m_data.size() < 3)
            return false;

        if (m_tmpSize < m_data.size())
        {
            delete[] m_tmp;
            m_tmpSize = m_data.size() + 10;
            m_tmp = new double[m_tmpSize];
        }

        std::deque<Entry>::const_iterator it = m_data.begin();
        std::deque<Entry>::const_iterator end = m_data.end();
        double *p = &m_tmp[0];
        for (; it != end; ++it)
            *p++ = it->mass;

        size_t mid = m_data.size() / 2;
        std::nth_element(&m_tmp[0], &m_tmp[mid], &m_tmp[m_data.size()]);
        double med = m_tmp[mid];

        limits[0] = med * (1. - threshold);
        limits[1] = med;
        limits[2] = med * (1. + threshold);

        return mass < limits[0] || mass > limits[2];
    }

    void Reset(void)
    {
        m_data.clear();
    }
};

static const double DefaultMassChangeThreshold = 0.5;

enum {
    MIN_SEARCH_REGION = 5,
    DEFAULT_SEARCH_REGION = 15,
    MAX_SEARCH_REGION = 50,
};

BEGIN_EVENT_TABLE(GuiderOneStar, Guider)
    EVT_PAINT(GuiderOneStar::OnPaint)
    EVT_LEFT_DOWN(GuiderOneStar::OnLClick)
END_EVENT_TABLE()

// Define a constructor for the guide canvas
GuiderOneStar::GuiderOneStar(wxWindow *parent)
    : Guider(parent, XWinSize, YWinSize),
      m_massChecker(new MassChecker())
{
    SetState(STATE_UNINITIALIZED);
}

GuiderOneStar::~GuiderOneStar()
{
    delete m_massChecker;
}

void GuiderOneStar::LoadProfileSettings(void)
{
    Guider::LoadProfileSettings();

    double massChangeThreshold = pConfig->Profile.GetDouble("/guider/onestar/MassChangeThreshold",
            DefaultMassChangeThreshold);
    SetMassChangeThreshold(massChangeThreshold);

    bool massChangeThreshEnabled = pConfig->Profile.GetBoolean("/guider/onestar/MassChangeThresholdEnabled", massChangeThreshold != 1.0);
    SetMassChangeThresholdEnabled(massChangeThreshEnabled);

    int searchRegion = pConfig->Profile.GetInt("/guider/onestar/SearchRegion", DEFAULT_SEARCH_REGION);
    SetSearchRegion(searchRegion);
}

bool GuiderOneStar::GetMassChangeThresholdEnabled(void)
{
    return m_massChangeThresholdEnabled;
}

void GuiderOneStar::SetMassChangeThresholdEnabled(bool enable)
{
    m_massChangeThresholdEnabled = enable;
    pConfig->Profile.SetBoolean("/guider/onestar/MassChangeThresholdEnabled", enable);
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
        if (massChangeThreshold < 0.0)
        {
            throw ERROR_INFO("massChangeThreshold < 0");
        }

        m_massChangeThreshold = massChangeThreshold;
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);

        bError = true;
        m_massChangeThreshold = DefaultMassChangeThreshold;
    }

    pConfig->Profile.SetDouble("/guider/onestar/MassChangeThreshold", m_massChangeThreshold);

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
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        m_searchRegion = DEFAULT_SEARCH_REGION;
    }

    pConfig->Profile.SetInt("/guider/onestar/SearchRegion", m_searchRegion);

    return bError;
}

bool GuiderOneStar::SetCurrentPosition(usImage *pImage, const PHD_Point& position)
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

        Debug.AddLine(wxString::Format("SetCurrentPosition(%.2f,%.2f)", x, y ));

        if ((x <= 0) || (x >= pImage->Size.x))
        {
            throw ERROR_INFO("invalid x value");
        }

        if ((y <= 0) || (y >= pImage->Size.y))
        {
            throw ERROR_INFO("invalid y value");
        }

        m_massChecker->Reset();
        bError = !m_star.Find(pImage, m_searchRegion, x, y, pFrame->GetStarFindMode());
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

    return bError;
}

class AutoSelectFailFinder : public wxDirTraverser
{
public:
    wxString prefix;
    wxArrayString files;
    AutoSelectFailFinder(const wxString& prefix_) : prefix(prefix_) {  }
    wxDirTraverseResult OnFile(const wxString& filename)
    {
        wxFileName fn(filename);
        if (fn.GetFullName().StartsWith(prefix))
            files.Add(filename);
        return wxDIR_CONTINUE;
    }
    wxDirTraverseResult OnDir(const wxString& WXUNUSED(dirname))
    {
        return wxDIR_CONTINUE;
    }
};

static void RemoveOldAutoSelectFailFiles(const wxString& prefix, unsigned int keep_files)
{
    AutoSelectFailFinder finder(prefix);
    wxDir dir(Debug.GetLogDir());
    dir.Traverse(finder);

    finder.files.Sort();

    while (finder.files.size() >= keep_files)
    {
        wxRemoveFile(finder.files[0]);
        finder.files.RemoveAt(0);
    }
}

static void SaveAutoSelectFailedImg(usImage *pImage)
{
    static const wxString prefix = _T("PHD2_AutoSelectFail_");
    enum { KEEP_FILES = 10 };

    RemoveOldAutoSelectFailFiles(prefix, KEEP_FILES);

    wxString filename = prefix + wxDateTime::UNow().Format(_T("%Y-%m-%d_%H%M%S.fit"));

    Debug.AddLine("GuiderOneStar::AutoSelect failed. Saving image to " + filename);

    pImage->Save(wxFileName(Debug.GetLogDir(), filename).GetFullPath());
}

bool GuiderOneStar::AutoSelect(void)
{
    bool bError = false;

    usImage *pImage = CurrentImage();

    try
    {
        if (!pImage || !pImage->ImageData)
        {
            throw ERROR_INFO("No Current Image");
        }

        // If mount is not calibrated, we need to chose a star a bit farther
        // from the egde to allow for the motion of the star during
        // calibration
        //
        int edgeAllowance = 0;
        if (pMount && pMount->IsConnected() && !pMount->IsCalibrated())
            edgeAllowance = wxMax(edgeAllowance, pMount->CalibrationTotDistance());
        if (pSecondaryMount && pSecondaryMount->IsConnected() && !pSecondaryMount->IsCalibrated())
            edgeAllowance = wxMax(edgeAllowance, pSecondaryMount->CalibrationTotDistance());

        Star newStar;
        if (!newStar.AutoFind(*pImage, edgeAllowance, m_searchRegion))
        {
            throw ERROR_INFO("Unable to AutoFind");
        }

        m_massChecker->Reset();

        if (!m_star.Find(pImage, m_searchRegion, newStar.X, newStar.Y, Star::FIND_CENTROID))
        {
            throw ERROR_INFO("Unable to find");
        }

        if (SetLockPosition(m_star))
        {
            throw ERROR_INFO("Unable to set Lock Position");
        }

        if (GetState() == STATE_SELECTING)
        {
            // immediately advance the state machine now, rather than waiting for
            // the next exposure to complete. Socket server clients are going to
            // try to start guiding after selecting the star, but guiding will fail
            // to start if state is still STATE_SELECTING
            Debug.AddLine("AutoSelect: state = %d, call UpdateGuideState", GetState());
            UpdateGuideState(NULL, false);
        }

        UpdateImageDisplay();
        pFrame->pProfile->UpdateData(pImage, m_star.X, m_star.Y);
    }
    catch (wxString Msg)
    {
        if (pImage && pImage->ImageData)
        {
            SaveAutoSelectFailedImg(pImage);
        }

        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool GuiderOneStar::IsLocked(void)
{
    return m_star.WasFound();
}

const PHD_Point& GuiderOneStar::CurrentPosition(void)
{
    return m_star;
}

inline static wxRect SubframeRect(const PHD_Point& pos, int halfwidth)
{
    return wxRect(ROUND(pos.X - halfwidth),
                  ROUND(pos.Y - halfwidth),
                  2 * halfwidth + 1,
                  2 * halfwidth + 1);
}

wxRect GuiderOneStar::GetBoundingBox(void)
{
    enum { SUBFRAME_BOUNDARY_PX = 0 };

    GUIDER_STATE state = GetState();

    bool subframe;
    PHD_Point pos;

    switch (state) {
    case STATE_SELECTED:
    case STATE_CALIBRATING_PRIMARY:
    case STATE_CALIBRATING_SECONDARY:
        subframe = m_star.WasFound();
        pos = CurrentPosition();
        break;
    case STATE_GUIDING: {
        subframe = m_star.WasFound();  // true;
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
        box.Intersect(wxRect(0, 0, pCamera->FullSize.x, pCamera->FullSize.y));
        return box;
    }
    else
    {
        return wxRect(0, 0, 0, 0);
    }
}

int GuiderOneStar::GetMaxMovePixels(void)
{
    return m_searchRegion;
}

double GuiderOneStar::StarMass(void)
{
    return m_star.Mass;
}

double GuiderOneStar::SNR(void)
{
    return m_star.SNR;
}

int GuiderOneStar::StarError(void)
{
    return m_star.GetError();
}

void GuiderOneStar::InvalidateCurrentPosition(bool fullReset)
{
    m_star.Invalidate();

    if (fullReset)
    {
        m_star.X = m_star.Y = 0.0;
    }
}

static wxString StarStatusStr(const Star& star)
{
    if (!star.IsValid())
        return _("No star selected");

    switch (star.GetError())
    {
    case Star::STAR_LOWSNR:        return _("Star lost - low SNR");
    case Star::STAR_LOWMASS:       return _("Star lost - low mass");
    case Star::STAR_TOO_NEAR_EDGE: return _("Star too near edge");
    case Star::STAR_MASSCHANGE:    return _("Star lost - mass changed");
    default:                       return _("No star found");
    }
}

bool GuiderOneStar::UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo)
{
    if (!m_star.IsValid() && m_star.X == 0.0 && m_star.Y == 0.0)
    {
        Debug.AddLine("UpdateCurrentPosition: no star selected");
        errorInfo->starError = Star::STAR_ERROR;
        errorInfo->starMass = 0.0;
        errorInfo->starSNR = 0.0;
        errorInfo->status = _("No star selected");
        return true;
    }

    bool bError = false;

    try
    {
        Star newStar(m_star);

        if (!newStar.Find(pImage, m_searchRegion, pFrame->GetStarFindMode()))
        {
            errorInfo->starError = newStar.GetError();
            errorInfo->starMass = 0.0;
            errorInfo->starSNR = 0.0;
            errorInfo->status = StarStatusStr(newStar);
            m_star.SetError(newStar.GetError());
            throw ERROR_INFO("UpdateCurrentPosition():newStar not found");
        }

        // check to see if it seems like the star we just found was the
        // same as the original star.  We do this by comparing the
        // mass
        m_massChecker->SetExposure(pFrame->RequestedExposureDuration());
        double limits[3];
        if (m_massChangeThresholdEnabled &&
            m_massChecker->CheckMass(newStar.Mass, m_massChangeThreshold, limits))
        {
            m_star.SetError(Star::STAR_MASSCHANGE);
            errorInfo->starError = Star::STAR_MASSCHANGE;
            errorInfo->starMass = newStar.Mass;
            errorInfo->starSNR = newStar.SNR;
            errorInfo->status = StarStatusStr(m_star);
            pFrame->SetStatusText(wxString::Format(_("Mass: %.0f vs %.0f"), newStar.Mass, limits[1]), 1);
            Debug.Write(wxString::Format("UpdateGuideState(): star mass new=%.1f exp=%.1f thresh=%.0f%% range=(%.1f, %.1f)\n", newStar.Mass, limits[1], m_massChangeThreshold * 100, limits[0], limits[2]));
            m_massChecker->AppendData(newStar.Mass);
            throw THROW_INFO("massChangeThreshold error");
        }

        // update the star position, mass, etc.
        m_star = newStar;
        m_massChecker->AppendData(newStar.Mass);

        const PHD_Point& lockPos = LockPosition();
        if (lockPos.IsValid())
        {
            double distance = newStar.Distance(lockPos);
            UpdateCurrentDistance(distance);
        }

        pFrame->pProfile->UpdateData(pImage, m_star.X, m_star.Y);

        pFrame->AdjustAutoExposure(m_star.SNR);
        int exp;
        bool auto_exp;
        pFrame->GetExposureInfo(&exp, &auto_exp);
        if (auto_exp)
        {
            if (exp >= 1)
                errorInfo->status.Printf(_T("m=%.0f SNR=%.1f Exp=%0.1f s"), m_star.Mass, m_star.SNR, (double) exp / 1000.);
            else
                errorInfo->status.Printf(_T("m=%.0f SNR=%.1f Exp=%d ms"), m_star.Mass, m_star.SNR, exp);
        }
        else
            errorInfo->status.Printf(_T("m=%.0f SNR=%.1f"), m_star.Mass, m_star.SNR);
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        pFrame->ResetAutoExposure(); // use max exposure duration
    }

    return bError;
}

bool GuiderOneStar::IsValidLockPosition(const PHD_Point& pt)
{
    const usImage *pImage = CurrentImage();
    if (!pImage)
        return false;
    // this is a bit ugly as it is tightly coupled to Star::Find
    return pt.X >= 1 + m_searchRegion &&
        pt.X + 1 + m_searchRegion < pImage->Size.GetX() &&
        pt.Y >= 1 + m_searchRegion &&
        pt.Y + 1 + m_searchRegion < pImage->Size.GetY();
}

void GuiderOneStar::OnLClick(wxMouseEvent &mevent)
{
    try
    {
        if (mevent.GetModifiers() == wxMOD_CONTROL)
        {
            double const scaleFactor = ScaleFactor();
            wxRealPoint pt((double) mevent.m_x / scaleFactor,
                           (double) mevent.m_y / scaleFactor);
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
            InvalidateCurrentPosition(true);
        }
        else
        {
            if ((mevent.m_x <= m_searchRegion) || (mevent.m_x + m_searchRegion >= XWinSize) || (mevent.m_y <= m_searchRegion) || (mevent.m_y + m_searchRegion >= YWinSize))
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

            SetCurrentPosition(pImage, PHD_Point(StarX, StarY));

            if (!m_star.IsValid())
            {
                pFrame->SetStatusText(wxString::Format(_("No star found")));
            }
            else
            {
                SetLockPosition(m_star);
                pFrame->SetStatusText(wxString::Format(_("Selected star at (%.1f, %.1f)"), m_star.X, m_star.Y), 1);
                pFrame->SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"), m_star.Mass, m_star.SNR));
                EvtServer.NotifyStarSelected(CurrentPosition());
                SetState(STATE_SELECTED);
                pFrame->UpdateButtonsStatus();
                pFrame->pProfile->UpdateData(pImage, m_star.X, m_star.Y);
            }

            Refresh();
            Update();
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

inline static void DrawBox(wxClientDC& dc, const PHD_Point& star, int halfW, double scale)
{
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    double w = ROUND((halfW * 2 + 1) * scale);
    dc.DrawRectangle(int((star.X - halfW) * scale), int((star.Y - halfW) * scale), w, w);
}

// Define the repainting behaviour
void GuiderOneStar::OnPaint(wxPaintEvent& event)
{
    //wxAutoBufferedPaintDC dc(this);
    wxClientDC dc(this);
    wxMemoryDC memDC;

    try
    {
        if (PaintHelper(dc, memDC))
        {
            throw ERROR_INFO("PaintHelper failed");
        }
        // PaintHelper drew the image and any overlays
        // now decorate the image to show the selection

        // display bookmarks
        if (m_showBookmarks && m_bookmarks.size() > 0)
        {
            dc.SetPen(wxPen(wxColour(0,255,255),1,wxSOLID));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);

            for (std::vector<wxRealPoint>::const_iterator it = m_bookmarks.begin();
                 it != m_bookmarks.end(); ++it)
            {
                wxPoint p((int)(it->x * m_scaleFactor), (int)(it->y * m_scaleFactor));
                dc.DrawCircle(p, 3);
                dc.DrawCircle(p, 6);
                dc.DrawCircle(p, 12);
            }
        }

        GUIDER_STATE state = GetState();
        bool FoundStar = m_star.WasFound();

        if (state == STATE_SELECTED)
        {
            if (FoundStar)
                dc.SetPen(wxPen(wxColour(100,255,90), 1, wxSOLID));  // Draw the box around the star
            else
                dc.SetPen(wxPen(wxColour(230,130,30), 1, wxDOT));
            DrawBox(dc, m_star, m_searchRegion, m_scaleFactor);
        }
        else if (state == STATE_CALIBRATING_PRIMARY || state == STATE_CALIBRATING_SECONDARY)
        {
            // in the calibration process
            dc.SetPen(wxPen(wxColour(32,196,32), 1, wxSOLID));  // Draw the box around the star
            DrawBox(dc, m_star, m_searchRegion, m_scaleFactor);
        }
        else if (state == STATE_CALIBRATED || state == STATE_GUIDING)
        {
            // locked and guiding
            if (FoundStar)
                dc.SetPen(wxPen(wxColour(32,196,32), 1, wxSOLID));  // Draw the box around the star
            else
                dc.SetPen(wxPen(wxColour(230,130,30), 1, wxDOT));
            DrawBox(dc, m_star, m_searchRegion, m_scaleFactor);
        }

        // Image logging
        if (state >= STATE_SELECTED && pFrame->IsImageLoggingEnabled() && pFrame->m_frameCounter != pFrame->m_loggedImageFrame)
        {
            // only log each image frame once
            pFrame->m_loggedImageFrame = pFrame->m_frameCounter;

            if (pFrame->GetLoggedImageFormat() == LIF_RAW_FITS) // Save star image as a FITS
            {
                SaveStarFITS();
            }
            else  // Save star image as a JPEG
            {
                double LockX = LockPosition().X;
                double LockY = LockPosition().Y;

                wxBitmap SubBmp(60,60,-1);
                wxMemoryDC tmpMdc;
                tmpMdc.SelectObject(SubBmp);
                memDC.SetPen(wxPen(wxColor(0,255,0),1,wxDOT));
                memDC.DrawLine(0, LockY * m_scaleFactor, XWinSize, LockY * m_scaleFactor);
                memDC.DrawLine(LockX*m_scaleFactor, 0, LockX*m_scaleFactor, YWinSize);
    #ifdef __APPLEX__
                tmpMdc.Blit(0,0,60,60,&memDC,ROUND(m_star.X*m_scaleFactor)-30,Displayed_Image->GetHeight() - ROUND(m_star.Y*m_scaleFactor)-30,wxCOPY,false);
    #else
                tmpMdc.Blit(0,0,60,60,&memDC,ROUND(m_star.X*m_scaleFactor)-30,ROUND(m_star.Y*m_scaleFactor)-30,wxCOPY,false);
    #endif
                //          tmpMdc.Blit(0,0,200,200,&Cdc,0,0,wxCOPY);

                wxString fname = Debug.GetLogDir() + PATHSEPSTR + "PHD_GuideStar" + wxDateTime::Now().Format(_T("_%j_%H%M%S")) + ".jpg";
                wxImage subImg = SubBmp.ConvertToImage();
                // subImg.Rescale(120, 120);  zoom up (not now)
                if (pFrame->GetLoggedImageFormat() == LIF_HI_Q_JPEG)
                {
                    // set high(ish) JPEG quality
                    subImg.SetOption(wxIMAGE_OPTION_QUALITY, 100);
                }
                subImg.SaveFile(fname, wxBITMAP_TYPE_JPEG);
                tmpMdc.SelectObject(wxNullBitmap);
            }
        }
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

void GuiderOneStar::SaveStarFITS()
{
    double StarX = m_star.X;
    double StarY = m_star.Y;
    usImage *pImage = CurrentImage();
    usImage tmpimg;

    tmpimg.Init(60,60);
    int start_x = ROUND(StarX)-30;
    int start_y = ROUND(StarY)-30;
    if ((start_x + 60) > pImage->Size.GetWidth())
        start_x = pImage->Size.GetWidth() - 60;
    if ((start_y + 60) > pImage->Size.GetHeight())
        start_y = pImage->Size.GetHeight() - 60;
    int x,y, width;
    width = pImage->Size.GetWidth();
    unsigned short *usptr = tmpimg.ImageData;
    for (y=0; y<60; y++)
        for (x=0; x<60; x++, usptr++)
            *usptr = *(pImage->ImageData + (y+start_y)*width + (x+start_x));

    wxString fname = Debug.GetLogDir() + PATHSEPSTR + "PHD_GuideStar" + wxDateTime::Now().Format(_T("_%j_%H%M%S")) + ".fit";

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
    PHD_fits_create_file(&fptr, fname, false, &status);
    if (!status)
    {
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
        sprintf(keystring,"%s", (const char *) pImage->GetImgStartTime().c_str());
        if (!status) fits_write_key(fptr, TSTRING, keyname, keystring, keycomment, &status);

        sprintf(keyname,"EXPOSURE");
        sprintf(keycomment,"Exposure time [s]");
        float dur = (float) pImage->ImgExpDur / 1000.0;
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
    PHD_fits_close_file(fptr);
}

wxString GuiderOneStar::GetSettingsSummary()
{
    // return a loggable summary of guider configs
    wxString s = wxString::Format(_T("Search region = %d px, Star mass tolerance "), GetSearchRegion());

    if (GetMassChangeThresholdEnabled())
        s += wxString::Format(_T("= %.1f%%\n"), GetMassChangeThreshold() * 100.0);
    else
        s += _T("disabled\n");

    return s;
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
                                     wxSize(width+30, -1), wxSP_ARROW_KEYS, MIN_SEARCH_REGION, MAX_SEARCH_REGION, DEFAULT_SEARCH_REGION, _T("Search"));
    DoAdd(_("Search region (pixels)"), m_pSearchRegion,
          _("How many pixels (up/down/left/right) do we examine to find the star? Default = 15"));

    m_pEnableStarMassChangeThresh = new wxCheckBox(pParent, STAR_MASS_ENABLE, _("Star mass change detection"));
    DoAdd(m_pEnableStarMassChangeThresh, _("Check to enable star mass change detection. When enabled, "
        "PHD skips frames when the guide star mass changes by an amount greater than the Star mass tolerance setting."));

    pParent->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &GuiderOneStar::GuiderOneStarConfigDialogPane::OnStarMassEnableChecked, this, STAR_MASS_ENABLE);

    width = StringWidth(_T("100.0"));
    m_pMassChangeThreshold = new wxSpinCtrlDouble(pParent, wxID_ANY,_T("foo2"), wxPoint(-1,-1),
            wxSize(width+30, -1), wxSP_ARROW_KEYS, 0.1, 100.0, 0.0, 1.0,_T("MassChangeThreshold"));
    m_pMassChangeThreshold->SetDigits(1);
    DoAdd(_("Star mass tolerance"), m_pMassChangeThreshold,
          _("When star mass change detection is enabled, this is the tolerance for star mass changes between frames, in percent. "
          "Larger values are more tolerant (less sensitive) to star mass changes. Valid range is 10-100, default is 50. "
          "If star mass change detection is not enabled then this setting is ignored."));
}

GuiderOneStar::GuiderOneStarConfigDialogPane::~GuiderOneStarConfigDialogPane(void)
{
}

void GuiderOneStar::GuiderOneStarConfigDialogPane::LoadValues(void)
{
    GuiderConfigDialogPane::LoadValues();

    bool starMassEnabled = m_pGuiderOneStar->GetMassChangeThresholdEnabled();
    m_pEnableStarMassChangeThresh->SetValue(starMassEnabled);
    m_pMassChangeThreshold->Enable(starMassEnabled);
    m_pMassChangeThreshold->SetValue(100.0 * m_pGuiderOneStar->GetMassChangeThreshold());
    m_pSearchRegion->SetValue(m_pGuiderOneStar->GetSearchRegion());
}

void GuiderOneStar::GuiderOneStarConfigDialogPane::UnloadValues(void)
{
    m_pGuiderOneStar->SetMassChangeThresholdEnabled(m_pEnableStarMassChangeThresh->GetValue());
    m_pGuiderOneStar->SetMassChangeThreshold(m_pMassChangeThreshold->GetValue() / 100.0);
    m_pGuiderOneStar->SetSearchRegion(m_pSearchRegion->GetValue());

    GuiderConfigDialogPane::UnloadValues();
}

void GuiderOneStar::GuiderOneStarConfigDialogPane::OnStarMassEnableChecked(wxCommandEvent& event)
{
    m_pMassChangeThreshold->Enable(event.IsChecked());
}
