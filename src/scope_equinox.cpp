/*
 *  scope_equinox.cpp
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2008-2010 Craig Stark.
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

#ifdef GUIDE_EQUINOX

#if defined (__APPLE__)
#include <CoreServices/CoreServices.h>
//#include <CoreServices/AppleEvents.h>
//#include <Foundation/Foundation.h>
//#include <AppleEvents.h>
//#include <AEDataModel.h>

// Code originally from Darryl @ Equinox

OSErr ScopeEquinox::E6AESendRoutine(double ewCorrection, double nsCorrection, int mountcode)
{
// correction values (+- seconds) to send to E6

    OSErr err;
    FourCharCode E6Sig = 'MPj6';  // the Equinox 6 creator signature
    if (mountcode == SCOPE_EQMAC)
        E6Sig = 'EQMC';
    FourCharCode phdSig = 'PhDG';  // ***** you need to fill in your app signature here ******
    AEAddressDesc addDesc;

    AEEventClass evClass = 'phdG';  // the phd guide class.
    AEEventID evID = 'evGD';  // the phd guide event.
    AEKeyword  keyObject;
    AESendMode mode = kAEWaitReply;  //  you want something back

    // create Apple Event with Equinox 6 signature

    err = AECreateDesc( typeApplSignature, (Ptr) &E6Sig, sizeof(FourCharCode), &addDesc );  // make a description
    if( err != noErr ) return err;

    err = AECreateAppleEvent( evClass, evID, &addDesc, kAutoGenerateReturnID, kAnyTransactionID, &E6Event );  // create the AE
    if( err != noErr ) {
        AEDisposeDesc( &addDesc );
        return err;
    }

    // create the return Apple Event with your signature (so I know where to send it)

    err = AECreateDesc( typeApplSignature, (Ptr) &phdSig, sizeof(FourCharCode), &addDesc );
    if( err != noErr ) {
        AEDisposeDesc( &E6Event );
        return err;
    }

    err = AECreateAppleEvent( evClass, evID, &addDesc, kAutoGenerateReturnID, kAnyTransactionID, &E6Return );
    if( err != noErr ) {
        AEDisposeDesc( &E6Event );
        AEDisposeDesc( &addDesc );
        return err;
    }

    // put the correction values into parameters - I have used doubles for a ew and ns seconds correction

    keyObject = 'prEW';  // EW correction AE parameter (+ = east, - = west)
    err = AEPutParamPtr( &E6Event, keyObject, typeIEEE64BitFloatingPoint,  &ewCorrection, sizeof(double) );
    if( err != noErr ) {
        AEDisposeDesc( &E6Event );
        AEDisposeDesc( &addDesc );
        return err;
    }

    keyObject = 'prNS';  // NS correction AE parameter (+ = north, - = south)
    err = AEPutParamPtr( &E6Event, keyObject, typeIEEE64BitFloatingPoint, &nsCorrection, sizeof(double) );
    if( err != noErr ) {
        AEDisposeDesc( &E6Event );
        AEDisposeDesc( &addDesc );
        return err;
    }

    // you now have the send AE, the return AE and the correction values in AE parameters - so send it!

    err = AESendMessage( &E6Event, &E6Return, mode, kAEDefaultTimeout );  // you can specify a wait time (in ticks)
    if( err != noErr ) {  // Note: an error of -600 means E6 is not currently running
        AEDisposeDesc( &E6Event );
        AEDisposeDesc( &addDesc );
        return err;
    }

    // at this point you have received the return AE from E6, so go read the return code (what do you want returned ??)
    // the return code could indicate that E6 got the AE, can do it, or can't for some reason. You do NOT want to wait
    // until the corrections have been applied - you should time that on your own.

    keyObject = 'prRC';  // get return code
    Size returnSize;
    DescType returnType;
    err = AEGetParamPtr( &E6Return, keyObject, typeSInt16, &returnType, &E6ReturnCode, sizeof(SInt16), &returnSize );

    AEDisposeDesc( &E6Event );
    AEDisposeDesc( &addDesc );
    return 0;
}

bool ScopeEquinox::Connect()
{
    // Check the E6 connection by sending 0,0 to it and checking E6Return
    OSErr err = E6AESendRoutine(0.0,0.0,SCOPE_EQUINOX);
    wxString prefix = "E6";
//  if (mountcode == SCOPE_EQMAC) prefix = "EQMAC";
    if (E6ReturnCode == -1) {
        wxMessageBox (prefix + " responded it's not connected to a mount",_("Error"));
        return true;
    }
    else if (err == -600) {
        wxMessageBox (prefix + " not running",_("Error"));
        return true;
    }

    Scope::Connect();

    return false;
}

Mount::MOVE_RESULT ScopeEquinox::Guide(GUIDE_DIRECTION direction, int duration)
{
    double NSTime = 0.0;
    double EWTime = 0.0;

    switch (direction) {
        case NORTH:
            NSTime = (double) duration / 1000.0;
            break;
        case SOUTH:
            NSTime = (double) duration / -1000.0;
            break;
        case EAST:
            EWTime = (double) duration / 1000.0;
            break;
        case WEST:
            EWTime = (double) duration / -1000.0;
            break;
        case NONE:
            break;
    }

    OSErr err = E6AESendRoutine(EWTime, NSTime,SCOPE_EQUINOX);
    wxString prefix = "E6";
    //if (mountcode == SCOPE_EQMAC) prefix = "EQMAC";
    if (E6ReturnCode == -1) {
        pFrame->Alert(prefix + _(" responded it's not connected to a mount"));
        return MOVE_ERROR;
    }
    else if (err == -600) {
        pFrame->Alert(prefix + _(" not running"));
        return MOVE_ERROR;
    }
    wxMilliSleep(duration);

    return MOVE_OK;
}

#endif
#endif /* GUIDE_EQUINOX */
