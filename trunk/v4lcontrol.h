/*
 *  v4lcontrol.h
 *  PHD Guiding
 *
 *  Created by Steffen Elste
 *  Copyright (c) 2010 Steffen Elste.
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

#ifndef V4LCONTROL_H_INCLUDED
#define V4LCONTROL_H_INCLUDED

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <libv4lconvert.h>

#include <wx/hashmap.h>
#include <wx/arrstr.h>


class V4LControl {
public:
	V4LControl(int fd, const struct v4l2_queryctrl &ctrl);

    bool update();
    bool reset();

    int fd;
    int cid;
    int type;
    int defaultValue;
    int value;
    int min, max, step;

    wxString name;
    wxArrayString choices;

private:
    void enumerateMenuControls(const struct v4l2_queryctrl &ctrl);
};


WX_DECLARE_HASH_MAP(int, V4LControl*, wxIntegerHash, wxIntegerEqual, V4LControlMap);


#endif // V4LCONTROL_H_INCLUDED
