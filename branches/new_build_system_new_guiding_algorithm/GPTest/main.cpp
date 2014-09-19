//
//  main.cpp
//  GPTest
//
//  Created by Stephan Wenninger on 11/08/14.
//  Copyright (c) 2014 open-phd-guiding. All rights reserved.
//

#include <iostream>
#include "GPImpl.h"
#include <gtest/gtest.h>

class GPImplTest : public ::testing::Test
{
public:
    GPImplTest()
    : a(Eigen::MatrixXd(4,3))
    , a_(Eigen::MatrixXd(4,3))
    , b(Eigen::MatrixXd(4,5))
    , c(Eigen::MatrixXd(3,4))
    , sqdistc(Eigen::MatrixXd(4,4))
    , sqdistab(Eigen::MatrixXd(3,5))
    {
        a <<  3, 5, 5
            , 4, 6, 6
            , 3, 2, 3
            , 1, 0, 3;

        a_ <<  3, 5, 5
             , 4, 6, 6
             , 3, 2, 3
             , 1, 0, 3;
        
        b <<  1, 4, 5, 6, 7
            , 3, 4, 5, 6, 7
            , 0, 2, 4, 20, 2
            , 2, 3, -2, -2, 2;

        c <<  1, 2, 3, 4
            , 4, 5, 6, 7
            , 6, 7, 8, 9;

        // Computed by Matlab
        sqdistc <<
              0, 3, 12, 27
            , 3, 0, 3 , 12
            ,12, 3, 0 ,  3
            ,27,12, 3 ,  0;

        sqdistab <<
          15,  6, 15, 311, 27
        , 33, 14,  9, 329,  9
        , 35,  6, 27, 315,  7;
    }
    
    virtual ~GPImplTest()
    {
        
    }
    
    GPImpl impl;
    Eigen::MatrixXd  a, a_, b, c, sqdistc, sqdistab;
    
};


TEST_F(GPImplTest, squareDistanceTest)
{
    // Test argument oder
    ASSERT_EQ(impl.squareDistance(a, b), impl.squareDistance(b, a).transpose());
    
    // Test that two identical Matrices give the same result
    // (wether they are the same object or not)
    ASSERT_EQ(impl.squareDistance(a, a_), impl.squareDistance(a, a));

    // Test that the implementation gives the same result as the Matlab
    // implementation
    ASSERT_EQ(impl.squareDistance(c, c), sqdistc);
    ASSERT_EQ(impl.squareDistance(a, b), sqdistab);
}


int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

