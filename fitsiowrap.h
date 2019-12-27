/*
*  fitsiowrap.h
*  PHD Guiding
*
*  Created by Andy Galasso
*  Copyright (c) 2014 Andy Galasso
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
*    Neither the name of Craig Stark, Stark Labs,
*     Bret McKee, Dad Dog Development, Ltd, nor the names of its
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

#ifndef FITSIOWRAP_INCLUDED
#define FITSIOWRAP_INCLUDED

#include "fitsio.h"

extern int PHD_fits_open_diskfile(fitsfile **fptr, const wxString& filename, int iomode, int *status);
extern int PHD_fits_create_file(fitsfile **fptr, const wxString& filename, bool clobber, int *status);
extern void PHD_fits_close_file(fitsfile *fptr);

class FITSHdrWriter
{
    fitsfile *fptr;
    int *status;

public:

    FITSHdrWriter(fitsfile *fptr_, int *status_) : fptr(fptr_), status(status_) { }

    void write(const char *key, float val, const char *comment) {
        fits_write_key(fptr, TFLOAT, const_cast<char *>(key), &val, const_cast<char *>(comment), status);
    }

    void write(const char *key, unsigned int val, const char *comment) {
        fits_write_key(fptr, TUINT, const_cast<char *>(key), &val, const_cast<char *>(comment), status);
    }

    void write(const char *key, int val, const char *comment) {
        fits_write_key(fptr, TINT, const_cast<char *>(key), &val, const_cast<char *>(comment), status);
    }

    void write(const char *key, const char *val, const char *comment) {
        fits_write_key(fptr, TSTRING, const_cast<char *>(key), const_cast<char *>(val), const_cast<char *>(comment), status);
    }

    void write(const char *key, const wxDateTime& t, const wxDateTime::TimeZone& z, const char *comment) {
        wxString s = t.Format("%Y-%m-%dT%H:%M:%S", z) + wxString::Format(".%03d", t.GetMillisecond(z));
        write(key, (const char *) s.c_str(), comment);
    }
};

#endif
