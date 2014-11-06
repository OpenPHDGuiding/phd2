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

class ScopeINDI : public Scope, INDI::BaseClient {
private:
    INumberVectorProperty *coord_set_prop;
    ISwitchVectorProperty *abort_prop;
    INumberVectorProperty *MotionRate;
    ISwitchVectorProperty *moveNS;
    ISwitch               *moveN;
    ISwitch               *moveS;
    ISwitchVectorProperty *moveEW;
    ISwitch               *moveE;
    ISwitch               *moveW;
    INumberVectorProperty *pulseGuideNS;
    INumber               *pulseN;
    INumber               *pulseS;
    INumberVectorProperty *pulseGuideEW;
    INumber               *pulseE;
    INumber               *pulseW;
    ITextVectorProperty   *scope_port;
    INDI::BaseDevice      *scope_device;
    long     INDIport;
    wxString INDIhost;
    wxString INDIMountName;
    wxString INDIMountPort;
    bool     modal;
    bool     ready;
    void     ClearStatus();

protected:
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property *property) {}
    virtual void newBLOB(IBLOB *bp) {}
    virtual void newSwitch(ISwitchVectorProperty *svp);
    virtual void newNumber(INumberVectorProperty *nvp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);
    virtual void newText(ITextVectorProperty *tvp);
    virtual void newLight(ILightVectorProperty *lvp) {}
    virtual void serverConnected();
    virtual void serverDisconnected(int exit_code);
    
public:
    ScopeINDI();

    bool     Connect(void);
    bool     Disconnect(void);
    void     ShowPropertyDialog();
    bool     HasSetupDialog(void) const;
    void     SetupDialog();
    void     CheckState();
    void     NewProp(struct indi_prop_t *iprop);
    void     StartMove(int direction);
    void     StopMove(int direction);
    void     PulseGuide(int direction, int duration);
    bool     IsReady() {return ready;};
    bool     CanPulseGuide() { return pulseGuideNS && pulseGuideEW;};
    void     DoGuiding(int direction, int duration_msec);
    MOVE_RESULT Guide(GUIDE_DIRECTION direction, int duration);
};

#endif /* GUIDE_INDI */
