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

};


#endif /* defined(GP_IMPL) */
