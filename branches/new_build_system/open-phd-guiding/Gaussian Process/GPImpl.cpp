//
//  GPImpl.cpp
//  PHD
//
//  Created by Stephan Wenninger on 11/08/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//



#ifndef __APPLE__
#include "phd.h"
#endif

#include "GPImpl.h"
#include <iostream>

GPImpl::GPImpl()
{
    hyperParams = Eigen::VectorXd(5);
    hyperParams =
    (hyperParams << 5.234         // P_ell, length-scale of the periodic kernel
                  , 300           // P_p, period-length of the periodic kernel
                  , 0.355         // P_sf, signal-variance of the periodic kernel
                  , 200           // SE_ell, length-scale of the SE-kernel
                  , sqrt(2)*0.55*0.2 // tau, model variance for delta kernel
    ).finished().array().log();
}


/**
 The mean of the two Matrices is subtracted beforehand, because the squared 
 error is independent of the mean and this makes the squares smaller.
 If the two Matrices have the same address, it means the function was called 
 from the overloaded version, and thus the mean only has to be computed once.
 */
Eigen::MatrixXd GPImpl::squareDistance(const Eigen::MatrixXd &a
                                     , const Eigen::MatrixXd &b)
{
    auto   aRows = a.rows();
    double aCols = a.cols();
    double bCols = b.cols();
    
    Eigen::MatrixXd
        am(aRows,(int)aCols)           // a subtracted by the mean of a
      , bm(aRows,(int)bCols)           // b subtracted by the mean of b
      , result((int)aCols,(int)bCols); // the final aColsxbCols Matrix
    
    Eigen::VectorXd mean(aRows);

    if (&a == &b) // Same address?
    {
        mean = a.rowwise().mean();
        am = a.colwise() - mean;
        bm = am;
    }
    else
    {
        if (aRows != b.rows())
        {
            // TODO Show error when the two Matrices have different dimension
            // of rows (i.e. different dimensions of the vectors stored in the
            // columns)
        }

        mean = aCols/(aCols+bCols) * a.rowwise().mean()
             + bCols/(bCols+aCols) * b.rowwise().mean();
        
        am = a.colwise() - mean;
        bm = b.colwise() - mean;
    }
    
    auto a_square = am.array().square().colwise().sum().transpose().rowwise()
                   .replicate(bCols);
    auto b_square = bm.array().square().colwise().sum().colwise()
                   .replicate(aCols);
    auto twoab    = 2 * (am.transpose()) * bm;
    
    return (a_square.matrix() + b_square.matrix()) - twoab;
}

Eigen::MatrixXd GPImpl::squareDistance(const Eigen::MatrixXd &a)
{
    return squareDistance(a,a);
}


/**
 The function computes the combined Kernel k_p * k_se and their derivatives.

 The following equations show how the kernel is computed and which abbreviations
 are used to compute subexpressions and the derivatives.
 Periodic Kernel
 k_p  = svP * exp( -2 * (sin^2(pi/periodLength * (t-t´) / lengthScaleP^2)))
      = svP * exp( -2 * (sin(pi/periodLength * (t-t´) / lengthScaleP))^2)
      = svP * exp( -2 * (sin(P1) / lengthscaleP)^2)
      = svP * exp( -2 * S1^2)
      = svP * exp( -2 * Q1)
      = K1
 
 svP = signalVarianceP^2

 Squared Exponential Kernel
 k_se = exp ( -1 * (t- t´)^2 / (2 * lengthScaleSE^2))
      = exp (-1/2 * (t-t´)^2 / lengthScaleSE^2)
      = exp (-1/2 * E2)
      = K2
 
 Derivatives (.* is elementwise multiplication of matrices)
 D1 = 4 * K1 .* Q1 .* K2
 D2 = 4/lengthScaleP * K1 .* S1 .* cos(P1) .* P1 .* K2
 D3 = 2 * K1 .* K2
 D4 = K2 .* E2 .* K1

 The Matlab implementation has different cases for y=="diag" and a case where
 y is not given. These cases are never reached, though. So they are left out 
 at first.
 */
GPImpl::MatrixStdVecPair
GPImpl::combinedKernelCovariance(const Eigen::Vector4d &params
                                 , const Eigen::MatrixXd &x
                                 , const Eigen::MatrixXd &y)
{
    double lsP  = exp(params(LengthScalePIndex));
    double plP  = exp(params(PeriodLengthPIndex));
    double svP  = exp(2*params(SignalVariancePIndex)); // signal variance is squared
    double lsSE = exp(params(LengthScaleSEIndex));

    // Compute Distances
    Eigen::MatrixXd squareDistanceXY = squareDistance(x.transpose(), y.transpose());
    Eigen::MatrixXd distanceXY = squareDistanceXY.array().sqrt();

    // Periodic Kernel
    auto P1 = (M_PI * distanceXY / plP).array();
    auto S1 = P1.array().sin();
    auto Q1 = S1.square();
    auto K1 = (-2 * Q1).exp() * svP;

    // Squared Exponential Kernel
    auto E2 = squareDistanceXY / pow(lsSE,2);
    auto K2 = (-0.5 * E2).array().exp();

    // Combined Kernel
    auto K = K1.array() * K2;

    // Derivatives
    std::vector<Eigen::MatrixXd> derivatives(4);

    derivatives[0] = 4 * K1.array() * Q1.array() * K2;
    derivatives[1] = 4/lsP * K1.array() * S1.array() * P1.cos().array()
                           * P1.array() * K2;
    derivatives[2] = 2 * K1.array() * K2;
    derivatives[3] = K2.array() * E2.array() * K1;
    
    return std::make_pair(K, derivatives);

}


/**
 
 */
GPImpl::MatrixPair
GPImpl::covarianceDirac(const double tau , const Eigen::MatrixXd &x1
                                         , const Eigen::MatrixXd &x2)
{
    double tauSquared = exp(tau * 2);

    Eigen::VectorXd x1Col = x1.col(1);
    Eigen::VectorXd x2Col = x2.col(1);
    Eigen::MatrixXd covariance(x1Col.size(),x2Col.size());

    for (auto rows = 0; rows < covariance.rows(); rows++)
    {
        for (auto cols = 0; cols < covariance.cols(); cols++)
        {
            covariance(rows,cols) = x1Col(rows) == x2Col(cols) ? tauSquared : 0;
        }
    }

    auto derivative = 2 * covariance;

    return std::make_pair(covariance, derivative);
}


/**
 
 */
GPImpl::MatrixStdVecPair
GPImpl::covariance(const Eigen::VectorXd &params
                   , const Eigen::MatrixXd &x1
                   , const Eigen::MatrixXd &x2)
{
    MatrixStdVecPair cov1 = combinedKernelCovariance(params.head(4)
                                                     , x1.col(1)
                                                     , x2.col(1));
    MatrixPair cov2 = covarianceDirac(params(TauIndex), x1, x2);

    Eigen::MatrixXd covariance = cov1.first + cov2.first;
    auto derivative = cov1.second;
    derivative.push_back(cov2.second);

    return std::make_pair(covariance, derivative);
}









