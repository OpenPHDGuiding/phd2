/*
 *  star.h
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

#ifndef POLYSTAR_H_INCLUDED
#define POLYSTAR_H_INCLUDED

#include "point.h"
#include "star.h"
#include "starlist.h"

class PolyStar : public PHD_Point
{
public:
	const int	CENTROID_MARKER_RADIUS	= 15;
	const int	CENTROID_MARKER_TAB_LEN = 5;
	const int	CENTROID_PEN_WIDTH		= 1;

#ifdef KOR_OUT
    enum FindMode
    {
        FIND_CENTROID,
        FIND_PEAK,
    };

    enum FindResult
    {
        STAR_OK=0,
        STAR_SATURATED,
        STAR_LOWSNR,
        STAR_LOWMASS,
        STAR_TOO_NEAR_EDGE,
        STAR_MASSCHANGE,
        STAR_ERROR,
    };

    double Mass;
    double SNR;
#endif

	PolyStar(void);
	PolyStar(const PolyStar &poly_star);
	PolyStar(std::vector<Star> starList, int maxStars);
	~PolyStar();

	PolyStar&	operator=(const PolyStar& rhs);

	int			AddStar(Star star);
	bool		RemoveStar(Star star, int distance);
	int			len(void) const;

	bool		IsValid(void) const;
    void		Invalidate(void);
	void		RemoveStars(void);

	PHD_Point	getCentroid(void) const;
	double		getMass(void) const;
	double		getSNR(void) const;

// KOR_OUT    bool		AutoFind(const usImage& image, int edgeAllowance, int searchRegion, double scaleFactor, bool rotation);
	bool		Find(const usImage *pImg, int searchRegion, Star::FindMode mode);
	void		markStars(wxClientDC &dc, wxColor color, int searchRegion, double scaleFactor, bool mark_SNR_MASS);
	void		markCentroid(wxClientDC &dc, wxColor color, int searchRegion, double scaleFactor);
	void		makePolygon(void);
	void		makeCentroid(void);

	Star		GetStar(const size_t star_num);

	void		LogGuiding(bool includeHeader, PHD_Point& lockPosition);

	static void		debugDump(const char* label, const PolyStar &poly_star);

#ifdef KOR_OUT
    /*
     * Note: contrary to most boolean PHD functions, the star find functions return
     *       a boolean indicating success instead of a boolean indicating an
     *       error
     */
    bool Find(const usImage *pImg, int searchRegion, FindMode mode);
    bool Find(const usImage *pImg, int searchRegion, int X, int Y, FindMode mode);
    bool WasFound(FindResult result);
    bool WasFound(void);

    void SetError(FindResult error);
    FindResult GetError(void) const;
#endif



private:
	std::vector<Star>	m_starList;
	PHD_Point			m_centroid;
	double				m_mass;
	double				m_SNR;

#ifdef KOR_OUT
    FindResult m_lastFindResult;
#endif
};

#ifdef KOR_OUT
inline PolyStar::FindResult PolyStar::GetError(void) const
{
    return m_lastFindResult;
}
#endif
#endif /* POLYSTAR_H_INCLUDED */
