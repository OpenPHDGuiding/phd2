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

void Star::SetError(FindResult error=STAR_ERROR)
{
    m_lastFindResult = error;
}

bool Star::Find(usImage *pImg, int searchRegion, int base_x, int base_y)
{
    FindResult Result = STAR_OK;
    double X = base_x;
    double Y = base_y;

    try
    {
        Debug.AddLine(wxString::Format("Star::Find(0x%p, %d, %d, %d)", pImg, searchRegion, base_x, base_y));

        if (base_x < 0 || base_y < 0)
        {
            throw ERROR_INFO("cooridnates are invalid");
        }

        // ADD BIT ABOUT USING PREVIOUS DELTA TO FIGURE NEW STARTING SPOT TO LOOK - -maybe have this be an option

        unsigned short max=0, nearmax1=0, nearmax2=0, localmin = 65535;
        unsigned long maxlval=0, mean=0;
        double localmean = 0.0;

        unsigned short *dataptr = pImg->ImageData;
        int rowsize = pImg->Size.GetWidth();
        int searchsize = searchRegion * 2 + 1;

        // u-left corner of local area
        int start_x = base_x - searchRegion;
        int start_y = base_y - searchRegion;

        int x, y;

        double mass=0, mx=0.0, my=0.0, val;
        unsigned long lval;
        unsigned sval;

        // make sure the star is not too near the edge
        if (start_x < 0 || start_x + searchRegion >= pImg->Size.GetWidth() ||
            start_y < 0 || start_y + searchRegion >= pImg->Size.GetHeight())
        {
            Result = STAR_TOO_NEAR_EDGE;
            throw ERROR_INFO("Star too near edge");
        }

        if (start_y == 0)
            start_y = 1;

        // compute localmin and localmean, which we need to find the star
        for (y=0; y<searchsize; y++) {
            for (x=0; x<searchsize; x++) {
                if (*(dataptr + (start_x + x) + rowsize * (start_y + y-1)) < localmin)
                    localmin = *(dataptr + (start_x + x) + rowsize * (start_y + y-1));
                localmean = localmean + (double)  *(dataptr + (start_x + x) + rowsize * (start_y + y-1));

            }
        }
        localmean = localmean / (double) (searchsize * searchsize);

        // get rough guess on star's location
        for (y=0; y<searchsize; y++) {
            for (x=0; x<searchsize; x++) {
                lval = *(dataptr + (start_x + x) + rowsize * (start_y + y)) +  // combine adjacent pixels to smooth image
                    *(dataptr + (start_x + x+1) + rowsize * (start_y + y)) +        // find max of this smoothed area and set
                    *(dataptr + (start_x + x-1) + rowsize * (start_y + y)) +        // base_x and y to be this spot
                    *(dataptr + (start_x + x) + rowsize * (start_y + y+1)) +
                    *(dataptr + (start_x + x) + rowsize * (start_y + y-1)) +
                    *(dataptr + (start_x + x) + rowsize * (start_y + y));  // weigh current pixel by 2x
                if (lval >= maxlval) {
                    base_x = start_x + x;
                    base_y = start_y + y;
                    maxlval = lval;
                }
                sval = *(dataptr + (start_x + x) + rowsize * (start_y + y)) -localmin;
                if ( sval >= max) {
                    nearmax2 = nearmax1;
                    nearmax1 = max;
                    max = sval;
                }
                mean = mean + sval;
            }
        }
        mean = mean / (searchsize * searchsize);

        // should be close now, hone in
        int ft_range = 15; // must be odd
        int hft_range = ft_range / 2;

        // we try these thresholds in this order trying to get a mass >= 10
        double thresholds[] =
        {
            localmean + ((double) max + localmin - localmean) / 10.0,  // Note: max already has localmin pulled from it
            localmean,
            localmin
        };

        for(int i=0; i < sizeof(thresholds)/sizeof(thresholds[0]) && mass < 10.0; i++)
        {
            mass = mx = my = 0.000001;
            double threshold = thresholds[i];
            for (y=0; y<ft_range; y++) {
                for (x=0; x<ft_range; x++) {
                    val = (double) *(dataptr + (base_x + (x-hft_range)) + rowsize*(base_y + (y-hft_range))) - threshold;
                    if (val < 0.0)
                        val=0.0;
                    mx = mx + (double) (base_x + x-hft_range) * val;
                    my = my + (double) (base_y + y-hft_range) * val;
                    mass = mass + val;
                }
            }
        }

        SNR = (double) max / (double) mean;
        Mass = mass;

        if (mass < 10.0) {
            Result = STAR_LOWMASS;
        }
        else if (SNR < 3.0) {
            Result = STAR_LOWSNR;
        }
        else {
            X = mx / mass;
            Y = my / mass;
            if (max == nearmax2) {
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
    SetXY(X, Y);
    m_lastFindResult = Result;

    bool bReturn = WasFound(Result);

    if (!bReturn)
    {
        Mass = 0.0;
        SNR = 0.0;
    }

    Debug.AddLine(wxString::Format("Star::Find returns %d, X=%lf, Y=%lf", bReturn, X, Y));

    return bReturn;
}

bool Star::Find(usImage *pImg, int searchRegion)
{
    return Find(pImg, searchRegion, X, Y);
}

bool Star::AutoFind(usImage *pImg)
{
    bool bFound = false;
    int xpos=0, ypos=0;

    float A, B1, B2, C1, C2, C3, D1, D2, D3;
    int x, y, i, linesize;
    unsigned short *uptr;

    linesize = pImg->Size.GetWidth();
    double PSF[14] = { 0.906, 0.584, 0.365, .117, .049, -0.05, -.064, -.074, -.094 };
    double mean;
    double PSF_fit;
    double BestPSF_fit = 0.0;

    // OK, do seem to need to run 3x3 median first
    Median3(*pImg);

    /* PSF Grid is:
        D3 D3 D3 D3 D3 D3 D3 D3 D3
        D3 D3 D3 D2 D1 D2 D3 D3 D3
        D3 D3 C3 C2 C1 C2 C3 D3 D3
        D3 D2 C2 B2 B1 B2 C2 D3 D3
        D3 D1 C1 B1 A  B1 C1 D1 D3
        D3 D2 C2 B2 B1 B2 C2 D3 D3
        D3 D3 C3 C2 C1 C2 C3 D3 D3
        D3 D3 D3 D2 D1 D2 D3 D3 D3
        D3 D3 D3 D3 D3 D3 D3 D3 D3

        1@A
        4@B1, B2, C1, and C3
        8@C2, D2
        48 * D3
        */
    for (y=40; y<(pImg->Size.GetHeight()-40); y++)
    {
        for (x=40; x<(linesize-40); x++)
        {
            A =  (float) *(pImg->ImageData + linesize * y + x);
            B1 = (float) *(pImg->ImageData + linesize * (y-1) + x) + (float) *(pImg->ImageData + linesize * (y+1) + x) + (float) *(pImg->ImageData + linesize * y + (x + 1)) + (float) *(pImg->ImageData + linesize * y + (x-1));
            B2 = (float) *(pImg->ImageData + linesize * (y-1) + (x-1)) + (float) *(pImg->ImageData + linesize * (y-1) + (x+1)) + (float) *(pImg->ImageData + linesize * (y+1) + (x + 1)) + (float) *(pImg->ImageData + linesize * (y+1) + (x-1));
            C1 = (float) *(pImg->ImageData + linesize * (y-2) + x) + (float) *(pImg->ImageData + linesize * (y+2) + x) + (float) *(pImg->ImageData + linesize * y + (x + 2)) + (float) *(pImg->ImageData + linesize * y + (x-2));
            C2 = (float) *(pImg->ImageData + linesize * (y-2) + (x-1)) + (float) *(pImg->ImageData + linesize * (y-2) + (x+1)) + (float) *(pImg->ImageData + linesize * (y+2) + (x + 1)) + (float) *(pImg->ImageData + linesize * (y+2) + (x-1)) +
                (float) *(pImg->ImageData + linesize * (y-1) + (x-2)) + (float) *(pImg->ImageData + linesize * (y-1) + (x+2)) + (float) *(pImg->ImageData + linesize * (y+1) + (x + 2)) + (float) *(pImg->ImageData + linesize * (y+1) + (x-2));
            C3 = (float) *(pImg->ImageData + linesize * (y-2) + (x-2)) + (float) *(pImg->ImageData + linesize * (y-2) + (x+2)) + (float) *(pImg->ImageData + linesize * (y+2) + (x + 2)) + (float) *(pImg->ImageData + linesize * (y+2) + (x-2));
            D1 = (float) *(pImg->ImageData + linesize * (y-3) + x) + (float) *(pImg->ImageData + linesize * (y+3) + x) + (float) *(pImg->ImageData + linesize * y + (x + 3)) + (float) *(pImg->ImageData + linesize * y + (x-3));
            D2 = (float) *(pImg->ImageData + linesize * (y-3) + (x-1)) + (float) *(pImg->ImageData + linesize * (y-3) + (x+1)) + (float) *(pImg->ImageData + linesize * (y+3) + (x + 1)) + (float) *(pImg->ImageData + linesize * (y+3) + (x-1)) +
                (float) *(pImg->ImageData + linesize * (y-1) + (x-3)) + (float) *(pImg->ImageData + linesize * (y-1) + (x+3)) + (float) *(pImg->ImageData + linesize * (y+1) + (x + 3)) + (float) *(pImg->ImageData + linesize * (y+1) + (x-3));
            D3 = 0.0;
            uptr = pImg->ImageData + linesize * (y-4) + (x-4);
            for (i=0; i<9; i++, uptr++)
                D3 = D3 + *uptr;
            uptr = pImg->ImageData + linesize * (y-3) + (x-4);
            for (i=0; i<3; i++, uptr++)
                D3 = D3 + *uptr;
            uptr = uptr + 2;
            for (i=0; i<3; i++, uptr++)
                D3 = D3 + *uptr;
            D3 = D3 + (float) *(pImg->ImageData + linesize * (y-2) + (x-4)) + (float) *(pImg->ImageData + linesize * (y-2) + (x+4)) + (float) *(pImg->ImageData + linesize * (y-2) + (x-3)) + (float) *(pImg->ImageData + linesize * (y-2) + (x-3)) +
                (float) *(pImg->ImageData + linesize * (y+2) + (x-4)) + (float) *(pImg->ImageData + linesize * (y+2) + (x+4)) + (float) *(pImg->ImageData + linesize * (y+2) + (x - 3)) + (float) *(pImg->ImageData + linesize * (y+2) + (x-3)) +
                (float) *(pImg->ImageData + linesize * y + (x + 4)) + (float) *(pImg->ImageData + linesize * y + (x-4));

            uptr = pImg->ImageData + linesize * (y+4) + (x-4);
            for (i=0; i<9; i++, uptr++)
                D3 = D3 + *uptr;
            uptr = pImg->ImageData + linesize * (y+3) + (x-4);
            for (i=0; i<3; i++, uptr++)
                D3 = D3 + *uptr;
            uptr = uptr + 2;
            for (i=0; i<3; i++, uptr++)
                D3 = D3 + *uptr;

            mean = (A+B1+B2+C1+C2+C3+D1+D2+D3)/85.0;
            PSF_fit = PSF[0] * (A-mean) + PSF[1] * (B1 - 4.0*mean) + PSF[2] * (B2 - 4.0 * mean) +
                PSF[3] * (C1 - 4.0*mean) + PSF[4] * (C2 - 8.0*mean) + PSF[5] * (C3 - 4.0 * mean) +
                PSF[6] * (D1 - 4.0*mean) + PSF[7] * (D2 - 8.0*mean) + PSF[8] * (D3 - 48.0 * mean);


            if (PSF_fit > BestPSF_fit)
            {
                BestPSF_fit = PSF_fit;
                xpos = x;
                ypos = y;
            }
        }
    }

    if (xpos != 0 && ypos != 0)
    {
        bFound = true;
        SetXY(xpos, ypos);
    }

    Debug.AddLine(wxString::Format("Autofind returns %d, xpos=%d, ypos=%d", bFound, xpos, ypos));

    return bFound;
}
