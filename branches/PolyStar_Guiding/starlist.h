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

#ifndef STARLISH_H_INCLUDED
#define STARLISH_H_INCLUDED

//#include "point.h"
#include "star.h"

class StarList
{
public:
    StarList(void);
    ~StarList();
	
	void				clearStarLists(void);

	bool				AutoFind(const usImage& image, int searchRegion, double scaleFactor, double minSNR, double maxSNR, double minMass, int BGSigma);
	void				UpdateCurrentPosition(const usImage& image, int searchRegion);
	wxRect				GetSearchArea(void);
	void				LabelImage(wxClientDC &dc, double scaleFactor);
	std::vector<Star>	GetAcceptedStars(void);
	wxColour			GetStarColor(int color);

	static void			DebugPrintStars(const char* label, std::vector<Star> starList);

	static const double		DEF_PEAK_LIMIT;
	static const int		DEF_CLOSE_LIMIT;
	static const int		DEF_EDGE_LIMIT;
	static const double		DUP_LIMIT;

	enum StarSymbol
	{
		CIRCLE,
		X
	};

	enum STAR_COLORS
	{
		COLOR_ACCEPTED,
		COLOR_SATURATED,
		COLOR_HOT_PIXEL,
		COLOR_TOO_CLOSE,
		COLOR_LOW_SNR,
		COLOR_HIGH_SNR,
		COLOR_LOW_PEAK,
		COLOR_LOW_MASS,
		COLOR_NEAR_EDGE,
		COLOR_STAR_ERROR,
		NUM_STAR_COLORS						// Indicates how many colors there are ...
	};

private:

	wxRect	m_searchArea;

	std::vector<Star>	m_acceptedStars;		// Accepted Stars
	std::vector<Star>	m_hotPixels;			// Stars identified and rejected as hot pixels
	std::vector<Star>	m_lowSNR;				// Stars rejected due to low SNR
	std::vector<Star>	m_highSNR;				// Stars rejected due to high SNR (hot pixel group?)
	std::vector<Star>	m_lowMass;				// Stars rejected due to low Mass
	std::vector<Star>	m_lowPeak;				// Stars rejected due to low PEAK values
	std::vector<Star>	m_tooClose;				// Stars rejected because they were too close to each other
	std::vector<Star>	m_nearEdge;				// Stars rejected because they were too near the edge
	std::vector<Star>	m_rotationOOB;			// Stars rejected because they might rotate off the image
	std::vector<Star>	m_saturated;			// Stars that were identified as saturated
	std::vector<Star>	m_starError;			// Stars that had some error during Star::Find()

	double	m_peakLimit;						// Star will be rejected if its PEAK value is less than this value
	int		m_closeLimit;						// Stars will be rejected if they are closer together than this number of pixels added to the searchArea
	int		m_edgeLimit;						// Star will be rejected if it closer to the edge than this number of pixels.  Note
												//   that this is the after calibration limit.  If the mount has not yet been
												//   calibrated, the edge limit will be increased.


	wxColour	m_starColor[NUM_STAR_COLORS];




};

#endif /* STARLISH_H_INCLUDED */
