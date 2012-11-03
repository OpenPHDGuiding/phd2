/*
 *  point.h
 *  PHD Guiding
 *
 *  Created by Bret McKee
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

#ifndef POINT_H_INCLUDED
#define POINT_H_INCLUDED

#include <math.h>

class Point
{
public:
    const double X;
    const double Y;

    Point(const double x, const double y):
        X(x), Y(y)
    {
    }

    Point(Point const * const p):
        X(p->X), Y(p->Y)
    {
    }

    Point(const Point &p):
        X(p.X), Y(p.Y)
    {
    }

    Point(void):
        X(0), Y(0)
    {
    }

    double dx(Point p)
    {
        double dRet = X-p.X;

        return dRet;
    }

    double dx(Point *pPoint)
    {
        return dx(*pPoint);
    }

    double dy(Point p)
    {
        double dRet = Y-p.Y;

        return dRet;
    }

    double dy(Point *pPoint)
    {
        return dy(*pPoint);
    }

    double Distance(Point p)
    {
        double dX = dx(p);
        double dY = dy(p);
        double dRet = sqrt(dX*dX + dY*dY);

        return dRet;
    }

    double Distance(Point *pPoint)
    {
        return Distance(*pPoint);
    }

    double Angle(Point p)
    {
        double dX = X-p.X;
        double dY = Y-p.Y;
        double dRet = 0.0;

        // man pages vary on whether atan2 deals well with dx == 0 && dy == 0,
        // so I handle it explictly
        if (dX != 0 || dY != 0)
        {
            dRet = atan2(dY, dX);
        }

        return dRet;
    }

    double Angle(Point *pPoint)
    {
        return Angle(*pPoint);
    }
};

class LineSegment
{
public:
    Point start, end;

    LineSegment(Point p1, Point p2):
        start(p1), end(p2)
    {
    }
};

#endif /* POINT_H_INCLUDED */
