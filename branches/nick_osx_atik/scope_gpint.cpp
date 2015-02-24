/*
 *  ShoeString.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
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

#ifdef GUIDE_GPINT

// ------------------ Parallel port version using inpuout32

/* ----Prototypes of Inp and Outp--- */
short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);

bool ScopeGpInt::Connect(void) {
    short reg = Inp32(port);

    reg = reg & 0x0F;  // Deassert all directions, protect low bits
    Out32(port,reg);
    Scope::Connect();
    return false;
}

bool ScopeGpInt::Disconnect(void) {
    short reg = Inp32(port);

    reg = reg & 0x0F;  // Deassert all directions, protect low bits
    Out32(port,reg);
    Scope::Disconnect();
    return false;
}

Mount::MOVE_RESULT ScopeGpInt::Guide(GUIDE_DIRECTION direction, int duration)
{
    short reg = Inp32(port);

    reg = reg & 0x0F;  // Deassert all directions
    switch (direction) {
        case NORTH: reg = reg ^ 0x80; break;    // Dec+
        case SOUTH: reg = reg ^ 0x40; break;    // Dec-
        case EAST: reg = reg ^ 0x10; break;     // RA-
        case WEST: reg = reg ^ 0x20; break;     // RA+
    }
    Out32(port,reg);
    WorkerThread::MilliSleep(duration, WorkerThread::INT_ANY);
    reg = reg & 0x0F;  // Deassert all directions
    Out32(port,reg);

    return MOVE_OK;
}


#endif /* GUIDE_GPINT */
