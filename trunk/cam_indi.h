/*
 *  cam_INDI.h
 *  PHD Guiding
 *
 *  Created by Geoffrey Hausheer.
 *  Copyright (c) 2009 Geoffrey Hausheer.
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

#ifndef _CAM_INDI_H_
#define _CAM_INDI_H_

struct indi_t;
struct indi_prop_t;

class Camera_INDIClass : public GuideCamera {
private:
    struct indi_prop_t *expose_prop;
    struct indi_prop_t *frame_prop;
    struct indi_prop_t *frame_type_prop;
    struct indi_prop_t *binning_prop;
    struct indi_prop_t *video_prop;
    int     img_count;
    bool    ready;
    bool    has_blob;
public:
    bool    modal;
    wxString indi_name;
    wxString indi_port;
    bool    is_connected;

    bool    ReadFITS(usImage& img);
    bool    ReadStream(usImage& img);
    struct  indi_elem_t *blob_elem;
    virtual bool    Capture(int duration, usImage& img, wxRect subframe = wxRect(0,0,0,0), bool recon=false);
    bool    Connect();      // Opens up and connects to cameras
    bool    Disconnect();
    void    InitCapture() { return; }
    void    ShowPropertyDialog();
    void    CheckState();
    void    NewProp(struct indi_prop_t *iprop);
    Camera_INDIClass();
};

extern Camera_INDIClass Camera_INDI;
#endif


