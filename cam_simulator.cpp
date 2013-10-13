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

#define SIMMODE 3   // 1=FITS, 2=BMP, 3=Generate

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

#define NR_STARS_DEFAULT 20
#define NR_HOT_PIXELS_DEFAULT 8
#define NOISE_DEFAULT 2.0
#define NOISE_MAX 5.0
#define DEC_BACKLASH_DEFAULT 11.0
#define DEC_BACKLASH_MAX 30.0
#define PE_DEFAULT 3.5
#define PE_MAX 20.0
#define DEC_DRIFT_DEFAULT (4.8 / 60.0)
#define DEC_DRIFT_MAX (10.0 / 60.0)
#define SEEING_DEFAULT 0.4
#define SEEING_MAX 1.0
#define CAM_ANGLE_DEFAULT 15.0
#define GUIDE_RATE_DEFAULT 3.5
#define GUIDE_RATE_MAX 8.0
#define PIER_SIDE_DEFAULT PIER_SIDE_EAST
#define REVERSE_DEC_PULSE_ON_WEST_SIDE_DEFAULT true
#define CLOUDS_INTEN_DEFAULT 10

static void load_sim_params()
{
    SimCamParams::nr_stars = pConfig->Profile.GetInt("/SimCam/nr_stars", NR_STARS_DEFAULT);
    SimCamParams::nr_hot_pixels = pConfig->Profile.GetInt("/SimCam/nr_hot_pixels", NR_HOT_PIXELS_DEFAULT);
    SimCamParams::noise_multiplier = pConfig->Profile.GetDouble("/SimCam/noise", NOISE_DEFAULT);
    SimCamParams::dec_backlash = pConfig->Profile.GetDouble("/SimCam/dec_backlash", DEC_BACKLASH_DEFAULT);
    SimCamParams::pe_scale = pConfig->Profile.GetDouble("/SimCam/pe_scale", PE_DEFAULT);
    SimCamParams::dec_drift_rate = pConfig->Profile.GetDouble("/SimCam/dec_drift", DEC_DRIFT_DEFAULT);
    SimCamParams::seeing_scale = pConfig->Profile.GetDouble("/SimCam/seeing_scale", SEEING_DEFAULT);
    SimCamParams::cam_angle = pConfig->Profile.GetDouble("/SimCam/cam_angle", CAM_ANGLE_DEFAULT);
    SimCamParams::guide_rate = pConfig->Profile.GetDouble("/SimCam/guide_rate", GUIDE_RATE_DEFAULT);
    SimCamParams::pier_side = (PierSide) pConfig->Profile.GetInt("/SimCam/pier_side", PIER_SIDE_DEFAULT);
    SimCamParams::reverse_dec_pulse_on_west_side = pConfig->Profile.GetBoolean("/SimCam/reverse_dec_pulse_on_west_side", REVERSE_DEC_PULSE_ON_WEST_SIDE_DEFAULT);
}

static void save_sim_params()
{
    pConfig->Profile.SetInt("/SimCam/nr_stars", SimCamParams::nr_stars);
    pConfig->Profile.SetInt("/SimCam/nr_hot_pixels", SimCamParams::nr_hot_pixels);
    pConfig->Profile.SetDouble("/SimCam/noise", SimCamParams::noise_multiplier);
    pConfig->Profile.SetDouble("/SimCam/dec_backlash", SimCamParams::dec_backlash);
    pConfig->Profile.SetDouble("/SimCam/pe_scale", SimCamParams::pe_scale);
    pConfig->Profile.SetDouble("/SimCam/dec_drift", SimCamParams::dec_drift_rate);
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
    m_Name = "AO-Simulator";
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
}

// get a pair of normally-distributed independent random values
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
    for (unsigned int r = 0; r < subframe.GetHeight(); r++, p0 += img.Size.GetWidth())
    {
        unsigned short *const end = p0 + subframe.GetWidth();
        for (unsigned short *p = p0; p < end; p++)
            *p = (unsigned short) (SimCamParams::clouds_inten * ((double) gain / 10.0 * offset * exptime / 100.0 + ((rand() % (gain * 100)) / 30.0)));
    }
}

void SimCamState::FillImage(usImage& img, const wxRect& subframe, int exptime, int gain, int offset)
{
    unsigned int const nr_stars = stars.size();

    // start with original star positions
    wxVector<wxRealPoint> pos(nr_stars);
    for (unsigned int i = 0; i < nr_stars; i++)
        pos[i] = stars[i].pos;

    double const now = timer.Time() / 1000.;

    // simulate periodic error in RA
    double const period[] = { 230.5, 122.0, 49.4, 9.56, 76.84, };
    double const amp[] =    { 1.44,    .49,  .16, .098,   .10, };
    double const phase[] =  { 0.0,     1.4, 98.8, 35.9, 150.4, };
    double pe = 0.;
    for (unsigned int i = 0; i < WXSIZEOF(period); i++)
        pe += amp[i] * cos((now - phase[i]) / period[i] * 2. * M_PI);
    pe *= SimCamParams::pe_scale;

    // add in pe
    for (unsigned int i = 0; i < nr_stars; i++)
        pos[i].x += pe;

    // simulate drift in DEC
    double const drift = now * SimCamParams::dec_drift_rate;
    for (unsigned int i = 0; i < nr_stars; i++) {
        pos[i].y += drift;
    }

    // simulate seeing
    //  todo: simulate decreasing seeing scale with increased exposure time
    double r[2];
    rand_normal(r);
    for (unsigned int i = 0; i < nr_stars; i++) {
        pos[i].x += r[0] * SimCamParams::seeing_scale;
        pos[i].y += r[1] * SimCamParams::seeing_scale;
    }

    // add-in mount pointing offset
    for (unsigned int i = 0; i < nr_stars; i++) {
        pos[i].x += ra_ofs;
        pos[i].y += dec_ofs.val();
    }

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
    if (!pCamera->ShutterState) {
        for (unsigned int i = 0; i < nr_stars; i++) {
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
}

bool Camera_SimClass::Connect()
{
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

Camera_SimClass::~Camera_SimClass()
{
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
    for (unsigned int r = 0; r < subframe.GetHeight(); r++, p0 += img.Size.GetWidth())
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
    wxSlider *stars;
    wxSlider *hotpx;
    wxSlider *noise;
    wxSlider *dec_backlash;
    wxSlider *pe;
    wxSlider *dec_drift;
    wxSlider *seeing;
    wxSlider *cam_angle;
    wxSlider *guide_rate;
    wxSlider *ao_angle;
    wxSlider *ao_step_size;
    wxCheckBox *reverse_dec_pulse_on_west;
    PierSide pier_side;
    wxStaticText *pier_side_label;
    wxCheckBox *clouds;

    SimCamDialog(wxWindow *parent);
    ~SimCamDialog() { }
    void OnReset(wxCommandEvent& event);
    void OnPierFlip(wxCommandEvent& event);
    void UpdatePierSideLabel();

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SimCamDialog, wxDialog)
    EVT_BUTTON(wxID_RESET, SimCamDialog::OnReset)
    EVT_BUTTON(wxID_CONVERT, SimCamDialog::OnPierFlip)
END_EVENT_TABLE()

static wxSizer *label_slider(wxWindow *parent, const wxString& label, wxSlider **pslider, int val, int minval, int maxval)
{
    wxSize label_size = parent->GetTextExtent(_T("MMMMMMMM"));
    wxSize slider_size = parent->GetTextExtent(_T("MMMMMMMMMMMMMM"));
    slider_size.SetHeight(slider_size.GetHeight() * 4);
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, label_size, wxALIGN_RIGHT));
    wxSlider *slider = new wxSlider(parent, wxID_ANY, val, minval, maxval, wxDefaultPosition, slider_size, wxSL_HORIZONTAL | wxSL_VALUE_LABEL);
    *pslider = slider;
    sizer->Add(slider);
    return sizer;
}

SimCamDialog::SimCamDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, _("Camera Simulator"))
{
    wxBoxSizer *sizerl = new wxBoxSizer(wxVERTICAL);

    sizerl->Add(label_slider(this, _("Stars"),        &stars,        SimCamParams::nr_stars, 0, 100));
    sizerl->Add(label_slider(this, _("Hot Pixels"),   &hotpx,        SimCamParams::nr_hot_pixels, 0, 100));
    sizerl->Add(label_slider(this, _("Noise"),        &noise,        (int)floor(SimCamParams::noise_multiplier * 100.0 / NOISE_MAX), 0, 100));
    sizerl->Add(label_slider(this, _("Dec Backlash"), &dec_backlash, (int)floor(SimCamParams::dec_backlash * 100.0 / DEC_BACKLASH_MAX), 0, 100));
    sizerl->Add(label_slider(this, _("PE"),           &pe,           (int)floor(SimCamParams::pe_scale * 100.0 / PE_MAX), 0, 100));

    wxBoxSizer *sizerr = new wxBoxSizer(wxVERTICAL);
    sizerr->Add(label_slider(this, _("DEC Drift"),    &dec_drift,    (int)floor(SimCamParams::dec_drift_rate * 100.0 / DEC_DRIFT_MAX), 0, 100));
    sizerr->Add(label_slider(this, _("Seeing"),       &seeing,       (int)floor(SimCamParams::seeing_scale * 100.0 / SEEING_MAX), 0, 100));
    sizerr->Add(label_slider(this, _("Cam Angle"),    &cam_angle,    (int)floor(SimCamParams::cam_angle + 0.5), 0, 359));
    sizerr->Add(label_slider(this, _("Guide Rate"),   &guide_rate,   (int)floor(SimCamParams::guide_rate * 100.0 / GUIDE_RATE_MAX), 0, 100));
    sizerr->Add(0, 0, 2, wxEXPAND, 5);

    wxBoxSizer *sizerlr = new wxBoxSizer(wxHORIZONTAL);
    sizerlr->Add(sizerl);
    sizerlr->Add(sizerr);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(sizerlr, wxSizerFlags().Border(wxALL).Expand());

    clouds = new wxCheckBox(this, wxID_ANY, _("Clouds"));
    clouds->SetToolTip(_("Simulate clouds blocking stars"));
    clouds->SetValue(SimCamParams::clouds_inten > 0);
    sizer->Add(clouds, wxSizerFlags().Border(wxALL).Expand());

    reverse_dec_pulse_on_west = new wxCheckBox(this, wxID_ANY, _("Reverse Dec pulse on West side of pier"));
    reverse_dec_pulse_on_west->SetToolTip(_("Simulate a mount that reverses guide pulse direction after a meridian flip, like an ASCOM pulse-guided mount."));
    reverse_dec_pulse_on_west->SetValue(SimCamParams::reverse_dec_pulse_on_west_side);
    sizer->Add(reverse_dec_pulse_on_west, wxSizerFlags().Border(wxALL).Expand());

    wxBoxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);
    sizer1->Add(new wxButton(this, wxID_CONVERT, _("Pier Flip")), wxSizerFlags().Border(wxALL).Expand());
    pier_side = SimCamParams::pier_side;

    pier_side_label = new wxStaticText(this, wxID_ANY, _("Side of Pier: MMMMM"));
    sizer1->Add(pier_side_label, wxSizerFlags().Border(wxALL).Expand());

    sizer->Add(sizer1);

    wxButton *btn = new wxButton(this, wxID_RESET, _("Reset"));
    btn->SetToolTip(_("Reset all values to application defaults"));
    sizer->Add(btn, wxSizerFlags().Border(wxALL));

    wxBoxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(sizer, wxSizerFlags(0).Expand());
    main_sizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), wxSizerFlags(0).Expand().Border(wxALL, 10));
    SetSizerAndFit(main_sizer);

    UpdatePierSideLabel();
}

void SimCamDialog::OnReset(wxCommandEvent& event)
{
    stars->SetValue(NR_STARS_DEFAULT);
    hotpx->SetValue(NR_HOT_PIXELS_DEFAULT);
    noise->SetValue((int)floor(NOISE_DEFAULT * 100.0 / NOISE_MAX));
    dec_backlash->SetValue((int)floor(DEC_BACKLASH_DEFAULT * 100.0 / DEC_BACKLASH_MAX));
    pe->SetValue((int)floor(PE_DEFAULT * 100.0 / PE_MAX));
    dec_drift->SetValue((int)floor(DEC_DRIFT_DEFAULT * 100.0 / DEC_DRIFT_MAX));
    seeing->SetValue((int)floor(SEEING_DEFAULT * 100.0 / SEEING_MAX));
    cam_angle->SetValue((int)floor(CAM_ANGLE_DEFAULT + 0.5));
    guide_rate->SetValue((int)floor(GUIDE_RATE_DEFAULT * 100.0 / GUIDE_RATE_MAX));
    reverse_dec_pulse_on_west->SetValue(REVERSE_DEC_PULSE_ON_WEST_SIDE_DEFAULT);
    pier_side = PIER_SIDE_DEFAULT;
    UpdatePierSideLabel();
    clouds->SetValue(false);
}

void SimCamDialog::OnPierFlip(wxCommandEvent& event)
{
    int angle = cam_angle->GetValue();
    angle += 180;
    if (angle >= 360)
        angle -= 360;
    cam_angle->SetValue(angle);
    pier_side = OtherSide(pier_side);
    UpdatePierSideLabel();
}

void SimCamDialog::UpdatePierSideLabel()
{
    pier_side_label->SetLabel(wxString::Format("Side of pier: %s", pier_side == PIER_SIDE_EAST ? _("East") : _("West")));
}

void Camera_SimClass::ShowPropertyDialog()
{
    SimCamDialog dlg(pFrame);
    if (dlg.ShowModal() == wxID_OK)
    {
        SimCamParams::nr_stars = dlg.stars->GetValue();
        SimCamParams::nr_hot_pixels = dlg.hotpx->GetValue();
        SimCamParams::noise_multiplier = (double) dlg.noise->GetValue() * NOISE_MAX / 100.0;
        SimCamParams::dec_backlash =     (double) dlg.dec_backlash->GetValue() * DEC_BACKLASH_MAX / 100.0;
        SimCamParams::pe_scale =         (double) dlg.pe->GetValue() * PE_MAX / 100.0;
        SimCamParams::dec_drift_rate =   (double) dlg.dec_drift->GetValue() * DEC_DRIFT_MAX / 100.0;
        SimCamParams::seeing_scale =     (double) dlg.seeing->GetValue() * SEEING_MAX / 100.0;
        SimCamParams::cam_angle =        (double) dlg.cam_angle->GetValue();
        SimCamParams::guide_rate =       (double) dlg.guide_rate->GetValue() * GUIDE_RATE_MAX / 100.0;
        SimCamParams::pier_side = dlg.pier_side;
        SimCamParams::reverse_dec_pulse_on_west_side = dlg.reverse_dec_pulse_on_west->GetValue();
        SimCamParams::clouds_inten = dlg.clouds->GetValue() ? CLOUDS_INTEN_DEFAULT : 0;
        save_sim_params();
        sim->Initialize();
    }
}

#endif
