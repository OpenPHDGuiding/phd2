/*
 *  cam_INI.cpp
 *  PHD Guiding
 *
 *  Created by Geoffrey Hausheer.
 *  Copyright (c) 2009 Geoffrey Hausheer.
 *  All rights reserved.
 *
 *  This source code is distrubted under the following "BSD" license
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *    Neither the name of Craig Stark, Stark Labs nor the names of its contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "phd.h"
#include "scope.h"

#ifdef GUIDE_INDI

#include <stdio.h>
#include <string.h>

#include "tele_INDI.h"

extern "C" {
#include "libindiclient/indi.h"
#include "libindiclient/indigui.h"
}

extern struct indi_t *INDIClient;
extern long INDIport;
extern wxString INDIhost;
Telescope_INDIClass INDIScope;

void INDI_PulseGuideScope (int direction, int duration)
{
    INDIScope.DoGuiding(direction, duration);
}

bool INDI_ScopeConnect()
{
    return INDIScope.Connect();
}

static void connect_cb(struct indi_prop_t *iprop, void *data)
{
    Telescope_INDIClass *cb = (Telescope_INDIClass *)(data);
    cb->is_connected = (iprop->state == INDI_STATE_IDLE || iprop->state == INDI_STATE_OK) && indi_prop_get_switch(iprop, "CONNECT");
    printf("Telescope connected state: %d\n", cb->is_connected);
    cb->CheckState();
}

static void new_prop_cb(struct indi_prop_t *iprop, void *callback_data)
{
    Telescope_INDIClass *cb = (Telescope_INDIClass *)(callback_data);
    cb->NewProp(iprop);
}

static void tele_move_cb(struct indi_prop_t *iprop, void *callback_data)
{
    //We don't actually need to keep track of movement at the moment
    Telescope_INDIClass *cb = (Telescope_INDIClass *)(callback_data);
    (void)(cb);
}


void Telescope_INDIClass::CheckState()
{
    if(is_connected && (
        (moveNS && moveEW) ||
        (pulseGuideNS && pulseGuideEW)))
    {
        if (! ready) {
            printf("Telescope is ready\n");
            ready = true;
            if (modal) {
                modal = false;
            }
        }
    }
}

void Telescope_INDIClass::NewProp(struct indi_prop_t *iprop)
{
    if (strcmp(iprop->name, "EQUATORIAL_EOD_COORD_REQUEST") == 0) {
        coord_set_prop = iprop;
    }
    else if (strcmp(iprop->name, "EQUATORIAL_EOD_COORD") == 0) {
        //indi_prop_add_cb(iprop, tele_new_coords_cb, this);
    }
    else if (strcmp(iprop->name, "ABORT") == 0) {
        abort_prop = iprop;
    }
    else if (strcmp(iprop->name, "TELESCOPE_MOTION_NS") == 0) {
        moveNS = iprop;
        indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, this);
    }
    else if (strcmp(iprop->name, "TELESCOPE_MOTION_WE") == 0) {
        moveEW = iprop;
        indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, this);
    }
    else if (strcmp(iprop->name, "TELESCOPE_TIMED_GUIDE_NS") == 0) {
        pulseGuideNS = iprop;
        indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, this);
    }
    else if (strcmp(iprop->name, "TELESCOPE_TIMED_GUIDE_WE") == 0) {
        pulseGuideEW = iprop;
        indi_prop_add_cb(iprop, (IndiPropCB)tele_move_cb, this);
    }
    else if (strcmp(iprop->name, "DEVICE_PORT") == 0 && serial_port.Length()) {
        indi_send(iprop, indi_prop_set_string(iprop, "PORT", serial_port.ToAscii()));
        indi_dev_set_switch(iprop->idev, "CONNECTION", "CONNECT", TRUE);
    }
    else if (strcmp(iprop->name, "CONNECTION") == 0) {
        indi_prop_add_cb(iprop, (IndiPropCB)connect_cb, this);
        indi_send(iprop, indi_prop_set_switch(iprop, "CONNECT", TRUE));
    }
    CheckState();
}

bool Telescope_INDIClass::Connect() {
    wxLongLong msec;
    if (! INDIClient) {
        INDIClient = indi_init(INDIhost.ToAscii(), INDIport, "PHDGuiding");
        if (! INDIClient) {
            return true;
        }
    }
    if (indi_name.IsEmpty()) {
        printf("No INDI telescope is set.  Please set INDImount in the preferences file\n");
        return true;
    }
    indi_device_add_cb(INDIClient, indi_name.ToAscii(), (IndiDevCB)new_prop_cb, this);

    modal = true;
    msec = wxGetLocalTimeMillis();
    while(modal && wxGetLocalTimeMillis() - msec < 10 * 1000) {
        ::wxSafeYield();
    }
    modal = false;

    if(! ready)
        return true;
    return false;

}

void Telescope_INDIClass::PulseGuide(int direction, int duration_msec)
{
    double duration = duration_msec / 1000.0;
    switch (direction) {
        case EAST:
            indi_send(pulseGuideEW,
                      indi_prop_set_number(pulseGuideEW, "TIMED_GUIDE_E", duration));
            break;
        case WEST:
            indi_send(pulseGuideEW,
                      indi_prop_set_number(pulseGuideEW, "TIMED_GUIDE_W", duration));
            break;
        case NORTH:
            indi_send(pulseGuideNS,
                      indi_prop_set_number(pulseGuideNS, "TIMED_GUIDE_N", duration));
            break;
        case SOUTH:
            indi_send(pulseGuideNS,
                      indi_prop_set_number(pulseGuideNS, "TIMED_GUIDE_S", duration));
            break;
    }
}

void Telescope_INDIClass::StartMove(int direction)
{
    switch (direction) {
        case EAST:
            indi_send(moveEW,
                      indi_prop_set_switch(moveEW, "MOTION_EAST", TRUE));
            break;
        case WEST:
            indi_send(moveEW,
                      indi_prop_set_switch(moveEW, "MOTION_WEST", TRUE));
            break;
        case NORTH:
            indi_send(moveNS,
                      indi_prop_set_switch(moveNS, "MOTION_NORTH", TRUE));
            break;
        case SOUTH:
            indi_send(moveNS,
                      indi_prop_set_switch(moveNS, "MOTION_SOUTH", TRUE));
            break;
    }
}

void Telescope_INDIClass::StopMove(int direction)
{
    switch (direction) {
        case EAST:
        case WEST:
            indi_prop_set_switch(moveEW, "MOTION_EAST", FALSE);
            indi_prop_set_switch(moveEW, "MOTION_WEST", FALSE);
            indi_send(moveEW, NULL);
            break;
        case NORTH:
        case SOUTH:
            indi_prop_set_switch(moveNS, "MOTION_NORTH", FALSE);
            indi_prop_set_switch(moveNS, "MOTION_SOUTH", FALSE);
            indi_send(moveNS, NULL);
            break;
    }
}

void Telescope_INDIClass::DoGuiding(int direction, int duration_msec)
{
    wxLongLong msec;
    if (! ready)
        return;

    printf("Timed move: %d %d %d\n", direction, duration_msec, CanPulseGuide());

    if (CanPulseGuide()) {
        // We can submit a timed guide, and let the mount take care of it
        PulseGuide(direction, duration_msec);
        msec = wxGetLocalTimeMillis();
        while(wxGetLocalTimeMillis() - msec < duration_msec) {
            wxTheApp->Yield();
        }
        return;
    }
    StartMove(direction);
    // We should probably use an event or something here, as this method means
    // PHD will be unresponsive during a move
    msec = wxGetLocalTimeMillis();
    while(wxGetLocalTimeMillis() - msec < duration_msec) {
        wxTheApp->Yield();
    }
    StopMove(direction);
}

#endif
