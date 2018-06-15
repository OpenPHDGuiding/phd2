/*
*  guide_algorithm_butterworth.h
*  PHD Guiding
*
*  Created by Ken Self
*  Copyright (c) 2018 Ken Self
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
*    Neither the name of openphdguiding.org nor the names of its
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

#ifndef FILTER_FACTORY_H_INCLUDED
#define FILTER_FACTORY_H_INCLUDED
#include <complex>

enum FILTER_DESIGN
{
    BESSEL,
    BUTTERWORTH,
    CHEBYCHEV,    
};

class FilterFactory
{
public:
    std::vector<double> xcoeffs, ycoeffs;
    double gain() { return ::hypot(dc_gain.imag(), dc_gain.real()); };
    double corner() { return 1 / raw_alpha1; };
    int order() { return m_order;  };
    FilterFactory( FILTER_DESIGN f, int o, double p );
private:
    const double  PI = 3.14159265358979323846;  // Microsoft C++ does not define M_PI ! 
    const double TWOPI = (2.0 * PI);
    const double EPS = 1e-10;
    FILTER_DESIGN filt;
    int m_order;
    double raw_alpha1, raw_alpha2, raw_alphaz;
    bool isMzt;

    std::complex<double> dc_gain, fc_gain, hf_gain;
    double warped_alpha1, warped_alpha2;
    double chripple;
    std::vector<std::complex<double>> bessel_poles;
    std::vector<std::complex<double>> spoles, szeros;
    std::vector<std::complex<double>> zpoles, zzeros;

    void splane();
    void setpole(std::complex<double>);
    void prewarp();
    void normalize();
    void zplane();
    std::complex<double> bilinear(std::complex<double>);
    void expandpoly();
    void expand(std::vector<std::complex<double>>, std::vector<std::complex<double>>&);
    void multin(std::complex<double>, std::vector<std::complex<double>>&);
    std::complex<double> eval(std::vector<std::complex<double>> coeffs, std::complex<double> z);
};

inline void FilterFactory::setpole(std::complex<double> z)
{
    if (z.real() < 0.0)
    {
//        if (polemask & 1)
            spoles.push_back(z);
//        polemask >>= 1;
    }
}
inline std::complex<double> FilterFactory::bilinear(std::complex<double> pz)
{
    return (2.0 + pz) / (2.0 - pz);
}

#endif /* FILTER_FACTORY_H_INCLUDED */


