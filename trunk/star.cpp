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

bool Star::Find(usImage *pImg, int searchRegion, int base_x, int base_y, FindMode mode)
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

    Debug.AddLine(wxString::Format("Star::Find returns %d, X=%.2f, Y=%.2f", bReturn, newX, newY));

    return bReturn;
}

bool Star::Find(usImage *pImg, int searchRegion, FindMode mode)
{
    return Find(pImg, searchRegion, X, Y, mode);
}

bool Star::AutoFind(usImage *pImg, int extraEdgeAllowance)
{
    if (!pImg->Subframe.IsEmpty())
    {
        Debug.AddLine("Autofind called on subframe, returning error");
        return false; // not found
    }

    Debug.AddLine(wxString::Format("Star::AutoFind called with edgeAllowance = %d", extraEdgeAllowance));

    // OK, do seem to need to run 3x3 median first
    Median3(*pImg);

    int linesize = pImg->Size.GetWidth();

    const double PSF[] = { 0.906, 0.584, 0.365, .117, .049, -0.05, -.064, -.074, -.094 };

    double BestPSF_fit = 0.0;
    int xpos = 0, ypos = 0;

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

    enum { MIN_EDGE_DIST = 40 };
    int edgeDist = MIN_EDGE_DIST + extraEdgeAllowance;

    for (int y = edgeDist; y < pImg->Size.GetHeight() - edgeDist; y++)
    {
        for (int x = edgeDist; x < linesize - edgeDist; x++)
        {
            float A, B1, B2, C1, C2, C3, D1, D2, D3;

            A = (float) *(pImg->ImageData + linesize * y + x);
            B1 = (float) *(pImg->ImageData + linesize * (y-1) +  x)    + (float) *(pImg->ImageData + linesize * (y+1) +  x)    + (float) *(pImg->ImageData + linesize *  y +    (x + 1)) + (float) *(pImg->ImageData + linesize *  y +    (x-1));
            B2 = (float) *(pImg->ImageData + linesize * (y-1) + (x-1)) + (float) *(pImg->ImageData + linesize * (y-1) + (x+1)) + (float) *(pImg->ImageData + linesize * (y+1) + (x + 1)) + (float) *(pImg->ImageData + linesize * (y+1) + (x-1));
            C1 = (float) *(pImg->ImageData + linesize * (y-2) +  x)    + (float) *(pImg->ImageData + linesize * (y+2) +  x)    + (float) *(pImg->ImageData + linesize *  y +    (x + 2)) + (float) *(pImg->ImageData + linesize *  y +    (x-2));
            C2 = (float) *(pImg->ImageData + linesize * (y-2) + (x-1)) + (float) *(pImg->ImageData + linesize * (y-2) + (x+1)) + (float) *(pImg->ImageData + linesize * (y+2) + (x + 1)) + (float) *(pImg->ImageData + linesize * (y+2) + (x-1)) +
                 (float) *(pImg->ImageData + linesize * (y-1) + (x-2)) + (float) *(pImg->ImageData + linesize * (y-1) + (x+2)) + (float) *(pImg->ImageData + linesize * (y+1) + (x + 2)) + (float) *(pImg->ImageData + linesize * (y+1) + (x-2));
            C3 = (float) *(pImg->ImageData + linesize * (y-2) + (x-2)) + (float) *(pImg->ImageData + linesize * (y-2) + (x+2)) + (float) *(pImg->ImageData + linesize * (y+2) + (x + 2)) + (float) *(pImg->ImageData + linesize * (y+2) + (x-2));
            D1 = (float) *(pImg->ImageData + linesize * (y-3) +  x)    + (float) *(pImg->ImageData + linesize * (y+3) +  x)    + (float) *(pImg->ImageData + linesize *  y +    (x + 3)) + (float) *(pImg->ImageData + linesize *  y +    (x-3));
            D2 = (float) *(pImg->ImageData + linesize * (y-3) + (x-1)) + (float) *(pImg->ImageData + linesize * (y-3) + (x+1)) + (float) *(pImg->ImageData + linesize * (y+3) + (x + 1)) + (float) *(pImg->ImageData + linesize * (y+3) + (x-1)) +
                 (float) *(pImg->ImageData + linesize * (y-1) + (x-3)) + (float) *(pImg->ImageData + linesize * (y-1) + (x+3)) + (float) *(pImg->ImageData + linesize * (y+1) + (x + 3)) + (float) *(pImg->ImageData + linesize * (y+1) + (x-3));
            D3 = 0.0;
            const unsigned short *uptr = pImg->ImageData + linesize * (y-4) + (x-4);
            int i;
            for (i=0; i<9; i++, uptr++)
                D3 += *uptr;
            uptr = pImg->ImageData + linesize * (y-3) + (x-4);
            for (i=0; i<3; i++, uptr++)
                D3 += *uptr;
            uptr += 3;
            for (i=0; i<3; i++, uptr++)
                D3 += *uptr;
            D3 += (float) *(pImg->ImageData + linesize * (y-2) + (x-4)) + (float) *(pImg->ImageData + linesize * (y-2) + (x+4)) + (float) *(pImg->ImageData + linesize * (y-2) + (x-3)) + (float) *(pImg->ImageData + linesize * (y-2) + (x+3)) +
                  (float) *(pImg->ImageData + linesize * (y+2) + (x-4)) + (float) *(pImg->ImageData + linesize * (y+2) + (x+4)) + (float) *(pImg->ImageData + linesize * (y+2) + (x-3)) + (float) *(pImg->ImageData + linesize * (y+2) + (x+3)) +
                  (float) *(pImg->ImageData + linesize *  y    + (x+4)) + (float) *(pImg->ImageData + linesize *  y    + (x-4)) +
                  (float) *(pImg->ImageData + linesize * (y-1) + (x-4)) + (float) *(pImg->ImageData + linesize * (y-1) + (x+4)) + (float) *(pImg->ImageData + linesize * (y+1) + (x-4)) + (float) *(pImg->ImageData + linesize * (y+1) + (x+4));

            uptr = pImg->ImageData + linesize * (y+4) + (x-4);
            for (i=0; i<9; i++, uptr++)
                D3 += *uptr;
            uptr = pImg->ImageData + linesize * (y+3) + (x-4);
            for (i=0; i<3; i++, uptr++)
                D3 += *uptr;
            uptr += 3;
            for (i=0; i<3; i++, uptr++)
                D3 += *uptr;

            double mean = (A + B1 + B2 + C1 + C2 + C3 + D1 + D2 + D3) / 81.0;
            double PSF_fit = PSF[0] * (A - mean) + PSF[1] * (B1 - 4.0 * mean) + PSF[2] * (B2 - 4.0 * mean) +
                PSF[3] * (C1 - 4.0 * mean) + PSF[4] * (C2 - 8.0 * mean) + PSF[5] * (C3 - 4.0 * mean) +
                PSF[6] * (D1 - 4.0 * mean) + PSF[7] * (D2 - 8.0 * mean) + PSF[8] * (D3 - 44.0 * mean);

            if (PSF_fit > BestPSF_fit)
            {
                BestPSF_fit = PSF_fit;
                xpos = x;
                ypos = y;
            }
        }
    }

    bool bFound = false;
    if (xpos != 0 && ypos != 0)
    {
        bFound = true;
        SetXY(xpos, ypos);
    }

    Debug.AddLine(wxString::Format("Autofind returns %d, xpos=%d, ypos=%d", bFound, xpos, ypos));

    return bFound;
}
