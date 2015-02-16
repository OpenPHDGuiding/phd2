/*
 * Copyright 2014-2015, Max Planck Society.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
/* Created by Edgar Klenske <edgar.klenske@tuebingen.mpg.de> and 
 * Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de>
 *
 * Unit Test for the implementation of the BFGS optimization algorithm.
 *
 */

#include <gtest/gtest.h>
#include "objective_function.h"
#include "bfgs_optimizer.h"

TEST(BFGSTest, bfgs_xsquared) {
    bfgs_optimizer::XSquared objective_function;
    int number_of_linesearches = 15;

    Eigen::MatrixXd hessian_guess(1, 1);
    hessian_guess << 0.6;
    double step_length_guess = 0.8;

    // First optimizer, with initial guess
    bfgs_optimizer::BFGS bfgs(&objective_function,
                              number_of_linesearches,
                              hessian_guess,
                              step_length_guess);

    double expected_result_with_hessian_approx = 0;

    // Start at x = 2
    Eigen::VectorXd initial_guess(1);
    initial_guess << 2;

    // minimize
    double bfgs_result = bfgs.minimize(initial_guess)(0);

    EXPECT_NEAR(bfgs_result, expected_result_with_hessian_approx, 1.0e-10);

    // Second minimizer, without initial guesses
    bfgs_optimizer::BFGS bfgs2(&objective_function,
                               number_of_linesearches);
    double bfgs2_result = bfgs2.minimize(initial_guess)(0);
    double expected_result_without_hessian_approx = 0;
    EXPECT_NEAR(bfgs2_result, expected_result_without_hessian_approx, 1.0e-15);
}

TEST(BFGSTest, xSquaredTest) {
    bfgs_optimizer::XSquared xsquared;

    Eigen::VectorXd loc0(1);
    Eigen::VectorXd loc1(1);
    Eigen::VectorXd loc2(1);
    Eigen::VectorXd loc3(1);
    loc0 << 1;
    loc1 << -1;
    loc2 << 0;
    loc3 << 2;

    typedef bfgs_optimizer::ObjectiveFunction::ValueAndDerivative
        ValueAndDerivative;

    double function_value;
    Eigen::VectorXd derivative(1);

    Eigen::VectorXd derive0(1);
    Eigen::VectorXd derive1(1);
    Eigen::VectorXd derive2(1);
    Eigen::VectorXd derive3(1);
    derive0 << 2;
    derive1 << -2;
    derive2 << 0;
    derive3 << 4;

    xsquared.evaluate(loc0, &function_value, &derivative);
    EXPECT_EQ(function_value, 1);
    EXPECT_EQ(derivative, derive0);
    xsquared.evaluate(loc1, &function_value, &derivative);
    EXPECT_EQ(function_value, 1);
    EXPECT_EQ(derivative, derive1);

    xsquared.evaluate(loc2, &function_value, &derivative);
    EXPECT_EQ(function_value, 0);
    EXPECT_EQ(derivative, derive2);

    xsquared.evaluate(loc3, &function_value, &derivative);
    EXPECT_EQ(function_value, 4);
    EXPECT_EQ(derivative, derive3);
}

TEST(BFGSTest, Rosenbrock_minimum_test) {
    bfgs_optimizer::RosenbrockFunction rosenbrock(1,100);

    Eigen::VectorXd min_location(2);
    min_location << 1 , 1;

    double function_value;
    Eigen::VectorXd derivative(2);

    rosenbrock.evaluate(min_location, &function_value, &derivative);

    EXPECT_EQ(function_value, 0);
    EXPECT_EQ(derivative, Eigen::VectorXd::Zero(2));
}

TEST(BFGSTest, bfgs_rosenbrock_43_linesearches) {
    /*
     * The Matlab Code finds the "exact" minimum (at (1.0, 1.0)) after 23
     * linesearches
     */

    bfgs_optimizer::RosenbrockFunction rosenbrock(1,100);
    int number_of_linesearches = 43;

    Eigen::VectorXd initial_guess(2);
    initial_guess << 3, 10;

    bfgs_optimizer::BFGS bfgs(&rosenbrock, number_of_linesearches);

    Eigen::VectorXd bfgs_result = bfgs.minimize(initial_guess);
    double bfgs_min_x = bfgs_result(0);
    double bfgs_min_y = bfgs_result(1);

    double expected_min_x = 1.0;
    double expected_min_y = 1.0;
    double max_error = 1e-4;

    EXPECT_NEAR(bfgs_min_x, expected_min_x, max_error);
    EXPECT_NEAR(bfgs_min_y, expected_min_y, max_error);
}

void evaluate(const Eigen::VectorXd& x, double* function_value,
              Eigen::VectorXd* derivative)  {
  double x_as_double = x(0);
  *function_value = x_as_double * x_as_double;
  (*derivative)(0) = 2 * x_as_double;
}

TEST(BFGSTest, function_pointer_test) {
  bfgs_optimizer::FunctionPointerObjective fpo(evaluate);

  Eigen::VectorXd loc0(1);
  Eigen::VectorXd loc1(1);
  Eigen::VectorXd loc2(1);
  Eigen::VectorXd loc3(1);
  loc0 << 1;
  loc1 << -1;
  loc2 << 0;
  loc3 << 2;

  double function_value;
  Eigen::VectorXd derivative(1);

  Eigen::VectorXd derive0(1);
  Eigen::VectorXd derive1(1);
  Eigen::VectorXd derive2(1);
  Eigen::VectorXd derive3(1);
  derive0 << 2;
  derive1 << -2;
  derive2 << 0;
  derive3 << 4;

  fpo.evaluate(loc0, &function_value, &derivative);
  EXPECT_EQ(function_value, 1);
  EXPECT_EQ(derivative, derive0);
  fpo.evaluate(loc1, &function_value, &derivative);
  EXPECT_EQ(function_value, 1);
  EXPECT_EQ(derivative, derive1);

  fpo.evaluate(loc2, &function_value, &derivative);
  EXPECT_EQ(function_value, 0);
  EXPECT_EQ(derivative, derive2);

  fpo.evaluate(loc3, &function_value, &derivative);
  EXPECT_EQ(function_value, 4);
  EXPECT_EQ(derivative, derive3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
