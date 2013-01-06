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

#ifndef STAR_H_INCLUDED
#define STAR_H_INCLUDED

#include "point.h"

class Star:public Point
{

public:
    enum FindResult
    {
        STAR_OK=0,
        STAR_SATURATED,
        STAR_LOWSNR,
        STAR_LOWMASS,
        STAR_TOO_NEAR_EDGE,
        STAR_LARGEMOTION,
        STAR_ERROR,
    };

    double Mass;
    double SNR;
    FindResult LastFindResult;

    Star(void)
    {
        Invalidate();
    }

    ~Star()
    {
    }

    bool Find(usImage *pImg);
    bool Find(usImage *pImg, int X, int Y);

    bool WasFound(FindResult result)
    {
        bool bReturn = false;

        if (result == STAR_OK || result == STAR_SATURATED)
        {
            bReturn = true;
        }

        return bReturn;
    }

    bool WasFound(void)
    {
        return WasFound(LastFindResult);
    }

    virtual void Invalidate(void)
    {
        Mass = 0.0;
        SNR = 0.0;
        LastFindResult = STAR_ERROR;
        Point::Invalidate();
    }

    void SetError(FindResult error=STAR_ERROR)
    {
        LastFindResult = error;
    }
};

#endif /* STAR_H_INCLUDED */
