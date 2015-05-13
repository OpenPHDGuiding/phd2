/*
 *  cam_INDI.h
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

#include <libindi/baseclient.h>
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>

#include "indi_gui.h"

class Camera_INDIClass : public GuideCamera, public INDI::BaseClient {
private:
    INumberVectorProperty *expose_prop;
    INumberVectorProperty *frame_prop;
    INumber               *frame_x;
    INumber               *frame_y;
    INumber               *frame_width;
    INumber               *frame_height;
    ISwitchVectorProperty *frame_type_prop;
    INumberVectorProperty *ccdinfo_prop;
    INumberVectorProperty *binning_prop;
    ISwitchVectorProperty *video_prop;
    ITextVectorProperty   *camera_port;
    INDI::BaseDevice      *camera_device;
    INumberVectorProperty *pulseGuideNS_prop;
    INumber               *pulseN_prop;
    INumber               *pulseS_prop;
    INumberVectorProperty *pulseGuideEW_prop;
    INumber               *pulseE_prop;
    INumber               *pulseW_prop;
    IndiGui  *gui ;
    IBLOB    *cam_bp;
    bool     has_blob;
    bool     modal;
    bool     ready;
    double   PixSizeX; 
    double   PixSizeY; 
    long     INDIport;
    wxString INDIhost;
    wxString INDICameraName;
    long     INDICameraCCD;
    wxString INDICameraCCDCmd;
    wxString INDICameraBlobName;
    wxString INDICameraPort;
    wxRect   m_roi;
    void     SetCCDdevice();
    void     ClearStatus(); 
    void     CheckState();
    void     CameraDialog();
    void     CameraSetup();
    bool     ReadFITS(usImage& img, bool takeSubframe, const wxRect& subframe);
    bool     ReadStream(usImage& img);
    
protected:
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void removeDevice(INDI::BaseDevice *dp) {}
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property *property) {}
    virtual void newBLOB(IBLOB *bp);
    virtual void newSwitch(ISwitchVectorProperty *svp);
    virtual void newNumber(INumberVectorProperty *nvp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);
    virtual void newText(ITextVectorProperty *tvp);
    virtual void newLight(ILightVectorProperty *lvp) {}
    virtual void serverConnected();
    virtual void serverDisconnected(int exit_code);

public:
    Camera_INDIClass();
    ~Camera_INDIClass();
    bool    Connect();      // Opens up and connects to cameras
    bool    Disconnect();
    bool    HasNonGuiCapture(void);
    void    ShowPropertyDialog();

    bool    Capture(int duration, usImage& img, int options, const wxRect& subframe);

    bool    ST4PulseGuideScope(int direction, int duration);
    bool    ST4HasNonGuiMove(void);
};

#endif


