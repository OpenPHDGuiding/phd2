/*
 *  cam_indi.h
 *  PHD Guiding
 *
 *  Created by Geoffrey Hausheer.
 *  Copyright (c) 2009 Geoffrey Hausheer.
 *  All rights reserved.
 *
 *  Redraw for libindi/baseclient by Patrick Chevalley
 *  Copyright (c) 2014 Patrick Chevalley
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

#include "phdindiclient.h"
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>

#include "indi_gui.h"

class CameraINDI : public GuideCamera, public PhdIndiClient
{
private:
    ISwitchVectorProperty *connection_prop;
    INumberVectorProperty *expose_prop;
    INumberVectorProperty *frame_prop;
    INumber               *frame_x;
    INumber               *frame_y;
    INumber               *frame_width;
    INumber               *frame_height;
    ISwitchVectorProperty *frame_type_prop;
    INumberVectorProperty *ccdinfo_prop;
    INumberVectorProperty *binning_prop;
    INumber               *binning_x;
    INumber               *binning_y;
    ISwitchVectorProperty *video_prop;
    ITextVectorProperty   *camera_port;
    INDI::BaseDevice      *camera_device;
    INumberVectorProperty *pulseGuideNS_prop;
    INumber               *pulseN_prop;
    INumber               *pulseS_prop;
    INumberVectorProperty *pulseGuideEW_prop;
    INumber               *pulseE_prop;
    INumber               *pulseW_prop;

    wxMutex sync_lock;
    wxCondition sync_cond;
    bool guide_active;
    GuideAxis guide_active_axis;

    IndiGui  *m_gui;
    IBLOB    *cam_bp;
    usImage  *StackImg;
    int      StackFrames;
    bool     stacking;
    bool     has_blob;
    bool     has_old_videoprop;
    bool     first_frame;
    bool     modal;
    bool     ready;
    wxByte   m_bitsPerPixel;
    double   PixSize;
    double   PixSizeX;
    double   PixSizeY;
    wxRect   m_maxSize;
    wxByte   m_curBinning;
    bool     HasBayer;
    long     INDIport;
    wxString INDIhost;
    wxString INDICameraName;
    long     INDICameraCCD;
    wxString INDICameraCCDCmd;
    wxString INDICameraBlobName;
    wxString INDICameraPort;
    bool     INDICameraForceVideo;
    bool     INDICameraForceExposure;
    wxRect   m_roi;

    bool     ConnectToDriver(RunInBg *ctx);
    void     SetCCDdevice();
    void     ClearStatus();
    void     CheckState();
    void     CameraDialog();
    void     CameraSetup();
    bool     ReadFITS(usImage& img, bool takeSubframe, const wxRect& subframe);
    bool     ReadStream(usImage& img);
    bool     StackStream();

protected:
    void newDevice(INDI::BaseDevice *dp) override;
#ifndef INDI_PRE_1_0_0
    void removeDevice(INDI::BaseDevice *dp) override;
#endif
    void newProperty(INDI::Property *property) override;
    void removeProperty(INDI::Property *property) override {}
    void newBLOB(IBLOB *bp) override;
    void newSwitch(ISwitchVectorProperty *svp) override;
    void newNumber(INumberVectorProperty *nvp) override;
    void newMessage(INDI::BaseDevice *dp, int messageID) override;
    void newText(ITextVectorProperty *tvp) override;
    void newLight(ILightVectorProperty *lvp) override {}
    void serverConnected() override;
    void IndiServerDisconnected(int exit_code) override;

public:
    CameraINDI();
    ~CameraINDI();
    bool    Connect(const wxString& camId) override;
    bool    Disconnect() override;
    bool    HasNonGuiCapture() override;
    wxByte  BitsPerPixel() override;
    bool    GetDevicePixelSize(double *pixSize) override;
    void    ShowPropertyDialog() override;

    bool    Capture(int duration, usImage& img, int options, const wxRect& subframe) override;

    bool    ST4PulseGuideScope(int direction, int duration) override;
    bool    ST4HasNonGuiMove() override;
};

#endif
