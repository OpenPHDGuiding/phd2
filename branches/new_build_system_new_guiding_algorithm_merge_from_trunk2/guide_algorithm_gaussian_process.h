//
//  guide_gaussian_process.h
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

#ifndef GUIDE_GAUSSIAN_PROCESS
#define GUIDE_GAUSSIAN_PROCESS

#include "guide_algorithm.h"

class wxStopWatch;

class GuideGaussianProcess : public GuideAlgorithm
{
private:
    struct gp_guide_parameters;
    class GuideGaussianProcessDialogPane;

    gp_guide_parameters* parameters;

    void HandleTimestamps();
    void HandleMeasurements(double input);
    void HandleModifiedMeasurements(double input);

protected:

    virtual double GetControlGain();
    virtual bool SetControlGain(double control_gain);

public:
    GuideGaussianProcess(Mount *pMount, GuideAxis axis);
    virtual ~GuideGaussianProcess(void);
    virtual GUIDE_ALGORITHM Algorithm(void);
    
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent);
    virtual double result(double input);
    virtual void reset();
    virtual wxString GetSettingsSummary();
    virtual wxString GetGuideAlgorithmClassName(void) const { return "Gaussian Process"; }
    
};

#endif  // GUIDE_GAUSSIAN_PROCESS
