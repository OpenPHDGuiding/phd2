#ifndef FILTER_FACTORY_H_INCLUDED
#define FILTER_FACTORY_H_INCLUDED
/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth, Bessel or Chebyshev filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

/*inline double asinh(double x)
  { // Microsoft C++ does not define
    return log(x + sqrt(1.0 + sqr(x)));
  }

inline double fix(double x)
  { // nearest integer 
    return (x >= 0.0) ? floor(0.5+x) : -floor(0.5-x);
  }
*/
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
    FilterFactory(std::string name, FILTER_DESIGN f, int o, double p, bool z );
private:
    const double  PI = 3.14159265358979323846;  // Microsoft C++ does not define M_PI ! 
    const double TWOPI = (2.0 * PI);
    const double EPS = 1e-10;
    FILTER_DESIGN filt;
    int order;
    double raw_alpha1, raw_alpha2, raw_alphaz;
    bool isMzt;

    std::complex<double> dc_gain, fc_gain, hf_gain;
    double warped_alpha1, warped_alpha2;
    double chripple;
    std::vector<std::complex<double>> bessel_poles;
    std::vector<std::complex<double>> spoles, szeros;
    std::vector<std::complex<double>> zpoles, zzeros;
    //    double qfactor;
    //    bool infq;
    //    uint polemask;

    void splane();
    void setpole(std::complex<double> );
    void prewarp();
    void normalize();
    void zplane();
    std::complex<double> bilinear(std::complex<double> );
    void expandpoly();
    void expand(std::vector<std::complex<double>>, std::vector<std::complex<double>>);
    void multin(std::complex<double>, std::vector<std::complex<double>>);
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


