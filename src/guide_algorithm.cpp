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

wxString GuideAlgorithm::GetConfigPath() const
{
    return "/" + m_pMount->GetMountClassName() + "/GuideAlgorithm/" +
        (m_guideAxis == GUIDE_X ? "X/" : "Y/") + GetGuideAlgorithmClassName();
}

wxString GuideAlgorithm::GetAxis() const
{
    return m_guideAxis == GUIDE_RA ? _("RA") : _("DEC");
}

// Default technique to force a reset on algo parameters is simply to remove the keys from the Registry - a subsequent creation of the algo
// class will then use default values for everything.  If this is too brute-force for a particular algo, the function can be overridden.
// For algos that use a min-move parameter, a smart value will be applied based on image scale
void GuideAlgorithm::ResetParams()
{
    wxString configPath = GetConfigPath();
    pConfig->Profile.DeleteGroup(configPath);
    if (GetMinMove() >= 0.)
        SetMinMove(SmartDefaultMinMove());
}

double GuideAlgorithm::SmartDefaultMinMove(int focalLength, double pixelSize, int binning)
{
    double imageScale = MyFrame::GetPixelScale(pixelSize, focalLength, binning);
    // Following based on empirical data with a range of image scales
    return wxMax(0.1515 + 0.1548 / imageScale, 0.15);
}

double GuideAlgorithm::SmartDefaultMinMove()
{
    try
    {
        double focalLength = pFrame->GetFocalLength();
        if (focalLength != 0)
        {
            return GuideAlgorithm::SmartDefaultMinMove(focalLength, pCamera->GetCameraPixelSize(), pCamera->Binning);
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

void GuideAlgorithm::AdjustMinMoveSpinCtrl(wxSpinCtrlDouble* minMoveCtrl)
{
    // Always use current AD values for all params affecting image scale
    double smartMove = GuideAlgorithm::SmartDefaultMinMove(pFrame->pAdvancedDialog->GetFocalLength(), pFrame->pAdvancedDialog->GetPixelSize(), pFrame->pAdvancedDialog->GetBinning());
    minMoveCtrl->SetValue(smartMove);
}

void GuideAlgorithm::GuidingStarted()
{
}

void GuideAlgorithm::GuidingStopped()
{
    reset();
}

void GuideAlgorithm::GuidingPaused()
{
}

void GuideAlgorithm::GuidingResumed()
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

void GuideAlgorithm::DirectMoveApplied(double amt)
{
}

void GuideAlgorithm::GuidingDisabled()
{
    // By default, guide star deflections will be accumulated even with guiding disabled - algo can override if this is a problem
}

void GuideAlgorithm::GuidingEnabled()
{
    // Discard whatever history was accumulated with guiding disabled
    reset();
}

void GuideAlgorithm::GetParamNames(wxArrayString& names) const
{
}

bool GuideAlgorithm::GetParam(const wxString& name, double *val) const
{
    return false;
}
bool GuideAlgorithm::SetParam(const wxString& name, double val)
{
    return false;
}
