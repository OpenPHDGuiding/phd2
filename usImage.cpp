/*
 *  usimage.cpp
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

#include <algorithm>

bool usImage::Init(const wxSize& size)
{
    // Allocates space for image and sets params up
    // returns true on error

    unsigned int prev = NPixels;
    NPixels = size.GetWidth() * size.GetHeight();
    Size = size;
    Subframe = wxRect(0, 0, 0, 0);
    MinADU = MaxADU = MedianADU = 0;

    if (NPixels != prev)
    {
        delete[] ImageData;

        if (NPixels)
        {
            ImageData = new unsigned short[NPixels];
            if (!ImageData)
            {
                NPixels = 0;
                return true;
            }
        }
        else
            ImageData = nullptr;
    }

    return false;
}

void usImage::SwapImageData(usImage& other)
{
    unsigned short *t = ImageData;
    ImageData = other.ImageData;
    other.ImageData = t;
}

void usImage::CalcStats()
{
    if (!ImageData || !NPixels)
        return;

    MinADU = 65535; MaxADU = 0;
    FiltMin = 65535; FiltMax = 0;

    if (Subframe.IsEmpty())
    {
        // full frame, no subframe

        for (unsigned int i = 0; i < NPixels; i++)
        {
            unsigned short d = ImageData[i];
            if (d < MinADU) MinADU = d;
            if (d > MaxADU) MaxADU = d;
        }

        unsigned short *tmpdata = new unsigned short[NPixels];

        Median3(tmpdata, ImageData, Size, wxRect(Size));

        const unsigned short *src = tmpdata;
        for (unsigned int i = 0; i < NPixels; i++)
        {
            unsigned short d = *src++;
            if (d < FiltMin) FiltMin = d;
            if (d > FiltMax) FiltMax = d;
        }

        memcpy(tmpdata, ImageData, NPixels * sizeof(unsigned short));
        std::nth_element(tmpdata, tmpdata + NPixels / 2, tmpdata + NPixels);
        MedianADU = tmpdata[NPixels / 2];

        delete[] tmpdata;
    }
    else
    {
        // Subframe

        unsigned int pixcnt = Subframe.width * Subframe.height;
        unsigned short *tmpdata = new unsigned short[pixcnt];

        unsigned short *dst;

        dst = tmpdata;
        for (int y = 0; y < Subframe.height; y++)
        {
            const unsigned short *src = ImageData + Subframe.x + (Subframe.y + y) * Size.GetWidth();
            for (int x = 0; x < Subframe.width; x++)
            {
                unsigned short d = *src;
                if (d < MinADU) MinADU = d;
                if (d > MaxADU) MaxADU = d;
                *dst++ = *src++;
            }
        }

        dst = new unsigned short[pixcnt];

        Median3(dst, tmpdata, Subframe.GetSize(), wxRect(Subframe.GetSize()));

        const unsigned short *src = dst;
        for (unsigned int i = 0; i < pixcnt; i++)
        {
            unsigned short d = *src++;
            if (d < FiltMin) FiltMin = d;
            if (d > FiltMax) FiltMax = d;
        }

        memcpy(dst, tmpdata, pixcnt * sizeof(unsigned short));
        std::nth_element(dst, dst + pixcnt / 2, dst + pixcnt);
        MedianADU = dst[pixcnt / 2];

        delete[] dst;
        delete[] tmpdata;
    }
}

static unsigned char * buildGammaLookupTable(int blevel, int wlevel, double power)
{
    unsigned char * result = new unsigned char[0x10000];

    if (blevel < 0) blevel = 0;
    if (wlevel < 0) wlevel = 0;
    if (blevel > 0xffff) blevel = 0xffff;
    if (wlevel > 0xffff) blevel = 0xffff;

    for(int i = 0; i <= blevel; ++i) {
        result[i] = 0;
    }

    float range = wlevel - blevel;
    for(int i = blevel + 1; i < wlevel; ++i) {
        float d = (i - blevel) / range;
        result[i] = pow(d, (float) power) * 255.0;
    }

    for(int i = wlevel; i < 0x10000; ++i) {
        result[i] = 255;
    }

    return result;
}

bool usImage::CopyToImage(wxImage **rawimg, int blevel, int wlevel, double power)
{
    wxImage *img = *rawimg;

    if (!img || !img->Ok() || (img->GetWidth() != Size.GetWidth()) || (img->GetHeight() != Size.GetHeight()) ) // can't reuse bitmap
    {
        delete img;
        img = new wxImage(Size.GetWidth(), Size.GetHeight(), false);
    }

    unsigned char *ImgPtr = img->GetData();
    unsigned short *RawPtr = ImageData;

    unsigned char * lutTable = buildGammaLookupTable(blevel, wlevel, power);

    for (unsigned int i = 0; i < NPixels; i++, RawPtr++ )
    {
        unsigned short v = *RawPtr;
        unsigned char d = lutTable[v];
        *ImgPtr++ = d;
        *ImgPtr++ = d;
        *ImgPtr++ = d;
    }

    delete(lutTable);

    *rawimg = img;
    return false;
}

bool usImage::BinnedCopyToImage(wxImage **rawimg, int blevel, int wlevel, double power)
{
    wxImage *img;
    unsigned char *ImgPtr;
    unsigned short *RawPtr;
    int x,y;
    float d;
    //, s_factor;
    int full_xsize, full_ysize;
    int use_xsize, use_ysize;

    full_xsize = Size.GetWidth();
    full_ysize = Size.GetHeight();
    use_xsize = full_xsize;
    use_ysize = full_ysize;
    if (use_xsize % 2) use_xsize--;
    if (use_ysize % 2) use_ysize--;
//  Binsize = 2;

    img = *rawimg;
    if ((!img->Ok()) || (img->GetWidth() != (full_xsize/2)) || (img->GetHeight() != (full_ysize/2)) ) {  // can't reuse bitmap
        delete img;  // Clear out current image if it exists
        img = new wxImage(full_xsize/2, full_ysize/2, false);
    }
    ImgPtr = img->GetData();
    RawPtr = ImageData;

    unsigned char * lutTable = buildGammaLookupTable(blevel, wlevel, power);

    for (y=0; y<use_ysize; y+=2) {
        for (x=0; x<use_xsize; x+=2) {
            RawPtr = ImageData + x + y*full_xsize;
            int v = (*RawPtr + *(RawPtr+1) + *(RawPtr+full_xsize) + *(RawPtr+1+full_xsize)) / 4;

            unsigned char d = lutTable[v];
            *ImgPtr++ = d;
            *ImgPtr++ = d;
            *ImgPtr++ = d;
        }
    }

    delete lutTable;
    *rawimg = img;
    return false;
}

void usImage::InitImgStartTime()
{
    ImgStartTime = wxDateTime::UNow();
}

bool usImage::Save(const wxString& fname, const wxString& hdrNote) const
{
    bool bError = false;

    try
    {
        fitsfile *fptr;  // FITS file pointer
        int status = 0;  // CFITSIO status value MUST be initialized to zero!

        PHD_fits_create_file(&fptr, fname, true, &status);

        long fsize[] = {
            (long) Size.GetWidth(),
            (long) Size.GetHeight(),
        };
        fits_create_img(fptr, USHORT_IMG, 2, fsize, &status);

        FITSHdrWriter hdr(fptr, &status);

        float exposure = (float) ImgExpDur / 1000.0;
        hdr.write("EXPOSURE", exposure, "Exposure time in seconds");

        if (ImgStackCnt > 1)
            hdr.write("STACKCNT", (unsigned int) ImgStackCnt, "Stacked frame count");

        if (!hdrNote.IsEmpty())
            hdr.write("USERNOTE", hdrNote.utf8_str(), 0);

        hdr.write("DATE", wxDateTime::UNow(), wxDateTime::UTC, "file creation time, UTC");
        hdr.write("DATE-OBS", ImgStartTime, wxDateTime::UTC, "Image capture start time, UTC");
        hdr.write("CREATOR", wxString(APPNAME _T(" ") FULLVER).c_str(), "Capture software");
        hdr.write("PHDPROFI", pConfig->GetCurrentProfile().c_str(), "PHD2 Equipment Profile");

        if (pCamera)
        {
            hdr.write("INSTRUME", pCamera->Name.c_str(), "Instrument name");
            unsigned int b = pCamera->Binning;
            hdr.write("XBINNING", b, "Camera X Bin");
            hdr.write("YBINNING", b, "Camera Y Bin");
            hdr.write("CCDXBIN", b, "Camera X Bin");
            hdr.write("CCDYBIN", b, "Camera Y Bin");
            float sz = b * pCamera->GetCameraPixelSize();
            hdr.write("XPIXSZ", sz, "pixel size in microns (with binning)");
            hdr.write("YPIXSZ", sz, "pixel size in microns (with binning)");
            unsigned int g = (unsigned int) pCamera->GuideCameraGain;
            hdr.write("GAIN", g, "PHD Gain Value (0-100)");
            unsigned int bpp = pCamera->BitsPerPixel();
            hdr.write("CAMBPP", bpp, "Camera resolution, bits per pixel");
        }

        if (pPointingSource)
        {
            double ra, dec, st;
            bool err = pPointingSource->GetCoordinates(&ra, &dec, &st);
            if (!err)
            {
                hdr.write("RA", (float) (ra * 360.0 / 24.0), "Object Right Ascension in degrees");
                hdr.write("DEC", (float) dec, "Object Declination in degrees");

                {
                    int h = (int) ra;
                    ra -= h;
                    ra *= 60.0;
                    int m = (int) ra;
                    ra -= m;
                    ra *= 60.0;
                    hdr.write("OBJCTRA", wxString::Format("%02d %02d %06.3f", h, m, ra).c_str(), "Object Right Ascension in hms");
                }

                {
                    int sign = dec < 0.0 ? -1 : +1;
                    dec *= sign;
                    int d = (int) dec;
                    dec -= d;
                    dec *= 60.0;
                    int m = (int) dec;
                    dec -= m;
                    dec *= 60.0;
                    hdr.write("OBJCTDEC", wxString::Format("%c%d %02d %06.3f", sign < 0 ? '-' : '+', d, m, dec).c_str(), "Object Declination in dms");
                }
            }

            PierSide p = pPointingSource->SideOfPier();
            if (p != PierSide::PIER_SIDE_UNKNOWN)
                hdr.write("PIERSIDE", (unsigned int) p, "Side of Pier 0=East 1=West");
        }

        float sc = (float) pFrame->GetCameraPixelScale();
        hdr.write("SCALE", sc, "Image scale (arcsec / pixel)");
        hdr.write("PIXSCALE", sc, "Image scale (arcsec / pixel)");
        hdr.write("PEDESTAL", (unsigned int) Pedestal, "dark subtraction bias value");
        hdr.write("SATURATE", (1U << BitsPerPixel) - 1, "Data value at which saturation occurs");

        const PHD_Point& lockPos = pFrame->pGuider->LockPosition();
        if (lockPos.IsValid())
        {
            hdr.write("PHDLOCKX", (float) lockPos.X, "PHD2 lock position x");
            hdr.write("PHDLOCKY", (float) lockPos.Y, "PHD2 lock position y");
        }

        if (!Subframe.IsEmpty())
        {
            hdr.write("PHDSUBFX", (unsigned int) Subframe.x, "PHD2 subframe x");
            hdr.write("PHDSUBFY", (unsigned int) Subframe.y, "PHD2 subframe y");
            hdr.write("PHDSUBFW", (unsigned int) Subframe.width, "PHD2 subframe width");
            hdr.write("PHDSUBFH", (unsigned int) Subframe.height, "PHD2 subframe height");
        }

        long fpixel[3] = { 1, 1, 1 };
        fits_write_pix(fptr, TUSHORT, fpixel, NPixels, ImageData, &status);

        PHD_fits_close_file(fptr);

        bError = status ? true : false;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

static bool fhdr_int(fitsfile *fptr, const char *key, int *val)
{
    char *k = const_cast<char *>(key);
    int status = 0;
    fits_read_key(fptr, TINT, k, val, nullptr, &status);
    return status == 0;
}

bool usImage::Load(const wxString& fname)
{
    bool bError = false;

    try
    {
        if (!wxFileExists(fname))
        {
            pFrame->Alert(_("File does not exist - cannot load ") + fname);
            throw ERROR_INFO("File does not exist");
        }

        int status = 0;  // CFITSIO status value MUST be initialized to zero!
        fitsfile *fptr;  // FITS file pointer
        if (!PHD_fits_open_diskfile(&fptr, fname, READONLY, &status))
        {
            int hdutype;
            if (fits_get_hdu_type(fptr, &hdutype, &status) || hdutype != IMAGE_HDU)
            {
                pFrame->Alert(_("FITS file is not of an image: ") + fname);
                throw ERROR_INFO("Fits file is not an image");
            }

            // Get HDUs and size
            int naxis = 0;
            fits_get_img_dim(fptr, &naxis, &status);
            long fsize[3];
            fits_get_img_size(fptr, 2, fsize, &status);
            int nhdus = 0;
            fits_get_num_hdus(fptr, &nhdus, &status);
            if ((nhdus != 1) || (naxis != 2)) {
                pFrame->Alert(wxString::Format(_("Unsupported type or read error loading FITS file %s"), fname));
                throw ERROR_INFO("unsupported type");
            }
            if (Init((int) fsize[0], (int) fsize[1]))
            {
                pFrame->Alert(wxString::Format(_("Memory allocation error loading FITS file %s"), fname));
                throw ERROR_INFO("Memory Allocation failure");
            }
            long fpixel[3] = { 1, 1, 1 };
            if (fits_read_pix(fptr, TUSHORT, fpixel, (int)(fsize[0] * fsize[1]), nullptr, ImageData, nullptr, &status)) { // Read image
                pFrame->Alert(wxString::Format(_("Error reading data from FITS file %s"), fname));
                throw ERROR_INFO("Error reading");
            }

            char *key = const_cast<char *>("EXPOSURE");
            float exposure;
            status = 0;
            fits_read_key(fptr, TFLOAT, key, &exposure, nullptr, &status);
            if (status == 0)
                ImgExpDur = (int) (exposure * 1000.0);

            int stackcnt;
            if (fhdr_int(fptr, "STACKCNT", &stackcnt))
                ImgStackCnt = stackcnt;

            int pedestal;
            if (fhdr_int(fptr, "PEDESTAL", &pedestal))
              Pedestal = (unsigned short) pedestal;

            int saturate;
            if (fhdr_int(fptr, "SATURATE", &saturate))
                BitsPerPixel = saturate > 255 ? 16 : 8;

            wxRect subf;
            bool ok = fhdr_int(fptr, "PHDSUBFX", &subf.x);
            if (ok) ok = fhdr_int(fptr, "PHDSUBFY", &subf.y);
            if (ok) ok = fhdr_int(fptr, "PHDSUBFW", &subf.width);
            if (ok) ok = fhdr_int(fptr, "PHDSUBFH", &subf.height);
            if (ok) Subframe = subf;

            PHD_fits_close_file(fptr);

            CalcStats();
        }
        else
        {
            pFrame->Alert(wxString::Format(_("Error opening FITS file %s"), fname));
            throw ERROR_INFO("error opening file");
        }
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        bError = true;
    }

    return bError;
}

bool usImage::CopyFrom(const usImage& src)
{
    if (Init(src.Size))
        return true;
    memcpy(ImageData, src.ImageData, NPixels * sizeof(unsigned short));
    return false;
}

bool usImage::Rotate(double theta, bool mirror)
{
    wxImage *pImg = 0;

    CalcStats();

    CopyToImage(&pImg, MinADU, MaxADU, 1.0);

    wxImage mirrored = *pImg;

    if (mirror)
    {
        mirrored = pImg->Mirror(false);
    }

    wxImage rotated = mirrored.Rotate(theta, wxPoint(0,0));

    CopyFromImage(rotated);

    delete pImg;

    return false;
}

bool usImage::CopyFromImage(const wxImage& img)
{
    Init(img.GetSize());

    const unsigned char *pSrc = img.GetData();
    unsigned short *pDest = ImageData;

    for (unsigned int i = 0; i < NPixels; i++)
    {
        *pDest++ = ((unsigned short) *pSrc) << 8;
        pSrc += 3;
    }

    return false;
}
