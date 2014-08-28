/*
 *  scope_INDI.cpp
 *  PHD Guiding
 *
 *  Ported by Hans Lambermont in 2014 from tele_INDI.cpp which has Copyright (c) 2009 Geoffrey Hausheer.
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
#include "phd.h"

#ifdef GUIDE_INDI

extern "C" {
    #include "libindiclient/indi.h"
    #include "libindiclient/indigui.h"
}

extern struct indi_t *INDIClient;
extern long INDIport;
extern wxString INDIhost;
extern wxString INDIMountName;

static void tele_move_cb(struct indi_prop_t * /*iprop*/, void *callback_data) {
//printf("entering ScopeINDI tele_move_cb\n");
    //We don't actually need to keep track of movement at the moment
    ScopeINDI *cb = (ScopeINDI *)(callback_data);
    (void)(cb);
}

static void tele_new_prop_cb(struct indi_prop_t *iprop, void *callback_data) {
//printf("entering ScopeINDI tele_new_prop_cb\n");
    ScopeINDI *cb = (ScopeINDI *)(callback_data);
    cb->NewProp(iprop);
}

static void tele_connect_cb(struct indi_prop_t *iprop, void *data) {
//printf("entering ScopeINDI tele_connect_cb\n");
    ScopeINDI *cb = (ScopeINDI *)(data);
    if ( (iprop->state == INDI_STATE_IDLE || iprop->state ==
        INDI_STATE_OK) && indi_prop_get_switch(iprop, "CONNECT") ) {
        cb->Mount::Connect();
    }
    printf("Telescope connected state: %d\n", cb->IsConnected() );
    cb->CheckState();
}

ScopeINDI::ScopeINDI() {
    m_Name = wxString("INDI Mount");
}

void ScopeINDI::CheckState() {
//printf("entering ScopeINDI::CheckState\n");
    if(IsConnected() && (
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

bool ScopeINDI::Connect() {
//printf("entering ScopeINDI::Connect\n");
    wxLongLong msec;
    if (! INDIClient) {
        INDIClient = indi_init(INDIhost.ToAscii(), INDIport, "PHDGuiding");
        if (! INDIClient) {
            return true;
        }
    }
    if (INDIMountName.IsEmpty()) {
        printf("No INDI telescope is set.  Please set INDImount in the preferences file\n");
        return true;
    }
    indi_device_add_cb(INDIClient, INDIMountName.ToAscii(), (IndiDevCB)tele_new_prop_cb, this);

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

bool ScopeINDI::Disconnect() {
//printf("entering ScopeINDI::Disconnect\n");
    return false;
}

void ScopeINDI::NewProp(struct indi_prop_t *iprop) {
//printf("entering ScopeINDI::NewProp\n");
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
        indi_prop_add_cb(iprop, (IndiPropCB)tele_connect_cb, this);
        indi_send(iprop, indi_prop_set_switch(iprop, "CONNECT", TRUE));
    }
    CheckState();
}

Mount::MOVE_RESULT ScopeINDI::Guide(GUIDE_DIRECTION direction, int duration_msec) {
//printf("entering ScopeINDI::Guide\n");

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
        case NONE:
			printf("error ScopeINDI::Guide NONE\n");
            break;
    }
    wxMilliSleep(duration);

    return MOVE_OK;
}

#endif /* GUIDE_INDI */
