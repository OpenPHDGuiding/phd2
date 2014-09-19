//
//  GPImpl.h
//  PHD
//
//  Created by Stephan Wenninger on 11/08/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#ifndef GP_IMPL
#define GP_IMPL


#include <vector>
#include <Eigen/Dense>

class GPImpl {
    
    enum paramIndices { LengthScaleP
                      , PeriodLengthP
                      , SignalVarianceP
                      , LengthScaleSE
                      , Tau
                      };
    
public:
    GPImpl();
    Eigen::VectorXd hyperParams;
    
    /**
     Pairwise distance of two sets of vectors, The vectors are stored in the
     columns of the two parameter Matrices.

     @param a
     a Matrix of size Dxn

     @param b
     a Matrix of size Dxm

     @result
     a Matrix of size nxm containing all pairwise squared distances
    */
    Eigen::MatrixXd squareDistance(const Eigen::MatrixXd &a
                                 , const Eigen::MatrixXd &b);

    /**
     Overloaded version of squaredDistance
     */
    Eigen::MatrixXd squareDistance(const Eigen::MatrixXd &a);

    /**
     Covariance function for a kombined kernel (Squared Exponential (SE) and 
     Periodic (P))
     
     @param params 
        4 hyperparameters for the two kernels (lengthscales etc.)
     
     @param x
     @param y
        The two Measurements at time t and t´
     
     @result
       The result is a pair with the first component being the actual 
       Covariance Matrix.
       The second component of the pair is a std::vector of Matrices
       which contains the derivatives of the Covariance Matrix with respect
       to each of the 4 hyperparameters.
     */
    std::pair<Eigen::MatrixXd, std::vector<Eigen::MatrixXd> >
    combinedKernelCovariance (const Eigen::Vector4d &params
                                , const Eigen::MatrixXd &x
                                , const Eigen::MatrixXd &y);



};


#endif /* defined(GP_IMPL) */
