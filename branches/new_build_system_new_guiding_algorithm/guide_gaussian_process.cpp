//
//  guide_gaussian_process.cpp
//  PHD
//
//  Created by Stephan Wenninger on 11/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#include "guide_gaussian_process.h"
#include "phd.h"

GuideGaussianProcess::GuideGaussianProcess(Mount *pMount, GuideAxis axis)
    : GuideAlgorithm(pMount, axis)
{
    reset();
}


double GuideGaussianProcess::result(double input)
{
    
}


void GuideGaussianProcess::reset()
{
    return;
}
