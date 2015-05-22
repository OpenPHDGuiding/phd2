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

class Star : public PHD_Point
{
public:
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

    Star(void);
    ~Star();

    /*
     * Note: contrary to most boolean PHD functions, the star find functions return
     *       a boolean indicating success instead of a boolean indicating an
     *       error
     */
    bool Find(const usImage *pImg, int searchRegion, FindMode mode);
    bool Find(const usImage *pImg, int searchRegion, int X, int Y, FindMode mode);
    bool AutoFind(const usImage& image, int edgeAllowance, int searchRegion);

    bool WasFound(FindResult result);
    bool WasFound(void);
    void Invalidate(void);
    void SetError(FindResult error);
    FindResult GetError(void) const;
private:
    FindResult m_lastFindResult;
};

inline Star::FindResult Star::GetError(void) const
{
    return m_lastFindResult;
}

#endif /* STAR_H_INCLUDED */
