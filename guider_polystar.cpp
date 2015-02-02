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

// KOR - 25-Jan-15 - TODO: I'm adding these here temporarily.  Eventually, colors should standardized and
//   configurable and these will go away in favor of some kind of global color array with an enum to
//   identify what color is used for what.  But, for know, I need to get this working, checked in, and
//   available for others to test.
#define COLOR_CYAN				(wxColour(0x00, 0xFF, 0xFF))
#define COLOR_RED				(wxColour(0xFF, 0x00, 0x00))
#define COLOR_LIGHTGREEN		(wxColour(0x90, 0xEE, 0x90))
#define COLOR_ORANGE			(wxColour(0xFF, 0xA5, 0x00))
#define COLOR_GREEN				(wxColour(0x00, 0x80, 0x00))




// KOR_OUT #define SCALE_UP_SMALL  // Currently problematic as the box for the star is drawn in the wrong spot.

#if ((wxMAJOR_VERSION < 3) && (wxMINOR_VERSION < 9))
#define wxPENSTYLE_DOT wxDOT
#endif

#ifdef KOR_CONV_ABSTRACT_CLASS

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
#endif

BEGIN_EVENT_TABLE(GuiderPolyStar, Guider)
	EVT_PAINT(GuiderPolyStar::OnPaint)
	EVT_LEFT_DOWN(GuiderPolyStar::OnLClick)
END_EVENT_TABLE()


const double SNR_RATIO = 0.1;

// Define a constructor for the guide canvas
//GuiderPolyStar::GuiderPolyStar(wxWindow *parent) : Guider(parent, XWinSize, YWinSize), m_massChecker(new MassChecker())
GuiderPolyStar::GuiderPolyStar(wxWindow *parent) : GuiderOneStar(parent)
{
    SetState(STATE_UNINITIALIZED);

	m_rotation = false;					// TODO: move this to the control panel "Do you expect significant field rotation during the exposure?"
}

GuiderPolyStar::~GuiderPolyStar()
{
#ifdef KOR_CONV_ABSTRACT_CLASS

	delete m_massChecker;
#endif
}


//******************************************************************************
void GuiderPolyStar::LoadProfileSettings(void)
{
    GuiderOneStar::LoadProfileSettings();

	SetFailOneStar(pConfig->Profile.GetBoolean("/guider/polystar/FailOneStar", true));
	SetMaxStars(pConfig->Profile.GetInt("/guider/polystar/MaxStars", DEF_STARS));
	SetAutoSNR(pConfig->Profile.GetBoolean("/guider/polystar/AutoSNR", true));
	SetMinSNR(pConfig->Profile.GetDouble("/guider/polystar/MinSNR", DEF_SNR));
	SetMaxSNR(pConfig->Profile.GetDouble("/guider/polystar/MaxSNR", MAX_SNR));
	SetMinMass(pConfig->Profile.GetDouble("/guider/polystar/MinMass", DEF_MASS));
	SetBGSigma(pConfig->Profile.GetInt("/guider/polystar/BGSigma", DEF_BGS));

	Debug.AddLine("+++GuiderPolyStar::LoadProfileSettings() - FailOneStar:%d  MaxStars:%d  AutoSNR:%d  MinSNR:%5.1f  MaxSNR:%5.1f  MinMass:%6.1f  BGSigma:%d", m_failOneStar, m_maxStars, m_autoSNR, m_minSNR, m_maxSNR, m_minMass, m_BGSigma);

	return;
}

//******************************************************************************
void GuiderPolyStar::SetFailOneStar(bool failOneStar)
{
	m_failOneStar = failOneStar; 
	pConfig->Profile.SetBoolean("/guider/polystar/FailOneStar", failOneStar);
	return;
}

//******************************************************************************
void GuiderPolyStar::SetMaxStars(int maxStars)			
{ 
	m_maxStars = maxStars; 
	pConfig->Profile.SetInt("/guider/polystar/MaxStars", maxStars);
	return;
}

//******************************************************************************
void GuiderPolyStar::SetAutoSNR(bool autoSNR)
{
	m_autoSNR = autoSNR;
	pConfig->Profile.SetBoolean("/guider/polystar/AutoSNR", autoSNR);
	return;
}

//******************************************************************************
void GuiderPolyStar::SetMinSNR(double minSNR)
{
	m_minSNR = minSNR; 
	pConfig->Profile.SetDouble("/guider/polystar/MinSNR", minSNR);
	return;
}

//******************************************************************************
void GuiderPolyStar::SetMaxSNR(double maxSNR)
{ 
	m_maxSNR = maxSNR; 
	pConfig->Profile.SetDouble("/guider/polystar/MaxSNR", maxSNR);
	return;
}

//******************************************************************************
void GuiderPolyStar::SetMinMass(double minMass)
{ 
	m_minMass = minMass; 
	pConfig->Profile.SetDouble("/guider/polystar/MinMass", minMass);
	return;
}

//******************************************************************************
void GuiderPolyStar::SetBGSigma(int BGSigma)
{ 
	m_BGSigma = BGSigma; 
	pConfig->Profile.SetInt("/guider/polystar/BGSigma", BGSigma);
	return;
}

#ifdef KOR_CONV_ABSTRACT_CLASS
//******************************************************************************
bool GuiderPolyStar::SetLockPosition(const PolyStar& poly_star)
{
	if (!poly_star.IsValid())
	{
		throw ERROR_INFO("PolyStar is not valid");
	}

	PHD_Point lock_pos(poly_star.getCentroid());
	Debug.AddLine("GuiderPolyStar::SetLockPosition - using PolyStar centroid at (%.2f, %.2f)", lock_pos.X, lock_pos.Y);
	return Guider::SetLockPosition(lock_pos);
}
#endif

//******************************************************************************
bool GuiderPolyStar::IsLocked(void)
{
	// TODO: Add test for lastFindResult on each star.  See Star::wasFound()
	if (m_polyStar.IsValid())
		return true;
	return false;
}

//******************************************************************************
bool GuiderPolyStar::SetCurrentPosition(usImage *pImage, const PHD_Point& position)
{
   
	if (m_guideOneStar)
		GuiderOneStar::SetCurrentPosition(pImage, position);
	
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

		m_virtualGuideStar.SetXY(position.X, position.Y);
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

    Debug.AddLine("GuiderPolyStar::AutoSelect failed. Saving image to " + filename);

    pImage->Save(wxFileName(Debug.GetLogDir(), filename).GetFullPath());
}







//******************************************************************************
bool GuiderPolyStar::AutoSelect(void)
{
    bool bError = false;

    usImage *pImage = CurrentImage();
	Star one_star;

	double min_SNR = m_minSNR;
	double max_SNR = m_maxSNR;

	Debug.AddLine("+++ auto select initial SNR limits - min:%5.1f  max:%5.1f", min_SNR, max_SNR);

	m_guideOneStar = false;
    try
    {
        if (!pImage || !pImage->ImageData)
        {
            throw ERROR_INFO("No Current Image");
        }

		if (m_autoSNR)
		{
			// KOR TODO: Move this code to Star::AutoFind() and then remove it from both
			//   here and from GuiderOneStar::AutoSelect()
			int edgeAllowance = 0;
			if (pMount && pMount->IsConnected() && !pMount->IsCalibrated())
				edgeAllowance = wxMax(edgeAllowance, pMount->CalibrationTotDistance());
			if (pSecondaryMount && pSecondaryMount->IsConnected() && !pSecondaryMount->IsCalibrated())
				edgeAllowance = wxMax(edgeAllowance, pSecondaryMount->CalibrationTotDistance());


			if (!one_star.AutoFind(*pImage, edgeAllowance, GetSearchRegion()))
				throw ERROR_INFO("Cannot find initial star for AutoSNR");

			one_star.Find(pImage, GetSearchRegion(), Star::FIND_CENTROID);

			Debug.AddLine("+++ AutoSNR - star SNR:%5.1f", one_star.SNR);

			// Set SNR limits based on the star that was auto selected by original algorithm
			min_SNR = one_star.SNR - one_star.SNR * SNR_RATIO;
			max_SNR = one_star.SNR + one_star.SNR * SNR_RATIO;
			Debug.AddLine("+++ AutoSNR - min:%5.1f  max:%5.1f", min_SNR, max_SNR);
		}
		
		m_starList.AutoFind(*pImage, GetSearchRegion(), m_scaleFactor, min_SNR, max_SNR, m_minMass, m_BGSigma);
		m_polyStar = PolyStar(m_starList.GetAcceptedStars(), m_maxStars);

		if (m_polyStar.len() <= 1 && m_failOneStar)
		{
			m_guideOneStar = true;
			return GuiderOneStar::AutoSelect();
		}

		if (!m_polyStar.IsValid())
			throw ERROR_INFO("Unable to find");
		
		if (SetLockPosition(m_polyStar.getCentroid()))
            throw ERROR_INFO("Unable to set Lock Position");

        if (GetState() == STATE_SELECTING)
        {
            // immediately advance the state machine now, rather than waiting for
            // the next exposure to complete. Socket server clients are going to
            // try to start guiding after selecting the star, but guiding will fail
            // to start if state is still STATE_SELECTING
			// KOR - 16-Nov-14 - this might be a problem -- we are going to need to 
			//    wait until we know the real position of the stars in the polygon
			//    before we allow the imager to start. 
            Debug.AddLine("guiderPolyStar::AutoSelect() - state = %d, call UpdateGuideState", GetState());
            UpdateGuideState(NULL, false);
        }

        UpdateImageDisplay();
		pFrame->pProfile->UpdateData(pImage, m_polyStar.GetStar(0).X, m_polyStar.GetStar(0).Y);

#ifdef BRET_AO_DEBUG
        if (pMount && !pMount->IsCalibrated())
        {
            //pMount->SetCalibration(-2.61, -1.04, 0.30, 0.26);
            //pMount->SetCalibration(-2.61, -1.04, 1.0, 1.0);
            //pMount->SetCalibration(0.0, M_PI/2, 0.005, 0.005);
            pMount->SetCalibration(M_PI/4 + 0.0, M_PI/4 + M_PI/2, 1.0, 1.0);
        }
#endif

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







//******************************************************************************
const PHD_Point& GuiderPolyStar::CurrentPosition(void)
{
	if (m_guideOneStar)
		return GuiderOneStar::CurrentPosition();
	
    return m_virtualGuideStar;
}

//******************************************************************************
bool GuiderPolyStar::SetLockPosition(const PHD_Point& lockPos)
{
	// KOR TODO: not sure this is right.  I need to track down all calls to SetLockPosition and make
	//   sure that the PHD_Point is the correct one.
	if (m_guideOneStar)
		return GuiderOneStar::SetLockPosition(lockPos);

	if (!lockPos.IsValid())
	{
		throw ERROR_INFO("Lock Position is not valid");
	}

	Debug.AddLine("GuiderPolyStar::SetLockPosition - using PolyStar centroid at (%.2f, %.2f)", lockPos.X, lockPos.Y);

	// Don't know if we are going to keep this or not - added to do the logging to test whether or not the alrithm really works.
	m_guideLockPosition = lockPos;

	return Guider::SetLockPosition(lockPos);
}

//******************************************************************************
bool GuiderPolyStar::UpdateCurrentPosition(usImage *pImage, FrameDroppedInfo *errorInfo)
{
	if (m_guideOneStar)
		return GuiderOneStar::UpdateCurrentPosition(pImage, errorInfo);

	Debug.AddLine("GuiderPolyStar::UpdateCurrentPosition - entered");

	// We should update the StarList position after an autofind, but before we actually
	//   star guiding or calibrating.  Add code to enforce this.
	switch (GetState())
	{
	case STATE_UNINITIALIZED:
	case STATE_SELECTING:
	case STATE_SELECTED:
		m_starList.UpdateCurrentPosition(*pImage, GetSearchRegion());
		break;
	}

	if (!m_polyStar.IsValid())
	{
		Debug.AddLine("GuiderPolyStar::UpdateCurrentPosition() - no PolyStar selected");
//		errorInfo->starError = Star::STAR_ERROR;		// TODO: PolyStar::ERROR
		errorInfo->starMass = 0.0;
		errorInfo->starSNR = 0.0;
		errorInfo->status = _("No star selected");
		return true;
	}

    bool bError = false;

    try
    {
		if (!m_polyStar.Find(pImage, GetSearchRegion(), pFrame->GetStarFindMode()))
		{
			//			errorInfo->starError = m_polyStar.GetError();		//TODO: Add functionality to PolyStar
			//			errorInfo->starMass = 0.0;
			//			errorInfo->starSNR = 0.0;
			//			errorInfo->status = StarStatusStr(m_polyStar);
			throw ERROR_INFO("GUiderPolyStar::UpdateCurrentPosition() - Cannot find all stars in polyStar not found");
		}
		m_virtualGuideStar.SetXY(m_polyStar.getCentroid().X, m_polyStar.getCentroid().Y);
		
#ifdef KOR_OUT

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
            Debug.Write(wxString::Format("KOR - UpdateGuideState(): star mass new=%.1f exp=%.1f thresh=%.0f%% range=(%.1f, %.1f)\n", newStar.Mass, limits[1], m_massChangeThreshold * 100, limits[0], limits[2]));
            m_massChecker->AppendData(newStar.Mass);
            throw THROW_INFO("massChangeThreshold error");
        }


        // update the star position, mass, etc.
        m_star = newStar;
        m_massChecker->AppendData(newStar.Mass);

#endif
		const PHD_Point& lockPos = LockPosition();
		Debug.AddLine("PolyStar::UpdateCurrentPosition() - lock - valid:%d  pos:(%g, %g)", lockPos.IsValid(), lockPos.X, lockPos.Y);
        if (lockPos.IsValid())
        {
            double distance = m_polyStar.getCentroid().Distance(lockPos);
            UpdateCurrentDistance(distance);
        }

		pFrame->pProfile->UpdateData(pImage, m_polyStar.GetStar(0).X, m_polyStar.GetStar(0).Y);

// KOR_OUT        pFrame->AdjustAutoExposure(m_star.SNR);

		errorInfo->status.Printf(_T("Avg Mass=%.0f SNR=%.1f"), m_polyStar.getMass(), m_polyStar.getSNR());

    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
        pFrame->ResetAutoExposure(); // use max exposure duration
    }

    return bError;
}


#ifdef KOR_CONV_ABSTRACT_CLASS
bool GuiderPolyStar::IsValidLockPosition(const PHD_Point& pt)
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
#endif

//******************************************************************************
void GuiderPolyStar::InvalidateCurrentPosition(bool fullReset)
{
	m_polyStar.RemoveStars();
	m_starList.clearStarLists();
	return;
}

//******************************************************************************
void GuiderPolyStar::OnLClick(wxMouseEvent &mevent)
{
	Debug.AddLine("+++ GuiderPolyStar::OnLClick() - entered");
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
			goto exit;
        }

        if (GetState() > STATE_SELECTED)
        {
            mevent.Skip();
            throw THROW_INFO("Skipping event because state > STATE_SELECTED");
        }

		if (mevent.GetModifiers() == wxMOD_SHIFT)
		{
			if (m_guideOneStar)
				GuiderOneStar::InvalidateCurrentPosition(true);
			else
				InvalidateCurrentPosition(true);

			goto exit;
		}

		if ((mevent.m_x <= GetSearchRegion()) || (mevent.m_x + GetSearchRegion() >= XWinSize) || (mevent.m_y <= GetSearchRegion()) || (mevent.m_y + GetSearchRegion() >= YWinSize))
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

		Star new_star;
		new_star.Find(pImage, GetSearchRegion(), StarX, StarY, Star::FIND_CENTROID); 
		new_star.Find(pImage, GetSearchRegion(), Star::FIND_CENTROID);

		if (new_star.SNR < 0.1)
		{
			pFrame->SetStatusText(wxString::Format(_("No star found")));
			goto exit;
		}

		// Clicked an existing star - remove it
		if (m_polyStar.RemoveStar(new_star, GetSearchRegion()) == true)
		{
			Debug.AddLine("+++ GuiderPolyStar::OnLClick() - removed star at (%7.2f, %7.2f) - SNR:%5.1f - total stars:%d", new_star.X, new_star.Y, new_star.SNR, m_polyStar.len());
			switch (m_polyStar.len())
			{
			case 0:
				GuiderOneStar::InvalidateCurrentPosition(true);
				SetState(STATE_SELECTING);
				pFrame->UpdateButtonsStatus();
				goto exit;

			case 1:
				m_guideOneStar = true;
				SetCurrentPosition(pImage, m_polyStar.GetStar(0));
				SetLockPosition(m_polyStar.GetStar(0));
				break;

			default:
				m_polyStar.makePolygon();
				m_polyStar.makeCentroid();

				SetCurrentPosition(pImage, m_polyStar.getCentroid());
				SetLockPosition(m_polyStar.getCentroid());
				break;
			}


			EvtServer.NotifyStarSelected(CurrentPosition());
			pFrame->pProfile->UpdateData(pImage, new_star.X, new_star.Y);
			goto exit;
		}

		// Clicked a new start - add it
		m_polyStar.AddStar(new_star);
		pFrame->SetStatusText(wxString::Format(_("Selected star at (%.1f, %.1f)"), new_star.X, new_star.Y), 1);
		pFrame->SetStatusText(wxString::Format(_T("m=%.0f SNR=%.1f"), new_star.Mass, new_star.SNR));

		if (m_polyStar.len() == 1)
		{
			m_guideOneStar = true;

			SetCurrentPosition(pImage, new_star);
			SetLockPosition(new_star);
			EvtServer.NotifyStarSelected(CurrentPosition());
			pFrame->pProfile->UpdateData(pImage, new_star.X, new_star.Y);
			Debug.AddLine("+++ GuiderPolyStar::OnLClick() - selected first star at (%7.2f, %7.2f) - SNR:%5.1f", new_star.X, new_star.Y, new_star.SNR);
		}
		else
		{
			m_guideOneStar = false;

			m_polyStar.makePolygon();
			m_polyStar.makeCentroid();

			SetCurrentPosition(pImage, m_polyStar.getCentroid());
			SetLockPosition(m_polyStar.getCentroid());
			EvtServer.NotifyStarSelected(CurrentPosition());
			pFrame->pProfile->UpdateData(pImage, m_polyStar.GetStar(0).X, m_polyStar.GetStar(0).Y);
			Debug.AddLine("+++ GuiderPolyStar::OnLClick() - selected additional star at (%7.2f, %7.2f) - SNR:%5.1f - total stars:%d", new_star.X, new_star.Y, new_star.SNR, m_polyStar.len());
		}

		SetState(STATE_SELECTED);
		pFrame->UpdateButtonsStatus();
	}
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);
    }

exit:
	Refresh();
	Update();
	return;
}

//******************************************************************************
inline static void DrawBox(wxClientDC& dc, const PHD_Point& star, int halfW, double scale)
{
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	double w = ROUND((halfW * 2 + 1) * scale);
	dc.DrawRectangle(int((star.X - halfW) * scale), int((star.Y - halfW) * scale), w, w);
}

//******************************************************************************
static void MarkLockPosition(wxClientDC& dc, const wxColour color, const int searchRegion, const double scaleFactor, const PHD_Point& lockPos)
{
	dc.SetPen(wxPen(color, 1, wxSOLID));

	int width = round(searchRegion / 2);

	// Crosshairs
	dc.DrawLine((lockPos.X - width) * scaleFactor, lockPos.Y * scaleFactor, (lockPos.X + width + 1) * scaleFactor, lockPos.Y * scaleFactor);
	dc.DrawLine(lockPos.X * scaleFactor, (lockPos.Y - width) * scaleFactor, lockPos.X * scaleFactor, (lockPos.Y + width + 1) * scaleFactor);

	// Diamond
	dc.DrawLine(lockPos.X * scaleFactor - width, lockPos.Y * scaleFactor, lockPos.X * scaleFactor, lockPos.Y * scaleFactor - width);
	dc.DrawLine(lockPos.X * scaleFactor, lockPos.Y * scaleFactor - width, lockPos.X * scaleFactor + width, lockPos.Y * scaleFactor);
	dc.DrawLine(lockPos.X * scaleFactor + width, lockPos.Y * scaleFactor, lockPos.X * scaleFactor, lockPos.Y * scaleFactor + width);
	dc.DrawLine(lockPos.X * scaleFactor, lockPos.Y * scaleFactor + width, lockPos.X * scaleFactor - width, lockPos.Y * scaleFactor);

	return;
}

// Define the repainting behaviour
void GuiderPolyStar::OnPaint(wxPaintEvent& event)
{

	//KOR - 20-Dec-14 - TODO: during selection, the starlist labels flash and then the single star box come up.  We need to figure out a way to preserver the labels.


	wxClientDC		dc(this);
	wxMemoryDC		memDC;

	GUIDER_STATE	state		= GetState();

	try
	{
		if (PaintHelper(dc, memDC))
		{
			throw ERROR_INFO("PaintHelper failed");
		}
		// PaintHelper drew the image and any overlays
		// now decorate the image to show the selection

		dc.SetBrush(*wxTRANSPARENT_BRUSH);

		if (state == STATE_UNINITIALIZED || state == STATE_SELECTING || state == STATE_SELECTED)
			m_starList.LabelImage(dc, m_scaleFactor);
	}
	catch (wxString Msg)
	{
		POSSIBLY_UNUSED(Msg);
	}
	
	if (m_guideOneStar)
		return GuiderOneStar::OnPaint(dc, memDC);
	
	static bool currentlyGuiding = false;
	try
    {
        // display bookmarks
        if (m_showBookmarks && m_bookmarks.size() > 0)
        {
			dc.SetPen(wxPen(COLOR_CYAN, 1, wxSOLID));
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

		if (state != STATE_GUIDING)
			currentlyGuiding = false;

		bool FoundStar = m_virtualGuideStar.IsValid();
		bool LabelSNRMass = true;

		// For STATE_SELECTED, we want to diplay the accepted and rejected star markers.
		//   Once we start calibrating or guiding, we only want to display the PolyStar
		//   markers.
		if (state == STATE_UNINITIALIZED || state == STATE_SELECTING || state == STATE_SELECTED)
		{
			m_starList.LabelImage(dc, m_scaleFactor);
			LabelSNRMass = false;
		}

		// If we have a PolyStar, we want to display its markers regardless of program
		//   state, but we may not add the SNR and Mass Label if it is already there.
		//   If we have a PolyStar, we might have a lock position ...
		// TODO: make sure we clear out lock position is we lose the PolyStar.
		if (m_polyStar.IsValid())
		{
			m_polyStar.markStars(dc, m_starList.GetStarColor(StarList::COLOR_ACCEPTED), GetSearchRegion(), m_scaleFactor, LabelSNRMass);
			m_polyStar.markCentroid(dc, m_starList.GetStarColor(StarList::COLOR_ACCEPTED), GetSearchRegion(), m_scaleFactor);

			const PHD_Point lockPos = LockPosition();
			if (lockPos.IsValid())
				MarkLockPosition(dc, COLOR_RED, GetSearchRegion(), m_scaleFactor, lockPos);
		}


		if (state == STATE_CALIBRATING_PRIMARY || state == STATE_CALIBRATING_SECONDARY)
        {
            // in the calibration process
            dc.SetPen(wxPen(COLOR_LIGHTGREEN, 1, wxSOLID));  // Draw the box around the star
            DrawBox(dc, m_virtualGuideStar, GetSearchRegion(), m_scaleFactor);
        }
        else if (state == STATE_CALIBRATED || state == STATE_GUIDING)
        {
            // locked and guiding
            if (FoundStar)
                dc.SetPen(wxPen(COLOR_LIGHTGREEN, 1, wxSOLID));  // Draw the box around the star
            else
				dc.SetPen(wxPen(COLOR_ORANGE, 1, wxDOT));
			DrawBox(dc, m_virtualGuideStar, GetSearchRegion(), m_scaleFactor);

			if (state == STATE_GUIDING)
			{
				m_polyStar.LogGuiding(!currentlyGuiding, m_guideLockPosition);
				currentlyGuiding = true;
			}
        }

		// Image logging
        if (state >= STATE_SELECTED && pFrame->IsImageLoggingEnabled() && pFrame->m_frameCounter != pFrame->m_loggedImageFrame)
        {
            // only log each image frame once
            pFrame->m_loggedImageFrame = pFrame->m_frameCounter;

            if (pFrame->GetLoggedImageFormat() == LIF_RAW_FITS) // Save star image as a FITS
            {
                SaveStarFITS2();
            }
            else  // Save star image as a JPEG
            {
                double LockX = LockPosition().X;
                double LockY = LockPosition().Y;

                wxBitmap SubBmp(60,60,-1);
                wxMemoryDC tmpMdc;
                tmpMdc.SelectObject(SubBmp);
				memDC.SetPen(wxPen(COLOR_GREEN, 1, wxDOT));
				memDC.DrawLine(0, LockY * m_scaleFactor, XWinSize, LockY * m_scaleFactor);
                memDC.DrawLine(LockX*m_scaleFactor, 0, LockX*m_scaleFactor, YWinSize);
    #ifdef __APPLEX__
                tmpMdc.Blit(0,0,60,60,&memDC,ROUND(m_star.X*m_scaleFactor)-30,Displayed_Image->GetHeight() - ROUND(m_star.Y*m_scaleFactor)-30,wxCOPY,false);
    #else
                tmpMdc.Blit(0,0,60,60,&memDC,ROUND(m_virtualGuideStar.X*m_scaleFactor)-30,ROUND(m_virtualGuideStar.Y*m_scaleFactor)-30,wxCOPY,false);
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

void GuiderPolyStar::SaveStarFITS2()
{
    double StarX = m_virtualGuideStar.X;
    double StarY = m_virtualGuideStar.Y;
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
    fits_close_file(fptr,&status);
}

#ifdef KOR_PANE
wxString GuiderPolyStar::GetSettingsSummary()
{
    // return a loggable summary of guider configs
    wxString s = wxString::Format(_T("Search region = %d px, Star mass tolerance "), GetSearchRegion());

    if (GetMassChangeThresholdEnabled())
        s += wxString::Format(_T("= %.1f%%\n"), GetMassChangeThreshold() * 100.0);
    else
        s += _T("disabled\n");

    return s;
}
#endif

//******************************************************************************
ConfigDialogPane *GuiderPolyStar::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuiderPolyStarConfigDialogPane(pParent, this);
}

//******************************************************************************
GuiderPolyStar::GuiderPolyStarConfigDialogSubPane::GuiderPolyStarConfigDialogSubPane(wxWindow* pParent, GuiderPolyStar* pGuider) : ConfigDialogPane(_T("Multi-Star Guiding"), pParent)
{
	int width = StringWidth(_T("0000"));

	m_pGuiderPolyStar = pGuider;

	m_pFailOneStar = new wxCheckBox(pParent, wxID_ANY, _("Fall back to One Star Guiding"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, _T("FailOneStar"));
	m_pFailOneStar->SetValue(true);
	DoAdd(m_pFailOneStar, _("Guide on one star if unable to identify multiple acceptable stars."));

	m_pMaxStars = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxSize(width + 30, -1), wxSP_ARROW_KEYS, MIN_STARS, MAX_STARS, DEF_STARS, _T("MaxStars"));
	DoAdd(_("Maximum Number of Stars"), m_pMaxStars, _("What is the Maximum Number of Stars to include in the guiding polygon?"));
	
	wxStaticBoxSizer* sz1 = new wxStaticBoxSizer(wxHORIZONTAL, pParent, "Signal to Noise Ratio");
	
	m_pAutoSNR = new wxCheckBox(pParent, wxID_ANY, _("Auto"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, _T("AutoSNR"));
	m_pAutoSNR->SetToolTip(_("Select stars with SNR based on the star that single star autoselect would have chosen"));
	sz1->Add(m_pAutoSNR);

	pParent->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &GuiderPolyStar::GuiderPolyStarConfigDialogSubPane::OnAutoSNRChecked, this, m_pAutoSNR->GetId());

	m_pMinSNR = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width + 30, -1), wxSP_ARROW_KEYS, MIN_SNR, MAX_SNR, DEF_SNR, _T("MinSNR"));
	wxSizer* sz_min_SNR = MakeLabeledControl("  Min", m_pMinSNR, "Minimum SNR that a star may have and still be included in the guiding polygon.");
	sz1->Add(sz_min_SNR);

	m_pMaxSNR = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width + 30, -1), wxSP_ARROW_KEYS, MIN_SNR, MAX_SNR, MAX_SNR, _T("MaxSNR"));
	wxSizer* sz_max_SNR = MakeLabeledControl("  Max", m_pMaxSNR, "Maximum SNR that a star may have and still be included in the guiding polygon.");
	sz1->Add(sz_max_SNR);
	Add(sz1);

	m_pMinMass = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(2 * width + 30, -1), wxSP_ARROW_KEYS, MIN_MASS, MAX_MASS, DEF_MASS, _T("MinMass"));
	DoAdd(_("Minimum Acceptable MASS Limit"), m_pMinMass, _("What is the Maximum MASS for a start to be accepted in the guiding polygon?"));

	m_pBGSigma = new wxSpinCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(width + 30, -1), wxSP_ARROW_KEYS, MIN_BGS, MAX_BGS, DEF_BGS, _T("BGSigma"));
	DoAdd(_("Background Noise Sigma"), m_pBGSigma, _("Pixel values less than this number of sigmas above the mean background level will be considered noise."));

	return;
}

//******************************************************************************
GuiderPolyStar::GuiderPolyStarConfigDialogSubPane::~GuiderPolyStarConfigDialogSubPane(void)
{
	return;
}

//******************************************************************************
void GuiderPolyStar::GuiderPolyStarConfigDialogSubPane::LoadValues(void)
{
	m_pFailOneStar->SetValue(m_pGuiderPolyStar->GetFailOneStar());
	m_pMaxStars->SetValue(m_pGuiderPolyStar->GetMaxStars());
	m_pAutoSNR->SetValue(m_pGuiderPolyStar->GetAutoSNR());
	m_pMinSNR->SetValue(m_pGuiderPolyStar->GetMinSNR());
	m_pMaxSNR->SetValue(m_pGuiderPolyStar->GetMaxSNR());
	m_pMinMass->SetValue(m_pGuiderPolyStar->GetMinMass());
	m_pBGSigma->SetValue(m_pGuiderPolyStar->GetBGSigma());

	m_pMinSNR->Enable(!m_pGuiderPolyStar->GetAutoSNR());
	m_pMaxSNR->Enable(!m_pGuiderPolyStar->GetAutoSNR());

	return;
}

//******************************************************************************
void GuiderPolyStar::GuiderPolyStarConfigDialogSubPane::UnloadValues(void)
{
	m_pGuiderPolyStar->SetFailOneStar(m_pFailOneStar->GetValue());
	m_pGuiderPolyStar->SetMaxStars(m_pMaxStars->GetValue());
	m_pGuiderPolyStar->SetAutoSNR(m_pAutoSNR->GetValue());
	m_pGuiderPolyStar->SetMinSNR(m_pMinSNR->GetValue());
	m_pGuiderPolyStar->SetMaxSNR(m_pMaxSNR->GetValue());
	m_pGuiderPolyStar->SetMinMass(m_pMinMass->GetValue());
	m_pGuiderPolyStar->SetBGSigma(m_pBGSigma->GetValue());

	return;
}

//******************************************************************************
void GuiderPolyStar::GuiderPolyStarConfigDialogSubPane::OnAutoSNRChecked(wxCommandEvent& event)
{
	m_pMinSNR->Enable(!event.IsChecked());
	m_pMaxSNR->Enable(!event.IsChecked());
	return;
}

//******************************************************************************
GuiderPolyStar::GuiderPolyStarConfigDialogPane::GuiderPolyStarConfigDialogPane(wxWindow *pParent, GuiderPolyStar *pGuider)
	: GuiderOneStarConfigDialogPane(pParent, pGuider)
{
	m_polyStarParams = new GuiderPolyStarConfigDialogSubPane(pParent, pGuider);
	DoAdd(m_polyStarParams);

	return;
}

//******************************************************************************
GuiderPolyStar::GuiderPolyStarConfigDialogPane::~GuiderPolyStarConfigDialogPane(void)
{
	return;
}

//******************************************************************************
void GuiderPolyStar::GuiderPolyStarConfigDialogPane::LoadValues(void)
{
	GuiderOneStarConfigDialogPane::LoadValues();
	m_polyStarParams->LoadValues();
	return;
}

//******************************************************************************
void GuiderPolyStar::GuiderPolyStarConfigDialogPane::UnloadValues(void)
{
	m_polyStarParams->UnloadValues();
	GuiderOneStarConfigDialogPane::UnloadValues();
	return;
}

