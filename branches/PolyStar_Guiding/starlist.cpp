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
#include "starlist.h"

//******************************************************************************
const double	StarList::DEF_PEAK_LIMIT	= 100.0;
const int		StarList::DEF_CLOSE_LIMIT	= 5;
const int		StarList::DEF_EDGE_LIMIT	= 20;
const double	StarList::DUP_LIMIT			= 5.0;

//******************************************************************************
StarList::StarList()
{
// KOR_OUT 	m_snrLimit = DEF_SNR_LIMIT;
// KOR_OUT 	m_massLimit = DEF_MASS_LIMIT;
	m_peakLimit = DEF_PEAK_LIMIT;
	m_closeLimit = DEF_CLOSE_LIMIT;
	m_edgeLimit = DEF_EDGE_LIMIT;

	// These colors are acceptable for use with a red filter over the screen
	m_starColor[COLOR_ACCEPTED] = wxColour(0xFF, 0xFF, 0x00);
	m_starColor[COLOR_SATURATED] = wxColour(0xA0, 0xA0, 0xA0);
	m_starColor[COLOR_HOT_PIXEL] = wxColour(0x00, 0x66, 0x66);
	m_starColor[COLOR_TOO_CLOSE] = wxColour(0x66, 0xFF, 0xFF);
	m_starColor[COLOR_LOW_SNR] = wxColour(0x00, 0x99, 0x00);
	m_starColor[COLOR_HIGH_SNR] = wxColour(0x99, 0xFF, 0x99);
	m_starColor[COLOR_LOW_PEAK] = wxColour(0x00, 0x80, 0xFF);
	m_starColor[COLOR_LOW_MASS] = wxColour(0x00, 0x66, 0xCC);
	m_starColor[COLOR_NEAR_EDGE] = wxColour(0x33, 0x33, 0xFF);
	m_starColor[COLOR_STAR_ERROR] = wxColour(0x99, 0x99, 0xFF); 
}

//******************************************************************************
StarList::~StarList(void)
{
}

wxColour StarList::GetStarColor(int color)
{
	assert(color >= 0 && color < NUM_STAR_COLORS);
	return m_starColor[color];
}

//******************************************************************************
wxRect StarList::GetSearchArea(void)
{
	return m_searchArea;
}

//******************************************************************************
std::vector<Star> StarList::GetAcceptedStars(void)
{
	return m_acceptedStars;
}

//******************************************************************************
void StarList::UpdateCurrentPosition(const usImage& image, int searchRegion)
{	
	for (size_t ndx = 0; ndx < m_acceptedStars.size(); ndx++)
		m_acceptedStars[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_hotPixels.size(); ndx++)
		m_hotPixels[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_lowSNR.size(); ndx++)
		m_lowSNR[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_highSNR.size(); ndx++)
		m_highSNR[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_lowMass.size(); ndx++)
		m_lowMass[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_lowPeak.size(); ndx++)
		m_lowPeak[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_tooClose.size(); ndx++)
		m_tooClose[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_nearEdge.size(); ndx++)
		m_nearEdge[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_saturated.size(); ndx++)
		m_saturated[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	for (size_t ndx = 0; ndx < m_starError.size(); ndx++)
		m_starError[ndx].Find(&image, searchRegion, Star::FIND_CENTROID);

	return;
}

//wxRect	StarList::m_searchArea;
//******************************************************************************
struct FloatImg
{
    float*	px;
    wxSize	Size;
    int		NPixels;

    FloatImg() : px(0) { }

#ifdef KOR_OUT
	FloatImg(const FloatImg& img) : px(0)		// KOR - 24 Nov 14 - added constructor to make code a little easier
	{
		Init(Size);
		for (int i = 0; i < NPixels; i++)
			px[i] = (float)img.px[i];
	}
#endif

    FloatImg(const wxSize& size) : px(0) 
	{ 
		Init(size); 
	}

    FloatImg(const usImage& img) : px(0) 
	{
        Init(img.Size);
        for (int i = 0; i < NPixels; i++)
            px[i] = (float) img.ImageData[i];
    }

    ~FloatImg() 
	{ 
		delete[] px; 
	}

	void Init(const wxSize& sz)
	{
		delete[] px;
		Size = sz;
		NPixels = Size.GetWidth() * Size.GetHeight();
		px = new float[NPixels];
	}

    void Swap(FloatImg& other) 
	{ 
		std::swap(px, other.px); 
		std::swap(Size, other.Size); 
		std::swap(NPixels, other.NPixels); 
	}
};


static void GetStats(double *mean, double *stdev, const FloatImg& img, const wxRect& win)
{
    // Determine the mean and standard deviation
    double sum = 0.0;
    double a = 0.0;
    double q = 0.0;
    double k = 1.0;
    double km1 = 0.0;

    const int width = img.Size.GetWidth();
    const float *p0 = &img.px[win.GetTop() * width + win.GetLeft()];
    for (int y = 0; y < win.GetHeight(); y++)
    {
        const float *end = p0 + win.GetWidth();
        for (const float *p = p0; p < end; p++)
        {
            double const x = (double) *p;
            sum += x;
            double const a0 = a;
            a += (x - a) / k;
            q += (x - a0) * (x - a);
            km1 = k;
            k += 1.0;
        }
        p0 += width;
    }

    *mean = sum / km1;
    *stdev = sqrt(q / km1);
}


// un-comment to save the intermediate autofind image
#define SAVE_AUTOFIND_IMG

static void SaveImage(const FloatImg& img, const char *name)
{
#ifdef SAVE_AUTOFIND_IMG
    float maxv = img.px[0];
    float minv = img.px[0];

    for (int i = 1; i < img.NPixels; i++)
    {
        if (img.px[i] > maxv)
            maxv = img.px[i];
        if (img.px[i] < minv)
            minv = img.px[i];
    }

    usImage tmp;
    tmp.Init(img.Size);
    for (int i = 0; i < tmp.NPixels; i++)
    {
        tmp.ImageData[i] = (unsigned short)(((double) img.px[i] - minv) * 65535.0 / (maxv - minv));
    }

    tmp.Save(wxFileName(Debug.GetLogDir(), name).GetFullPath());
#endif // SAVE_AUTOFIND_IMG
}


static void psf_conv(FloatImg& dst, const FloatImg& src)
{
    dst.Init(src.Size);

    //                       A      B1     B2    C1     C2    C3     D1     D2     D3
    const double PSF[] = { 0.906, 0.584, 0.365, .117, .049, -0.05, -.064, -.074, -.094 };

    int const width = src.Size.GetWidth();
    int const height = src.Size.GetHeight();

    memset(dst.px, 0, src.NPixels * sizeof(float));

    /* PSF Grid is:
    D3 D3 D3 D3 D3 D3 D3 D3 D3
    D3 D3 D3 D2 D1 D2 D3 D3 D3
    D3 D3 C3 C2 C1 C2 C3 D3 D3
    D3 D2 C2 B2 B1 B2 C2 D2 D3
    D3 D1 C1 B1 A  B1 C1 D1 D3
    D3 D2 C2 B2 B1 B2 C2 D2 D3
    D3 D3 C3 C2 C1 C2 C3 D3 D3
    D3 D3 D3 D2 D1 D2 D3 D3 D3
    D3 D3 D3 D3 D3 D3 D3 D3 D3

    1@A
    4@B1, B2, C1, C3, D1
    8@C2, D2
    44 * D3
    */

    int psf_size = 4;

    for (int y = psf_size; y < height - psf_size; y++)
    {
        for (int x = psf_size; x < width - psf_size; x++)
        {
            float A, B1, B2, C1, C2, C3, D1, D2, D3;

#define PX(dx, dy) *(src.px + width * (y + (dy)) + x + (dx))
            A =  PX(+0, +0);
            B1 = PX(+0, -1) + PX(+0, +1) + PX(+1, +0) + PX(-1, +0);
            B2 = PX(-1, -1) + PX(+1, -1) + PX(-1, +1) + PX(+1, +1);
            C1 = PX(+0, -2) + PX(-2, +0) + PX(+2, +0) + PX(+0, +2);
            C2 = PX(-1, -2) + PX(+1, -2) + PX(-2, -1) + PX(+2, -1) + PX(-2, +1) + PX(+2, +1) + PX(-1, +2) + PX(+1, +2);
            C3 = PX(-2, -2) + PX(+2, -2) + PX(-2, +2) + PX(+2, +2);
            D1 = PX(+0, -3) + PX(-3, +0) + PX(+3, +0) + PX(+0, +3);
            D2 = PX(-1, -3) + PX(+1, -3) + PX(-3, -1) + PX(+3, -1) + PX(-3, +1) + PX(+3, +1) + PX(-1, +3) + PX(+1, +3);
            D3 = PX(-4, -2) + PX(-3, -2) + PX(+3, -2) + PX(+4, -2) + PX(-4, -1) + PX(+4, -1) + PX(-4, +0) + PX(+4, +0) + PX(-4, +1) + PX(+4, +1) + PX(-4, +2) + PX(-3, +2) + PX(+3, +2) + PX(+4, +2);
#undef PX
            int i;
            const float *uptr;

            uptr = src.px + width * (y - 4) + (x - 4);
            for (i = 0; i < 9; i++)
                D3 += *uptr++;

            uptr = src.px + width * (y - 3) + (x - 4);
            for (i = 0; i < 3; i++)
                D3 += *uptr++;
            uptr += 3;
            for (i = 0; i < 3; i++)
                D3 += *uptr++;

            uptr = src.px + width * (y + 3) + (x - 4);
            for (i = 0; i < 3; i++)
                D3 += *uptr++;
            uptr += 3;
            for (i = 0; i < 3; i++)
                D3 += *uptr++;

            uptr = src.px + width * (y + 4) + (x - 4);
            for (i = 0; i < 9; i++)
                D3 += *uptr++;

            double mean = (A + B1 + B2 + C1 + C2 + C3 + D1 + D2 + D3) / 81.0;
            double PSF_fit = PSF[0] * (A - mean) + PSF[1] * (B1 - 4.0 * mean) + PSF[2] * (B2 - 4.0 * mean) +
                PSF[3] * (C1 - 4.0 * mean) + PSF[4] * (C2 - 8.0 * mean) + PSF[5] * (C3 - 4.0 * mean) +
                PSF[6] * (D1 - 4.0 * mean) + PSF[7] * (D2 - 8.0 * mean) + PSF[8] * (D3 - 44.0 * mean);

            dst.px[width * y + x] = (float) PSF_fit;
        }
    }
}

static void Downsample(FloatImg& dst, const FloatImg& src, int downsample)
{
    int width = src.Size.GetWidth();
    int dw = src.Size.GetWidth() / downsample;
    int dh = src.Size.GetHeight() / downsample;

    dst.Init(wxSize(dw, dh));

    for (int yy = 0; yy < dh; yy++)
    {
        for (int xx = 0; xx < dw; xx++)
        {
            float sum = 0.0;
            for (int j = 0; j < downsample; j++)
                for (int i = 0; i < downsample; i++)
                    sum += src.px[(yy * downsample + j) * width + xx * downsample + i];
            float val = sum / (downsample * downsample);
            dst.px[yy * dw + xx] = val;
        }
    }
}

// This is just a copy of the one in star.cpp.  I can't use that one since it is
//   private.  If we want to combine, we can make it a separate class or something.
//   For now, I'm just duplicating it.
struct SL_Peak
{
    int x;
    int y;
    float val;

	SL_Peak() { }
	SL_Peak(int x_, int y_, float val_) : x(x_), y(y_), val(val_) { }
	bool operator<(const SL_Peak& rhs) const { return val < rhs.val; }
};


#ifdef KOR_OUT
static void RemoveItems(std::set<SL_Peak>& stars, const std::set<int>& to_erase)
{
    int n = 0;
	for (std::set<SL_Peak>::iterator it = stars.begin(); it != stars.end(); n++)
    {
        if (to_erase.find(n) != to_erase.end())
        {
			std::set<SL_Peak>::iterator next = it;
            ++next;
            stars.erase(it);
            it = next;
        }
        else
            ++it;
    }
}
#endif



//******************************************************************************
void StarList::clearStarLists(void)
{
	m_acceptedStars.clear();
	m_hotPixels.clear();
	m_lowSNR.clear();
	m_highSNR.clear();
	m_lowMass.clear();
	m_lowPeak.clear();
	m_tooClose.clear();
	m_nearEdge.clear();
	m_rotationOOB.clear();
	m_saturated.clear();
	m_starError.clear();
	return;
}

#ifdef KOR_OUT
//******************************************************************************
static bool findStarInVector(std::vector<Star>& list, Star& star)
{
	for (size_t ndx = 0; ndx < list.size(); ndx++)
	{
		if (star.Distance(list[ndx]) < StarList::DUP_LIMIT)
			return true;
	}
	return false;
}
#endif

#ifdef KOR_OUT
//******************************************************************************
static void extractSearchRect(const usImage& src, FloatImg& dst, const wxRect& srch_rect)
{
	int w = srch_rect.GetWidth();
	int h = srch_rect.GetHeight();
	
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int src_ndx = (srch_rect.GetTop() + y) * src.Size.GetWidth() + srch_rect.GetLeft() + x;
			int dst_ndx = y * w + x;
			dst.px[dst_ndx] = src.ImageData[src_ndx];
		}
	}
	return;
}
#endif

//******************************************************************************
bool StarList::AutoFind(const usImage& image, int searchRegion, double scaleFactor, double minSNR, double maxSNR, double minMass, int BGSigma)
{
	const int		DOWNSAMPLE_FACTOR	= 1;
	const int		CONV_RADIUS			= 4;
	const int		NUM_STAR_CANDIDATES = 100;
	const double	CANDIDATE_THRESHOLD	= 3.0;
	const int		SEARCH_REGION_PAD	= 5;		// Stars closer than the diagonal of the searchRegion + this are eliminated
	const double	PEAK_LIMIT_FACTOR	= 5.0;		// Stars that are more than this factor in peak difference are not eliminated
	const double	INT_ROUND_LIMIT		= 0.000001;	// For testing double == int (abs(double - int) < INT_ROUND_LIMIT)

	clearStarLists();

	if (!image.Subframe.IsEmpty())
	{
		Debug.AddLine("StarList::Autofind called on subframe");
		return false;
	}

	wxBusyCursor busy;

	Debug.AddLine("StarList::AutoFind() - searchRegion:%d  minSNR:%5.1f  maxSNR:%5.1f  minMass:%6.1f  BG Sigma:%d", searchRegion, minSNR, maxSNR, minMass, BGSigma);

	usImage smoothed;
	smoothed.CopyFrom(image);						// run a 3x3 median first to eliminate hot pixels
	SaveImage(FloatImg(smoothed), "PHD2_AutoFind_orig.fit");

	Median3(smoothed);
	SaveImage(FloatImg(smoothed), "PHD2_AutoFind_smoothed.fit");
		
	FloatImg	convoluted_image(smoothed);			// This will end up being the convoluted image on which
													//   will run the star finding algorithm

	if (DOWNSAMPLE_FACTOR > 1)						// downsample the source image
	{
		FloatImg tmp;
		Downsample(tmp, convoluted_image, DOWNSAMPLE_FACTOR);
		convoluted_image.Swap(tmp);
		SaveImage(convoluted_image, "PHD2_AutoFind_downsampled.fit");
	}

	{												// run the PSF convolution
		FloatImg tmp;
		psf_conv(tmp, convoluted_image);
		convoluted_image.Swap(tmp);
	}
	SaveImage(convoluted_image, "PHD2_AutoFind_convolution.fit");

	std::set<SL_Peak> star_candidates;					// Set of unique star candidates sorted by ascending intensity

	int dw = convoluted_image.Size.GetWidth();      // width of the downsampled image
	int dh = convoluted_image.Size.GetHeight();     // height of the downsampled image
	wxRect convRect(CONV_RADIUS, CONV_RADIUS, dw - 2 * CONV_RADIUS, dh - 2 * CONV_RADIUS);  // region containing valid data

	double global_mean, global_stdev;
	GetStats(&global_mean, &global_stdev, convoluted_image, convRect);

	Debug.AddLine("StarList::AutoFind - global mean = %.1f, stdev %.1f", global_mean, global_stdev);
	Debug.AddLine("StarList::AutoFind - using threshold = %.1f", CANDIDATE_THRESHOLD);

	// find each local maximum
	// For each point on the screen:
	//    if the pixel value is negative, skip the pixel
	//    Otherwise, the pixel is a local maximum is no other pixel in the 8-pixel block
	//       around it has a higher value.
	wxRect search_area(CONV_RADIUS * 2, CONV_RADIUS * 2, dw - (CONV_RADIUS * 4), dh - (CONV_RADIUS * 4));
	Debug.AddLine("Starlist::AutoFind - initial search area for PEAK selection: (%d, %d) to (%d, %d)", search_area.GetLeft(), search_area.GetTop(), search_area.GetRight(), search_area.GetBottom());

	struct PeakStar
	{
		Star	star;
		wxPoint	pos;
		double	peak;
	};
	std::vector<struct PeakStar>	all_stars;

	for (size_t y = search_area.GetTop(); y < search_area.GetBottom(); y++)
	{
		for (size_t x = search_area.GetLeft(); x < search_area.GetRight(); x++)
		{
			float val = convoluted_image.px[(dw * y) + x];
			bool ismax = false;
			if (val > 0.0)
			{
				ismax = true;
				for (int j = -CONV_RADIUS; j <= CONV_RADIUS; j++)
				{
					for (int i = -CONV_RADIUS; i <= CONV_RADIUS; i++)
					{
						if (i == 0 && j == 0)
							continue;
						if (convoluted_image.px[(dw * (y + j)) + (x + i)] > val)
						{
							ismax = false;
							break;
						}
					}
				}
			}

			if (!ismax)
				continue;

			int imgx = x * DOWNSAMPLE_FACTOR + CONV_RADIUS * 2;		// Convert coords back to original, non-downsampled, non-trimmed image
			int imgy = y * DOWNSAMPLE_FACTOR + CONV_RADIUS * 2;

			// We will automatically exclude any peak values that are within BGSigma stdevs of the mean
			//   background value.  If there is any chance that this was a star, it would be one 
			//   that we do not want anyway.  BGSigma should be chosen to maximize real stars while 
			//   minimizeing the selection of local peaks.
			if (val < global_mean + (global_stdev * BGSigma))
				continue;
				
			Star star;
			star.Find(&image, searchRegion, imgx, imgy, Star::FIND_CENTROID);
			star.Find(&image, searchRegion, Star::FIND_CENTROID);	// Refind from the center to ensure the optimum position, SNR, and Mass

			if (star.SNR < 0.1 && star.Mass < 0.1)					// Did not find a star (or anything that looks kind of like a star)
				continue;

			// When we find the centroid of a single hot pixel, it ends up being at an exact coordinate.  
			//   So, here we are looking for something like (100.000, 200.000). I guess we have a 1 in 
			//   100000 chance of calling a real star a hot pixel ...			
			if (abs(star.X - round(star.X)) < INT_ROUND_LIMIT && abs(star.Y - round(star.Y)) < INT_ROUND_LIMIT)
			{
				m_hotPixels.push_back(star);
				continue;
			}

			Debug.AddLine("KOR - local max (%4d, %4d) - val:%8.1f - star: (%8.4f, %8.4f)  SNR:%5.1f  mass:%6.1f", x, y, val, star.X, star.Y, star.SNR, star.Mass);

			// See if we have already done a star at this position
			bool replaced = false;
			for (size_t ndx = 0; ndx < all_stars.size(); ndx++)
			{
				double dist = hypot(abs(double(x - all_stars[ndx].pos.x)), abs(double(y - all_stars[ndx].pos.y)));
				if (dist < StarList::DUP_LIMIT)
				{
					if (val > all_stars[ndx].peak)
						all_stars[ndx].peak = val;
					if (star.SNR > all_stars[ndx].star.SNR)
						all_stars[ndx].star = star;
					replaced = true;
					break;
				}
			}
			if (replaced)
				continue;

			// We don't already have a star here, so add a new one
			struct PeakStar sp;
			sp.star = star;
			sp.pos = wxPoint(x, y);
			sp.peak = val;

			all_stars.push_back(sp);
		}
	}

	// Now we have a list of all of the stars (or things that look like stars--anything we might be temped to guide on!)
	
	// If stars are too close to each other, we may oscillate between them during 
	//   guiding.  So, we have to remove any pair of stars (or even group of stars)
	//   that will fit in the same searchRegion.
	// We add these stars to the too close list so we can keep track of them.  We
	//   also remove them from the all stars list so that we don't include them in
	//   any of the other lists.
	{
		// Make sure we can handle the diagonal plus a little extra ...
		const double srch_limit = (double)searchRegion * sqrt(2) + SEARCH_REGION_PAD;

		// Note that we have to use the PEAK-based coordinates because the Star-based
		//   coordinates may have already been combined.  The Star::Find() may have 
		//   found a "better" star that was in the same searchRegion.
		std::vector<wxPoint> remove_list;
		for (size_t ndx_a = 0; ndx_a < all_stars.size(); ndx_a++)
		{
			for (size_t ndx_b = 0; ndx_b < all_stars.size(); ndx_b++)
			{
				// Same star: skip it
				if (ndx_a == ndx_b)
					continue;

				double dist = hypot(abs(double(all_stars[ndx_a].pos.x - all_stars[ndx_b].pos.x)), abs(double(all_stars[ndx_a].pos.y - all_stars[ndx_b].pos.y)));
				// Not close: skip it
				if (dist > srch_limit)
					continue;

				// Bright star close to a dim star.  Star::Find() will do OK, so skip it
				//   Actually, I don't think this is likely to happen.  We used the Star::Find()
				//   to identify the stars to begin with, so we really shouldn't have the dimmer
				//   star in the list.  Oh well, just in case ...
				if (MAX(all_stars[ndx_a].peak, all_stars[ndx_b].peak) / MIN(all_stars[ndx_a].peak, all_stars[ndx_b].peak) > PEAK_LIMIT_FACTOR)
				{
					Debug.AddLine("StarList::AutoFind() - removing close dim star (%3d, %3d) %7.1f  (%3d, %3d) %7.1f", all_stars[ndx_a].pos.x, all_stars[ndx_a].pos.y, all_stars[ndx_a].peak, all_stars[ndx_b].pos.x, all_stars[ndx_b].pos.y, all_stars[ndx_b].peak);
					if (all_stars[ndx_a].peak < all_stars[ndx_b].peak)
						remove_list.push_back(all_stars[ndx_a].pos);
					else
						remove_list.push_back(all_stars[ndx_b].pos);
					
					continue;
				}

				// Add this one to the "remove" list, but leave it in the "all" list so we can compare
				//   other stars to it
				Debug.AddLine("StarList::AutoFind() - removing close star - (%3d, %3d) (%8.4f, %8.4f) Peak:%7.1f  SNR:%6.1f  Mass:%6.1f", all_stars[ndx_a].pos.x, all_stars[ndx_a].pos.y, all_stars[ndx_a].star.X, all_stars[ndx_a].star.Y, all_stars[ndx_a].peak, all_stars[ndx_a].star.SNR, all_stars[ndx_a].star.Mass);
//				Star close_star;
//				close_star.SetXY(all_stars[ndx_a].pos.x, all_stars[ndx_a].pos.y);

				Star close_star(all_stars[ndx_a].star);
				m_tooClose.push_back(close_star);
				remove_list.push_back(all_stars[ndx_a].pos);
				break;
			}
		}

		// Now, remove the stars in the remove list from the all stars list
		for (size_t ndx_a = 0; ndx_a < remove_list.size(); ndx_a++)
		{
			for (size_t ndx_b = 0; ndx_b < all_stars.size(); ndx_b++)
			{
				if (remove_list[ndx_a] == all_stars[ndx_b].pos)
				{
					all_stars.erase(all_stars.begin() + ndx_b);
					break;
				}
			}
		}
	}

	m_edgeLimit = DEF_EDGE_LIMIT;
	if (pMount && pMount->IsConnected() && !pMount->IsCalibrated())
		m_edgeLimit = wxMax(m_edgeLimit, pMount->CalibrationTotDistance());
	if (pSecondaryMount && pSecondaryMount->IsConnected() && !pSecondaryMount->IsCalibrated())
		m_edgeLimit = wxMax(m_edgeLimit, pSecondaryMount->CalibrationTotDistance());
		
	m_searchArea = wxRect(m_edgeLimit, m_edgeLimit, image.Size.GetWidth() - 2 * m_edgeLimit, image.Size.GetHeight() - 2 * m_edgeLimit);
	Debug.AddLine("StarList::AutoFind() - edge limit bounds (%3d, %3d) (%3d, %3d)", m_searchArea.GetLeft(), m_searchArea.GetTop(), m_searchArea.GetRight(), m_searchArea.GetBottom());


	// From this point on, we can consider the stars individually.  We have
	//   already eliminated any duplicates and any stars that are too close
	//   together.
	for (size_t ndx = 0; ndx < all_stars.size(); ndx++)
	{
		// We need to eliminate stars that are too close the edge of the image
		//   and might drift off during calibration, field rotation, dithering,
		//   or just bad guiding.
		{
			bool oob = false;
			if (all_stars[ndx].star.X < (double)m_searchArea.GetLeft() || all_stars[ndx].star.X >(double)m_searchArea.GetRight())
				oob = true;
			if (!oob && (all_stars[ndx].star.Y < (double)m_searchArea.GetTop() || all_stars[ndx].star.Y > m_searchArea.GetBottom()))
				oob = true;
			if (oob)
			{
				m_nearEdge.push_back(all_stars[ndx].star);
				continue;
			}
		}
		//Both SNR and PEAK limits need to be configurable so that the user can set them decently for 
		//   their equipment and exposure times as well as for the current sky conditions.

		// Eliminate the stars that are saturated--we won't get a good centroid
		//   on them.  If there are too many saturated stars, the user should reduce
		//   the exposure.
		if (all_stars[ndx].star.GetError() == Star::FindResult::STAR_SATURATED)
		{
			m_saturated.push_back(all_stars[ndx].star);
			continue;
		}

		if (all_stars[ndx].star.GetError() != Star::FindResult::STAR_OK)
		{
			m_starError.push_back(all_stars[ndx].star);
			continue;
		}

		// We want a star with a decent SNR.  
		if (all_stars[ndx].star.SNR < minSNR)
		{
			m_lowSNR.push_back(all_stars[ndx].star);
			continue;
		}

		// But if it is too high, it might be a group of hot pixels
		if (all_stars[ndx].star.SNR > maxSNR)
		{
			m_highSNR.push_back(all_stars[ndx].star);
			continue;
		}

#ifdef KOR_OUT
		// If you plot the value of the peak, you see a definite grouping of stars vs. non-stars.  There
		//   is probably some way to do this as a calculation.
		if (all_stars[ndx].peak < m_peakLimit)
		{
			m_lowPeak.push_back(all_stars[ndx].star);
			continue;
		}
#endif

		// We eliminate stars with too low of a mass.  Even really thin clouds 
		//   can make us lose a really low mass star
		if (all_stars[ndx].star.Mass < minMass)
		{
			m_lowMass.push_back(all_stars[ndx].star);
			continue;
		}
		

		// OK, if we made it this far, the star is a keeper
		m_acceptedStars.push_back(all_stars[ndx].star);
	}

	StarList::DebugPrintStars("StarList::Autofind - Rejected (HOT PIXEL)", m_hotPixels);
	StarList::DebugPrintStars("StarList::Autofind - Rejected (TOO CLOSE)", m_tooClose);
	StarList::DebugPrintStars("StarList::Autofind - Rejected (LOW SNR)", m_lowSNR);	
	StarList::DebugPrintStars("StarList::Autofind - Rejected (HIGH SNR)", m_highSNR);
	StarList::DebugPrintStars("StarList::Autofind - Rejected (LOW PEAK)", m_lowPeak);
	StarList::DebugPrintStars("StarList::Autofind - Rejected (LOW MASS)", m_lowMass);
	StarList::DebugPrintStars("StarList::Autofind - Rejected (NEAR EDGE)", m_nearEdge);
	StarList::DebugPrintStars("StarList::Autofind - Rejected (STAR ERROR)", m_starError);
	StarList::DebugPrintStars("StarList::Autofind - Rejected (SATURATED)", m_saturated);
	StarList::DebugPrintStars("StarList::Autofind - Accepted Stars", m_acceptedStars);

	return true;
}

void make_X(wxClientDC &dc, int X, int Y, int width)
{
	assert(width > 2);

	dc.DrawLine(X - width, Y - width, X - 2, Y - 2);
	dc.DrawLine(X - width, Y + width, X - 2, Y + 2);
	dc.DrawLine(X + width, Y - width, X + 2, Y - 2);
	dc.DrawLine(X + width, Y + width, X + 2, Y + 2);
	return;
}

//******************************************************************************
static void LabelStar(wxClientDC& dc, wxColour color, double scaleFactor, Star& star, StarList::StarSymbol symbol, int symbol_size, bool print_label, wxString label, int* label_y_pos)
{
	wxFont		font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

	dc.SetFont(font);
	dc.SetPen(wxPen(color, 2, wxSOLID));

	wxPoint p((int)round((star.X * scaleFactor)), (int)round((star.Y * scaleFactor)));
	if (symbol == StarList::CIRCLE)
		dc.DrawCircle(p, symbol_size);
	else
		make_X(dc, p.x, p.y, symbol_size);

	dc.SetTextForeground(color);
	if (star.SNR > 0 && star.Mass > 0)
	{
		char	SNR_label[64];
		_snprintf(SNR_label, sizeof(SNR_label), "%.1f:%d", star.SNR, (int)star.Mass);
		dc.DrawText(SNR_label, p.x + symbol_size + 5, p.y - 7);
	}
		
	if (print_label)
	{
		wxPoint p(2 + symbol_size, *label_y_pos + 2 + symbol_size);
		if (symbol == StarList::CIRCLE)
			dc.DrawCircle(p, symbol_size);
		else
			make_X(dc, p.x, p.y, symbol_size);


		dc.DrawText(label, p.x + symbol_size + 5, p.y - symbol_size - 2);
		*label_y_pos += symbol_size * 2 + 5;
	}
	return;
}

//******************************************************************************
void StarList::LabelImage(wxClientDC &dc, double scaleFactor)
{
	const int	RADIUS = 5;
	int			y_pos = 0;
	int			x_pos = 2 + RADIUS;
	int			x_text_pos = x_pos + RADIUS + 5;

	Debug.AddLine("StarList::labelImage() - entered");

	dc.SetBrush(*wxTRANSPARENT_BRUSH);

	Debug.AddLine("StarList::LabelImage() - scaleFactor:%g x_pos:%d y_pos:%d x_text_pos:%d", scaleFactor, x_pos, y_pos, x_text_pos);
	
	for (size_t ndx = 0; ndx < m_acceptedStars.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_ACCEPTED], scaleFactor, m_acceptedStars[ndx], StarList::CIRCLE, RADIUS, (ndx == 0), "Accepted Stars", &y_pos);

	for (size_t ndx = 0; ndx < m_saturated.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_SATURATED], scaleFactor, m_saturated[ndx], StarList::CIRCLE, RADIUS, (ndx == 0), "Saturated", &y_pos);

	for (size_t ndx = 0; ndx < m_hotPixels.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_HOT_PIXEL], scaleFactor, m_hotPixels[ndx], StarList::X, RADIUS, (ndx == 0), "Hot Pixels", &y_pos);

	for (size_t ndx = 0; ndx < m_tooClose.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_TOO_CLOSE], scaleFactor, m_tooClose[ndx], StarList::X, RADIUS, (ndx == 0), "Too Close", &y_pos);

	for (size_t ndx = 0; ndx < m_lowSNR.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_LOW_SNR], scaleFactor, m_lowSNR[ndx], StarList::X, RADIUS, (ndx == 0), "Low SNR", &y_pos);

	for (size_t ndx = 0; ndx < m_highSNR.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_HIGH_SNR], scaleFactor, m_highSNR[ndx], StarList::X, RADIUS, (ndx == 0), "High SNR", &y_pos);

	for (size_t ndx = 0; ndx < m_lowPeak.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_LOW_PEAK], scaleFactor, m_lowPeak[ndx], StarList::X, RADIUS, (ndx == 0), "Low Peak", &y_pos);

	for (size_t ndx = 0; ndx < m_lowMass.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_LOW_MASS], scaleFactor, m_lowMass[ndx], StarList::X, RADIUS, (ndx == 0), "Low Mass", &y_pos);

	for (size_t ndx = 0; ndx < m_nearEdge.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_NEAR_EDGE], scaleFactor, m_nearEdge[ndx], StarList::X, RADIUS, (ndx == 0), "Near Edge", &y_pos);

	for (size_t ndx = 0; ndx < m_starError.size(); ndx++)
		LabelStar(dc, m_starColor[COLOR_STAR_ERROR], scaleFactor, m_starError[ndx], StarList::X, RADIUS, (ndx == 0), "Star Error", &y_pos);

	return;
}

//******************************************************************************
void StarList::DebugPrintStars(const char* label, std::vector<Star> star_list)
{
	Debug.AddLine(wxString::Format("%s", label));
	for (size_t ndx = 0; ndx < star_list.size(); ndx++)
		Debug.AddLine("  star %04d - (%8.4f, %8.4f)  SNR:%5.1f  mass:%8.1f", ndx, star_list[ndx].X, star_list[ndx].Y, star_list[ndx].SNR, star_list[ndx].Mass);
	return;
}
