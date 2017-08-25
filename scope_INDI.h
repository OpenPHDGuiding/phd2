/*
 *  scope_INDI.cpp
 *  PHD Guiding
 *
 *  Ported by Hans Lambermont in 2014 from tele_INDI.h which has Copyright (c) 2009 Geoffrey Hausheer.
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
#ifdef  GUIDE_INDI

#include <libindi/baseclient.h>
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>

class ScopeINDI : public Scope, public INDI::BaseClient
{
private:
    ISwitchVectorProperty *connection_prop;
    INumberVectorProperty *coord_prop;    
    INumberVectorProperty *MotionRate_prop;
    ISwitchVectorProperty *moveNS_prop;
    ISwitch               *moveN_prop;
    ISwitch               *moveS_prop;
    ISwitchVectorProperty *moveEW_prop;
    ISwitch               *moveE_prop;
    ISwitch               *moveW_prop;
    INumberVectorProperty *GuideRate_prop;
    INumberVectorProperty *pulseGuideNS_prop;
    INumber               *pulseN_prop;
    INumber               *pulseS_prop;
    INumberVectorProperty *pulseGuideEW_prop;
    INumber               *pulseE_prop;
    INumber               *pulseW_prop;
    ISwitchVectorProperty *oncoordset_prop;
    ISwitch               *setslew_prop;
    ISwitch               *settrack_prop;
    ISwitch               *setsync_prop;
    INumberVectorProperty *GeographicCoord_prop;
    INumberVectorProperty *SiderealTime_prop;
    ITextVectorProperty   *scope_port;
    ISwitchVectorProperty *pierside_prop;
    ISwitch               *piersideEast_prop;
    ISwitch               *piersideWest_prop;
    ISwitchVectorProperty *AbortMotion_prop;
    ISwitch               *Abort_prop;
    INDI::BaseDevice      *scope_device;
    long     INDIport;
    wxString INDIhost;
    wxString INDIMountName;
    wxString INDIMountPort;
    bool     modal;
    bool     ready;
    bool     eod_coord;
    void     ClearStatus();
    void     CheckState();
    
protected:
    void newDevice(INDI::BaseDevice *dp) override;
#ifndef INDI_PRE_1_0_0
    void removeDevice(INDI::BaseDevice *dp) override;
#endif
    void newProperty(INDI::Property *property) override;
    void removeProperty(INDI::Property *property) override {}
    void newBLOB(IBLOB *bp) override {}
    void newSwitch(ISwitchVectorProperty *svp) override;
    void newNumber(INumberVectorProperty *nvp) override;
    void newMessage(INDI::BaseDevice *dp, int messageID) override;
    void newText(ITextVectorProperty *tvp) override;
    void newLight(ILightVectorProperty *lvp) override {}
    void serverConnected() override;
    void serverDisconnected(int exit_code) override;
    
public:
    ScopeINDI();
    ~ScopeINDI();

    bool     Connect(void) override;
    bool     Disconnect(void) override;
    bool     HasSetupDialog(void) const override;
    void     SetupDialog() override;

    MOVE_RESULT Guide(GUIDE_DIRECTION direction, int duration) override;
    bool HasNonGuiMove(void) override;

    bool   CanPulseGuide() override { return (pulseGuideNS_prop && pulseGuideEW_prop); }
    bool   CanReportPosition(void) override { return coord_prop ? true : false; }
    bool   CanSlew(void) override { return coord_prop ? true : false; }
    bool   CanSlewAsync(void) override;
    bool   CanCheckSlewing(void) override { return coord_prop ? true : false; }

    double GetDeclination(void) override;
    bool   GetGuideRates(double *pRAGuideRate, double *pDecGuideRate) override;
    bool   GetCoordinates(double *ra, double *dec, double *siderealTime) override;
    bool   GetSiteLatLong(double *latitude, double *longitude) override;
    bool   SlewToCoordinates(double ra, double dec) override;
    bool   SlewToCoordinatesAsync(double ra, double dec) override;
    void   AbortSlew(void) override;
    bool   Slewing(void) override;
    PierSide SideOfPier(void) override;
};

#endif /* GUIDE_INDI */
