/*
 *  cam_simulator.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Reimplemented for PHD2 by Andy Galasso.
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

#ifdef SIMULATOR
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "cam_simulator.h"

#include <wx/gdicmn.h>
#include <wx/stopwatch.h>
#include <wx/radiobut.h>

#define SIMMODE 3   // 1=FITS, 2=BMP, 3=Generate
// #define SIMDEBUG

/* simulation parameters for SIMMODE = 3*/

struct SimCamParams
{
    static unsigned int width;
    static unsigned int height;
    static unsigned int border;
    static unsigned int nr_stars;
    static unsigned int nr_hot_pixels;
    static double noise_multiplier;
    static double dec_backlash;
    static double pe_scale;
    static double dec_drift_rate;
    static double seeing_scale;
    static double cam_angle;
    static double guide_rate;
    static PierSide pier_side;
    static bool reverse_dec_pulse_on_west_side;
    static unsigned int clouds_inten;
    static double inverse_imagescale;
    static bool use_pe;
    static bool use_default_pe_params;
    static double custom_pe_amp;
    static double custom_pe_period;
};

unsigned int SimCamParams::width = 752;          // simulated camera image width
unsigned int SimCamParams::height = 580;         // simulated camera image height
unsigned int SimCamParams::border = 12;          // do not place any stars within this size border
unsigned int SimCamParams::nr_stars;             // number of stars to generate
unsigned int SimCamParams::nr_hot_pixels;        // number of hot pixels to generate
double SimCamParams::noise_multiplier;           // noise factor, increase to increase noise
double SimCamParams::dec_backlash;               // dec backlash amount (pixels)
double SimCamParams::pe_scale;                   // scale factor controlling magnitude of simulated periodic error
double SimCamParams::dec_drift_rate;             // dec drift rate (pixels per second)
double SimCamParams::seeing_scale;               // simulated seeing scale factor
double SimCamParams::cam_angle;                  // simulated camera angle (degrees)
double SimCamParams::guide_rate;                 // guide rate, pixels per second
PierSide SimCamParams::pier_side;                // side of pier
bool SimCamParams::reverse_dec_pulse_on_west_side; // reverse dec pulse on west side of pier, like ASCOM pulse guided equatorial mounts
unsigned int SimCamParams::clouds_inten;          // clouds intensity blocking out stars
double SimCamParams::inverse_imagescale;         // pixel per arc-sec
bool SimCamParams::use_pe;
bool SimCamParams::use_default_pe_params;
double SimCamParams::custom_pe_amp;
double SimCamParams::custom_pe_period;
// Note: these are all in units appropriate for the UI
#define NR_STARS_DEFAULT 20
#define NR_HOT_PIXELS_DEFAULT 8
#define NOISE_DEFAULT 2.0
#define NOISE_MAX 5.0
#define DEC_BACKLASH_DEFAULT 1.0                  // arc-sec
#define DEC_BACKLASH_MAX 10.0
#define DEC_DRIFT_DEFAULT 3.0                     // arc-sec per minute
#define DEC_DRIFT_MAX 10.0
#define SEEING_DEFAULT 2.0                        // arc-sec FWHM
#define SEEING_MAX 4.0
#define CAM_ANGLE_DEFAULT 15.0
#define GUIDE_RATE_DEFAULT (0.5 * 15.0)           // multiples of sidereal rate, a-s/sec
#define GUIDE_RATE_MAX (1.0 * 15.0)
#define PIER_SIDE_DEFAULT PIER_SIDE_EAST
#define REVERSE_DEC_PULSE_ON_WEST_SIDE_DEFAULT true
#define CLOUDS_INTEN_DEFAULT 10
#define USE_PE_DEFAULT true
#define PE_SLIDER_DEFAULT 1.0
#define USE_PE_DEFAULT_PARAMS true
#define PE_CUSTOM_AMP_DEFAULT 2.0               // Give them a trivial 2 a-s 4 min smooth curve
#define PE_CUSTOM_PERIOD_DEFAULT 240.0

static void load_sim_params()
{
    SimCamParams::nr_stars = pConfig->Profile.GetInt("/SimCam/nr_stars", NR_STARS_DEFAULT);
    SimCamParams::nr_hot_pixels = pConfig->Profile.GetInt("/SimCam/nr_hot_pixels", NR_HOT_PIXELS_DEFAULT);
    SimCamParams::noise_multiplier = pConfig->Profile.GetDouble("/SimCam/noise", NOISE_DEFAULT);
    SimCamParams::use_pe = (pConfig->Profile.GetBoolean ("/SimCam/use_pe", USE_PE_DEFAULT));
    SimCamParams::pe_scale = wxMin(1.0, (pConfig->Profile.GetDouble("/SimCam/pe_scale", PE_SLIDER_DEFAULT)));
    SimCamParams::use_default_pe_params = (pConfig->Profile.GetBoolean("/SimCam/use_default_pe", USE_PE_DEFAULT_PARAMS));
    SimCamParams::custom_pe_amp = (pConfig->Profile.GetDouble("/SimCam/pe_cust_amp", PE_CUSTOM_AMP_DEFAULT));
    SimCamParams::custom_pe_period = (pConfig->Profile.GetDouble("/SimCam/pe_cust_period", PE_CUSTOM_PERIOD_DEFAULT));
    SimCamParams::dec_drift_rate = (pConfig->Profile.GetDouble("/SimCam/dec_drift", DEC_DRIFT_DEFAULT)*SimCamParams::inverse_imagescale / 60.0);  //a-s per min is saved
    SimCamParams::seeing_scale = pConfig->Profile.GetDouble("/SimCam/seeing_scale", SEEING_DEFAULT);        // FWHM a-s
    SimCamParams::cam_angle = pConfig->Profile.GetDouble("/SimCam/cam_angle", CAM_ANGLE_DEFAULT);
    SimCamParams::guide_rate = pConfig->Profile.GetDouble("/SimCam/guide_rate", GUIDE_RATE_DEFAULT);
    // backlash is arc-secs in UI - map to px for internal use
    SimCamParams::dec_backlash = pConfig->Profile.GetDouble("/SimCam/dec_backlash", DEC_BACKLASH_DEFAULT) * SimCamParams::inverse_imagescale;
    SimCamParams::pier_side = (PierSide) pConfig->Profile.GetInt("/SimCam/pier_side", PIER_SIDE_DEFAULT);
    SimCamParams::reverse_dec_pulse_on_west_side = pConfig->Profile.GetBoolean("/SimCam/reverse_dec_pulse_on_west_side", REVERSE_DEC_PULSE_ON_WEST_SIDE_DEFAULT);
}

static void save_sim_params()
{
    pConfig->Profile.SetInt("/SimCam/nr_stars", SimCamParams::nr_stars);
    pConfig->Profile.SetInt("/SimCam/nr_hot_pixels", SimCamParams::nr_hot_pixels);
    pConfig->Profile.SetDouble("/SimCam/noise", SimCamParams::noise_multiplier);
    pConfig->Profile.SetDouble("/SimCam/dec_backlash", SimCamParams::dec_backlash / (SimCamParams::inverse_imagescale));
    pConfig->Profile.SetBoolean ("/SimCam/use_pe", SimCamParams::use_pe);
    pConfig->Profile.SetBoolean("/SimCam/use_default_pe", SimCamParams::use_default_pe_params);
    pConfig->Profile.SetDouble("/SimCam/pe_scale", SimCamParams::pe_scale);
    pConfig->Profile.SetDouble("/SimCam/pe_cust_amp", SimCamParams::custom_pe_amp);
    pConfig->Profile.SetDouble("/SimCam/pe_cust_period", SimCamParams::custom_pe_period);
    pConfig->Profile.SetDouble("/SimCam/dec_drift", (SimCamParams::dec_drift_rate * 60.0/SimCamParams::inverse_imagescale));
    pConfig->Profile.SetDouble("/SimCam/seeing_scale", SimCamParams::seeing_scale);
    pConfig->Profile.SetDouble("/SimCam/cam_angle", SimCamParams::cam_angle);
    pConfig->Profile.SetDouble("/SimCam/guide_rate", SimCamParams::guide_rate);
    pConfig->Profile.SetInt("/SimCam/pier_side", (int) SimCamParams::pier_side);
    pConfig->Profile.SetBoolean("/SimCam/reverse_dec_pulse_on_west_side", SimCamParams::reverse_dec_pulse_on_west_side);
}

#ifdef STEPGUIDER_SIMULATOR

struct SimAoParams
{
    static unsigned int max_position; // max position in steps
    static double scale;           // arcsec per step
    static double angle;           // angle relative to camera (degrees)
};

unsigned int SimAoParams::max_position = 45;
double SimAoParams::scale = 0.10;
double SimAoParams::angle = 35.0;

static StepGuiderSimulator *s_sim_ao;

StepGuiderSimulator::StepGuiderSimulator(void)
{
    m_Name = _("AO-Simulator");
}

StepGuiderSimulator::~StepGuiderSimulator(void)
{
}

bool StepGuiderSimulator::Connect(void)
{
    if (StepGuider::Connect())
        return true;

    ZeroCurrentPosition();

    s_sim_ao = this;

    Debug.AddLine("AO Simulator Connected");

    return false;
}

bool StepGuiderSimulator::Disconnect(void)
{
    if (StepGuider::Disconnect())
        return true;

    if (s_sim_ao == this) {
        Debug.AddLine("AO Simulator Disconnected");
        s_sim_ao = 0;
    }

    return false;
}

bool StepGuiderSimulator::Step(GUIDE_DIRECTION direction, int steps)
{
    // parent class maintains x/y offsets, so nothing to do here
    return false;
}

int StepGuiderSimulator::MaxPosition(GUIDE_DIRECTION direction)
{
    return SimAoParams::max_position;
}

#endif // STEPGUIDER_SIMULATOR

// value with backlash
//   There is an index value, and a lower and upper limit separated by the
//   backlash amount. When the index moves past the upper limit, it carries
//   both limits along, likewise for the lower limit. The current value is
//   the value of the upper limit.
struct BacklashVal {
    double cur;    // current index value
    double upper;  // upper limit
    double amount; // backlash amount (lower limit is upper - amount)

    BacklashVal() { }

    BacklashVal(double backlash_amount)
        : cur(0), upper(backlash_amount), amount(backlash_amount) { }

    double val() const { return upper; }

    void incr(double d) {
        cur += d;
        if (d > 0.) {
            if (cur > upper)
                upper = cur;
        }
        else if (d < 0.) {
            if (cur < upper - amount)
                upper = cur + amount;
        }
    }
};

struct SimStar {
    wxRealPoint pos;
    int inten;
};

struct SimCamState {
    unsigned int width;
    unsigned int height;
    wxVector<SimStar> stars; // star positions and intensities (ra, dec)
    wxVector<wxPoint> hotpx; // hot pixels
    double ra_ofs;        // assume no backlash in RA
    BacklashVal dec_ofs;  // simulate backlash in DEC
    wxStopWatch timer;    // platform-independent timer
    Camera_SimClass *pThisCam;  // needed for fractional px offsets
#ifdef SIMDEBUG
    wxFFile DebugFile;
    double last_ra_move;
    double last_dec_move;
#endif

    void Initialize();
    void FillImage(usImage& img, const wxRect& subframe, int exptime, int gain, int offset);
};

void SimCamState::Initialize()
{
    width = SimCamParams::width;
    height = SimCamParams::height;
    // generate stars at random positions but no closer than 12 pixels from any edge
    unsigned int const nr_stars = SimCamParams::nr_stars;
    stars.resize(nr_stars);
    unsigned int const border = SimCamParams::border;

    srand(2); // always generate the same stars
    for (unsigned int i = 0; i < nr_stars; i++) {
        // generate stars in ra/dec coordinates
        stars[i].pos.x = (double)(rand() % (width - 2 * border)) - 0.5 * width;
        stars[i].pos.y = (double)(rand() % (height - 2 * border)) - 0.5 * height;
        stars[i].inten = 20 + rand() % 80;
    }
    // generate hot pixels
    unsigned int const nr_hot = SimCamParams::nr_hot_pixels;
    hotpx.resize(nr_hot);
    for (unsigned int i = 0; i < nr_hot; i++) {
        hotpx[i].x = rand() % width;
        hotpx[i].y = rand() % height;
    }
    srand(clock());
    ra_ofs = 0.;
    dec_ofs = BacklashVal(SimCamParams::dec_backlash);
#ifdef SIMDEBUG
    DebugFile.Open ("Sim_Debug.txt", "w");
    DebugFile.Write ("PE, Drift, RA_Seeing, Dec_Seeing, Total_X, Total_Y, RA_Ofs, Dec_Ofs, \n");
#endif

}

// get a pair of normally-distributed independent random values - Box-Muller algorithm, sigma=1
static void rand_normal(double r[2])
{
    double u = (double) rand() / RAND_MAX;
    double v = (double) rand() / RAND_MAX;
    double const a = sqrt(-2.0 * log(u));
    double const p = 2 * M_PI * v;
    r[0] = a * cos(p);
    r[1] = a * sin(p);
}

inline static unsigned short *pixel_addr(usImage& img, int x, int y)
{
    if (x < 0 || x >= img.Size.x)
        return 0;
    if (y < 0 || y >= img.Size.y)
        return 0;
    return &img.Pixel(x, y);
}

inline static void set_pixel(usImage& img, int x, int y, unsigned short val)
{
    unsigned short *const addr = pixel_addr(img, x, y);
    if (addr)
        *addr = val;
}

inline static void incr_pixel(usImage& img, int x, int y, unsigned int val)
{
    unsigned short *const addr = pixel_addr(img, x, y);
    if (addr) {
        unsigned int t = *addr;
        t += val;
        if (t > (unsigned int)(unsigned short)-1)
            *addr = (unsigned short)-1;
        else
            *addr = (unsigned short)t;
    }
}

static void render_star(usImage& img, const wxRect& subframe, const wxRealPoint& p, int inten)
{
    static unsigned int const STAR[][7] = {{  0,  0,  1,  1,  1,  0,  0,},
                                           {  0,  2, 11, 17, 11,  2,  0,},
                                           {  1, 11, 47, 78, 47, 11,  1,},
                                           {  1, 17, 78,128, 78, 17,  1,},
                                           {  1, 11, 47, 78, 47, 11,  1,},
                                           {  0,  2, 11, 17, 11,  2,  0,},
                                           {  0,  0,  1,  1,  1,  0,  0,}};
    for (int sx = -3; sx <= 3; sx++) {
        int const cx = (int) floor(p.x + (double) sx / 2.0 + 0.5);
        if (cx < subframe.GetLeft() || cx > subframe.GetRight())
            continue;
        for (int sy = -3; sy <= 3; sy++) {
            int const cy = (int) floor(p.y + (double) sy / 2.0 + 0.5);
            if (cy < subframe.GetTop() || cy > subframe.GetBottom())
                continue;
            int incr = inten * STAR[sy + 3][sx + 3] / 256;
            if (incr > (unsigned short)-1)
                incr = (unsigned short)-1;
            incr_pixel(img, cx, cy, incr);
        }
    }
}

static void render_clouds(usImage& img, const wxRect& subframe, int exptime, int gain, int offset)
{
    unsigned short *p0 = &img.Pixel(subframe.GetLeft(), subframe.GetTop());
    for (int r = 0; r < subframe.GetHeight(); r++, p0 += img.Size.GetWidth())
    {
        unsigned short *const end = p0 + subframe.GetWidth();
        for (unsigned short *p = p0; p < end; p++)
            *p = (unsigned short) (SimCamParams::clouds_inten * ((double) gain / 10.0 * offset * exptime / 100.0 + ((rand() % (gain * 100)) / 30.0)));
    }
}

void SimCamState::FillImage(usImage& img, const wxRect& subframe, int exptime, int gain, int offset)
{
    unsigned int const nr_stars = stars.size();
#ifdef SIMDEBUG
    static int CountUp (0);
    if (CountUp == 0)
    {
        // Changes in the setup dialog are hard to track - just make sure we are using the params we think we are
        Debug.AddLine (wxString::Format("SimDebug: img_scale: %.3f, seeing_scale: %.3f", 1.0/SimCamParams::inverse_imagescale, SimCamParams::seeing_scale));
    }
    CountUp++;
#endif
    // start with original star positions
    wxVector<wxRealPoint> pos(nr_stars);
    for (unsigned int i = 0; i < nr_stars; i++)
        pos[i] = stars[i].pos;

    double const now = timer.Time() / 1000.;

    // Compute PE - canned PE terms create some "steep" sections of the curve
    static double const period[] = { 230.5, 122.0, 49.4, 9.56, 76.84, };
    static double const amp[] =    {2.02, 0.69, 0.22, 0.137, 0.14};   // in a-s
    static double const phase[] =  { 0.0,     1.4, 98.8, 35.9, 150.4, };
    double pe = 0.;

    if (SimCamParams::use_pe)
    {
        if (SimCamParams::use_default_pe_params)
        {
            for (unsigned int i = 0; i < WXSIZEOF(period); i++)
                pe += amp[i] * cos((now - phase[i]) / period[i] * 2. * M_PI);
            pe *= (SimCamParams::pe_scale * SimCamParams::inverse_imagescale);      // modulated PE in px
        }
        else
        {
            pe = SimCamParams::custom_pe_amp * cos(now/ SimCamParams::custom_pe_period * 2.0 * M_PI) * SimCamParams::inverse_imagescale;
        }
    }

    // simulate drift in DEC
    double const drift = now * SimCamParams::dec_drift_rate;

    // simulate seeing
    double seeing [2] = {0};
    static const double seeing_adjustment = (2.345 * 1.4 * 2.4);        //FWHM, geometry, empirical

    if (SimCamParams::seeing_scale > 0)
    {
        rand_normal (seeing);
        seeing [0] = seeing[0] * SimCamParams::seeing_scale/seeing_adjustment * SimCamParams::inverse_imagescale;
        seeing [1] = seeing[1] * SimCamParams::seeing_scale/seeing_adjustment * SimCamParams::inverse_imagescale;
    }

    // Compute total movements from all sources - ra_ofs and dec_ofs are cumulative sums of all guider movements relative to zero-point
    double total_shift_x = pe + seeing[0] + ra_ofs;
    double total_shift_y = drift + seeing[1] + dec_ofs.val();
    if (!s_sim_ao)
    {
        // For non-AO operations, split into integral and fractional adjustments
        double intX = 0, intY = 0;
        double fracX = 0, fracY = 0;
        fracX = modf(total_shift_x, &intX);
        fracY = modf(total_shift_y, &intY);
        pThisCam->SetGuideXAdjustment (fracX);     // These will be applied in mount.cpp
        pThisCam->SetGuideYAdjustment (fracY);
#ifdef SIMDEBUG
        Debug.AddLine (wxString::Format("SimDebug: Requested cum deflection in X of %.3f + %.3f", intX, fracX));
        Debug.AddLine (wxString::Format("SimDebug: Requested cum deflection in Y of %.3f + %.3f", intY, fracY));
#endif

        // Apply just the integral adjustments
        for (unsigned int i = 0; i < nr_stars; i++)
        {
            pos[i].x += intX;
            pos[i].y += intY;
        }
    }
    else
    {
        // For AO, apply the full amount
        for (unsigned int i = 0; i < nr_stars; i++)
        {
            pos[i].x += total_shift_x;
            pos[i].y += total_shift_y;
        }
    }

#ifdef SIMDEBUG
        DebugFile.Write(wxString::Format( "%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f\n",
        pe, drift, seeing[0], seeing[1], total_shift_x, total_shift_y,
        ra_ofs, dec_ofs.val()));
#endif
    // convert to camera coordinates
    wxVector<wxRealPoint> cc(nr_stars);
    double const angle = SimCamParams::cam_angle * M_PI / 180.;
    double const cos_t = cos(angle);
    double const sin_t = sin(angle);
    for (unsigned int i = 0; i < nr_stars; i++) {
        cc[i].x = pos[i].x * cos_t - pos[i].y * sin_t + width / 2.0;
        cc[i].y = pos[i].x * sin_t + pos[i].y * cos_t + height / 2.0;
    }

#ifdef STEPGUIDER_SIMULATOR
    // add-in AO offset
    if (s_sim_ao) {
        double const ao_angle = SimAoParams::angle * M_PI / 180.;
        double const cos_a = cos(ao_angle);
        double const sin_a = sin(ao_angle);
        double const ao_x = (double) s_sim_ao->CurrentPosition(RIGHT) * SimAoParams::scale;
        double const ao_y = (double) s_sim_ao->CurrentPosition(UP) * SimAoParams::scale;
        double const dx = ao_x * cos_a - ao_y * sin_a;
        double const dy = ao_x * sin_a + ao_y * cos_a;
        for (unsigned int i = 0; i < nr_stars; i++) {
            cc[i].x += dx;
            cc[i].y += dy;
        }
    }
#endif // STEPGUIDER_SIMULATOR

    // render each star
    if (!pCamera->ShutterState)
    {
        for (unsigned int i = 0; i < nr_stars; i++)
        {
            unsigned short const newval =
                stars[i].inten * exptime * gain + (int)((double) gain / 10.0 * offset * exptime / 100.0 + (rand() % (gain * 100)));

            render_star(img, subframe, cc[i], newval);
        }
    }

    if (SimCamParams::clouds_inten)
        render_clouds(img, subframe, exptime, gain, offset);

    // render hot pixels
    for (unsigned int i = 0; i < hotpx.size(); i++)
        if (subframe.Contains(hotpx[i]))
            set_pixel(img, hotpx[i].x, hotpx[i].y, (unsigned short) -1);
}

Camera_SimClass::Camera_SimClass()
    : sim(new SimCamState())
{
    Connected = false;
//  HaveBPMap = FALSE;
//  NBadPixels=-1;
//  ConnectedModel = 1;
    Name=_T("Simulator");
    FullSize = wxSize(640,480);
    m_hasGuideOutput = true;
    HasShutter = true;
    HasGainControl = true;
    HasSubframes = true;
    HasPropertyDialog = true;
    m_isSimulator = true;
    m_guideXAdjustment = 0;
    m_guideYAdjustment = 0;
    sim->pThisCam = this;
}

bool Camera_SimClass::Connect()
{
    SimCamParams::inverse_imagescale = 1.0/pFrame->GetCameraPixelScale();
    load_sim_params();
    sim->Initialize();
    Connected = true;

    return false;
}

bool Camera_SimClass::Disconnect()
{
    Connected = false;
    return false;
}

void Camera_SimClass::SetGuideXAdjustment (double amt)
{
    m_guideXAdjustment = amt;
}
void Camera_SimClass::SetGuideYAdjustment (double amt)
{
    m_guideYAdjustment = amt;
}

Camera_SimClass::~Camera_SimClass()
{
#ifdef SIMDEBUG
    sim->DebugFile.Close();
#endif
    delete sim;
}

#if SIMMODE==1
bool Camera_SimClass::CaptureFull(int WXUNUSED(duration), usImage& img, bool recon) {
    int xsize, ysize;
//  unsigned short *dataptr;
//  int i;
    fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
    int hdutype, naxis;
    int nhdus=0;
    long fits_size[2];
    long fpixel[3] = {1,1,1};
//  char keyname[15];
//  char keystrval[80];

#if defined (__APPLE__)
    if ( !fits_open_file(&fptr, "/Users/stark/dev/PHD/simimage.fit", READONLY, &status) ) {
#else
    if ( !fits_open_diskfile(&fptr, "phd011412.fit", READONLY, &status) ) {
#endif
        if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU) {
            (void) wxMessageBox(wxT("FITS file is not of an image"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }

       // Get HDUs and size
        fits_get_img_dim(fptr, &naxis, &status);
        fits_get_img_size(fptr, 2, fits_size, &status);
        xsize = (int) fits_size[0];
        ysize = (int) fits_size[1];
        fits_get_num_hdus(fptr,&nhdus,&status);
        if ((nhdus != 1) || (naxis != 2)) {
           (void) wxMessageBox(_T("Unsupported type or read error loading FITS file"),_("Error"),wxOK | wxICON_ERROR);
           return true;
        }
        if (img.Init(xsize,ysize)) {
            wxMessageBox(_T("Memory allocation error"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, NULL, img.ImageData, NULL, &status) ) { // Read image
            (void) wxMessageBox(_T("Error reading data"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        fits_close_file(fptr,&status);
    }
    return false;

}
#endif

#if SIMMODE==2
bool Camera_SimClass::CaptureFull(int WXUNUSED(duration), usImage& img) {
    int xsize, ysize;
    wxImage disk_image;
    unsigned short *dataptr;
    unsigned char *imgptr;
    int i;

    bool retval = disk_image.LoadFile("/Users/stark/dev/PHD/simimage.bmp");
    if (!retval) {
        wxMessageBox(_T("Cannot load simulated image"));
        return true;
    }
    xsize = disk_image.GetWidth();
    ysize = disk_image.GetHeight();
    if (img.Init(xsize,ysize)) {
        wxMessageBox(_T("Memory allocation error"),_("Error"),wxOK | wxICON_ERROR);
        return true;
    }

    dataptr = img.ImageData;
    imgptr = disk_image.GetData();
    for (i=0; i<img.NPixels; i++, dataptr++, imgptr++) {
        *dataptr = (unsigned short) *imgptr;
        imgptr++; imgptr++;
    }
    QuickLRecon(img);
    return false;

}
#endif

#if SIMMODE==3

static void fill_noise(usImage& img, const wxRect& subframe, int exptime, int gain, int offset)
{
    unsigned short *p0 = &img.Pixel(subframe.GetLeft(), subframe.GetTop());
    for (int r = 0; r < subframe.GetHeight(); r++, p0 += img.Size.GetWidth())
    {
        unsigned short *const end = p0 + subframe.GetWidth();
        for (unsigned short *p = p0; p < end; p++)
            *p = (unsigned short) (SimCamParams::noise_multiplier * ((double) gain / 10.0 * offset * exptime / 100.0 + (rand() % (gain * 100))));
    }
}

bool Camera_SimClass::Capture(int duration, usImage& img, wxRect subframe, bool recon)
{
    long long start = wxGetUTCTimeMillis().GetValue();

    FullSize = wxSize(sim->width, sim->height);

    bool usingSubframe = UseSubframes;
    if (subframe.width <= 0 || subframe.height <= 0)
        usingSubframe = false;
    if (!usingSubframe)
        subframe = wxRect(0, 0, FullSize.GetWidth(), FullSize.GetHeight());

    int const exptime = duration;
    int const gain = 30;
    int const offset = 100;

    if (img.NPixels != (int)(sim->width * sim->height)) {
        if (img.Init(sim->width, sim->height)) {
            wxMessageBox(_T("Memory allocation error"),_("Error"), wxOK | wxICON_ERROR);
            return true;
        }
    }

    if (usingSubframe)
        memset(img.ImageData, 0, img.NPixels * sizeof(unsigned short));

    fill_noise(img, subframe, exptime, gain, offset);

    sim->FillImage(img, subframe, exptime, gain, offset);

    if (usingSubframe)
        img.Subframe = subframe;

    if (recon) SubtractDark(img);

    long long now = wxGetUTCTimeMillis().GetValue();
    if (now < start + duration)
        wxMilliSleep(start + duration - now);

    return false;
}

bool Camera_SimClass::ST4PulseGuideScope(int direction, int duration)
{
    double d = SimCamParams::guide_rate * duration / 1000.0;

    if (SimCamParams::pier_side == PIER_SIDE_WEST && SimCamParams::reverse_dec_pulse_on_west_side)
    {
        // after pier flip, North/South have opposite affect on declination
        switch (direction) {
        case NORTH: direction = SOUTH; break;
        case SOUTH: direction = NORTH; break;
        }
    }

    switch (direction) {
    case WEST:    sim->ra_ofs += d;      break;
    case EAST:    sim->ra_ofs -= d;      break;
    case NORTH:   sim->dec_ofs.incr(d);  break;
    case SOUTH:   sim->dec_ofs.incr(-d); break;
    default: return true;
    }
    wxMilliSleep(duration);
    return false;
}

PierSide Camera_SimClass::SideOfPier(void) const
{
    return SimCamParams::pier_side;
}

static PierSide OtherSide(PierSide side)
{
    return side == PIER_SIDE_EAST ? PIER_SIDE_WEST : PIER_SIDE_EAST;
}

void Camera_SimClass::FlipPierSide(void)
{
    SimCamParams::pier_side = OtherSide(SimCamParams::pier_side);
    SimCamParams::cam_angle += 180.0;
    if (SimCamParams::cam_angle >= 360.0)
        SimCamParams::cam_angle -= 360.0;
    Debug.AddLine("CamSimulator FlipPierSide: side = %d  cam_angle = %.1f", SimCamParams::pier_side, SimCamParams::cam_angle);
}

#endif // SIMMODE==3

#if SIMMODE == 4
bool Camera_SimClass::CaptureFull(int WXUNUSED(duration), usImage& img, bool recon) {
    int xsize, ysize;
    //  unsigned short *dataptr;
    //  int i;
    fitsfile *fptr;  // FITS file pointer
    int status = 0;  // CFITSIO status value MUST be initialized to zero!
    int hdutype, naxis;
    int nhdus=0;
    long fits_size[2];
    long fpixel[3] = {1,1,1};
    //  char keyname[15];
    //  char keystrval[80];
    static int frame = 0;
    static int step = 1;
    char fname[256];
    sprintf(fname,"/Users/stark/dev/PHD/simimg/DriftSim_%d.fit",frame);
    if ( !fits_open_file(&fptr, fname, READONLY, &status) ) {
        if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU) {
            (void) wxMessageBox(wxT("FITS file is not of an image"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }

        // Get HDUs and size
        fits_get_img_dim(fptr, &naxis, &status);
        fits_get_img_size(fptr, 2, fits_size, &status);
        xsize = (int) fits_size[0];
        ysize = (int) fits_size[1];
        fits_get_num_hdus(fptr,&nhdus,&status);
        if ((nhdus != 1) || (naxis != 2)) {
            (void) wxMessageBox(wxString::Format("Unsupported type or read error loading FITS file %d %d",nhdus,naxis),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        if (img.Init(xsize,ysize)) {
            wxMessageBox(_T("Memory allocation error"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        if (fits_read_pix(fptr, TUSHORT, fpixel, xsize*ysize, NULL, img.ImageData, NULL, &status) ) { // Read image
            (void) wxMessageBox(_T("Error reading data"),_("Error"),wxOK | wxICON_ERROR);
            return true;
        }
        fits_close_file(fptr,&status);
        frame = frame + step;
        if (frame > 440) {
            step = -1;
            frame = 439;
        }
        else if (frame < 0) {
            step = 1;
            frame = 1;
        }

    }
    return false;

}
#endif

struct SimCamDialog : public wxDialog
{
    wxSlider *pStarsSlider;
    wxSlider *pHotpxSlider;
    wxSlider *pNoiseSlider;
    wxSpinCtrlDouble *pBacklashSpin;
    wxSpinCtrlDouble *pDriftSpin;
    wxSpinCtrlDouble *pGuideRateSpin;
    wxSpinCtrlDouble *pCameraAngleSpin;
    wxSpinCtrlDouble *pSeeingSpin;
    wxCheckBox* pCloudsCbx;
    wxCheckBox *pUsePECbx;
    wxCheckBox *pReverseDecPulseCbx;
    PierSide pPierSide;
    wxStaticText *pPiersideLabel;
    wxRadioButton *pPEDefaultRb;
    wxSlider *pPEDefSlider;
    wxRadioButton *pPECustomRb;
    wxTextCtrl *pPECustomAmp;
    wxTextCtrl *pPECustomPeriod;

    SimCamDialog(wxWindow *parent);
    ~SimCamDialog() { }
    void OnReset(wxCommandEvent& event);
    void OnPierFlip(wxCommandEvent& event);
    void UpdatePierSideLabel();
    void OnRbDefaultPE (wxCommandEvent& evt);
    void OnRbCustomPE (wxCommandEvent& evt);
    void OnOkClick (wxCommandEvent& evt);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SimCamDialog, wxDialog)
    EVT_BUTTON(wxID_RESET, SimCamDialog::OnReset)
    EVT_BUTTON(wxID_CONVERT, SimCamDialog::OnPierFlip)


END_EVENT_TABLE()

// Utility functions for adding controls with specified properties
static wxSlider* NewSlider (wxWindow* parent, int val, int minval, int maxval, wxString tooltip)
{
    wxSlider *pNewCtrl  = new wxSlider(parent, wxID_ANY, val, minval, maxval, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    pNewCtrl->SetToolTip (tooltip);
    return (pNewCtrl);
}

static  wxSpinCtrlDouble* NewSpinner (wxWindow* parent, double val, double minval, double maxval, double inc,
    wxString tooltip)
{
    wxSpinCtrlDouble *pNewCtrl = new wxSpinCtrlDouble (parent, wxID_ANY, _T("foo2"), wxPoint(-1, -1),
        wxDefaultSize, wxSP_ARROW_KEYS, minval, maxval, val, inc);
    pNewCtrl->SetDigits (2);
    pNewCtrl->SetToolTip (tooltip);
    return (pNewCtrl);
}

static wxCheckBox* NewCheckBox (wxWindow* parent, bool val, wxString label, wxString tooltip)
{
    wxCheckBox *pNewCtrl = new wxCheckBox (parent, wxID_ANY, label);
    pNewCtrl->SetValue (val);
    pNewCtrl->SetToolTip (tooltip);
    return (pNewCtrl);
}

// Utility function to add the <label, input> pairs to a grid including tool-tips
static void AddTableEntryPair (wxWindow* parent, wxFlexGridSizer* pTable, wxString label, wxWindow* pControl)
{
    wxStaticText *pLabel = new wxStaticText(parent, wxID_ANY, label + _(": "),wxPoint(-1,-1),wxSize(-1,-1));
    pTable->Add (pLabel, 1, wxALL, 5);
    pTable->Add (pControl, 1, wxALL, 5);
}

static wxTextCtrl* AddCustomPEField (wxWindow* parent, wxFlexGridSizer* pTable, wxString label, wxString tip, double val)
{
    int width;
    int height;

    parent->GetTextExtent(_("999.9"), &width, &height);
    wxTextCtrl *pCtrl = new wxTextCtrl(parent, wxID_ANY, _T("    "), wxDefaultPosition, wxSize(width+30, -1));
    pCtrl->SetValue (wxString::Format("%.1f", val));
    pCtrl->SetToolTip (tip);
    AddTableEntryPair (parent, pTable, label, pCtrl);
    return pCtrl;
}


static void SetRBState (SimCamDialog* pParent, bool using_defaults)
{
    pParent->pPEDefSlider->Enable(using_defaults);
    pParent->pPECustomAmp->Enable(!using_defaults);
    pParent->pPECustomPeriod->Enable(!using_defaults);
}

// Event handlers
void SimCamDialog::OnRbDefaultPE (wxCommandEvent& evt)
{
    SetRBState (this, true);
}
void SimCamDialog::OnRbCustomPE (wxCommandEvent& evt)
{
    SetRBState (this, false);
}

// Need to enforce semantics on free-form user input
void SimCamDialog::OnOkClick (wxCommandEvent& evt)
{
    bool bOk = false;
    if (pPECustomRb->GetValue())
    {
        wxString sAmp = pPECustomAmp->GetValue();
        wxString sPeriod = pPECustomPeriod->GetValue();
        double amp = 0;
        double period = 0;
        if (sAmp.ToDouble (&amp) && sPeriod.ToDouble (&period))
            if (amp > 0 && period > 0)
                bOk = true;
            else
                wxMessageBox (_("PE amplitude and period must be > 0"), "Error", wxOK | wxICON_ERROR);
        else
            wxMessageBox (_("PE amplitude and period must be numbers > 0"), "Error", wxOK | wxICON_ERROR);

    }
    if (bOk)
        wxDialog::EndModal (wxID_OK);
}

SimCamDialog::SimCamDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, _("Camera Simulator"))
{
    wxBoxSizer *pVSizer = new wxBoxSizer (wxVERTICAL);
    double imageScale = pFrame->GetCameraPixelScale ();

    SimCamParams::inverse_imagescale = 1.0/imageScale;

    // Camera group controls
    wxStaticBoxSizer *pCamGroup = new wxStaticBoxSizer (wxVERTICAL, this, _("Camera"));
    wxFlexGridSizer *pCamTable = new wxFlexGridSizer (1, 6, 15, 15);
    pStarsSlider = NewSlider (this, SimCamParams::nr_stars, 1, 100, _("Number of simulated stars"));
    AddTableEntryPair (this, pCamTable, _("Stars"), pStarsSlider);
    pHotpxSlider = NewSlider (this, SimCamParams::nr_hot_pixels, 0, 50, _("Number of hot pixels"));
    AddTableEntryPair (this, pCamTable, _("Hot pixels"), pHotpxSlider);
    pNoiseSlider = NewSlider (this, (int)floor(SimCamParams::noise_multiplier * 100 / NOISE_MAX), 0, 100,  _("% Simulated noise"));
    AddTableEntryPair (this, pCamTable, _("Noise"), pNoiseSlider);
    pCamGroup->Add (pCamTable);

    // Mount group controls
    wxStaticBoxSizer *pMountGroup = new wxStaticBoxSizer (wxVERTICAL, this, _("Mount"));
    wxFlexGridSizer *pMountTable = new wxFlexGridSizer (1, 6, 15, 15);
    pBacklashSpin = NewSpinner (this, SimCamParams::dec_backlash * imageScale, 0, DEC_BACKLASH_MAX, 0.1, _("Dec backlash, arc-secs"));
    AddTableEntryPair (this, pMountTable, _("Dec backlash"), pBacklashSpin);
    pDriftSpin = NewSpinner (this, SimCamParams::dec_drift_rate * 60.0 * imageScale, 0, DEC_DRIFT_MAX, 0.5, _("Dec drift, arc-sec/min"));
    AddTableEntryPair (this, pMountTable, _("Dec drift"), pDriftSpin);
    pGuideRateSpin = NewSpinner (this, SimCamParams::guide_rate / 15.0, 0.25, GUIDE_RATE_MAX, 0.25, _("Guide rate, x sidereal"));
    AddTableEntryPair (this, pMountTable, _("Guide rate"), pGuideRateSpin);
    pMountGroup->Add (pMountTable);

    // Add embedded group for PE info (still within mount group)
    wxStaticBoxSizer *pPEGroup = new wxStaticBoxSizer (wxVERTICAL, this, _("PE"));
    pUsePECbx = NewCheckBox (this, SimCamParams::use_pe, _("Apply PE"), _("Simulate periodic error"));
    wxBoxSizer *pPEHorSizer = new wxBoxSizer(wxHORIZONTAL);
    // Default PE parameters
    wxFlexGridSizer *pPEDefaults = new wxFlexGridSizer (1, 3, 10, 10);
    pPEDefaultRb = new wxRadioButton(this, wxID_ANY, _("Default curve"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    pPEDefaultRb->SetValue (SimCamParams::use_default_pe_params);
    pPEDefaultRb->Bind (wxEVT_COMMAND_RADIOBUTTON_SELECTED, &SimCamDialog::OnRbDefaultPE, this);                // Event handler binding
    wxStaticText *pSliderLabel = new wxStaticText(this, wxID_ANY, _("Percent maximum:"),wxPoint(-1,-1),wxSize(-1,-1));
    pPEDefSlider = NewSlider (this, SimCamParams::pe_scale * 100, 0, 100, "% of max");
    pPEDefaults->Add (pPEDefaultRb);
    pPEDefaults->Add (pSliderLabel, wxSizerFlags().Border(wxLEFT, 6));
    pPEDefaults->Add (pPEDefSlider);
    // Custom PE parameters
    wxFlexGridSizer *pPECustom = new wxFlexGridSizer (1, 5, 10, 10);
    pPECustomRb = new wxRadioButton(this, wxID_ANY, _("Custom curve"), wxDefaultPosition, wxDefaultSize);
    pPECustomRb->SetValue(!SimCamParams::use_default_pe_params);
    pPECustomRb->Bind (wxEVT_COMMAND_RADIOBUTTON_SELECTED, &SimCamDialog::OnRbCustomPE, this);              // Event handler binding
    pPECustom->Add (pPECustomRb, wxSizerFlags().Border(wxTOP, 4));
    pPECustomAmp = AddCustomPEField (this, pPECustom, _("Amplitude: "), _("Amplitude, arc-secs"), SimCamParams::custom_pe_amp);
    pPECustomPeriod = AddCustomPEField (this, pPECustom, _("Period: "), _("Period, seconds"), SimCamParams::custom_pe_period);
    // VSizer for default and custom controls
    wxBoxSizer *pPEVSizer = new wxBoxSizer(wxVERTICAL);
    pPEVSizer->Add(pPEDefaults, wxSizerFlags().Border(wxLEFT, 60));
    pPEVSizer->Add (pPECustom, wxSizerFlags().Border(wxLEFT, 60));
    // Finish off the whole PE group
    pPEHorSizer->Add(pUsePECbx);
    pPEHorSizer->Add (pPEVSizer);
    pPEGroup->Add (pPEHorSizer);

    // Now add some miscellaneous mount-related stuff (still within mount group)
    wxBoxSizer *pMiscSizer = new wxBoxSizer(wxHORIZONTAL);
    pReverseDecPulseCbx = NewCheckBox (this, SimCamParams::reverse_dec_pulse_on_west_side, _("Reverse Dec pulse on west side of pier"),
    _("Simulate a mount that reverses guide pulse direction after a meridian flip, like an ASCOM pulse-guided mount."));
    pPierSide = SimCamParams::pier_side;
    pPiersideLabel = new wxStaticText(this, wxID_ANY, _("Side of Pier: MMMMM"));
    pMiscSizer->Add (pReverseDecPulseCbx, wxSizerFlags().Border(10).Expand());
    pMiscSizer->Add(new wxButton(this, wxID_CONVERT, _("Pier Flip")), wxSizerFlags().Border(wxLEFT, 30).Expand());
    pMiscSizer->Add(pPiersideLabel , wxSizerFlags().Border(wxLEFT, 30).Expand());
    pMountGroup->Add (pPEGroup, wxSizerFlags().Center().Border(10).Expand());
    pMountGroup->Add (pMiscSizer, wxSizerFlags().Border(wxTOP, 10).Expand());

    // Session group controls
    wxStaticBoxSizer *pSessionGroup = new wxStaticBoxSizer (wxVERTICAL, this, _("Session"));
    wxFlexGridSizer *pSessionTable = new wxFlexGridSizer (1, 5, 15, 15);
    pCameraAngleSpin = NewSpinner (this, SimCamParams::cam_angle, 0, 360.0, 10, _("Camera angle, degrees"));
    AddTableEntryPair (this, pSessionTable, _("Camera angle"), pCameraAngleSpin);
    pSeeingSpin = NewSpinner (this, SimCamParams::seeing_scale, 0, SEEING_MAX, 0.5, _("Seeing, FWHM arc-sec"));
    AddTableEntryPair (this, pSessionTable, _("Seeing"), pSeeingSpin);
    pCloudsCbx = new wxCheckBox(this, wxID_ANY, _("Star fading due to clouds"));
    pSessionGroup->Add (pSessionTable);
    pSessionGroup->Add (pCloudsCbx);

    pVSizer->Add (pCamGroup, wxSizerFlags().Border(10).Expand());
    pVSizer->Add (pMountGroup, wxSizerFlags().Border(wxTOP, 20));
    pVSizer->Add (pSessionGroup, wxSizerFlags().Border(wxTOP, 20).Expand());

    // Now deal with the buttons
    wxBoxSizer *pButtonSizer = new wxBoxSizer( wxHORIZONTAL );
    wxButton *pBtn = new wxButton(this, wxID_RESET, _("Reset"));
    pBtn->SetToolTip(_("Reset all values to application defaults"));
    pButtonSizer->Add(
        pBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
    // Need to handle the OK event ourselves to validate text input fields
    pBtn = new wxButton(this, wxID_OK, _("OK"));
    pBtn->Bind (wxEVT_COMMAND_BUTTON_CLICKED, &SimCamDialog::OnOkClick, this);
    pButtonSizer->Add(
        pBtn,
        wxSizerFlags(0).Align(0).Border(wxALL, 10));
     pButtonSizer->Add(
        new wxButton( this, wxID_CANCEL, _("Cancel") ),
        wxSizerFlags(0).Align(0).Border(wxALL, 10));

     //position the buttons centered with no border
     pVSizer->Add(
        pButtonSizer,
        wxSizerFlags(0).Center() );

    SetSizerAndFit (pVSizer);
    SetRBState (this, pPEDefaultRb->GetValue());        // Enable matching PE-related controls
    UpdatePierSideLabel();
}

void SimCamDialog::OnReset(wxCommandEvent& event)
{
    pStarsSlider->SetValue(NR_STARS_DEFAULT);
    pHotpxSlider->SetValue(NR_HOT_PIXELS_DEFAULT);
    pNoiseSlider->SetValue((int)floor(NOISE_DEFAULT * 100.0 / NOISE_MAX));
    pBacklashSpin->SetValue(DEC_BACKLASH_DEFAULT);

    pDriftSpin->SetValue(DEC_DRIFT_DEFAULT);
    pSeeingSpin->SetValue(SEEING_DEFAULT);
    pCameraAngleSpin->SetValue(CAM_ANGLE_DEFAULT);
    pGuideRateSpin->SetValue(GUIDE_RATE_DEFAULT / GUIDE_RATE_MAX);
    pReverseDecPulseCbx->SetValue(REVERSE_DEC_PULSE_ON_WEST_SIDE_DEFAULT);
    pUsePECbx->SetValue (USE_PE_DEFAULT);
    pPEDefaultRb->SetValue (USE_PE_DEFAULT_PARAMS);
    pPECustomRb->SetValue(!USE_PE_DEFAULT_PARAMS);
    pPEDefSlider->SetValue(PE_SLIDER_DEFAULT * 100.0);
    pPECustomAmp->SetValue(wxString::Format("%0.1f",PE_CUSTOM_AMP_DEFAULT));
    pPECustomPeriod->SetValue(wxString::Format("%0.1f", PE_CUSTOM_PERIOD_DEFAULT));
    pPierSide = PIER_SIDE_DEFAULT;
    SetRBState( this, USE_PE_DEFAULT_PARAMS);
    UpdatePierSideLabel();
    pCloudsCbx->SetValue(false);
}

void SimCamDialog::OnPierFlip(wxCommandEvent& event)
{
    int angle = pCameraAngleSpin->GetValue();
    angle += 180;
    if (angle >= 360)
        angle -= 360;
    pCameraAngleSpin->SetValue(angle);
    pPierSide= OtherSide(pPierSide);
    UpdatePierSideLabel();
}

void SimCamDialog::UpdatePierSideLabel()
{
    pPiersideLabel->SetLabel(wxString::Format("Side of pier: %s", pPierSide == PIER_SIDE_EAST ? _("East") : _("West")));
}

void Camera_SimClass::ShowPropertyDialog()
{
    SimCamDialog dlg(pFrame);
    double imageScale = pFrame->GetCameraPixelScale();              // arc-sec/pixel, defaults to 1.0 if no user specs
    SimCamParams::inverse_imagescale = 1.0/imageScale;              // keep current - might have gotten changed in brain dialog
    if (dlg.ShowModal() == wxID_OK)
    {
        SimCamParams::nr_stars = dlg.pStarsSlider->GetValue();
        SimCamParams::nr_hot_pixels = dlg.pHotpxSlider->GetValue();
        SimCamParams::noise_multiplier = (double) dlg.pNoiseSlider->GetValue() * NOISE_MAX / 100.0;
        SimCamParams::dec_backlash =     (double) dlg.pBacklashSpin->GetValue() * SimCamParams::inverse_imagescale;    // a-s -> px
        sim->dec_ofs = BacklashVal(SimCamParams::dec_backlash);

        SimCamParams::use_pe = dlg.pUsePECbx->GetValue();
        SimCamParams::use_default_pe_params = dlg.pPEDefaultRb->GetValue();
        if (SimCamParams::use_default_pe_params)
            SimCamParams::pe_scale = dlg.pPEDefSlider->GetValue() / 100.0;
        else
        {
            dlg.pPECustomAmp->GetValue().ToDouble (&SimCamParams::custom_pe_amp);
            dlg.pPECustomPeriod->GetValue().ToDouble (&SimCamParams::custom_pe_period);
        }
        SimCamParams::dec_drift_rate =   (double) dlg.pDriftSpin->GetValue() /(imageScale*60.0);  // a-s per min to px per second
        SimCamParams::seeing_scale =     (double) dlg.pSeeingSpin->GetValue();                      // already in a-s
        SimCamParams::cam_angle =        (double) dlg.pCameraAngleSpin->GetValue();
        SimCamParams::guide_rate =       (double) dlg.pGuideRateSpin->GetValue() * 15.0;
        SimCamParams::pier_side = dlg.pPierSide;
        SimCamParams::reverse_dec_pulse_on_west_side = dlg.pReverseDecPulseCbx->GetValue();
        SimCamParams::clouds_inten = dlg.pCloudsCbx->GetValue() ? CLOUDS_INTEN_DEFAULT : 0;
        save_sim_params();
        sim->Initialize();
    }
}

#endif
