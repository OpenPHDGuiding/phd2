 /*
 *  indiclient.h
 *  PHD Guiding
 *
 *  Created by Markus Wieczorek.
 *  Copyright (c) 2014 Markus Wieczorek.
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
 
#ifndef _INDI_CLIENT_H_
#define _INDI_CLIENT_H_

//#include "indidevapi.h"
//#include "indicom.h"
#include <libindi/baseclient.h>
class MyINDIClient : public INDI::BaseClient
{
public:
MyINDIClient();
~MyINDIClient();
void setExpotime(int duration);
void setGain(int gain);
//void setTemperature();
//void takeExposure();
protected:
virtual void newDevice(INDI::BaseDevice *dp);
virtual void newProperty(INDI::Property *property);
virtual void removeProperty(INDI::Property *property) {}
virtual void newBLOB(IBLOB *bp);
virtual void newSwitch(ISwitchVectorProperty *svp) {}
virtual void newNumber(INumberVectorProperty *nvp);
virtual void newMessage(INDI::BaseDevice *dp, int messageID);
virtual void newText(ITextVectorProperty *tvp) {}
virtual void newLight(ILightVectorProperty *lvp) {}
virtual void serverConnected() {}
virtual void serverDisconnected(int exit_code) {}
private:
INDI::BaseDevice * indi_device;
};
#endif