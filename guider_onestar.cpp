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

#if ((wxMAJOR_VERSION < 3) && (wxMINOR_VERSION < 9))
#define wxPENSTYLE_DOT wxDOT
#endif

class MassChecker
{
    enum { DefaultTimeWindowMs = 22500 };

    struct Entry
    {
        wxLongLong_t time;
        double mass;
    };

    std::deque<Entry> m_data;
    double m_highMass;   // high-water mark
    double m_lowMass;    // low-water mark
    unsigned long m_timeWindow;
    double *m_tmp;
    size_t m_tmpSize;
    int m_exposure;
    bool m_isAutoExposure;

public:

    MassChecker()
        : m_highMass(0.),
          m_lowMass(9e99),
          m_tmp(0),
          m_tmpSize(0),
          m_exposure(0),
          m_isAutoExposure(false)
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

    void SetExposure(int exposure, bool isAutoExp)
    {
        if (isAutoExp != m_isAutoExposure)
        {
            m_isAutoExposure = isAutoExp;
            m_exposure = exposure;
            Reset();
        }
        else if (exposure != m_exposure)
        {
            m_exposure = exposure;
            if (!m_isAutoExposure)
            {
                Reset();
            }
        }
    }

    double AdjustedMass(double mass) const
    {
        return m_isAutoExposure ? mass / (double) m_exposure : mass;
    }

    void AppendData(double mass)
    {
        wxLongLong_t now = ::wxGetUTCTimeMillis().GetValue();
        wxLongLong_t oldest = now - m_timeWindow;

        while (m_data.size() > 0 && m_data.front().time < oldest)
            m_data.pop_front();

        Entry entry;
        entry.time = now;
        entry.mass = AdjustedMass(mass);
        m_data.push_back(entry);
    }

    bool CheckMass(double mass, double threshold, double limits[4])
    {
        if (m_data.size() < 5)
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

        if (med > m_highMass)
            m_highMass = med;
        if (med < m_lowMass)
            m_lowMass = med;

        // let the low water mark drift to follow the median so that it moves back up after a
        // period of intermittent clouds has brought it down
        m_lowMass += .05 * (med - m_lowMass);

        limits[0] = m_lowMass * (1. - threshold);
        limits[1] = med;
        limits[2] = m_highMass * (1. + threshold);
        // when mass is depressed by sky conditions, we still want to trigger a rejection when
        // there is a large spike in mass, even if it is still below the high water mark-based
        // threhold
        limits[3] = med * (1. + 2.0 * threshold);

        double adjmass = AdjustedMass(mass);
        bool reject = adjmass < limits[0] || adjmass > limits[2] || adjmass > limits[3];

        if (reject && m_isAutoExposure)
        {
            // convert back to mass-like numbers for logging by caller
            for (int i = 0; i < 4; i++)
                limits[i] *= (double) m_exposure;
        }

        return reject;
    }

    void Reset(void)
    {
        m_data.clear();
        m_highMass = 0.;
        m_lowMass = 9e99;
    }
};

static const double DefaultMassChangeThreshold = 0.5;

enum {
    MIN_SEARCH_REGION = 7,
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
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);

        bError = true;
        m_massChangeThreshold = DefaultMassChangeThreshold;
    }

    pConfig->Profile.SetDouble("/guider/onestar/MassChangeThreshold", m_massChangeThreshold);

    return bError;
}

bool GuiderOneStar::SetSearchRegion(int searchRegion)
{
    bool bError = false;

    try
    {
        if (searchRegion < MIN_SEARCH_REGION)
        {
            m_searchRegion = MIN_SEARCH_REGION;
            throw ERROR_INFO("searchRegion < MIN_SEARCH_REGION");
        }
        else if (searchRegion > MAX_SEARCH_REGION)
        {
            m_searchRegion = MAX_SEARCH_REGION;
            throw ERROR_INFO("searchRegion > MAX_SEARCH_REGION");
        }
        m_searchRegion = searchRegion;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
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

        Debug.Write(wxString::Format("SetCurrentPosition(%.2f,%.2f)\n", x, y ));

        if ((x <= 0) || (x >= pImage->Size.x))
        {
            throw ERROR_INFO("invalid x value");
        }

        if ((y <= 0) || (y >= pImage->Size.y))
        {
            throw ERROR_INFO("invalid y value");
        }

        m_massChecker->Reset();
        bError = !m_star.Find(pImage, m_searchRegion, x, y, pFrame->GetStarFindMode(),
                              GetMinStarHFD(), pCamera->GetSaturationADU());
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
        return _("No star selected");

    switch (star.GetError())
    {
    case Star::STAR_LOWSNR:        return _("Star lost - low SNR");
    case Star::STAR_LOWMASS:       return _("Star lost - low mass");
    case Star::STAR_LOWHFD:        return _("Star lost - low HFD");
    case Star::STAR_TOO_NEAR_EDGE: return _("Star too near edge");
    case Star::STAR_MASSCHANGE:    return _("Star lost - mass changed");
    default:                       return _("No star found");
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

bool GuiderOneStar::AutoSelect(void)
{
    bool error = false;

    usImage *image = CurrentImage();

    try
    {
        if (!image || !image->ImageData)
        {
            throw ERROR_INFO("No Current Image");
        }

        if (pFrame->pGuider->IsCalibratingOrGuiding())
        {
            throw ERROR_INFO("cannot auto-select star while calibrating or guiding");
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
        if (!newStar.AutoFind(*image, edgeAllowance, m_searchRegion))
        {
            throw ERROR_INFO("Unable to AutoFind");
        }

        m_massChecker->Reset();

        if (!m_star.Find(image, m_searchRegion, newStar.X, newStar.Y, Star::FIND_CENTROID, GetMinStarHFD(),
                         pCamera->GetSaturationADU()))
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
            Debug.Write(wxString::Format("AutoSelect: state = %d, call UpdateGuideState\n", GetState()));
            UpdateGuideState(NULL, false);
        }

        UpdateImageDisplay();
        pFrame->StatusMsg(wxString::Format(_("Auto-selected star at (%.1f, %.1f)"), m_star.X, m_star.Y));
        pFrame->UpdateStarInfo(m_star.SNR, m_star.GetError() == Star::STAR_SATURATED);
        pFrame->pProfile->UpdateData(image, m_star.X, m_star.Y);
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        error = true;
    }

    if (image && image->ImageData)
    {
        if (error)
            Debug.Write("GuiderOneStar::AutoSelect failed.\n");

        ImageLogger::LogAutoSelectImage(image, !error);
    }

    return error;
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
    return wxRect(ROUND(pos.X) - halfwidth,
                  ROUND(pos.Y) - halfwidth,
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
        box.Intersect(wxRect(pCamera->FullSize));
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

unsigned int GuiderOneStar::StarPeakADU(void)
{
    return m_star.IsValid() ? m_star.PeakVal : 0;
}

double GuiderOneStar::SNR(void)
{
    return m_star.SNR;
}

double GuiderOneStar::HFD(void)
{
    return m_star.HFD;
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

struct DistanceChecker
{
    bool enabled;
    wxLongLong_t expires;

    enum { ENABLED_INTERVAL_MS = 6 * 1000 };

    void Activate()
    {
        if (!enabled)
            Debug.Write("DistanceChecker: activated\n");

        enabled = true;
        expires = ::wxGetUTCTimeMillis().GetValue() + ENABLED_INTERVAL_MS;
    }

    bool CheckDistance(double distance)
    {
        if (!enabled)
            return true;

        wxLongLong_t now = ::wxGetUTCTimeMillis().GetValue();
        if (now < expires)
        {
            // Star was recently rejected. Check the guide error and reject the frame if the guide
            // error is large. This helps to handle the case of an object like a satellite
            // traversing the searach region and causing one or more bad frames, but with only the
            // first frame triggering the star rejection.

            double distanceThreshold = 2.0 * pFrame->pGuider->CurrentError();
            if (distance > distanceThreshold)
            {
                Debug.Write(wxString::Format("DistanceChecker: reject for large offset (%.2f > %.2f)\n", distance, distanceThreshold));
                return false;
            }
        }

        // "small" offset, safe to assume recovery complete
        Debug.Write("DistanceChecker: deactivated\n");
        enabled = false;

        return true;
    }
};

static DistanceChecker s_distanceChecker;


bool GuiderOneStar::UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo)
{
    if (!m_star.IsValid() && m_star.X == 0.0 && m_star.Y == 0.0)
    {
        Debug.Write("UpdateCurrentPosition: no star selected\n");
        errorInfo->starError = Star::STAR_ERROR;
        errorInfo->starMass = 0.0;
        errorInfo->starSNR = 0.0;
        errorInfo->status = _("No star selected");
        ImageLogger::LogImageStarDeselected(pImage);
        return true;
    }

    bool bError = false;

    try
    {
        Star newStar(m_star);

        if (!newStar.Find(pImage, m_searchRegion, pFrame->GetStarFindMode(), GetMinStarHFD(),
                          pCamera->GetSaturationADU()))
        {
            errorInfo->starError = newStar.GetError();
            errorInfo->starMass = 0.0;
            errorInfo->starSNR = 0.0;
            errorInfo->status = StarStatusStr(newStar);
            m_star.SetError(newStar.GetError());

            s_distanceChecker.Activate();
            ImageLogger::LogImage(pImage, *errorInfo);

            throw ERROR_INFO("UpdateCurrentPosition():newStar not found");
        }

        // check to see if it seems like the star we just found was the
        // same as the original star by comparing the mass
        if (m_massChangeThresholdEnabled)
        {
            int exposure;
            bool isAutoExp;
            pFrame->GetExposureInfo(&exposure, &isAutoExp);
            m_massChecker->SetExposure(exposure, isAutoExp);
            double limits[4];
            if (m_massChecker->CheckMass(newStar.Mass, m_massChangeThreshold, limits))
            {
                m_star.SetError(Star::STAR_MASSCHANGE);
                errorInfo->starError = Star::STAR_MASSCHANGE;
                errorInfo->starMass = newStar.Mass;
                errorInfo->starSNR = newStar.SNR;
                errorInfo->status = StarStatusStr(m_star);
                pFrame->StatusMsg(wxString::Format(_("Mass: %.f vs %.f"), newStar.Mass, limits[1]));
                Debug.Write(wxString::Format("UpdateCurrentPosition: star mass new=%.1f exp=%.1f thresh=%.0f%% limits=(%.1f, %.1f, %.1f)\n", newStar.Mass, limits[1], m_massChangeThreshold * 100., limits[0], limits[2], limits[3]));
                m_massChecker->AppendData(newStar.Mass);

                s_distanceChecker.Activate();
                ImageLogger::LogImage(pImage, *errorInfo);

                throw THROW_INFO("massChangeThreshold error");
            }
        }

        const PHD_Point& lockPos = LockPosition();
        double distance = lockPos.IsValid() ? newStar.Distance(lockPos) : -1.;

        if (!s_distanceChecker.CheckDistance(distance))
        {
            m_star.SetError(Star::STAR_ERROR);
            errorInfo->starError = Star::STAR_ERROR;
            errorInfo->starMass = newStar.Mass;
            errorInfo->starSNR = newStar.SNR;
            errorInfo->status = StarStatusStr(m_star);
            pFrame->StatusMsg(_("Recovering"));

            ImageLogger::LogImage(pImage, *errorInfo);

            throw THROW_INFO("CheckDistance error");
        }

        ImageLogger::LogImage(pImage, distance);

        // update the star position, mass, etc.
        m_star = newStar;
        m_massChecker->AppendData(newStar.Mass);

        if (lockPos.IsValid())
        {
            UpdateCurrentDistance(distance);
        }

        pFrame->pProfile->UpdateData(pImage, m_star.X, m_star.Y);

        pFrame->AdjustAutoExposure(m_star.SNR);
        pFrame->UpdateStarInfo(m_star.SNR, m_star.GetError() == Star::STAR_SATURATED);
        errorInfo->status = StarStatus(m_star);
    }
    catch (const wxString& Msg)
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
        //single shot move -- C.Johnson 3/2/2016
	if (mevent.GetModifiers() == wxMOD_ALTGR)
        	{
		//if no star selected or mount not connected, do nothing
		if((GetState() != STATE_SELECTED) || (!pMount->IsConnected()))
			return;

		//calculate the move distances
		double const scaleFactor = ScaleFactor();
		double slewX = m_star.X - ((double) mevent.m_x / scaleFactor);
            	double slewY = m_star.Y - ((double) mevent.m_y / scaleFactor);
		
		//make the move
		int slewres;	
		pFrame->StatusMsg(wxString::Format(_("Move Star %.1f %.1f"), slewX, slewY));	
		slewres = pMount->Move(PHD_Point(slewX,slewY), MOVETYPE_DIRECT);
		
		return;
		}//end single shot move

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
                pFrame->StatusMsg(wxString::Format(_("No star found")));
            }
            else
            {
                SetLockPosition(m_star);
                pFrame->StatusMsg(wxString::Format(_("Selected star at (%.1f, %.1f)"), m_star.X, m_star.Y));
                pFrame->UpdateStarInfo(m_star.SNR, m_star.GetError() == Star::STAR_SATURATED);
                EvtServer.NotifyStarSelected(CurrentPosition());
                SetState(STATE_SELECTED);
                pFrame->UpdateButtonsStatus();
                pFrame->pProfile->UpdateData(pImage, m_star.X, m_star.Y);
            }

            Refresh();
            Update();
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }
}

inline static void DrawBox(wxDC& dc, const PHD_Point& star, int halfW, double scale)
{
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    double w = ROUND((halfW * 2 + 1) * scale);
    dc.DrawRectangle(int((star.X - halfW) * scale), int((star.Y - halfW) * scale), w, w);
}

// Define the repainting behaviour
void GuiderOneStar::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
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
    }
    catch (const wxString& Msg)
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
    wxString imgLogDirectory;

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
    for (y = 0; y < 60; y++)
    {
        for (x = 0; x < 60; x++, usptr++)
            *usptr = *(pImage->ImageData + (y + start_y)*width + (x + start_x));
    }

    imgLogDirectory = Debug.GetLogDir() + PATHSEPSTR + "PHD2_Stars";
    if (!wxDirExists(imgLogDirectory))
        wxFileName::Mkdir(imgLogDirectory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxString fname = imgLogDirectory + PATHSEPSTR + "PHD_GuideStar" + wxDateTime::Now().Format(_T("_%j_%H%M%S")) + ".fit";

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

        time_t now = wxDateTime::GetTimeNow();
        struct tm *timestruct = gmtime(&now);
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

Guider::GuiderConfigDialogPane *GuiderOneStar::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuiderOneStarConfigDialogPane(pParent, this);
}

GuiderOneStar::GuiderOneStarConfigDialogPane::GuiderOneStarConfigDialogPane(wxWindow *pParent, GuiderOneStar *pGuider)
    : GuiderConfigDialogPane(pParent, pGuider)
{

}

void GuiderOneStar::GuiderOneStarConfigDialogPane::LayoutControls(Guider *pGuider, BrainCtrlIdMap& CtrlMap)
{
    GuiderConfigDialogPane::LayoutControls(pGuider, CtrlMap);
}

GuiderConfigDialogCtrlSet* GuiderOneStar::GetConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap)
{
    return new GuiderOneStarConfigDialogCtrlSet(pParent, pGuider, pAdvancedDialog, CtrlMap);
}

GuiderOneStarConfigDialogCtrlSet::GuiderOneStarConfigDialogCtrlSet(wxWindow *pParent, Guider *pGuider, AdvancedDialog *pAdvancedDialog, BrainCtrlIdMap& CtrlMap)
    : GuiderConfigDialogCtrlSet(pParent, pGuider, pAdvancedDialog, CtrlMap)
{
    assert(pGuider);

    m_pGuiderOneStar = (GuiderOneStar *)pGuider;
    int width;
    wxWindow* parent;

    width = StringWidth(_T("0000"));
    m_pSearchRegion = pFrame->MakeSpinCtrl(GetParentWindow(AD_szStarTracking), wxID_ANY, _T(" "), wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, MIN_SEARCH_REGION, MAX_SEARCH_REGION, DEFAULT_SEARCH_REGION, _T("Search"));
    wxSizer *pSearchRegion = MakeLabeledControl(AD_szStarTracking, _("Search region (pixels)"), m_pSearchRegion,
        _("How many pixels (up/down/left/right) do we examine to find the star? Default = 15"));

    wxStaticBoxSizer *pStarMass = new wxStaticBoxSizer(wxHORIZONTAL, GetParentWindow(AD_szStarTracking), _("Star Mass Detection"));
    m_pEnableStarMassChangeThresh = new wxCheckBox(GetParentWindow(AD_szStarTracking), STAR_MASS_ENABLE, _("Enable"));
    m_pEnableStarMassChangeThresh->SetToolTip(_("Check to enable star mass change detection. When enabled, "
        "PHD skips frames when the guide star mass changes by an amount greater than the setting for 'tolerance'."));

    GetParentWindow(AD_szStarTracking)->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &GuiderOneStarConfigDialogCtrlSet::OnStarMassEnableChecked, this, STAR_MASS_ENABLE);

    width = StringWidth(_T("100.0"));
    m_pMassChangeThreshold = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.1, 100.0, 0.0, 1.0, _T("MassChangeThreshold"));
    m_pMassChangeThreshold->SetDigits(1);
    wxSizer *pTolerance = MakeLabeledControl(AD_szStarTracking, _("Tolerance"), m_pMassChangeThreshold,
        _("When star mass change detection is enabled, this is the tolerance for star mass changes between frames, in percent. "
        "Larger values are more tolerant (less sensitive) to star mass changes. Valid range is 10-100, default is 50. "
        "If star mass change detection is not enabled then this setting is ignored."));
    pStarMass->Add(m_pEnableStarMassChangeThresh, wxSizerFlags(0).Border(wxTOP, 3));
    pStarMass->Add(pTolerance, wxSizerFlags(0).Border(wxLEFT, 40));

    parent = GetParentWindow(AD_szStarTracking);
    width = StringWidth(_("65535"));


    m_MinHFD = pFrame->MakeSpinCtrlDouble(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(width, -1), wxSP_ARROW_KEYS, 0.0, 10.0, 2.0, 0.5);
    m_MinHFD->SetDigits(1);
    wxSizer *pHFD = MakeLabeledControl(AD_szStarTracking, _("Minimum star HFD (pixels)"), m_MinHFD,
        _("The minimum star HFD (size) that will be used for identifying a guide star. "
          "This setting can be used to prevent PHD2 from guiding on a hot pixel. "
          "Use the Star Profile Tool to measure the HFD of a hot pixel and set the min HFD threshold "
          "a bit higher. When the HFD falls below this level, the hot pixel will be ignored."));
    wxFlexGridSizer *pTrackingParams = new wxFlexGridSizer(3, 2, 8, 15);
    pTrackingParams->Add(pSearchRegion, wxSizerFlags(0).Border(wxTOP, 12));
    pTrackingParams->Add(pStarMass,wxSizerFlags(0).Border(wxLEFT, 75));
    pTrackingParams->Add(pHFD, wxSizerFlags().Border(wxTOP, 3));

    AddGroup(CtrlMap, AD_szStarTracking, pTrackingParams);
}

GuiderOneStarConfigDialogCtrlSet::~GuiderOneStarConfigDialogCtrlSet()
{

}

void GuiderOneStarConfigDialogCtrlSet::LoadValues()
{
    bool starMassEnabled = m_pGuiderOneStar->GetMassChangeThresholdEnabled();
    m_pEnableStarMassChangeThresh->SetValue(starMassEnabled);
    m_pMassChangeThreshold->Enable(starMassEnabled);
    m_pMassChangeThreshold->SetValue(100.0 * m_pGuiderOneStar->GetMassChangeThreshold());
    m_pSearchRegion->SetValue(m_pGuiderOneStar->GetSearchRegion());
    m_MinHFD->SetValue(m_pGuiderOneStar->GetMinStarHFD());

    GuiderConfigDialogCtrlSet::LoadValues();
}

void GuiderOneStarConfigDialogCtrlSet::UnloadValues()
{
    m_pGuiderOneStar->SetMassChangeThresholdEnabled(m_pEnableStarMassChangeThresh->GetValue());
    m_pGuiderOneStar->SetMassChangeThreshold(m_pMassChangeThreshold->GetValue() / 100.0);
    m_pGuiderOneStar->SetSearchRegion(m_pSearchRegion->GetValue());
    m_pGuiderOneStar->SetMinStarHFD(m_MinHFD->GetValue());
    GuiderConfigDialogCtrlSet::UnloadValues();
}

void GuiderOneStarConfigDialogCtrlSet::OnStarMassEnableChecked(wxCommandEvent& event)
{
    m_pMassChangeThreshold->Enable(event.IsChecked());
}
