//
//  guide_gaussian_process.h
//  PHD
//
//  Created by Stephan Wenninger on 11/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#ifndef __PHD__guide_gaussian_process__
#define __PHD__guide_gaussian_process__

#include "phd.h"

class GuideGaussianProcess : public GuideAlgorithm
{
protected:
    
    
    
public:
    GuideGaussianProcess(Mount *pMount, GuideAxis axis);
    virtual ~GuideGaussianProcess(void);
    virtual GUIDE_ALGORITHM Algorithm(void);
    
    virtual double result(double input);
    virtual void reset();
    
};

#endif /* defined(__PHD__guide_gaussian_process__) */
