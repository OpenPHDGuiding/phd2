//
//  GPImpl.cpp
//  PHD
//
//  Created by Stephan Wenninger on 11/08/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

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
 If the two Matrices are the same (only checked by comparing their address, not
 elementwise) the mean only has to be computed once.
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

    // TODO Check if it´s worth comparing the Matrices elementwise
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
            // columns
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
