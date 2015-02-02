/*
 *  polystar.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Refactored by Bret McKee
 *  Copyright (c) 2006-2010 Craig Stark.
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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

static bool sort_stars_asc(Star i, Star j) { return(i.X < j.X); }
static bool sort_stars_desc(Star i, Star j) { return(i.X > j.X); }

//******************************************************************************
PolyStar::PolyStar(void)
{
	m_mass = 0.0;
	m_SNR = 0.0;
}

//******************************************************************************
PolyStar::PolyStar(const PolyStar &poly_star)
{
	m_starList = poly_star.m_starList;
	m_centroid = poly_star.getCentroid();
	m_mass = poly_star.getMass();
	m_SNR = poly_star.getSNR();
}

//******************************************************************************
PolyStar::PolyStar(std::vector<Star> starList, int maxStars)
{
	m_starList = starList;
	while (m_starList.size() > maxStars)
	{
		size_t pos = 0;
		for (size_t ndx = 1; ndx < m_starList.size(); ndx++)
		{
			if (m_starList[ndx].SNR < m_starList[pos].SNR)
				pos = ndx;
		}
		Debug.AddLine("PolyStar::Polystar() - have %d stars  need %d stars  removing star %d with SNR %4.1f", m_starList.size(), maxStars, pos, m_starList[pos].SNR);
		m_starList.erase(m_starList.begin() + pos);
	}

	if (m_starList.size() > 1)
	{
		makePolygon();
		makeCentroid();
	}
}

//******************************************************************************
PolyStar::~PolyStar(void)
{
}

//******************************************************************************
PolyStar&	PolyStar::operator=(const PolyStar& rhs)
{
	if (this != &rhs)
	{
		m_starList = rhs.m_starList;
		m_centroid = rhs.m_centroid;
	}
	return *this;
}

//******************************************************************************
int PolyStar::AddStar(Star star)
{
	m_starList.push_back(star);
	return m_starList.size();
}

//******************************************************************************
bool PolyStar::RemoveStar(Star star, int distance)
{
	bool	rc = false;

	for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
	{
		Debug.AddLine("   +++ PolyStar::RemoveStar() - checking star %d - starList:(%7.2f, %7.2f)  search:(%7.2f, %7.2f) - distance: %6.2f",
			ndx, m_starList[ndx].X, m_starList[ndx].Y, star.X, star.Y, m_starList[ndx].Distance(star));

		if (m_starList[ndx].Distance(star) < distance)
		{
			m_starList.erase(m_starList.begin() + ndx);
			rc = true;
			break;
		}
	}
	return rc;
}

//******************************************************************************
int PolyStar::len(void) const
{
	return m_starList.size();
}

//******************************************************************************
double PolyStar::getMass(void) const
{
	return m_mass;
}

//******************************************************************************
double PolyStar::getSNR(void) const
{
	return m_SNR;
}

//******************************************************************************
bool PolyStar::IsValid(void) const
{
	if (m_starList.size() < 2)
		return false;

	for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
	{
		if (!m_starList[ndx].IsValid())
			return false;
	}

	if (!m_centroid.IsValid())
		return false;

	return true;
}

//******************************************************************************
void PolyStar::Invalidate(void)
{
	//TODO: what goes here?
	m_mass = 0.0;
	m_SNR = 0.0;
}

//******************************************************************************
void PolyStar::RemoveStars(void)
{
	m_starList.clear();
	m_centroid.Invalidate();
	Invalidate();
}

//******************************************************************************
PHD_Point PolyStar::getCentroid(void) const
{
	assert(IsValid());
	return m_centroid;
}

//******************************************************************************
Star PolyStar::GetStar(const size_t star_num)
{
	return Star(m_starList[star_num]);
}

//******************************************************************************
bool PolyStar::Find(const usImage *pImg, int searchRegion, Star::FindMode mode)
{
	bool	rc = 0;				// PolyStar::Find() return code
	bool	loc_rc;				// Star::Find() return code

	Debug.AddLine("   +++ PolyStar::Find() - Updating Star Locations - %d stars", m_starList.size());
	for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
	{
		if ((loc_rc = m_starList[ndx].Find(pImg, searchRegion, mode)) == 0)
		{
			// TODO: what whould we do about the star mass checks ...
		}
		else
			rc = loc_rc;

	}

	if (rc == 1)					// Means every star was either OK or saturated
	{
		makeCentroid();

		double tot_mass = 0.0;
		double tot_SNR = 0.0;
		for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
		{
			tot_mass += m_starList[ndx].Mass;
			tot_SNR += m_starList[ndx].SNR;
		}

		m_mass = tot_mass / m_starList.size();
		m_SNR = tot_SNR / m_starList.size();
		Debug.AddLine("   +++ PolyStar::Find() - AVG Mass: %.1f  SNR:%.1f", m_mass, m_SNR);

	}

	return rc;
}

//******************************************************************************
// We have to generate a non-selfintersecting polygon, meaning none of the edges 
//   can cross each other.
void PolyStar::makePolygon(void)
{
	assert(m_starList.size() > 1);

	if (m_starList.size() == 2)			// Don't need to reorder 2 stars--it's just a line
		return;

	// Find leftmost and rightmost stars	
	size_t	left = 0;
	size_t	right = 0;

	for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
	{
		if (m_starList[ndx].X < m_starList[left].X)
			left = ndx;
		if (m_starList[ndx].X > m_starList[right].X)
			right = ndx;
	}

	double slope = (m_starList[right].Y - m_starList[left].Y) / (m_starList[right].X - m_starList[left].X);

	// Find stars that are above left to right line and those below it
	// Generate a slope from the leftmost star to the current star.  If that slope is 
	//   greater than the slope between the leftmost and rightmost stars, the current
	//   star is above those two stars.  Otherwise, it is below them.
	std::vector<Star>	stars_above;
	std::vector<Star>	stars_below;

	for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
	{
		if (ndx == left || ndx == right)
			continue;

		double tst_slope = (m_starList[ndx].Y - m_starList[left].Y) / (m_starList[ndx].X - m_starList[left].X);
		if (tst_slope >= slope)
			stars_above.push_back(m_starList[ndx]);
		else
			stars_below.push_back(m_starList[ndx]);
	}

	std::sort(stars_above.begin(), stars_above.end(), sort_stars_asc);
	std::sort(stars_below.begin(), stars_below.end(), sort_stars_desc);

	// Reorder starList so that stars are vertices of a non-intersecting polygon
	// The leftmost star will be first.  Then we add the stars that are above the 
	//   left-to-rightmost star line in ascending X order.  Then we add the
	//   rightmost star.  Then we add the stars that are below the line in 
	//   descending X order to close the polygon.
	Star	left_star = m_starList[left];
	Star	right_star = m_starList[right];
	int		pos = 0;

	m_starList[pos++] = left_star;
	for (size_t ndx = 0; ndx < stars_above.size(); ndx++)
		m_starList[pos++] = stars_above[ndx];
	m_starList[pos++] = right_star;
	for (size_t ndx = 0; ndx < stars_below.size(); ndx++)
		m_starList[pos++] = stars_below[ndx];

	StarList::DebugPrintStars("PolyStar::makePolygon - polygon stars:", m_starList);
	return;
}

//******************************************************************************
void PolyStar::makeCentroid(void)
{
	assert(m_starList.size() > 1);

	m_centroid.Invalidate();

	if (m_starList.size() == 2)		// For 2 stars, find the midpoint of the line between them
	{
		double x_mid = MIN(m_starList[0].X, m_starList[1].X) + abs(m_starList[0].X - m_starList[1].X) / 2.0;
		double y_mid = MIN(m_starList[0].Y, m_starList[1].Y) + abs(m_starList[0].Y - m_starList[1].Y) / 2.0;
		m_centroid.SetXY(x_mid, y_mid);
	}
	else							// For 3 or more, use the polygon centroid algorithm from wikipedia
	{
		double sum = 0.0;
		double sum_x = 0.0;
		double sum_y = 0.0;
		for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
		{
			size_t next = ndx + 1;
			if (next == m_starList.size())
				next = 0;

			sum += (m_starList[ndx].X * m_starList[next].Y - m_starList[next].X * m_starList[ndx].Y);
			sum_x += (m_starList[ndx].X + m_starList[next].X) * (m_starList[ndx].X * m_starList[next].Y - m_starList[next].X * m_starList[ndx].Y);
			sum_y += (m_starList[ndx].Y + m_starList[next].Y) * (m_starList[ndx].X * m_starList[next].Y - m_starList[next].X * m_starList[ndx].Y);
		}
		double area = sum / 2.0;
		m_centroid.SetXY(sum_x / (6 * area), sum_y / (6 * area));
	}

	Debug.AddLine("  Centroid    (%8.4f, %8.4f)", m_centroid.X, m_centroid.Y);
	return;
}

//******************************************************************************
void PolyStar::debugDump(const char* label, const PolyStar &poly_star)
{
	StarList::DebugPrintStars(label, poly_star.m_starList);

	if (poly_star.m_centroid.IsValid())
		Debug.AddLine("   Centroid: (%8.4f, %8.4f)", poly_star.m_centroid.X, poly_star.m_centroid.Y);
	else
		Debug.AddLine("   No centroid");

	return;
}

//******************************************************************************
void PolyStar::markStars(wxClientDC &dc, wxColour color, int searchRegion, double scaleFactor, bool mark_SNR_MASS)
{
	wxFont		font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	dc.SetFont(font);
	dc.SetTextForeground(color);

	Debug.AddLine(wxString::Format("PolyStar::markStar() - entered - %d stars", len()));

	if (m_starList.size() == 0)
		return;

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	double SR_scaled = searchRegion * scaleFactor;
	int SR_width = ROUND(SR_scaled * 2.0);

	for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
	{
		dc.SetPen(wxPen(color, 1, wxSOLID));
		wxPoint p((int)(m_starList[ndx].X * scaleFactor), (int)(m_starList[ndx].Y * scaleFactor));
		dc.DrawRectangle(ROUND(p.x - SR_scaled), ROUND(p.y - SR_scaled), SR_width, SR_width);

		size_t pos = ndx + 1;
		if (pos == m_starList.size())
			pos = 0;
		wxPoint q((int)(m_starList[pos].X * scaleFactor), (int)(m_starList[pos].Y * scaleFactor));
		dc.SetPen(wxPen(color, 1, wxPENSTYLE_DOT));
		dc.DrawLine(p.x, p.y, q.x, q.y);
		dc.DrawText(wxString::Format("%d", ndx), p.x, p.y + 7);

		if (mark_SNR_MASS && m_starList[pos].SNR > 0 && m_starList[pos].Mass > 0)
		{
			char	SNR_label[64];
			_snprintf(SNR_label, sizeof(SNR_label), "%.1f:%d", m_starList[pos].SNR, (int)m_starList[pos].Mass);
			dc.DrawText(SNR_label, p.x + 5, p.y - 7);
		}

	}

	return;
}

//******************************************************************************
void PolyStar::markCentroid(wxClientDC &dc, wxColour color, int searchRegion, double scaleFactor)
{
	if (!m_centroid.IsValid())
		return;

	wxPoint centroid((int)(m_centroid.X * scaleFactor), (int)(m_centroid.Y * scaleFactor));

	dc.SetPen(wxPen(color, CENTROID_PEN_WIDTH, wxSOLID));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);

	dc.DrawCircle(centroid, CENTROID_MARKER_RADIUS);
	dc.DrawLine(centroid.x - CENTROID_MARKER_RADIUS, centroid.y, centroid.x - CENTROID_MARKER_RADIUS + CENTROID_MARKER_TAB_LEN, centroid.y);
	dc.DrawLine(centroid.x + CENTROID_MARKER_RADIUS, centroid.y, centroid.x + CENTROID_MARKER_RADIUS - CENTROID_MARKER_TAB_LEN, centroid.y);
	dc.DrawLine(centroid.x, centroid.y - CENTROID_MARKER_RADIUS, centroid.x, centroid.y - CENTROID_MARKER_RADIUS + CENTROID_MARKER_TAB_LEN);
	dc.DrawLine(centroid.x, centroid.y + CENTROID_MARKER_RADIUS, centroid.x, centroid.y + CENTROID_MARKER_RADIUS - CENTROID_MARKER_TAB_LEN);

	return;
}

//******************************************************************************
void PolyStar::LogGuiding(bool includeHeader, PHD_Point& lockPosition)
{
	PolystarLog.ClearLine();

	if (includeHeader)
		PolystarLog.AddHeaderLine(*this);

	for (size_t ndx = 0; ndx < m_starList.size(); ndx++)
		PolystarLog.AddStar(m_starList[ndx]);

	PolystarLog.AddPoint(m_centroid.X, m_centroid.Y);
	PolystarLog.AddPoint(lockPosition.X, lockPosition.Y);
	PolystarLog.AddPoint(lockPosition.X - m_centroid.X, lockPosition.Y - m_centroid.Y);

	PolystarLog.LogLine();

	return;
}

