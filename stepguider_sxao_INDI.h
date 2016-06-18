/*
 *  stepguider_sxao_INDI.h
 *  PHD Guiding
 *
 *  Created by Hans Lambermont
 *  Copyright (c) 2016 Hans Lambermont
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
 *    Neither the name Craig Stark, Stark Labs nor the names of its
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

#ifndef STEPGUIDER_SXAO_INDI_H_INCLUDED
#define STEPGUIDER_SXAO_INDI_H_INCLUDED

#if defined(STEPGUIDER_SXAO_INDI)

#include <libindi/baseclient.h>
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>
//#include <libindi/indiguiderinterface.h>

#include "config_INDI.h"

class StepGuiderSxAoINDI: public StepGuider , public INDI::BaseClient
{
private:
    // INDI parts
    static const int MaxDeviceInitWaitMilliSeconds = 2000;
    static const int MaxDevicePropertiesWaitMilliSeconds = 5000;
    long     INDIport;
    wxString INDIhost;
    wxString INDIaoDeviceName;
    wxString INDIaoDevicePort;
    bool     modal;
    bool     ready;
    void     ClearStatus();
    void     CheckState();

    INumberVectorProperty *pulseGuideNS_prop;
    INumber               *pulseN_prop;
    INumber               *pulseS_prop;
    INumberVectorProperty *pulseGuideWE_prop;
    INumber               *pulseW_prop;
    INumber               *pulseE_prop;
    INumberVectorProperty *aoNS_prop;
    INumber               *aoN_prop;
    INumber               *aoS_prop;
    INumberVectorProperty *aoWE_prop;
    INumber               *aoW_prop;
    INumber               *aoE_prop;
    ISwitchVectorProperty *aoCenterUnjam_prop;
    ISwitch               *aoCenter_prop;
    ISwitch               *aoUnjam_prop;
    INDI::BaseDevice      *ao_device;
    ITextVectorProperty   *ao_port;
    ITextVectorProperty   *ao_driverInfo;
    IText                 *ao_driverName;
    IText                 *ao_driverExec;
    IText                 *ao_driverVersion;
    IText                 *ao_driverInterface;
    ITextVectorProperty   *ao_info;
    IText                 *ao_firmware;
    ILightVectorProperty  *ao_limit;
    ILight                *ao_limit_north;
    ILight                *ao_limit_south;
    ILight                *ao_limit_east;
    ILight                *ao_limit_west;

    // StepGuider parts
    static const int DefaultMaxSteps = 45;
	wxString m_Name;
    int m_maxSteps;
    int SxAoVersion;

    virtual bool Step(GUIDE_DIRECTION direction, int steps);
    virtual int MaxPosition(GUIDE_DIRECTION direction) const;
    virtual bool SetMaxPosition(int steps);
    virtual bool IsAtLimit(GUIDE_DIRECTION direction, bool *isAtLimit);

    bool FirmwareVersion(int *version);
    bool Unjam(void);
    bool Center(void);
    bool Center(unsigned char cmd);

    virtual bool    ST4HasGuideOutput(void);
    virtual bool    ST4HostConnected(void);
    virtual bool    ST4HasNonGuiMove(void);
    virtual bool    ST4PulseGuideScope(int direction, int duration);

protected:
    // INDI parts
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void removeDevice(INDI::BaseDevice *dp);
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property *property) {};
    virtual void newBLOB(IBLOB *bp) {};
    virtual void newSwitch(ISwitchVectorProperty *svp) {};
    virtual void newNumber(INumberVectorProperty *nvp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);
    virtual void newText(ITextVectorProperty *tvp) {};
    virtual void newLight(ILightVectorProperty *lvp) {};
    virtual void serverConnected();
    virtual void serverDisconnected(int exit_code);

public:
    StepGuiderSxAoINDI(void);
    ~StepGuiderSxAoINDI(void);

    // StepGuider parts
    virtual bool Connect(void);
    virtual bool Disconnect(void);
    virtual void ShowPropertyDialog(void); // just calling INDI SetupDialog()

    // INDI parts
    bool     HasSetupDialog(void) const;
    void     SetupDialog();
};

#endif // #if defined(STEPGUIDER_SXAO_INDI)
#endif // #ifndef STEPGUIDER_SXAO_INDI_H_INCLUDED
