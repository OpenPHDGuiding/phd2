/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth, Bessel or Chebyshev filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "filterfactory.h"

FilterFactory::FilterFactory(std:string name, FILTER_DESIGN f, int o, double p, bool z=false )
{
    bessel_poles = {
    /* table produced by /usr/fisher/bessel --  N.B. only one member of each C.Conj. pair is listed */
        std::complex<double>( -1.00000000000e+00, 0.00000000000e+00),
        std::complex<double>( -1.10160133059e+00, 6.36009824757e-01),
        std::complex<double>( -1.32267579991e+00, 0.00000000000e+00),
        std::complex<double>( -1.04740916101e+00, 9.99264436281e-01),
        std::complex<double>( -1.37006783055e+00, 4.10249717494e-01),
        std::complex<double>( -9.95208764350e-01, 1.25710573945e+00),
        std::complex<double>( -1.50231627145e+00, 0.00000000000e+00),
        std::complex<double>( -1.38087732586e+00, 7.17909587627e-01),
        std::complex<double>( -9.57676548563e-01, 1.47112432073e+00),
        std::complex<double>( -1.57149040362e+00, 3.20896374221e-01),
        std::complex<double>( -1.38185809760e+00, 9.71471890712e-01),
        std::complex<double>( -9.30656522947e-01, 1.66186326894e+00),
        std::complex<double>( -1.68436817927e+00, 0.00000000000e+00),
        std::complex<double>( -1.61203876622e+00, 5.89244506931e-01),
        std::complex<double>( -1.37890321680e+00, 1.19156677780e+00),
        std::complex<double>( -9.09867780623e-01, 1.83645135304e+00),
        std::complex<double>( -1.75740840040e+00, 2.72867575103e-01),
        std::complex<double>( -1.63693941813e+00, 8.22795625139e-01),
        std::complex<double>( -1.37384121764e+00, 1.38835657588e+00),
        std::complex<double>( -8.92869718847e-01, 1.99832584364e+00),
        std::complex<double>( -1.85660050123e+00, 0.00000000000e+00),
        std::complex<double>( -1.80717053496e+00, 5.12383730575e-01),
        std::complex<double>( -1.65239648458e+00, 1.03138956698e+00),
        std::complex<double>( -1.36758830979e+00, 1.56773371224e+00),
        std::complex<double>( -8.78399276161e-01, 2.14980052431e+00),
        std::complex<double>( -1.92761969145e+00, 2.41623471082e-01),
        std::complex<double>( -1.84219624443e+00, 7.27257597722e-01),
        std::complex<double>( -1.66181024140e+00, 1.22110021857e+00),
        std::complex<double>( -1.36069227838e+00, 1.73350574267e+00),
        std::complex<double>( -8.65756901707e-01, 2.29260483098e+00),
    };
    assert(o > 0);
    assert(p > 2.0);
    order = o;
    isMzt = z;
    polemask = ~0;                  // use all poles
    raw_alpha2 = raw_alpha1 = 1.0/p;
    spoles.clear();
    szeros.clear();

    splane();
    prewarp();
    normalize();
    zplane(!options & opt_z);
    expandpoly();
}
bool 

void FilterFactory::splane() // compute S-plane poles for prototype LP filter
{
// Bessel filter
// 
    if (filt == BESSEL)                           // Bessel filter
    {
        int p = (order * order) / 4;                // ptr into table
        if (order & 1)                              // If order is odd
            setpole(bessel_poles[p++]);
        for (int i = 0; i < order / 2; i++)
        {
            setpole(bessel_poles[p]);
            setpole(bessel_poles[p].conj());
            p++;
        }
    }
    if (file == BUTTERWORTH || filt == CHEBYCHEV)                // Butterworth filter
    {
        for (int i = 0; i < 2 * order; i++) 
        {
            double theta = (order & 1) ? (i * PI) / order : ((i + 0.5) * PI) / order;
            setpole(std::polar(1.0,theta));
        }
    }
    if (filt == CHEBYCHEV)                           // modify for Chebyshev (p. 136 DeFatta et al.) 
    { 
        if (chripple >= 0.0) 
        {
            fprintf(stderr, "mkfilter: Chebyshev ripple is %g dB; must be .lt. 0.0\n", chebrip);
            exit(1);        
        }
        double rip = pow(10.0, -chripple / 10.0);
        double eps = sqrt(rip - 1.0);
        double y = asinh(1.0 / eps) / (double) order;
        for (int i = 0; i < spoles.size(); i++) 
        {
            spoles[i].real *= sinh(y);
            spoles[i].imag *= cosh(y);
        }
    }
}

void FilterFactory::prewarp()
{
    /* for bilinear transform, perform pre-warp on alpha values */
    if (isMzt) 
    { //**Dont prewarp; or use z-transform
        warped_alpha1 = raw_alpha1;
        warped_alpha2 = raw_alpha2;
    }
    else 
    {
        warped_alpha1 = tan(PI * raw_alpha1) / PI;
        warped_alpha2 = tan(PI * raw_alpha2) / PI;
    }
}

void FilterFactory::normalize_lp() /* called for trad, not for -Re or -Pi */
{
    double w1 = TWOPI * warped_alpha1;
    for (int i = 0; i < spoles.size(); i++)
        spoles[i] = spoles[i] * w1;
    szeros.clear();
}

void FilterFactory::zplane() 
{
// given S-plane poles & zeros, compute Z-plane poles & zeros
// using bilinear transform or matched z-transform
    int i;
    zpoles.clear();
    zzeros.clear();
    if(isMzt)
    {
        for (i = 0; i < spoles.size(); i++)
            zpoles.push_back(bilinear(spoles[i]));
        for (i = 0; i < szeros.size(); i++)
            zzeros.push_back(bilinear(szeros[i]));
        while (zzeros.size() < zpoles.size())
            zzeros.push_back(-1.0);
    }
    else
    {
        for (i = 0; i < spoles.size(); i++)
            zpoles.push_back(spoles[i].exp());
        for (i = 0; i < szeros.size(); i++)
            zzeros.push_back(szeros[i].exp());
    }
}

void FilterFactory::expandpoly() /* given Z-plane poles & zeros, compute top & bot polynomials in Z, and then recurrence relation */
{
    std::vector<std::complex<double>> topcoeffs, botcoeffs;
    int i;
    expand(zzeros, topcoeffs);
    expand(zpoles, botcoeffs);
    dc_gain = eval(topcoeffs, 1.0)/eval(botcoeffs, 1.0);
    double theta = TWOPI * 0.5 * (raw_alpha1 + raw_alpha2); /* "jwT" for centre freq. */

    fc_gain = eval(topcoeffs, std::polar(1.0, theta))/eval(botcoeffs, std::polar(1.0, theta));
    hf_gain = eval(topcoeffs, -1.0)/eval(botcoeffs, -1.0);
    for (i = 0; i <= zzeros.size(); i++)
        xcoeffs[i] = +(topcoeffs[i].real / botcoeffs.back().real);
    for (i = 0; i <= botcoeffs.size(); i++)
        ycoeffs[i] = -(botcoeffs[i].real / botcoeffs.back().real);
}

void FilterFactory::expand(std::vector<std::complex<double>> pz, std::vector<std::complex<double>> coeffs)
{
    /* compute product of poles or zeros as a polynomial of z */
    int i;
    coeffs.clear();
    coeffs.push_back(1.0);
    for (i = 0; i < pz.size(); i++)
        coeffs.push_back(0.0);
    for (i = 0; i < pz.size(); i++)
        multin(pz[i], coeffs);
    /* check computed coeffs of z^k are all real */
    for (i = 0; i < pz.size() + 1; i++)
    {
        if (fabs(coeffs[i].imag) > EPS)
        {
            fprintf(stderr, "mkfilter: coeff of z^%d is not real; poles/zeros are not complex conjugates\n", i);
            exit(1);
        }
    }
}

void FilterFactory::multin(std::complex<double> w, std::vector<std::complex<double>> coeffs)
{
    /* multiply factor (z-w) into coeffs */
    std::complex<double> nw = -w;
    for (int i = coeffs.size()-1; i >= 1; i--)
        coeffs[i] = (nw * coeffs[i]) + coeffs[i - 1];
    coeffs[0] = nw * coeffs[0];
}

std::complex<double> FilterFactory::eval(std::vector<std::complex<double>> coeffs, std::complex<double> z)
  { /* evaluate polynomial in z, substituting for z */
    std::complex<double> sum = complex(0.0,0.0);
    for (int i = coeffs.size()-1; i >= 0; i--) 
        sum = (sum * z) + coeffs[i];
    return sum;
  }

//*************************************************************************************************************************
/*
static void printfilter() {
    printf("raw alpha1    = %14.10f\n", raw_alpha1);
    printf("raw alpha2    = %14.10f\n", raw_alpha2);

    if(!(options & (opt_re | opt_w | opt_z)) {
        printf("warped alpha1 = %14.10f\n", warped_alpha1);
        printf("warped alpha2 = %14.10f\n", warped_alpha2);
    }
    printgain("dc    ", dc_gain);
    printrecurrence();
}

static void printgain(char *str, complex gain) {
    double r = hypot(gain);
    printf("gain at %s:   mag = %15.9e", str, r);
    if (r > EPS) printf("   phase = %14.10f pi", atan2(gain) / PI);
    putchar('\n');
}

static void printrecurrence() // given (real) Z-plane poles & zeros, compute & print recurrence relation 
{
    printf("Recurrence relation:\n");
    printf("y[n] = ");
    int i;
    for (i = 0; i < zplane.numzeros + 1; i++) {
        if (i > 0) printf("     + ");
        double x = xcoeffs[i];
        double f = fmod(fabs(x), 1.0);
        char *fmt = (f < EPS || f > 1.0 - EPS) ? "%3g" : "%14.10f";
        putchar('(');
        printf(fmt, x);
        printf(" * x[n-%2d])\n", zplane.numzeros - i);
    }
    putchar('\n');
    for (i = 0; i < zplane.numpoles; i++) {
        printf("     + (%14.10f * y[n-%2d])\n", ycoeffs[i], zplane.numpoles - i);
    }
    putchar('\n');
}
*/
