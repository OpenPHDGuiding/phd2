/*
 *  image_math.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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
 *    Neither the name of Craig Stark, Stark Labs nor the names of its
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
#include "image_math.h"

int dbl_sort_func (double *first, double *second)
{
    if (*first < *second)
        return -1;
    else if (*first > *second)
        return 1;
    return 0;
}

float CalcSlope(const ArrayOfDbl& y)
{
    // Does a linear regression to calculate the slope

    int x, size;
    double s_xy, s_x, s_y, s_xx, nvalid;
    double retval;
    size = (int) y.GetCount();
    nvalid = 0.0;
    s_xy = 0.0;
    s_xx = 0.0;
    s_x = 0.0;
    s_y = 0.0;

    for (x=1; x<=size; x++) {
        if (1) {
            nvalid = nvalid + 1;
            s_xy = s_xy + (double) x * y[x-1];
            s_x = s_x + (double) x;
            s_y = s_y + y[x-1];
            s_xx = s_xx + (double) x * (double) x;
        }
    }

    retval = (nvalid * s_xy - (s_x * s_y)) / (nvalid * s_xx - (s_x * s_x));
    return (float) retval;
}

bool QuickLRecon(usImage& img)
{
    // Does a simple debayer of luminance data only -- sliding 2x2 window
    usImage Limg;
    int x, y;
    int xsize, ysize;

    xsize = img.Size.GetWidth();
    ysize = img.Size.GetHeight();
    if (Limg.Init(xsize,ysize)) {
        pFrame->Alert(_("Memory allocation error"));
        return true;
    }
    for (y=0; y<ysize-1; y++) {
        for (x=0; x<xsize-1; x++) {
            Limg.ImageData[x+y*xsize] = (img.ImageData[x+y*xsize] + img.ImageData[x+1+y*xsize] + img.ImageData[x+(y+1)*xsize] + img.ImageData[x+1+(y+1)*xsize]) / 4;
        }
        Limg.ImageData[x+y*xsize]=Limg.ImageData[(x-1)+y*xsize];  // Last one in this row -- just duplicate
    }
    for (x=0; x<xsize; x++)
        Limg.ImageData[x+(ysize-1)*xsize]=Limg.ImageData[x+(ysize-2)*xsize];  // Last row -- just duplicate

    img.SwapImageData(Limg);
    return false;
}

bool Median3(usImage& img)
{
    usImage tmp;
    tmp.Init(img.Size.GetWidth(), img.Size.GetHeight());
    bool err = Median3(tmp.ImageData, img.ImageData, img.Size.GetWidth(), img.Size.GetHeight());
    img.SwapImageData(tmp);
    return err;
}

inline static void swap(unsigned short& a, unsigned short& b)
{
    unsigned short const t = a;
    a = b;
    b = t;
}

inline static unsigned short median9(const unsigned short l[9])
{
    unsigned short l0 = l[0], l1 = l[1], l2 = l[2], l3 = l[3], l4 = l[4];
    unsigned short x;
    x = l[5];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    if (x < l3) swap(x, l3);
    if (x < l4) swap(x, l4);
    x = l[6];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    if (x < l3) swap(x, l3);
    if (x < l4) swap(x, l4);
    x = l[7];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    if (x < l3) swap(x, l3);
    if (x < l4) swap(x, l4);
    x = l[8];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    if (x < l3) swap(x, l3);
    if (x < l4) swap(x, l4);

    if (l1 > l0) l0 = l1;
    if (l2 > l0) l0 = l2;
    if (l3 > l0) l0 = l3;
    if (l4 > l0) l0 = l4;

    return l0;
}

inline static unsigned short median8(const unsigned short l[8])
{
    unsigned short l0 = l[0], l1 = l[1], l2 = l[2], l3 = l[3], l4 = l[4];
    unsigned short x;

    x = l[5];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    if (x < l3) swap(x, l3);
    if (x < l4) swap(x, l4);
    x = l[6];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    if (x < l3) swap(x, l3);
    if (x < l4) swap(x, l4);
    x = l[7];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    if (x < l3) swap(x, l3);
    if (x < l4) swap(x, l4);

    if (l2 > l0) swap(l2, l0);
    if (l2 > l1) swap(l2, l1);

    if (l3 > l0) swap(l3, l0);
    if (l3 > l1) swap(l3, l1);

    if (l4 > l0) swap(l4, l0);
    if (l4 > l1) swap(l4, l1);

    return (unsigned short)(((unsigned int) l0 + (unsigned int) l1) / 2);
}

inline static unsigned short median5(const unsigned short l[5])
{
    unsigned short l0 = l[0], l1 = l[1], l2 = l[2];
    unsigned short x;
    x = l[3];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);
    x = l[4];
    if (x < l0) swap(x, l0);
    if (x < l1) swap(x, l1);
    if (x < l2) swap(x, l2);

    if (l1 > l0) l0 = l1;
    if (l2 > l0) l0 = l2;

    return l0;
}

inline static unsigned short median3(const unsigned short l[3])
{
    unsigned short l0 = l[0], l1 = l[1], l2 = l[2];
    if (l2 < l0) swap(l2, l0);
    if (l2 < l1) swap(l2, l1);
    if (l1 > l0) l0 = l1;
    return l0;
}

bool Median3(unsigned short *dst, const unsigned short *src, int xsize, int ysize)
{
    int NPixels = xsize * ysize;

    for (int y = 1; y < ysize - 1; y++)
    {
        for (int x = 1; x < xsize - 1; x++)
        {
            unsigned short array[9];
            array[0] = src[(x - 1) + (y - 1)*xsize];
            array[1] = src[(x)+(y - 1)*xsize];
            array[2] = src[(x + 1) + (y - 1)*xsize];
            array[3] = src[(x - 1) + (y)*xsize];
            array[4] = src[(x)+(y)*xsize];
            array[5] = src[(x + 1) + (y)*xsize];
            array[6] = src[(x - 1) + (y + 1)*xsize];
            array[7] = src[(x)+(y + 1)*xsize];
            array[8] = src[(x + 1) + (y + 1)*xsize];
            dst[x + y * xsize] = median9(array);
        }
        dst[(xsize - 1) + y * xsize] = src[(xsize - 1) + y * xsize];  // 1st & Last one in this row -- just grab from orig
        dst[y * xsize] = src[y * xsize];
    }
    for (int x = 0; x < xsize; x++)
    {
        dst[x + (ysize - 1) * xsize] = src[x + (ysize - 1) * xsize];  // Last row -- just duplicate
        dst[x] = src[x];  // First row
    }

    return false;
}

bool SquarePixels(usImage& img, float xsize, float ysize)
{
    // Stretches one dimension to square up pixels
    int x,y;
    int newsize;
    usImage tempimg;
    unsigned short *ptr;
    unsigned short *optr;
    double ratio, oldposition;

    if (!img.ImageData)
        return true;
    if (xsize == ysize) return false;  // nothing to do

    // Copy the existing data
    if (tempimg.Init(img.Size.GetWidth(), img.Size.GetHeight())) {
        pFrame->Alert(_("Memory allocation error"));
        return true;
    }
    ptr = tempimg.ImageData;
    optr = img.ImageData;
    for (x=0; x<img.NPixels; x++, ptr++, optr++) {
        *ptr=*optr;
    }

    float weight;
    int ind1, ind2, linesize;
    // if X > Y, when viewing stock, Y is unnaturally stretched, so stretch X to match
    if (xsize > ysize) {
        ratio = ysize / xsize;
        newsize = ROUND((float) tempimg.Size.GetWidth() * (1.0/ratio));  // make new image correct size
        img.Init(newsize,tempimg.Size.GetHeight());
        optr=img.ImageData;
        linesize = tempimg.Size.GetWidth();  // size of an original line
        for (y=0; y<img.Size.GetHeight(); y++) {
            for (x=0; x<newsize; x++, optr++) {
                oldposition = x * ratio;
                ind1 = (unsigned int) floor(oldposition);
                ind2 = (unsigned int) ceil(oldposition);
                if (ind2 > (tempimg.Size.GetWidth() - 1)) ind2 = tempimg.Size.GetWidth() - 1;
                weight = ceil(oldposition) - oldposition;
                *optr = (unsigned short) (((float) *(tempimg.ImageData + y*linesize + ind1) * weight) + ((float) *(tempimg.ImageData + y*linesize + ind1) * (1.0 - weight)));
            }
        }

    }
    return false;
}

bool Subtract(usImage& light, const usImage& dark)
{
    if ((!light.ImageData) || (!dark.ImageData))
        return true;
    if (light.NPixels != dark.NPixels)
        return true;

    unsigned int left, top, width, height;
    if (light.Subframe.GetWidth() > 0 && light.Subframe.GetHeight() > 0)
    {
        left = light.Subframe.GetLeft();
        width = light.Subframe.GetWidth();
        top = light.Subframe.GetTop();
        height = light.Subframe.GetHeight();
    }
    else
    {
        left = top = 0;
        width = light.Size.GetWidth();
        height = light.Size.GetHeight();
    }

    int mindiff = 65535;

    unsigned short *pl0 = &light.Pixel(left, top);
    const unsigned short *pd0 = &dark.Pixel(left, top);
    for (unsigned int r = 0; r < height;
         r++, pl0 += light.Size.GetWidth(), pd0 += light.Size.GetWidth())
    {
        unsigned short *const endl = pl0 + width;
        unsigned short *pl;
        const unsigned short *pd;
        for (pl = pl0, pd = pd0; pl < endl; pl++, pd++)
        {
            int diff = (int) *pl - (int) *pd;
            if (diff < mindiff)
                mindiff = diff;
        }
    }

    int offset = 0;
    if (mindiff < 0) // dark was lighter than light
        offset = -mindiff;

    pl0 = &light.Pixel(left, top);
    pd0 = &dark.Pixel(left, top);
    for (unsigned int r = 0; r < height;
         r++, pl0 += light.Size.GetWidth(), pd0 += light.Size.GetWidth())
    {
        unsigned short *const endl = pl0 + width;
        unsigned short *pl;
        const unsigned short *pd;
        for (pl = pl0, pd = pd0; pl < endl; pl++, pd++)
        {
            int newval = (int) *pl - (int) *pd + offset;
            if (newval < 0) newval = 0; // shouldn't hit this...
            else if (newval > 65535) newval = 65535;
            *pl = (unsigned short) newval;
        }
    }

    return false;
}
