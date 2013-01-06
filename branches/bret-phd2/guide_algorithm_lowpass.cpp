/*
 *  guide_algorithm_lowpass.cpp
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

GuideAlgorithmLowpass::GuideAlgorithmLowpass(GuideAlgorithm *pChained)
    :GuideAlgorithm(pChained)
{
    double minMove    = pConfig->GetDouble("/GuideAlgorithm/Lowpass/minMove", 0.2);
    double slopeWeight    = pConfig->GetDouble("/GuideAlgorithm/Lowpass/SlopeWeight", 5.0);

    SetParms(minMove, slopeWeight);

    while (m_history.GetCount() < HISTORY_SIZE)
    {
        m_history.Add(0.0);
    }
}

GuideAlgorithmLowpass::~GuideAlgorithmLowpass(void)
{
}

bool GuideAlgorithmLowpass::SetParms(double minMove, double slopeWeight)
{
    bool bError = false;

    try
    {
        if (minMove <= 0)
        {
            throw ERROR_INFO("invalid minMove");
        }

        if (slopeWeight < 0.0)
        {
            throw ERROR_INFO("invalid slopeWeight");
        }

        m_slopeWeight = slopeWeight;

        pConfig->GetDouble("/GuideAlgorithm/Lowpass/SlopeWeight", m_slopeWeight);
        pConfig->GetDouble("/GuideAlgorithm/Lowpass/minMove", m_minMove);
    }
    catch (char *pErrorMsg)
    {
        POSSIBLY_UNUSED(pErrorMsg);
        bError = true;

        Debug.Write(wxString::Format("GuideAlgorithLowpass::SetPArms() caught exception: %s\n", pErrorMsg));
    }

    Debug.Write(wxString::Format("GuideAlgorithmLowpass::SetParms() returns %d, m_slopeWeight=%.2f\n", bError, m_slopeWeight));

    return bError;
}

double GuideAlgorithmLowpass::result(double input)
{
    if (m_pChained)
    {
        input = m_pChained->result(input);
    }

    m_history.Add(input);

    ArrayOfDbl sortedHistory(m_history);
    sortedHistory.Sort(dbl_sort_func);

    m_history.RemoveAt(0);

    double median = sortedHistory[sortedHistory.GetCount()/2];
    double slope = CalcSlope(m_history);
    double dReturn = input + m_slopeWeight*slope;

    if (dReturn > input)
    {
        Debug.Write(wxString::Format("GuideAlgorithmLowpassa::Result() input %.2f is > calculated value %.2f, using input\n", input, dReturn));
        dReturn = input;
    }

    if (fabs(input) < m_minMove)
    {
        dReturn = 0.0;
    }

    Debug.Write(wxString::Format("GuideAlgorithmLowpass::Result() returns %.2f from input %.2f\n", dReturn, input));

    return dReturn;
}
