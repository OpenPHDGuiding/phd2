/*
 *  scope_manual_pointing.h
 *  PHD Guiding
 *
 *  Created by Andy Galasso.
 *  Copyright (c) 2016 openphdguiding.org
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
 *    Neither the name of openphdguiding.org nor the names of its
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

#ifndef SCOPE_MANUAL_POINTING_INCLUDED
#define SCOPE_MANUAL_POINTING_INCLUDED

class ScopeManualPointing : public Scope
{
    double m_latitude; // degrees
    double m_longitude; // degrees
    double m_ra; // hours
    double m_dec; // radians
    PierSide m_sideOfPier;

public:
    static wxString GetDisplayName();
    bool Connect() override;
    MOVE_RESULT Guide(GUIDE_DIRECTION, int) override;
    double GetDeclination() override;
    bool GetCoordinates(double *ra, double *dec, double *siderealTime) override;
    bool GetSiteLatLong(double *latitude, double *longitude) override;
    PierSide SideOfPier() override;
    bool PreparePositionInteractive() override;
    bool CanReportPosition() override;
};

#endif
