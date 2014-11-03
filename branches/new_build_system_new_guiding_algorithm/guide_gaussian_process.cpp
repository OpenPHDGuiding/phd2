//
//  guide_gaussian_process.cpp
//  PHD
//
//  Created by Stephan Wenninger
//  Copyright 2014, Max Planck Society.

/*
*  This source code is distributed under the following "BSD" license
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*    Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*    Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*    Neither the name of Bret McKee, Dad Dog Development, nor the names of its
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
*/

#include "phd.h"
#include "guide_gaussian_process.h"
#include "UDPGuidingInteraction.h"

GuideGaussianProcess::GuideGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis),
      udpInteraction(_T("localhost"),_T("1308"),_T("1309")),
      timestamps_(180),
      measurements_(180),
      modified_measurements_(180)
{
    reset();
}

GuideGaussianProcess::~GuideGaussianProcess(void)
{
}


ConfigDialogPane *GuideGaussianProcess::GetConfigDialogPane(wxWindow *pParent)
{
    return new GuideGaussianProcessDialogPane(pParent, this);
}


GuideGaussianProcess::
GuideGaussianProcessDialogPane::
GuideGaussianProcessDialogPane(wxWindow *pParent, GuideGaussianProcess *pGuideAlgorithm)
    : ConfigDialogPane(_("Gaussian Process Guide Algorithm"),pParent)
{
    m_pGuideAlgorithm = pGuideAlgorithm;
    DoAdd(new wxStaticText(pParent, wxID_ANY, _("Nothing to Configure"),wxPoint(-1,-1),wxSize(-1,-1)));
}

GuideGaussianProcess::
GuideGaussianProcessDialogPane::
~GuideGaussianProcessDialogPane()
{
}

void GuideGaussianProcess::
    GuideGaussianProcessDialogPane::
    UnloadValues(void)
{
}

void GuideGaussianProcess::
    GuideGaussianProcessDialogPane::
    LoadValues(void)
{
}


GUIDE_ALGORITHM GuideGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

double GuideGaussianProcess::result(double input)
{
    double buf[] = {input};
    
    udpInteraction.sendToUDPPort(buf, sizeof(buf));
    udpInteraction.receiveFromUDPPort(buf, sizeof(buf)); // this command blocks until matlab sends back something

    return buf[0];


    
}


void GuideGaussianProcess::reset()
{
    timestamps_.clear();
    measurements_.clear();
    modified_measurements_.clear();
    return;
}
