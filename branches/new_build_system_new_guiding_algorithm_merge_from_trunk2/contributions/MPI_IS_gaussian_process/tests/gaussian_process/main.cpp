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
    , matlabOutput(std::vector<Eigen::MatrixXd>(4))
    , d1(Eigen::MatrixXd(4,4))
    , d2(Eigen::MatrixXd(4,4))
    , d3(Eigen::MatrixXd(4,4))
    , d4(Eigen::MatrixXd(4,4))
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

        covarianceHyperParams = impl.hyperParams.head(4);
        d1 <<
                           0, 6.05275937943207e-06, 2.62136628163941e-05, 6.64664894871547e-05,
        6.05275937943207e-06,                    0,	5.23884546168771e-05, 0.000108633128829636,
        2.62136628163941e-05, 5.23884546168771e-05,                    0, 1.61360875235300e-05,
        6.64664894871547e-05, 0.000108633128829636, 1.61360875235300e-05,                    0;

        d2 <<
                           0, 6.05209560553364e-06, 2.62012048165344e-05, 6.63862926089977e-05,
        6.05209560553364e-06,                    0, 5.23386548301785e-05, 0.000108418610925749,
        2.62012048165344e-05, 5.23386548301785e-05,                    0, 1.61313685275124e-05,
        6.63862926089977e-05, 0.000108418610925749, 1.61313685275124e-05,                    0;

        d3 <<
        0.252050000000000, 0.252034495470160, 0.251982830176306, 0.251879575556079,
        0.252034495470160, 0.252050000000000, 0.251915703157596, 0.251771267088757,
        0.251982830176306, 0.251915703157596, 0.252050000000000, 0.252008659656078,
        0.251879575556079, 0.251771267088757, 0.252008659656078, 0.252050000000000;

        d4 <<
                           0, 9.45129358013101e-06, 4.09472099036497e-05, 0.000103900324916883,
        9.45129358013101e-06,                    0, 8.18726035262187e-05, 0.000169945605284911,
        4.09472099036497e-05, 8.18726035262187e-05,                    0, 2.52008659656078e-05,
        0.000103900324916883, 0.000169945605284911, 2.52008659656078e-05, 0;

        matlabOutput[0] = d1;
        matlabOutput[1] = d2;
        matlabOutput[2] = d3;
        matlabOutput[3] = d4;
    }
    
    virtual ~GPImplTest()
    {
        
    }
    
    GPImpl impl;
    Eigen::MatrixXd a, a_, b, c, sqdistc, sqdistab;
    Eigen::VectorXd covarianceHyperParams;
    std::vector<Eigen::MatrixXd> matlabOutput;

    Eigen::MatrixXd d1, d2, d3, d4;
/*    */
    };


TEST_F(GPImplTest, squareDistanceTest)
{
    // Test argument oder
    ASSERT_EQ(impl.squareDistance(a, b), impl.squareDistance(b, a).transpose());
    
    // Test that two identical Matrices give the same result
    // (wether they are the same object or not)
    ASSERT_EQ(impl.squareDistance(a, a_), impl.squareDistance(a, a));
    ASSERT_EQ(impl.squareDistance(a), impl.squareDistance(a,a));

    // Test that the implementation gives the same result as the Matlab
    // implementation
    ASSERT_EQ(impl.squareDistance(c, c), sqdistc);
    ASSERT_EQ(impl.squareDistance(a, b), sqdistab);
}


TEST_F(GPImplTest, CombinedKernelCovarianceTest)
{
    auto result =
      impl.combinedKernelCovariance(covarianceHyperParams, a, a);

    auto covariance  = result.first;
    auto derivatives = result.second;

    // Comparing these to the matlab output yields the same result except for
    // rounding errors (Errors do not exceed 0.003)
    for (int i = 0; i < derivatives.size(); i++)
    {
        auto derived = derivatives[i];
        auto matlabDerived = matlabOutput[i];

        for (int col = 0; col < derived.cols(); col++)
        {
            for (int row = 0; row < derived.rows(); row++)
            {
                EXPECT_NEAR(matlabDerived(row,col), derived(row,col), 0.003);
               // ASSERT_DOUBLE_EQ(matlabDerived(row,col), derived(row,col));
            }
        }
    }


}

TEST_F(GPImplTest, CovarianceDiracTest)
{
    Eigen::MatrixXd m = Eigen::MatrixXd::Random(6, 4);
    GPImpl::MatrixPair result1   = impl.covarianceDirac(log(1), m, m);
    Eigen::MatrixXd identity  = Eigen::MatrixXd::Identity(m.rows(), m.rows());
    EXPECT_EQ(result1.first, identity);
    EXPECT_EQ(result1.second, 2 * identity);
}


TEST_F(GPImplTest, CovarianceTest)
{
    auto result = impl.covariance(impl.hyperParams, c, c);

    // Check that we really pushed the last derivative on to the vector
    // Checked in Matlab that none of the derivatives actually is all 0s 
    EXPECT_EQ(result.second.size(), 5);
    for (auto elem : result.second)
    {
        EXPECT_FALSE((elem.array() == 0).all());
    }

    // Matlab showed, that the last derivative of covariance(hyp,c,sqdistc)
    // is all 0s
    auto result2 = impl.covariance(impl.hyperParams, c, sqdistc);
    EXPECT_TRUE((result2.second[4].array() == 0).all());
}

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

