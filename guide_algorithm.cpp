/*
 *  guide_algorithm.cpp
 *  PHD Guiding
 *
 *  Created by Andy Galasso
 *  Copyright (c) 2013-2016 Andy Galasso
 *  All rights reserved.
 *
 *  Based upon work by Craig Stark and Bret McKee.
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


wxString GuideAlgorithm::GetConfigPath()
{
    return "/" + m_pMount->GetMountClassName() + "/GuideAlgorithm/" +
        (m_guideAxis == GUIDE_X ? "X/" : "Y/") + GetGuideAlgorithmClassName();
}

wxString GuideAlgorithm::GetAxis()
{
    return (m_guideAxis == GUIDE_RA ? _("RA") : _("DEC"));
}

// Default technique to force a reset on algo parameters is simply to remove the keys from the Registry - a subsequent creation of the algo 
// class will then use default values for everything.  If this is too brute-force for a particular algo, the function can be overridden. 
// For algos that use a min-move parameter, a smart value will be applied based on image scale
void GuideAlgorithm::ResetParams()
{
    wxString configPath = GetConfigPath();
    pConfig->Profile.DeleteGroup(configPath);
    if (GetMinMove() >= 0)
        SetMinMove(SmartDefaultMinMove());
}

double GuideAlgorithm::SmartDefaultMinMove()
{
    try
    {
        double focalLength = pFrame->GetFocalLength();
        if (focalLength != 0)
        {
            double imageScale = MyFrame::GetPixelScale(pCamera->GetCameraPixelSize(), focalLength, pCamera->Binning);
            // Following based on empirical data using a range of image scales - same as profile wizard
            return wxMax(0.1515 + 0.1548 / imageScale, 0.15);
        }
        else
            return 0.2;
    }
    catch (const wxString& Msg)
    {
        POSSIBLY_UNUSED(Msg);
        return 0.2;
    }
}

void GuideAlgorithm::GuidingStopped(void)
{
    reset();
}

void GuideAlgorithm::GuidingPaused(void)
{
}

void GuideAlgorithm::GuidingResumed(void)
{
    reset();
}

void GuideAlgorithm::GuidingDithered(double amt)
{
    reset();
}

void GuideAlgorithm::GuidingDitherSettleDone(bool success)
{
}

void GuideAlgorithm::GetParamNames(wxArrayString& names) const
{
}

bool GuideAlgorithm::GetParam(const wxString& name, double *val)
{
    return false;
}

bool GuideAlgorithm::SetParam(const wxString& name, double val)
{
    return false;
}
