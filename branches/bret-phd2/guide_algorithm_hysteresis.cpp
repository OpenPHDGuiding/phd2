/*
 *  guide_algorithm_ra.cpp
 *  PHD Guiding
 *
 *  Created by Bret McKee
 *  Copyright (c) 2012 Bret McKee
 *  All rights reserved.
 *
 *  Based upon work by Craig Stark.
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

#include "phd.h"

GuideAlgorithmHysteresis::GuideAlgorithmHysteresis(GuideAlgorithm *pChained)
    :GuideAlgorithm(pChained)
{
    double minMove    = pConfig->GetDouble("/GuideAlgorithm/DefaultRa/minMove", 0.2);
    double hysteresis = pConfig->GetDouble("/GuideAlgorithm/DefaultRa/hysteresis", 0.0);
    double aggression   = pConfig->GetDouble("/GuideAlgorithm/DefaultRa/RaAggression", 1.00);

    SetParms(minMove, hysteresis, aggression);
}

GuideAlgorithmHysteresis::~GuideAlgorithmHysteresis(void)
{
}

bool GuideAlgorithmHysteresis::SetParms(double minMove, double hysteresis, double aggression)
{
    bool bError = false;

    try
    {
        if (minMove < 0)
        {
            throw ERROR_INFO("invalid minMove");
        }

        if (hysteresis < 0 || hysteresis > 1.0)
        {
            throw ERROR_INFO("invalid hysteresis");
        }

        if (aggression <= 0.0 || aggression > 1.0)
        {
            throw ERROR_INFO("invalid aggression");
        }

        m_minMove = minMove;
        m_hysteresis = hysteresis;
        m_aggression = aggression;

        m_lastMove = 0.0;

        pConfig->SetDouble("/GuideAlgorithm/DefaultRa/minMove", m_minMove);
        pConfig->SetDouble("/GuideAlgorithm/DefaultRa/hysteresis", m_hysteresis);
        pConfig->SetDouble("/GuideAlgorithm/DefaultRa/RaAggression", m_aggression);
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;

        Debug.Write(wxString::Format("GuideAlgorithRa::SetPArms() caught exception: %s\n", pErrorMsg));
    }

    Debug.Write(wxString::Format("GuideAlgorithmHysteresis::SetParms() returns %d, m_minMove=%.2f m_hysteresis=%.2f m_aggression=%.2f\n", bError, m_minMove, m_hysteresis, m_aggression));
    return bError;
}

double GuideAlgorithmHysteresis::result(double input)
{
    if (m_pChained)
    {
        input = m_pChained->result(input);
    }

    double dReturn = (1.0-m_hysteresis)*input + m_hysteresis * m_lastMove;

    dReturn *= m_aggression;

    if (fabs(input) < m_minMove)
    {
        dReturn = 0.0;
    }

    m_lastMove = dReturn;

    Debug.Write(wxString::Format("GuideAlgorithmHysteresis::Result() returns %.2f from input %.2f\n", dReturn, input));

    return dReturn;
}
