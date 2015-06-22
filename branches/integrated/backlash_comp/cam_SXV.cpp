/*
 *  cam_SXV.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2008-2010 Craig Stark.
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
#if defined (SXV)
#include "camera.h"
#include "time.h"
#include "image_math.h"
#include <wx/choicdlg.h>

#include "cam_SXV.h"
extern Camera_SXVClass Camera_SXV;

static wxString NameFromModel(int model)
{
    wxString m;

    switch (model)
    {
        case 0x05: m = "SX-H5"; break;
        case 0x85: m = "SX-H5C"; break;
        case 0x09: m = "SX-H9"; break;
        case 0x89: m = "SX-H9C"; break;
        case 0x39: m = "SX-LS9"; break;
        case 0x19: m = "SX-SX9"; break;
        case 0x99: m = "SX-SX9C"; break;
        case 0x10: m = "SX-H16"; break;
        case 0x90: m = "SX-H16C"; break;
        case 0x11: m = "SX-H17"; break;
        case 0x91: m = "SX-H17C"; break;
        case 0x12: m = "SX-H18"; break;
        case 0x92: m = "SX-H18C"; break;
        case 0x23: m = "SX-H35"; break;
        case 0xB3: m = "SX-H35C"; break;
        case 0x24: m = "SX-H36"; break;
        case 0xB4: m = "SX-H36C"; break;
        case 0x56: m = "SX-H674"; break;
        case 0xB6: m = "SX-H674C"; break;
        case 0x57: m = "SX-H694"; break;
        case 0xB7: m = "SX-H694C"; break;
        case 0x28: m = "SX-H814"; break;
        case 0xA8: m = "SX-H814C"; break;
        case 0x29: m = "SX-H834"; break;
        case 0xA9: m = "SX-H834C"; break;
        case 0x3B: m = "SX-H825"; break;
        case 0xBB: m = "SX-H825C"; break;
        case 0x3C: m = "SX-US825"; break;
        case 0xBC: m = "SX-US825C"; break;
            // interlaced models
        case 0x45: m = "SX-MX5"; break;
        case 0x84: m = "SX-MX5C"; break;
        case 0x46: m = "SX-LX1"; break;
        case 0x47: m = "SX-MX7"; break;
        case 0xC7: m = "SX-MX7C"; break;
        case 0x48: m = "SX-MX8"; break;
        case 0xC8: m = "SX-MX8C"; break;
        case 0x49: m = "SX-MX9"; break;
        case 0x59: m = "SX-M25"; break;
        case 0x5A: m = "SX-M26"; break;
            // development models
        case 0x0C: m = "SX-DEV1"; break;
        case 0x0D: m = "SX-DEV2"; break;
        case 0x0E: m = "SX-DEV3"; break;
        case 0x0F: m = "SX-DEV4"; break;

        default: m = wxString::Format("SX Camera Model %d", model); break;
    }

    if (model == 70)
        m = _T("SXV-Lodestar");
    else if (model == 39)
        m = _T("SX CMOS Guider");
    else if (model == 0x39)
        m = _T("SX Superstar guider");

    return m;
}

Camera_SXVClass::Camera_SXVClass()
{
    Connected = false;
    Name = _T("Starlight Xpress SXV");
    FullSize = m_darkFrameSize = wxSize(1280, 1024);
    HasGainControl = false;
    m_hasGuideOutput = true;
    Interlaced = false;
    RawData = NULL;
    HasSubframes = true;
    PropertyDialogType = PROPDLG_WHEN_DISCONNECTED;
    SquarePixels = pConfig->Profile.GetBoolean("/camera/SXV/SquarePixels", false);
}

class SXCameraDlg : public wxDialog
{
public:
    wxCheckBox* m_squarePixels;

    SXCameraDlg(wxWindow *parent, wxWindowID id = wxID_ANY, const wxString& title = _("SX Camera Settings"),
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(268, 133), long style = wxDEFAULT_DIALOG_STYLE ); 
    ~SXCameraDlg() { }
};

SXCameraDlg::SXCameraDlg(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    SetSizeHints(wxDefaultSize, wxDefaultSize);

    wxBoxSizer *bSizer12 = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("Settings")), wxVERTICAL);

    m_squarePixels = new wxCheckBox( this, wxID_ANY, wxT("Square Pixels"), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizer3->Add( m_squarePixels, 0, wxALL, 5 );
    bSizer12->Add( sbSizer3, 1, wxEXPAND, 5 );

    wxStdDialogButtonSizer* sdbSizer2 = new wxStdDialogButtonSizer();
    wxButton *sdbSizer2OK = new wxButton(this, wxID_OK);
    wxButton* sdbSizer2Cancel = new wxButton(this, wxID_CANCEL);
    sdbSizer2->AddButton(sdbSizer2OK);
    sdbSizer2->AddButton(sdbSizer2Cancel);
    sdbSizer2->Realize();
    bSizer12->Add(sdbSizer2, 0, wxALL | wxEXPAND, 5);

    SetSizer(bSizer12);
    Layout();

    Centre(wxBOTH);
}

void Camera_SXVClass::ShowPropertyDialog()
{
    SXCameraDlg dlg(wxGetApp().GetTopWindow());
    dlg.m_squarePixels->SetValue(SquarePixels);
    if (dlg.ShowModal() == wxID_OK)
    {
        SquarePixels = dlg.m_squarePixels->GetValue();
        pConfig->Profile.SetBoolean("/camera/SXV/SquarePixels", SquarePixels);
    }
}

#if defined (__APPLE__)

int SXCamAttached (void *cam)
{
    // This should return 1 if the cam passed in here is considered opened, 0 otherwise
    //wxMessageBox(wxString::Format("Found SX cam model %d", (int) sxGetCameraModel(cam)));
    //SXVCamera.hCam = cam;
    return 0;
}

void SXCamRemoved (void *cam)
{
    //  CameraPresent = false;
    //SXVCamera.hCam = NULL;
}

#endif

bool Camera_SXVClass::Connect()
{
    // returns true on error

    bool retval = true;

#if defined(__WINDOWS__)

    HANDLE hCams[SXCCD_MAX_CAMS];

    int ncams = sxOpen(hCams);
    if (ncams == 0)
        return true;  // No cameras

    // Dialog to choose which Cam if # > 1  (note 0-indexed)
    if (ncams > 1)
    {
        wxArrayString Names;
        for (int i = 0; i < ncams; i++)
        {
            unsigned short model = sxGetCameraModel(hCams[i]);
            Names.Add(NameFromModel(model));
        }
        int i = wxGetSingleChoiceIndex(_("Select SX camera"), _("Camera choice"), Names);
        if (i == -1)
            return true;
        hCam = hCams[i];
    }
    else
        hCam = hCams[0];

#else  // OSX

    hCam = NULL;

    /*
    // Orig version
    static bool ProbeLoaded = false;
    if (!ProbeLoaded)
        sxProbe(SXCamAttached, SXCamRemoved);
    ProbeLoaded = true;
    int i, model;
    wxString tmp_name;
    wxArrayString Names;
    int ncams = 0;
    int portstatus;
    portstatus = sxCamPortStatus(0);
    portstatus = sxCamPortStatus(1);

    for (i=0; i<4; i++) {
        model = sxCamAvailable(i);
        if (model) {
            ncams++;
            tmp_name=wxString::Format("%d: SXV-%c%d%c",i,model & 0x40 ? 'M' : 'H', model & 0x1F, model & 0x80 ? 'C' : '\0');
            if (model == 70)
                tmp_name = wxString::Format("%d: SXV-Lodestar",i);
            Names.Add(tmp_name);
        }
    }
    if (ncams > 1) {
        wxString ChoiceString;
        ChoiceString=wxGetSingleChoice(_("Select SX camera"),_("Camera choice"),Names);
        if (ChoiceString.IsEmpty()) return true;
        ChoiceString=ChoiceString.Left(1);
        long lval;
        ChoiceString.ToLong(&lval);
        hCam = sxOpen((int) lval);
        sxReleaseOthers((int) lval);
    }
    else
        hCam = sxOpen(-1);
    portstatus = sxCamPortStatus(0);
    portstatus = sxCamPortStatus(1);
    */

     // New version
    int ncams = sx2EnumDevices();
    if (!ncams)
    {
        wxMessageBox(_T("No SX cameras found"), _("Error"));
        return true;
    }
    if (ncams > 1)
    {
        int i, model;
        wxString tmp_name;
        wxArrayString Names;
        void      *htmpCam;
        char devname[32];
        for (i=0; i<ncams; i++)
        {
    /*      htmpCam = sx2Open(i);
            if (htmpCam) {
                model = sxGetCameraModel(htmpCam);
                tmp_name = wxString::Format("%d: %d",i+1,model);
                Names.Add(tmp_name);
                wxMilliSleep(500);
                sx2Close(htmpCam);
                wxMilliSleep(500);
                htmpCam = NULL;
            }*/
            model = (int) sx2GetID(i);
            if (model)
            {
                sx2GetName(i,devname);
                tmp_name = wxString::Format("%d: %s",i+1,devname);
                Names.Add(tmp_name);
            }
        }

        wxString ChoiceString = wxGetSingleChoice(_("Select SX camera"), _("Camera choice"), Names);
        if (ChoiceString.IsEmpty())
            return true;
        ChoiceString = ChoiceString.Left(1);
        long lval;
        ChoiceString.ToLong(&lval);
        lval -= 1;
        hCam = sx2Open((int) lval);
    }
    else
        hCam = sx2Open(0);

    if (hCam == NULL)
        return true;
#endif

    retval = false;  // assume all good

    // Close all unused cameras if nCams > 1 after picking which one
    //  WRITE

    // Load parameters
    sxGetCameraParams(hCam, 0, &CCDParams);

    if (CCDParams.width == 0 || CCDParams.height == 0)
    {
        pFrame->Alert(_("Connect failed: could not retrieve camera parameters."));
        return true;
    }

    // deal with what if no porch in there ??
    // WRITE??

    CameraModel = sxGetCameraModel(hCam);

    Name = NameFromModel(CameraModel);

    if (CameraModel & 0x80)  // color
        ColorSensor = true;
    else
        ColorSensor = false;

    if (CameraModel & 0x40)
        Interlaced = true;
    else
        Interlaced = false;

    SubType = CameraModel & 0x1F;
    if (SubType == 25)
        Interlaced = false;

    if (Interlaced)
    {
        // The interlaced CCDs report the size of a field for the height
        m_darkFrameSize = wxSize(CCDParams.width, CCDParams.height);
        if (SquarePixels)
        {
            FullSize.SetWidth(CCDParams.width);
            // This is the height after squaring pixels.
            FullSize.SetHeight((int)floor((float)CCDParams.height * CCDParams.pix_height / CCDParams.pix_width));
            PixelSize = CCDParams.pix_height / 2.0;
        }
        else
        {
            FullSize = wxSize(CCDParams.width, CCDParams.height * 2);
            PixelSize = std::min(CCDParams.pix_width, CCDParams.pix_height / 2.f);
        }
    }
    else
    {
        FullSize = wxSize(CCDParams.width, CCDParams.height);
        PixelSize = std::min(CCDParams.pix_width, CCDParams.pix_height);
        m_darkFrameSize = FullSize;
    }

    if (CCDParams.extra_caps & 0x20)
        HasShutter = true;

    if (CameraModel == 39) // cmos guider
        HasSubframes = false;

    RawData = new unsigned short[CCDParams.width * CCDParams.height];

    if (tmpImg.Init(CCDParams.width, CCDParams.height))
    {
        Debug.AddLine("SX camera: tmpImg Init failed!");
        delete [] RawData;
        RawData = 0;
        retval = true;
    }

    Debug.AddLine("SX Camera: " + Name);
    Debug.AddLine(wxString::Format("SX Camera Params: %u x %u (reported as %u x %u) PixSz: %.2f x %.2f; #Pix: %u Array color type: %u,%u Interlaced: %d Model: %u, Subype: %u, Porch: %u,%u %u,%u Extras: %u",
            FullSize.GetWidth(), FullSize.GetHeight(), CCDParams.width, CCDParams.height,
            CCDParams.pix_width, CCDParams.pix_height, FullSize.GetHeight() * FullSize.GetWidth(),
            CCDParams.color_matrix, (int) ColorSensor, (int) Interlaced,
            CameraModel, SubType, CCDParams.hfront_porch, CCDParams.hback_porch, CCDParams.vfront_porch, CCDParams.vback_porch,
            CCDParams.extra_caps));

    if (!retval)
        Connected = true;

    return retval;
}

bool Camera_SXVClass::Disconnect()
{
    delete[] RawData;
    RawData = NULL;
    Connected = false;
    sxReset(hCam);

#ifdef __APPLE__
    sx2Close(hCam);
#else
    sxClose(hCam);
#endif

    hCam = NULL;

    return false;
}

static bool InitImgCMOSGuider(usImage& img, const wxSize& FullSize, const unsigned short *raw)
{
    // CMOS guider -- crop and clean

    // Re-assemble image
    int output_xsize = FullSize.GetWidth();
    int output_ysize = FullSize.GetHeight();

    output_xsize -= 16;  // crop off 16 from one side

    if (img.Init(output_xsize, output_ysize))
        return true;

    const unsigned short *rawptr = raw;
    unsigned short *dataptr = img.ImageData;

    for (int y = 0; y < output_ysize; y++)
    {
        int oddbias, evenbias;
        oddbias = evenbias = 0;
        for (int x = 0; x < 16; x += 2) { // Figure the offsets for this line
            oddbias += (int)*rawptr++;
            evenbias += (int)*rawptr++;
        }
        oddbias = oddbias / 8 - 1000;  // Create avg and pre-build in the offset to keep off of the floor
        evenbias = evenbias / 8 - 1000;
        for (int x = 0; x < output_xsize; x += 2) { // Load value into new image array pulling out right bias
            int val = (int)*rawptr++ - oddbias;
            if (val < 0.0) val = 0.0;  //Bounds check
            else if (val > 65535.0) val = 65535.0;
            *dataptr++ = (unsigned short)val;
            val = (int)*rawptr++ - evenbias;
            if (val < 0.0) val = 0.0;  //Bounds check
            else if (val > 65535.0) val = 65535.0;
            *dataptr++ = (unsigned short)val;
        }
    }

    return false;
}

static bool InitImgInterlacedInterp(usImage& img, const wxSize& FullSize, bool subframe, const wxRect& frame, const usImage& tmp)
{
    if (img.Init(FullSize))
        return true;

    if (subframe)
    {
        img.Subframe = frame;
        img.Clear();
    }

    const unsigned short *raw = tmp.ImageData;
    int const fullw = FullSize.GetWidth();
    int const framew = frame.GetWidth();
    int const xofs = frame.GetLeft();

    int end = frame.GetBottom();
    if ((end & 1) && end == FullSize.GetHeight() - 1)
        --end;

    for (int y = frame.GetTop(); y <= end; y++)
    {
        unsigned short *dst = img.ImageData + y * fullw + xofs;
        if ((y & 1) == 0)
        {
            // even row - copy directly
            const unsigned short *src = raw + (y / 2) * fullw + xofs;
            memcpy(dst, src, framew * sizeof(unsigned short));
        }
        else
        {
            // odd row - interpolate
            const unsigned short *src0 = raw + (y / 2) * fullw + xofs;
            const unsigned short *src1 = src0 + fullw;
            for (int x = 0; x < framew; x++)
                *dst++ = (*src0++ + *src1++) / 2;
        }
    }

    if ((frame.GetBottom() & 1) && frame.GetBottom() == FullSize.GetHeight() - 1)
    {
        unsigned short *dst = img.ImageData + frame.GetBottom() * fullw + xofs;
        const unsigned short *src = dst - fullw;
        memcpy(dst, src, framew * sizeof(unsigned short));
    }

    return false;
}

static bool InitImgInterlacedSquare(usImage& img, const wxSize& FullSize, bool subframe, const wxRect& frame,
                                    const sxccd_params_t& ccdparams, const usImage& tmp)
{
    // pixels are vertically binned. resample to create square, un-binned pixels
    //
    //  xsize = number of columns (752)
    //  ysize = number of rows read from camera (290)

    float const pw = ccdparams.pix_width;   // 8.5
    float const ph = ccdparams.pix_height;  // reported value is for the binned pixel (16.5)
    float const r0 = pw / ph;

    if (img.Init(FullSize))
        return true;

    if (subframe)
    {
        img.Subframe = frame;
        img.Clear();
    }

    const unsigned short *raw = tmp.ImageData;
    int const fullw = FullSize.GetWidth();
    int const framew = frame.GetWidth();
    int const xofs = frame.GetLeft();

    float y0 = frame.GetTop() * pw;
    float y1 = y0 + pw;
    int p0 = (int) floor(y0 / ph);

    for (int row = frame.GetTop(); row <= frame.GetBottom(); row++)
    {
        float yp1 = floor(y1 / ph);
        int p1 = (int) yp1;
        yp1 *= ph;

        unsigned short *dst = img.ImageData + row * fullw + xofs;
        if (p1 == p0)
        {
            const unsigned short *src = raw + p0 * fullw + xofs;
            for (int x = 0; x < framew; x++)
                *dst++ = (unsigned short)(r0 * (float) *src++);
        }
        else
        {
            float const r0 = (yp1 - y0) / ph;
            float const r1 = (y1 - yp1) / ph;
            const unsigned short *src0 = raw + p0 * fullw + xofs;
            const unsigned short *src1 = raw + p1 * fullw + xofs;
            for (int x = 0; x < framew; x++)
                *dst++ = (unsigned short)(r0 * (float)*src0++ + r1 * (float)*src1++);
        }

        y0 = y1;
        p0 = p1;
        y1 += pw;
    }

    return false;
}

static bool InitImgProgressive(usImage& img, unsigned int xofs, unsigned int yofs, unsigned int xsize,
    unsigned int ysize, bool subframe, const wxSize& FullSize, const unsigned short *raw)
{
    if (img.Init(FullSize))
        return true;

    if (subframe)
    {
        const unsigned short *src = raw;
        img.Subframe = wxRect(xofs, yofs, xsize, ysize);
        img.Clear();
        for (unsigned int y = 0; y < ysize; y++)
        {
            unsigned short *dst = img.ImageData + (yofs + y) * FullSize.GetWidth() + xofs;
            memcpy(dst, src, xsize * sizeof(unsigned short));
            src += xsize;
        }
    }
    else
    {
        memcpy(img.ImageData, raw, img.NPixels * sizeof(unsigned short));
    }

    return false;
}

#if defined (__WINDOWS__)
# define ReadPixels(hCam, RawData, NPixelsToRead) sxReadPixels((hCam), (RawData), (NPixelsToRead))
#else
# define ReadPixels(hCam, RawData, NPixelsToRead) sxReadPixels((hCam), (UInt8 *)(RawData), (NPixelsToRead), sizeof(unsigned short))
#endif

inline static void swap(unsigned short *&a, unsigned short *&b)
{
    unsigned short *tmp = a;
    a = b;
    b = tmp;
}

bool Camera_SXVClass::Capture(int duration, usImage& img, int options, const wxRect& subframeArg)
{
    bool takeSubframe = UseSubframes;
    wxRect subframe(subframeArg);

    if (subframe.width <= 0 || subframe.height <= 0)
    {
        takeSubframe = false;
    }

    if (HasShutter && ShutterClosed)
    {
        sxSetShutter(hCam, 1);  // Close the shutter if needed
        wxMilliSleep(200);
    }

    unsigned short xofs, yofs;
    unsigned short xsize, ysize;

    if (takeSubframe)
    {
        xofs = subframe.GetLeft();
        xsize = subframe.GetWidth();
        if (Interlaced)
        {
            // Interlaced cams are run in "high speed" mode (vertically binned)

            if (options & CAPTURE_RECON)
            {
                if (SquarePixels)
                {
                    //  incoming subframe coordinates are in squared pixel coordinate system, convert to camera pixel coordinates
                    float r = CCDParams.pix_width / CCDParams.pix_height;
                    unsigned int y0 = (unsigned int)floor(subframe.GetTop() * r);
                    unsigned int y1 = (unsigned int)floor(subframe.GetBottom() * r);
                    yofs = y0;
                    ysize = y1 - y0 + 1;
                }
                else
                {
                    unsigned int y0 = (unsigned int)subframe.GetTop() / 2;
                    // interpolation may require the next row
                    unsigned int y1 = (unsigned int)(subframe.GetBottom() + 1) / 2;
                    if (y1 >= CCDParams.height)
                        y1 = CCDParams.height - 1;
                    yofs = y0;
                    ysize = y1 - y0 + 1;
                }
            }
            else
            {
                tmpImg.Clear();

                // subframe represents actual binned pixels
                yofs = (unsigned int) subframe.GetTop();
                unsigned int y1 = (unsigned int) subframe.GetBottom();
                if (y1 > CCDParams.height - 1)
                    y1 = CCDParams.height - 1;
                ysize = y1 - yofs + 1;
            }
        }
        else
        {
            yofs = subframe.GetTop();
            ysize = subframe.GetHeight();
        }
    }
    else
    {
        subframe = FullSize;
        xofs = 0;
        yofs = 0;
        xsize = CCDParams.width;
        ysize = CCDParams.height;
    }

    bool UseInternalTimer = false;
    if (CameraModel == 39)
        UseInternalTimer = true;

    // Do exposure
    if (UseInternalTimer)
    {
        sxClearPixels(hCam, SXCCD_EXP_FLAGS_NOWIPE_FRAME, 0);
        sxExposePixels(hCam, SXCCD_EXP_FLAGS_FIELD_ODD, 0, xofs, yofs, xsize, ysize, 1, 1, duration);
    }
    else
    {
        sxClearPixels(hCam, 0, 0);
        WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY);
        sxLatchPixels(hCam, SXCCD_EXP_FLAGS_FIELD_BOTH, 0, xofs, yofs, xsize, ysize, 1, 1);
    }

    // do not return without reading pixels or camera will hang
    // if (WorkerThread::InterruptRequested())
    //    return true;

    int NPixelsToRead = xsize * ysize;

    ReadPixels(hCam, RawData, NPixelsToRead);  // stop exposure and read but only the one frame

    if (HasShutter && ShutterClosed)
    {
        sxSetShutter(hCam, 0);  // Open it back up
        wxMilliSleep(200);
    }

    // Re-assemble image

    if (!Interlaced)
    {
        bool error;

        if (CameraModel == 39)
            error = InitImgCMOSGuider(img, FullSize, RawData);
        else
            error = InitImgProgressive(img, xofs, yofs, xsize, ysize, takeSubframe, FullSize, RawData);

        if (error)
        {
            DisconnectWithAlert(CAPT_FAIL_MEMORY);
            return true;
        }

        if (options & CAPTURE_SUBTRACT_DARK) SubtractDark(img);

        return false;
    }

    // interlaced

    // prepare to for dark subtraction by copying the camera frame to the appropriate location in tmpImg

    if (takeSubframe)
    {
        const unsigned short *src = RawData;
        for (int y = 0; y < ysize; y++)
        {
            unsigned short *dst = tmpImg.ImageData + (yofs + y) * FullSize.GetWidth() + xofs;
            memcpy(dst, src, xsize * sizeof(unsigned short));
            src += xsize;
        }
        tmpImg.Subframe = wxRect(xofs, yofs, xsize, ysize);
    }
    else
    {
        swap(RawData, tmpImg.ImageData);
        tmpImg.Subframe = wxRect();
    }

    if (options & CAPTURE_SUBTRACT_DARK)
        SubtractDark(tmpImg);

    if (options & CAPTURE_RECON)
    {
        bool error;

        if (SquarePixels)
            error = InitImgInterlacedSquare(img, FullSize, takeSubframe, subframe, CCDParams, tmpImg);
        else
            error = InitImgInterlacedInterp(img, FullSize, takeSubframe, subframe, tmpImg);

        if (error)
        {
            DisconnectWithAlert(CAPT_FAIL_MEMORY);
            return true;
        }
    }
    else
    {
        if (img.Init(tmpImg.Size))
        {
            DisconnectWithAlert(CAPT_FAIL_MEMORY);
            return true;
        }

        if (takeSubframe)
            img.Subframe = tmpImg.Subframe;

        img.SwapImageData(tmpImg);
    }

    return false;
}

bool Camera_SXVClass::ST4PulseGuideScope(int direction, int duration)
{
    // Guide port values
    // West = 1
    // East = 8
    // North = 2
    // South = 4
    unsigned char dircmd = 0;
    switch (direction) {
        case WEST:
            dircmd = (unsigned char) 1;
            break;
        case EAST:
            dircmd = (unsigned char) 8;
            break;
        case NORTH:
            dircmd = (unsigned char) 2;
            break;
        case SOUTH:
            dircmd = (unsigned char) 4;
            break;
    }
    sxSetSTAR2000(hCam,dircmd);
    WorkerThread::MilliSleep(duration);
    dircmd = 0;
    sxSetSTAR2000(hCam,dircmd);

    return false;
}

#endif // SXV
