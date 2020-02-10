/*
 *  scopes.h
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

#ifndef SCOPES_H_INCLUDED
#define SCOPES_H_INCLUDED

#if defined (__WINDOWS__)

    #define GUIDE_ONCAMERA
    #define GUIDE_ONSTEPGUIDER
    #define GUIDE_ASCOM
    #define GUIDE_GPUSB
    #define GUIDE_GPINT
    #define GUIDE_INDI

#elif defined (__APPLE__)

    #define GUIDE_ONCAMERA
    #define GUIDE_ONSTEPGUIDER
    #define GUIDE_GPUSB
    #define GUIDE_GCUSBST4
    #define GUIDE_INDI
    #define GUIDE_EQUINOX
    //#define GUIDE_VOYAGER
    //#define GUIDE_NEB
    #define GUIDE_EQMAC

#elif defined (__linux__) || defined (__FreeBSD__)

    #define GUIDE_ONCAMERA
    #define GUIDE_ONSTEPGUIDER
    #define GUIDE_INDI

#endif // WINDOWS/APPLE/LINUX

#include "scope.h"
#include "scope_oncamera.h"
#include "scope_onstepguider.h"
#include "scope_ascom.h"
#include "scope_gpusb.h"
#include "scope_gpint.h"
#include "scope_voyager.h"
#include "scope_equinox.h"
#include "scope_eqmac.h"
#include "scope_GC_USBST4.h"
#include "scope_indi.h"
#include "scope_manual_pointing.h"

#endif /* SCOPES_H_INCLUDED */
