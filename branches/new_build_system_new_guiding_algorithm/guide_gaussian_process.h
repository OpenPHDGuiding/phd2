//
//  guide_gaussian_process.h
//  PHD
//
//  Created by Stephan Wenninger on 11/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#ifndef __PHD__guide_gaussian_process__
#define __PHD__guide_gaussian_process__

#include "guide_algorithm.h"
#include "phd.h"

class GuideGaussianProcess : public GuideAlgorithm
{
protected:
    
    
    
public:
    GuideGaussianProcess(Mount *pMount, GuideAxis axis);
    double result(double input);
    void reset();
    
    virtual ConfigDialogPane *GetConfigDialogPane(wxWindow *pParent)=0;
    virtual GraphControlPane *GetGraphControlPane(wxWindow *pParent, const wxString& label) { return NULL; };
    virtual wxString GetSettingsSummary() { return ""; }
    virtual wxString GetGuideAlgorithmClassName(void) const = 0;
};

#endif /* defined(__PHD__guide_gaussian_process__) */
