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
public:
    typedef std::pair<Eigen::MatrixXd, Eigen::MatrixXd> MatrixPair;
    typedef std::pair<Eigen::MatrixXd, std::vector<Eigen::MatrixXd> >
            MatrixStdVecPair;

    
    enum paramIndices { LengthScalePIndex
                      , PeriodLengthPIndex
                      , SignalVariancePIndex
                      , LengthScaleSEIndex
                      , TauIndex
                      };
    
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
    MatrixStdVecPair combinedKernelCovariance (const Eigen::Vector4d &params
                                                , const Eigen::MatrixXd &x
                                                , const Eigen::MatrixXd &y);

    /**
     Covariance function
     
     TODO Documentation

     @param tau

     
     @param x1, x2
        Two matrices
     
     @result 
        A pair consisting of the covariance matrix and the derivative
        of the matrix
     */
    MatrixPair covarianceDirac (const double tau
                                , const Eigen::MatrixXd &x1
                                , const Eigen::MatrixXd &x2);


    /**
     Covariance function that combines the result of the PeriodicSEKernel and
     the covarianceDirac function
     
     @param params
        5 Hyperparameters for the different Kernels
     
     @param x1, x2
        The two matrices we want to compute the covariance from
     
     @result
        A pair consisting of the covariance matrix and the derivative
     */
    MatrixStdVecPair covariance (const Eigen::VectorXd &params
                                 , const Eigen::MatrixXd &x1
                                 , const Eigen::MatrixXd &x2);




};


#endif /* defined(GP_IMPL) */
