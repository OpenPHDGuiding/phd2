/*
 *  phdindiclient.h
 *  PHD2 Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2018 Andy Galasso
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
 *    Neither the name of Open PHD Guiding, openphdguiding.org, nor the names of its
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
#ifndef PHDINDICLIENT_H
#define PHDINDICLIENT_H

#include <libindi/baseclient.h>
#include <libindi/basedevice.h>

class PhdIndiClient : public INDI::BaseClient
{
    bool m_disconnecting;

public:
    PhdIndiClient();
    ~PhdIndiClient();

public:
    bool connectServer()
#if INDI_VERSION_MAJOR >= 2 || (INDI_VERSION_MINOR == 9 && INDI_VERSION_RELEASE == 9)
    override // use override since 1.9.9
#endif
    ;

protected:
    void serverConnected() final;
    void serverDisconnected(int exit_code) final;

    virtual void IndiServerConnected() = 0;
    virtual void IndiServerDisconnected(int exit_code) = 0;

    // must use this in PHD2 rather than BaseClient::disconnectServer()
    bool DisconnectIndiServer();

#if INDI_VERSION_MAJOR >= 2
protected: // old deprecated interface INDI Version < 2.0.0
    virtual void newDevice(INDI::BaseDevice *dp) = 0;
    virtual void removeDevice(INDI::BaseDevice *dp) = 0;
    virtual void newProperty(INDI::Property *property) = 0;
    virtual void removeProperty(INDI::Property *property) = 0;

    virtual void newMessage(INDI::BaseDevice *dp, int messageID) = 0;
    virtual void newBLOB(IBLOB *bp) = 0;
    virtual void newSwitch(ISwitchVectorProperty *svp) = 0;
    virtual void newNumber(INumberVectorProperty *nvp) = 0;
    virtual void newText(ITextVectorProperty *tvp) = 0;
    virtual void newLight(ILightVectorProperty *lvp) = 0;

protected: // new interface INDI Version >= 2.0.0
    void newDevice(INDI::BaseDevice device) override
    {
        return newDevice((INDI::BaseDevice *)device);
    }

    void removeDevice(INDI::BaseDevice device) override
    {
        return removeDevice((INDI::BaseDevice *)device);
    }

    void newProperty(INDI::Property property) override
    {
        return newProperty((INDI::Property *)property);
    }

    void removeProperty(INDI::Property property) override
    {
        return removeProperty((INDI::Property *)property);
    }

    void updateProperty(INDI::Property property) override
    {
        switch (property.getType())
        {
        case INDI_NUMBER: return newNumber((INumberVectorProperty *)property);
        case INDI_SWITCH: return newSwitch((ISwitchVectorProperty *)property);
        case INDI_LIGHT:  return newLight((ILightVectorProperty *)property);
        case INDI_BLOB:   return newBLOB((IBLOB *)INDI::PropertyBlob(property)[0].cast());
        case INDI_TEXT:   return newText((ITextVectorProperty *)property);
        }
    }

    void newMessage(INDI::BaseDevice device, int messageID) override
    {
        return newMessage((INDI::BaseDevice *)device, messageID);
    }
#endif
};

#endif
