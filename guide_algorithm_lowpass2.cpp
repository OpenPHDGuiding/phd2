/*
 *  guide_algorithm_lowpass2.cpp
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

GuideAlgorithmLowpass2::GuideAlgorithmLowpass2(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis)
{
    reset();
}

GuideAlgorithmLowpass2::~GuideAlgorithmLowpass2(void)
{
}

GUIDE_ALGORITHM GuideAlgorithmLowpass2::Algorithm(void)
{
    return GUIDE_ALGORITHM_LOWPASS2;
}

void GuideAlgorithmLowpass2::reset(void)
{
    m_history.Empty();

    while (m_history.GetCount() < HISTORY_SIZE)
    {
        m_history.Add(0.0);
    }
}

double GuideAlgorithmLowpass2::result(double input)
{
    m_history.Add(input);

    double dReturn = CalcSlope(m_history);

    m_history.RemoveAt(0);

    if (fabs(dReturn) > fabs(input))
    {
        Debug.Write(wxString::Format("GuideAlgorithmLowpass2::Result() input %.2f is > calculated value %.2f, using input\n", input, dReturn));
        dReturn = input;
    }

    Debug.Write(wxString::Format("GuideAlgorithmLowpassa::Result() returns %.2f from input %.2f\n", dReturn, input));

    return dReturn;
}

ConfigDialogPane *GuideAlgorithmLowpass2::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideAlgorithmLowpass2ConfigDialogPane(pParent, this);
}

GuideAlgorithmLowpass2::
GuideAlgorithmLowpass2ConfigDialogPane::
GuideAlgorithmLowpass2ConfigDialogPane(wxWindow *pParent, GuideAlgorithmLowpass2 *pGuideAlgorithm)
    :ConfigDialogPane(_("Lowpass2 Guide Algorithm"), pParent)
{
    m_pGuideAlgorithm = pGuideAlgorithm;
    DoAdd(new wxStaticText(pParent, wxID_ANY, _("Nothing to Configure"),wxPoint(-1,-1),wxSize(-1,-1)));
}

GuideAlgorithmLowpass2::
GuideAlgorithmLowpass2ConfigDialogPane::
~GuideAlgorithmLowpass2ConfigDialogPane(void)
{
}

void GuideAlgorithmLowpass2::
GuideAlgorithmLowpass2ConfigDialogPane::
LoadValues(void)
{
}

void GuideAlgorithmLowpass2::
GuideAlgorithmLowpass2ConfigDialogPane::
UnloadValues(void)
{
}
