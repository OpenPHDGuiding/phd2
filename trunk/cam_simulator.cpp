/*
 *  cam_simulator.cpp
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

#ifdef SIMULATOR
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include "cam_simulator.h"

#include <wx/gdicmn.h>
#include <wx/stopwatch.h>

#define SIMMODE 3   // 1=FITS, 2=BMP, 3=Generate

/* simulation parameters for SIMMODE = 3*/
// TODO: some kind of UI to sepcify these at runtime?

struct SimCamParams
{
    static unsigned int width;
    static unsigned int height;
    static unsigned int border;
    static unsigned int nr_stars;
    static unsigned int nr_hot_pixels;
    static double dec_backlash;
    static double pe_scale;
    static double dec_drift_rate;
    static double seeing_scale;
    static double cam_angle;
    static double guide_rate;
};

unsigned int SimCamParams::width = 752;          // simulated camera image width
unsigned int SimCamParams::height = 580;         // simulated camera image height
unsigned int SimCamParams::border = 12;          // do not place any stars within this size border
unsigned int SimCamParams::nr_stars = 20;        // number of stars to generate
unsigned int SimCamParams::nr_hot_pixels = 8;    // number of hot pixels to generate
double SimCamParams::dec_backlash = 11.0;         // dec backlash amount (pixels)
double SimCamParams::pe_scale = 3.5;             // scale factor controlling magnitude of simulated periodic error
double SimCamParams::dec_drift_rate = 4.8 / 60.; // dec drift rate (pixels per second)
double SimCamParams::seeing_scale = 0.4;         // simulated seeing scale factor
double SimCamParams::cam_angle = 15.0;           // simulated camera angle (degrees)
double SimCamParams::guide_rate = 3.5;           // guide rate, pixels per second

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

    SimCamState();
    void FillImage(usImage& img, const wxRect& subframe, int exptime, int gain, int offset);
};

SimCamState::SimCamState()
{
    width = SimCamParams::width;
    height = SimCamParams::height;
    // generate stars at random positions but no closer than 12 pixels from any edge
    unsigned int const nr_stars = SimCamParams::nr_stars;
    stars.resize(nr_stars);
    unsigned int const border = SimCamParams::border;
    srand(2); // always generate the same stars
    for (unsigned int i = 0; i < nr_stars; i++) {
        stars[i].pos.x = border + (rand() % (width - 2 * border));
        stars[i].pos.y = border + (rand() % (height - 2 * border));
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
    double const p = 2 * PI * v;
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
        pe += amp[i] * cos((now - phase[i]) / period[i] * 2. * PI);
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
    double const angle = SimCamParams::cam_angle * PI / 180.;
    double const cos_t = cos(angle);
    double const sin_t = sin(angle);
    for (unsigned int i = 0; i < nr_stars; i++) {
        cc[i].x = pos[i].x * cos_t - pos[i].y * sin_t;
        cc[i].y = pos[i].x * sin_t + pos[i].y * cos_t;
    }

#ifdef STEPGUIDER_SIMULATOR
    // add-in AO offset
    if (s_sim_ao) {
        double const ao_angle = SimAoParams::angle * PI / 180.;
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
                stars[i].inten * exptime * gain + (int)((double) gain / 10.0 * (float) offset * (float) exptime / 100.0 + (rand() % (gain * 100)));
            render_star(img, subframe, cc[i], newval);
        }
    }

    // render hot pixels
    for (unsigned int i = 0; i < hotpx.size(); i++)
        if (subframe.Contains(hotpx[i]))
            set_pixel(img, hotpx[i].x, hotpx[i].y, (unsigned short) -1);
}

Camera_SimClass::Camera_SimClass()
    : sim(new SimCamState())
{
    Connected = FALSE;
//  HaveBPMap = FALSE;
//  NBadPixels=-1;
//  ConnectedModel = 1;
    Name=_T("Simulator");
    FullSize = wxSize(640,480);
    HasGuiderOutput = true;
    HasShutter = true;
    HasGainControl = true;
    HasSubframes = true;
}

bool Camera_SimClass::Connect() {
//  wxMessageBox(wxGetCwd());
    Connected = TRUE;
    return false;
}

bool Camera_SimClass::Disconnect() {
    Connected = FALSE;
    return false;
}

Camera_SimClass::~Camera_SimClass()
{
    delete sim;
}

#include "fitsio.h"

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
            *p = (unsigned short) ((double) gain / 10.0 * offset * exptime / 100.0 + (rand() % (gain * 100)));
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

    if (HaveDark && recon)
        Subtract(img, CurrentDarkFrame);

    long long now = wxGetUTCTimeMillis().GetValue();
    if (now < start + duration)
        wxMilliSleep(start + duration - now);

    return false;
}

bool Camera_SimClass::PulseGuideScope(int direction, int duration)
{
    double d = SimCamParams::guide_rate * duration / 1000.0;

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

#endif
