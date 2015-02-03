/*
 *  star.cpp
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

Star::Star(void)
{
    Invalidate();
    // Star is a bit quirky in that we use X and Y after the star is Invalidate()ed.
    X = Y = 0.0;
}

Star::~Star(void)
{
}

bool Star::WasFound(FindResult result)
{
    bool bReturn = false;

    if (IsValid() &&
        (result == STAR_OK || result == STAR_SATURATED))
    {
        bReturn = true;
    }

    return bReturn;
}

bool Star::WasFound(void)
{
    return WasFound(m_lastFindResult);
}

void Star::Invalidate(void)
{
    Mass = 0.0;
    SNR = 0.0;
    m_lastFindResult = STAR_ERROR;
    PHD_Point::Invalidate();
}

void Star::SetError(FindResult error)
{
    m_lastFindResult = error;
}

bool Star::Find(const usImage *pImg, int searchRegion, int base_x, int base_y, FindMode mode)
{
    FindResult Result = STAR_OK;
    double newX = base_x;
    double newY = base_y;

    try
    {
        Debug.AddLine(wxString::Format("Star::Find(0x%p, %d, %d, %d)", pImg, searchRegion, base_x, base_y));

        if (base_x < 0 || base_y < 0)
        {
            throw ERROR_INFO("coordinates are invalid");
        }

        // corners of search region
        int start_x = base_x - searchRegion;
        int start_y = base_y - searchRegion;
        int end_x = base_x + searchRegion;
        int end_y = base_y + searchRegion;

        // make sure we do not look outside the subframe
        if (pImg->Subframe.GetWidth() > 0)
        {
            start_x = wxMax(start_x, pImg->Subframe.GetLeft());
            start_y = wxMax(start_y, pImg->Subframe.GetTop());
            end_x = wxMin(end_x, pImg->Subframe.GetRight());
            end_y = wxMin(end_y, pImg->Subframe.GetBottom());
        }
        else
        {
            start_x = wxMax(start_x, 0);
            start_y = wxMax(start_y, 0);
            end_x = wxMin(end_x, pImg->Size.GetWidth() - 1);
            end_y = wxMin(end_y, pImg->Size.GetHeight() - 1);
        }

        const unsigned short *dataptr = pImg->ImageData;
        int rowsize = pImg->Size.GetWidth();

        // compute localmin and localmean, which we need to find the star
        unsigned short localmin = 65535;
        double localmean = 0.0;
        for (int y = start_y; y <= end_y; y++)
        {
            for (int x = start_x; x <= end_x; x++)
            {
                unsigned short val = *(dataptr + x + rowsize * y);
                if (val < localmin)
                    localmin = val;
                localmean += (double) val;
            }
        }

        double area = (double)((end_x - start_x + 1) * (end_y - start_y + 1));
        localmean /= area;

        // get rough guess on star's location by finding the peak value within the search region

        unsigned long maxlval = 0;
        unsigned short max = 0, nearmax1 = 0, nearmax2 = 0;
        unsigned long sum = 0;

        for (int y = start_y + 1; y <= end_y - 1; y++)
        {
            for (int x = start_x + 1; x <= end_x - 1; x++)
            {
                unsigned long lval;

                lval = *(dataptr + (x + 0) + rowsize * (y + 0)) +  // combine adjacent pixels to smooth image
                       *(dataptr + (x + 1) + rowsize * (y + 0)) +        // find max of this smoothed area and set
                       *(dataptr + (x - 1) + rowsize * (y + 0)) +        // base_x and y to be this spot
                       *(dataptr + (x + 0) + rowsize * (y + 1)) +
                       *(dataptr + (x + 0) + rowsize * (y - 1)) +
                       *(dataptr + (x + 0) + rowsize * (y + 0));  // weight current pixel by 2x

                if (lval >= maxlval)
                {
                    base_x = x;
                    base_y = y;
                    maxlval = lval;
                }

                unsigned short sval = *(dataptr + x + rowsize * y) - localmin;
                sum += sval;

                if (sval > max)
                    std::swap(sval, max);
                if (sval > nearmax1)
                    std::swap(sval, nearmax1);
                if (sval > nearmax2)
                    std::swap(sval, nearmax2);
            }
        }

        // SNR = max / mean = max / (sum / area) = max * area / sum
        if (sum > 0)
            SNR = (double) max * area / (double) sum;
        else
            SNR = 0.0;

        if (mode == FIND_PEAK)
        {
            // only finding the peak, we are done. Fill in an arbitrary Mass value
            newX = base_x;
            newY = base_y;
            Mass = max;
        }
        else
        {
            // should be close now, hone in by finding the weighted average position

            const int hft_range = 7;

            // we try these thresholds in this order trying to get a mass >= 10
            double thresholds[] =
            {
                localmean + ((double) max + localmin - localmean) / 10.0,  // Note: max already has localmin pulled from it
                localmean,
                (double) localmin
            };

            int startx1 = wxMax(start_x, base_x - hft_range);
            int starty1 = wxMax(start_y, base_y - hft_range);
            int endx1 = wxMin(end_x, base_x + hft_range);
            int endy1 = wxMin(end_y, base_y + hft_range);

            double mass = 0.0, mx = 0.0, my = 0.0;

            for (unsigned int i = 0; i < WXSIZEOF(thresholds) && mass < 10.0; i++)
            {
                mass = mx = my = 0.000001;
                double threshold = thresholds[i];
                for (int y = starty1; y <= endy1; y++)
                {
                    for (int x = startx1; x <= endx1; x++)
                    {
                        double val = (double) *(dataptr + x + rowsize * y) - threshold;
                        if (val > 0.0)
                        {
                            mx += (double) x * val;
                            my += (double) y * val;
                            mass += val;
                        }
                    }
                }
            }

            Mass = mass;

        /*
         
        Original Code: 
            if (mass < 10.0)
                Result = STAR_LOWMASS;
            else if (SNR < 3.0)
                Result = STAR_LOWSNR;
            else
            {
                newX = mx / mass;
                newY = my / mass;
                // even at saturation, the max values may vary a bit due to noise
                // Call it saturated if the the top three values are within 32 parts per 65535 of max
                if ((unsigned int)(max - nearmax2) * 65535U < 32U * (unsigned int) max)
                    Result = STAR_SATURATED;
            }
            */


	        // These modifications make it possible to find the Laser Point as a Star
	        if (mass < 5.0) {
	            Result = STAR_LOWMASS;
	        }
	        else if (SNR < 1.0) {
	            Result = STAR_LOWSNR;
	        }
	        else {
	            newX = mx / mass;
	            newY = my / mass;
	            if (max == nearmax2) {
	                Result = STAR_SATURATED;
	            }
	        }

        }
         
         

        
        
        
        
        
        
    }
    catch (wxString Msg)
    {
        POSSIBLY_UNUSED(Msg);

        if (Result == STAR_OK)
        {
            Result = STAR_ERROR;
        }
    }

    // update state
    SetXY(newX, newY);
    m_lastFindResult = Result;

    bool bReturn = WasFound(Result);

    if (!bReturn)
    {
        Mass = 0.0;
        SNR = 0.0;
    }

    Debug.AddLine(wxString::Format("Star::Find returns %d (%d), X=%.2f, Y=%.2f, Mass=%.f, SNR=%.1f",
        bReturn, Result, newX, newY, Mass, SNR));

    return bReturn;
}

bool Star::Find(const usImage *pImg, int searchRegion, FindMode mode)
{
    return Find(pImg, searchRegion, X, Y, mode);
}

struct FloatImg
{
    float *px;
    wxSize Size;
    int NPixels;

    FloatImg() : px(0) { }
    FloatImg(const wxSize& size) : px(0) { Init(size); }
    FloatImg(const usImage& img) : px(0) {
        Init(img.Size);
        for (int i = 0; i < NPixels; i++)
            px[i] = (float) img.ImageData[i];
    }
    ~FloatImg() { delete[] px; }
    void Init(const wxSize& sz) { delete[] px;  Size = sz; NPixels = Size.GetWidth() * Size.GetHeight(); px = new float[NPixels]; }
    void Swap(FloatImg& other) { std::swap(px, other.px); std::swap(Size, other.Size); std::swap(NPixels, other.NPixels); }
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
//#define SAVE_AUTOFIND_IMG

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

struct Peak
{
    int x;
    int y;
    float val;

    Peak() { }
    Peak(int x_, int y_, float val_) : x(x_), y(y_), val(val_) { }
    bool operator<(const Peak& rhs) const { return val < rhs.val; }
};

static void RemoveItems(std::set<Peak>& stars, const std::set<int>& to_erase)
{
    int n = 0;
    for (std::set<Peak>::iterator it = stars.begin(); it != stars.end(); n++)
    {
        if (to_erase.find(n) != to_erase.end())
        {
            std::set<Peak>::iterator next = it;
            ++next;
            stars.erase(it);
            it = next;
        }
        else
            ++it;
    }
}

bool Star::AutoFind(const usImage& image, int extraEdgeAllowance, int searchRegion)
{
    if (!image.Subframe.IsEmpty())
    {
        Debug.AddLine("Autofind called on subframe, returning error");
        return false; // not found
    }

    wxBusyCursor busy;

    Debug.AddLine(wxString::Format("Star::AutoFind called with edgeAllowance = %d searchRegion = %d", extraEdgeAllowance, searchRegion));

    // run a 3x3 median first to eliminate hot pixels
    usImage smoothed;
    smoothed.CopyFrom(image);
    Median3(smoothed);

    // convert to floating point
    FloatImg conv(smoothed);

    // downsample the source image
    const int downsample = 1;
    if (downsample > 1)
    {
        FloatImg tmp;
        Downsample(tmp, conv, downsample);
        conv.Swap(tmp);
    }

    // run the PSF convolution
    {
        FloatImg tmp;
        psf_conv(tmp, conv);
        conv.Swap(tmp);
    }

    enum { CONV_RADIUS = 4 };
    int dw = conv.Size.GetWidth();      // width of the downsampled image
    int dh = conv.Size.GetHeight();     // height of the downsampled image
    wxRect convRect(CONV_RADIUS, CONV_RADIUS, dw - 2 * CONV_RADIUS, dh - 2 * CONV_RADIUS);  // region containing valid data

    SaveImage(conv, "PHD2_AutoFind.fit");

    enum { TOP_N = 100 };  // keep track of the brightest stars
    std::set<Peak> stars;  // sorted by ascending intensity

    double global_mean, global_stdev;
    GetStats(&global_mean, &global_stdev, conv, convRect);

    Debug.AddLine("AutoFind: global mean = %.1f, stdev %.1f", global_mean, global_stdev);

    const double threshold = 0.1;
    Debug.AddLine("AutoFind: using threshold = %.1f", threshold);

    // find each local maximum
    int srch = 4;
    for (int y = convRect.GetTop() + srch; y <= convRect.GetBottom() - srch; y++)
    {
        for (int x = convRect.GetLeft() + srch; x <= convRect.GetRight() - srch; x++)
        {
            float val = conv.px[dw * y + x];
            bool ismax = false;
            if (val > 0.0)
            {
                ismax = true;
                for (int j = -srch; j <= srch; j++)
                {
                    for (int i = -srch; i <= srch; i++)
                    {
                        if (i == 0 && j == 0)
                            continue;
                        if (conv.px[dw * (y + j) + (x + i)] > val)
                        {
                            ismax = false;
                            break;
                        }
                    }
                }
            }
            if (!ismax)
                continue;

            // compare local maximum to mean value of surrounding pixels
            const int local = 7;
            double local_mean, local_stdev;
            wxRect localRect(x - local, y - local, 2 * local + 1, 2 * local + 1);
            localRect.Intersect(convRect);
            GetStats(&local_mean, &local_stdev, conv, localRect);

            // this is our measure of star intensity
            double h = (val - local_mean) / global_stdev;

            if (h < threshold)
            {
                //  Debug.AddLine(wxString::Format("AG: local max REJECT [%d, %d] PSF %.1f SNR %.1f", imgx, imgy, val, SNR));
                continue;
            }

            // coordinates on the original image
            int imgx = x * downsample + downsample / 2;
            int imgy = y * downsample + downsample / 2;

            stars.insert(Peak(imgx, imgy, h));
            if (stars.size() > TOP_N)
                stars.erase(stars.begin());
        }
    }

    for (std::set<Peak>::const_reverse_iterator it = stars.rbegin(); it != stars.rend(); ++it)
        Debug.AddLine("AutoFind: local max [%d, %d] %.1f", it->x, it->y, it->val);

    // merge stars that are very close into a single star
    {
        const int minlimitsq = 5 * 5;
    repeat:
        for (std::set<Peak>::const_iterator a = stars.begin(); a != stars.end(); ++a)
        {
            std::set<Peak>::const_iterator b = a;
            ++b;
            for (; b != stars.end(); ++b)
            {
                int dx = a->x - b->x;
                int dy = a->y - b->y;
                int d2 = dx * dx + dy * dy;
                if (d2 < minlimitsq)
                {
                    // very close, treat as single star
                    Debug.AddLine("AutoFind: merge [%d, %d] %.1f - [%d, %d] %.1f", a->x, a->y, a->val, b->x, b->y, b->val);
                    // erase the dimmer one
                    stars.erase(a);
                    goto repeat;
                }
            }
        }
    }

    // exclude stars that would fit within a single searchRegion box
    {
        // build a list of stars to be excluded
        std::set<int> to_erase;
        const int extra = 5; // extra safety margin
        const int fullw = searchRegion + extra;
        for (std::set<Peak>::const_iterator a = stars.begin(); a != stars.end(); ++a)
        {
            std::set<Peak>::const_iterator b = a;
            ++b;
            for (; b != stars.end(); ++b)
            {
                int dx = abs(a->x - b->x);
                int dy = abs(a->y - b->y);
                if (dx <= fullw && dy <= fullw)
                {
                    // stars closer than search region, exclude them both
                    // but do not let a very dim star eliminate a very bright star
                    if (b->val / a->val >= 5.0)
                    {
                        Debug.AddLine("AutoFind: close dim-bright [%d, %d] %.1f - [%d, %d] %.1f", a->x, a->y, a->val, b->x, b->y, b->val);
                    }
                    else
                    {
                        Debug.AddLine("AutoFind: too close [%d, %d] %.1f - [%d, %d] %.1f", a->x, a->y, a->val, b->x, b->y, b->val);
                        to_erase.insert(std::distance(stars.begin(), a));
                        to_erase.insert(std::distance(stars.begin(), b));
                    }
                }
            }
        }
        RemoveItems(stars, to_erase);
    }

    // exclude stars too close to the edge
    {
        enum { MIN_EDGE_DIST = 40 };
        int edgeDist = MIN_EDGE_DIST + extraEdgeAllowance;

        std::set<Peak>::iterator it = stars.begin();
        while (it != stars.end())
        {
            std::set<Peak>::iterator next = it;
            ++next;
            if (it->x <= edgeDist || it->x >= image.Size.GetWidth() - edgeDist ||
                it->y <= edgeDist || it->y >= image.Size.GetHeight() - edgeDist)
            {
                Debug.AddLine("AutoFind: too close to edge [%d, %d] %.1f", it->x, it->y, it->val);
                stars.erase(it);
            }
            it = next;
        }
    }

    // At first I tried running Star::Find on the survivors to find the best
    // star. This had the unfortunate effect of locating hot pixels which
    // the psf convolution so nicely avoids. So, don't do that!  -ag

    // find the brightest non-saturated star. If no non-saturated stars, settle for a saturated star.
    bool allowSaturated = false;
    while (true)
    {
        Debug.AddLine("AutoSelect: finding best star allowSaturated = %d", allowSaturated);

        for (std::set<Peak>::reverse_iterator it = stars.rbegin(); it != stars.rend(); ++it)
        {
            Star tmp;
            tmp.Find(&image, searchRegion, it->x, it->y, FIND_CENTROID);
            if (tmp.WasFound())
            {
                if (tmp.GetError() == STAR_SATURATED && !allowSaturated)
                {
                    Debug.AddLine("Autofind: star saturated [%d, %d] %.1f Mass %.f SNR %.1f", it->x, it->y, it->val, tmp.Mass, tmp.SNR);
                    continue;
                }
                SetXY(it->x, it->y);
                Debug.AddLine("Autofind returns star at [%d, %d] %.1f Mass %.f SNR %.1f", it->x, it->y, it->val, tmp.Mass, tmp.SNR);
                return true;
            }
        }

        if (allowSaturated)
            break; // no stars found

        Debug.AddLine("AutoFind: could not find a non-saturated star!");

        allowSaturated = true;
    }

    Debug.AddLine("Autofind: no star found");
    return false;
}
