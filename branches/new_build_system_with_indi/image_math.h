/*
 *  image_math.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  Copyright (c) 2015 Andy Galasso
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

#ifndef IMAGE_MATH_INCLUDED
#define IMAGE_MATH_INCLUDED

class DefectMap : public std::vector<wxPoint>
{
    int m_profileId;
    DefectMap(int profileId);
public:
    static void DeleteDefectMap(int profileId);
    static bool DefectMapExists(int profileId, bool showAlert = true);
    static DefectMap *LoadDefectMap(int profileId);
    static wxString DefectMapFileName(int profileId);
    static bool ImportFromProfile(int sourceId, int destId);
    DefectMap();
    void Save(const wxArrayString& mapInfo) const;
    bool FindDefect(const wxPoint& pt) const;
    void AddDefect(const wxPoint& pt);

};

extern bool QuickLRecon(usImage& img);
extern bool Median3(unsigned short *dst, const unsigned short *src, const wxSize& size, const wxRect& rect);
extern bool Median3(usImage& img);
extern bool SquarePixels(usImage& img, float xsize, float ysize);
extern int dbl_sort_func(double *first, double *second);
extern bool Subtract(usImage& light, const usImage& dark);
extern double CalcSlope(const ArrayOfDbl& y);
extern bool RemoveDefects(usImage& light, const DefectMap& defectMap);

struct DefectMapBuilderImpl;

struct DefectMapDarks
{
    usImage masterDark;
    usImage filteredDark;

    void BuildFilteredDark();
    void SaveDarks(const wxString& notes);
    void LoadDarks();
};

struct ImageStats
{
    double mean;
    double stdev;
    unsigned short median;
    unsigned short mad;
};

class DefectMapBuilder
{
    DefectMapBuilderImpl *m_impl;

public:

    DefectMapBuilder();
    ~DefectMapBuilder();

    void Init(DefectMapDarks& darks);
    const ImageStats& GetImageStats() const;
    void SetAggressiveness(int aggrCold, int aggrHot);
    int GetColdPixelCnt() const;
    int GetHotPixelCnt() const;
    void BuildDefectMap(DefectMap& defectMap, bool verbose) const;
    const wxArrayString& GetMapInfo() const;
};

inline static double norm(double val, double start, double end)
{
    double const range = end - start;
    double const ofs = val - start;
    return val - floor(ofs / range) * range;
}

inline static double norm_angle(double val)
{
    return norm(val, -M_PI, M_PI);
}

inline static double degrees(double radians)
{
    return radians * 180. / M_PI;
}

inline static double radians(double degrees)
{
    return degrees * M_PI / 180.;
}

#endif
