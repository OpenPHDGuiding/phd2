//
//  guide_gaussian_process.cpp
//  PHD
//
//  Created by Stephan Wenninger on 11/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#include "guide_gaussian_process.h"

GuideGaussianProcess::GuideGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis)
{
    reset();
}

GuideGaussianProcess::~GuideGaussianProcess(void)
{
}

GUIDE_ALGORITHM GuideGaussianProcess::Algorithm(void)
{
    return GUIDE_ALGORITHM_GAUSSIAN_PROCESS;
}

double GuideGaussianProcess::result(double input)
{
    double result = 0.0;
    
    
    return result;
}


void GuideGaussianProcess::reset()
{
    return;
}
