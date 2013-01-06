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

bool Star::Find(usImage *pImg, int base_x, int base_y)
{
    FindResult Result = STAR_OK;
    double X = base_x;
    double Y = base_y;

    try
    {
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
        int searchsize = SearchRegion * 2 + 1;

        // u-left corner of local area
        int start_x = base_x - SearchRegion; 
        int start_y = base_y - SearchRegion;

        int x, y;

        double mass=0, mx=0.0, my=0.0, val;
        unsigned long lval;
        unsigned sval;

        // make sure the star is not too near the edge
        if (start_x < 0 || start_x + SearchRegion >= pImg->Size.GetWidth() ||
            start_y < 0 || start_y + SearchRegion >= pImg->Size.GetHeight())
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
                    *(dataptr + (start_x + x+1) + rowsize * (start_y + y)) +		// find max of this smoothed area and set
                    *(dataptr + (start_x + x-1) + rowsize * (start_y + y)) +		// base_x and y to be this spot
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
        CropX = X - (CROPXSIZE/2);
        CropY = Y - (CROPYSIZE/2);
        if (CropX < 0) CropX = 0;
        else if ((CropX + CROPXSIZE) >= pImg->Size.GetWidth()) CropX = pImg->Size.GetWidth() - (CROPXSIZE + 1);
        if (CropY < 0) CropY = 0;
        else if ((CropY + CROPYSIZE) >= pImg->Size.GetHeight()) CropY = pImg->Size.GetHeight() - (CROPYSIZE + 1);

    }
    catch (char *ErrorMsg)
    {
        POSSIBLY_UNUSED(ErrorMsg);

        if (Result == STAR_OK)
        {
            Result = STAR_ERROR;
        }
    }

    // update state
    SetXY(X, Y);
    LastFindResult = Result;

    return WasFound(Result);
}

bool Star::Find(usImage *pImg)
{
    return Find(pImg, X, Y);
}
