//
//  guide_gaussian_process.cpp
//  PHD
//
//  Created by Stephan Wenninger on 11/07/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#include "guide_gaussian_process.h"
#include "matlab_interaction.h"

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
    double buf[] = {input};
    
    MatlabInteraction::sendToUDPPort(_T("localhost"),_T("1308"),buf, sizeof(buf));
    MatlabInteraction::receiveFromUDPPort(_T("1309"),buf); // this command blocks until matlab sends back something

    return buf[0];
}


void GuideGaussianProcess::reset()
{
    return;
}
