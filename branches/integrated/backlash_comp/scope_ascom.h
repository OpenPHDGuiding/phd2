/*
 *  scope_ascom.h
 *  PHD Guiding
 *
 *  Created by Craig Stark.
 *  Copyright (c) 2006-2010 Craig Stark.
 *  All rights reserved.
 *
 *  Modified by Bret McKee
 *  Copyright (c) 2012-2013 Bret McKee
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
 *    Neither the name of Bret McKee, Dad Dog Development,
 *     Craig Stark, Stark Labs nor the names of its
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

#ifndef SCOPE_ASCOM_INCLUDED
#define SCOPE_ASCOM_INCLUDED

#ifdef GUIDE_ASCOM

#include "comdispatch.h"

class ScopeASCOM : public Scope
{
    GITEntry m_gitEntry;

    // DISPIDs we reuse
    DISPID dispid_connected;
    DISPID dispid_ispulseguiding;
    DISPID dispid_isslewing;
    DISPID dispid_pulseguide;
    DISPID dispid_declination;
    DISPID dispid_rightascension;
    DISPID dispid_siderealtime;
    DISPID dispid_sitelatitude;
    DISPID dispid_sitelongitude;
    DISPID dispid_slewtocoordinates;
    DISPID dispid_raguiderate;
    DISPID dispid_decguiderate;
    DISPID dispid_sideofpier;
    DISPID dispid_abortslew;

    // other private varialbles
    bool m_bCanCheckPulseGuiding;
    bool m_bCanGetCoordinates;
    bool m_bCanGetGuideRates;
    bool m_bCanSlew;
    bool m_bCanPulseGuide;

    bool m_abortSlewWhenGuidingStuck;

    wxString m_choice; // name of chosen scope

    // private functions
    bool Create(DispatchObj& obj);
    bool IsGuiding(DispatchObj *pScopeDriver);
    bool IsSlewing(DispatchObj *pScopeDriver);
    void AbortSlew(DispatchObj *pScopeDriver);

public:
    ScopeASCOM(const wxString& choice);
    virtual ~ScopeASCOM(void);
    static wxArrayString EnumAscomScopes(void);

    virtual bool Connect(void);
    virtual bool Disconnect(void);

    virtual bool HasSetupDialog(void) const;
    virtual void SetupDialog(void);

    virtual bool HasNonGuiMove(void);

    virtual MOVE_RESULT Guide(GUIDE_DIRECTION direction, int durationMs);

    virtual double GetGuidingDeclination(void);
    virtual bool GetGuideRates(double *pRAGuideRate, double *pDecGuideRate);
    virtual bool GetCoordinates(double *ra, double *dec, double *siderealTime);
    virtual bool GetSiteLatLong(double *latitude, double *longitude);
    virtual bool CanSlew(void);
    virtual bool CanReportPosition(void);
    virtual bool CanPulseGuide(void);
    virtual bool SlewToCoordinates(double ra, double dec);
    virtual bool CanCheckSlewing(void);
    virtual bool Slewing(void);
    virtual PierSide SideOfPier(void);
};

#endif // GUIDE_ASCOM
#endif // SCOPE_ASCOM_INCLUDED
