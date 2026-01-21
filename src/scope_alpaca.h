/*
 *  scope_alpaca.h
 *  PHD Guiding
 *
 *  Copyright (c) 2026 PHD2 Developers
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

#ifndef SCOPE_ALPACA_INCLUDED
#define SCOPE_ALPACA_INCLUDED

#ifdef GUIDE_ALPACA

#include "alpaca_client.h"

class ScopeAlpaca : public Scope
{
private:
    AlpacaClient *m_client;
    wxString m_host;
    long m_port;
    long m_deviceNumber;

    // Capability flags
    bool m_canCheckPulseGuiding;
    bool m_canGetCoordinates;
    bool m_canGetGuideRates;
    bool m_canSlew;
    bool m_canSlewAsync;
    bool m_canPulseGuide;
    bool m_canGetSiteLatLong;

    // Private helper functions
    bool IsGuiding();
    bool IsSlewing();

public:
    ScopeAlpaca();
    virtual ~ScopeAlpaca();

    bool Connect() override;
    bool Disconnect() override;

    bool HasSetupDialog() const override;
    void SetupDialog() override;

    bool HasNonGuiMove() override;

    MOVE_RESULT Guide(GUIDE_DIRECTION direction, int durationMs) override;

    double GetDeclinationRadians() override;
    bool GetGuideRates(double *pRAGuideRate, double *pDecGuideRate) override;
    bool GetCoordinates(double *ra, double *dec, double *siderealTime) override;
    bool GetSiteLatLong(double *latitude, double *longitude) override;
    bool CanSlew() override;
    bool CanSlewAsync() override;
    bool CanReportPosition() override;
    bool CanPulseGuide() override;
    bool SlewToCoordinates(double ra, double dec) override;
    bool SlewToCoordinatesAsync(double ra, double dec) override;
    void AbortSlew() override;
    bool CanCheckSlewing() override;
    bool Slewing() override;
    PierSide SideOfPier() override;
};

class AlpacaScopeFactory
{
public:
    static Scope *MakeAlpacaScope();
};

#endif // GUIDE_ALPACA
#endif // SCOPE_ALPACA_INCLUDED

