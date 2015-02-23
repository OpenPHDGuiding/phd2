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
 
/* Created by Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de> and 
 * Edgar Klenske <edgar.klenske@tuebingen.mpg.de>
 */
 
#include <Eigen/Dense>
#include <vector>
#include <cstdint>
#include <iostream>


#include "covariance_functions.h"
#include "math_tools.h"

namespace covariance_functions {
MatrixStdVecPair covariance(const Eigen::VectorXd& params,
                            const Eigen::MatrixXd& x1, 
                            const Eigen::MatrixXd& x2) {
  covariance_functions::PeriodicSquareExponential covFuncPSE(params.head(4));
  covariance_functions::DiracDelta covFuncD(params.tail(1));
  MatrixStdVecPair cov1 = covFuncPSE.evaluate(x1.col(0),x2.col(0));

  MatrixStdVecPair cov2 = covFuncD.evaluate(x1.col(0), x2.col(0));

  Eigen::MatrixXd covariance = cov1.first + cov2.first;
  std::vector < Eigen::MatrixXd > derivative = cov1.second;
  derivative.push_back(cov2.second[0]);

  return std::make_pair(covariance, derivative);
}


PeriodicSquareExponential::PeriodicSquareExponential() :
    hyperParameters(Eigen::VectorXd()){ }

PeriodicSquareExponential::PeriodicSquareExponential(const Eigen::VectorXd &hyperParameters) {
  this->hyperParameters = hyperParameters;
}

MatrixStdVecPair PeriodicSquareExponential::evaluate(
  const Eigen::VectorXd& x, 
  const Eigen::VectorXd& y) {
  double lsP  = exp(hyperParameters(LengthScalePIndex));
  double plP  = exp(hyperParameters(PeriodLengthPIndex));
  // signal variance is squared
  double svP  = exp(2 * hyperParameters(SignalVariancePIndex));
  double lsSE = exp(hyperParameters(LengthScaleSEIndex));

  // Work with arrays internally, convert to matrix for return value.
  // This is because all the operations act elementwise.

  // Compute Distances
  Eigen::ArrayXXd squareDistanceXY = math_tools::squareDistance(
                                       x.transpose(),
                                       y.transpose());
  Eigen::ArrayXXd distanceXY = squareDistanceXY.sqrt();

  // Periodic Kernel
  Eigen::ArrayXXd P1 = (M_PI * distanceXY / plP);
  Eigen::ArrayXXd S1 = P1.sin() / lsP;
  Eigen::ArrayXXd Q1 = S1.square();
  Eigen::ArrayXXd K1 = (-2 * Q1).exp() * svP;

  // Square Exponential Kernel
  Eigen::ArrayXXd E2 = squareDistanceXY / pow(lsSE, 2);
  Eigen::ArrayXXd K2 = (-0.5 * E2).exp();

  // Combined Kernel
  Eigen::MatrixXd K = K1 * K2;

  // Derivatives
  std::vector<Eigen::MatrixXd> derivatives(4);

  derivatives[0] = 4 * K1 * Q1 * K2;
  derivatives[1] = 4 / lsP * K1 * S1 * P1.cos() * P1 * K2;
  derivatives[2] = 2 * K1 * K2;
  derivatives[3] = K2 * E2 * K1;

  return std::make_pair(K, derivatives);
}

void PeriodicSquareExponential::setParameters(const Eigen::VectorXd& params) {
  this->hyperParameters = params;
}

const Eigen::VectorXd& PeriodicSquareExponential::getParameters() const {
  return this->hyperParameters;
}

int PeriodicSquareExponential::getParameterCount() const {
  return this->hyperParameters.rows();
}

DiracDelta::DiracDelta(const Eigen::VectorXd& hyperParameters) {
  this->hyperParameters = hyperParameters;
}

MatrixStdVecPair DiracDelta::evaluate(const Eigen::VectorXd& x1,
                                      const Eigen::VectorXd& x2) {
  double sigma2 = exp(hyperParameters[0] * 2);

  const Eigen::VectorXd::ConstColXpr x1Col = x1.col(0);
  const Eigen::VectorXd::ConstColXpr x2Col = x2.col(0);
  Eigen::MatrixXd covariance(x1Col.size(), x2Col.size());

  for (int rows = 0; rows < covariance.rows(); rows++) {
    for (int cols = 0; cols < covariance.cols(); cols++) {
      covariance(rows, cols) = x1Col(rows) == x2Col(cols) ? sigma2 : 0;
    }
  }

  std::vector<Eigen::MatrixXd> derivative(1);
  derivative[0] = 2 * covariance;

  return std::make_pair(covariance, derivative);
}

void DiracDelta::setParameters(const Eigen::VectorXd& params) {
  this->hyperParameters = params;
}

const Eigen::VectorXd& DiracDelta::getParameters() const {
  return this->hyperParameters;
}

int DiracDelta::getParameterCount() const {
  return this->hyperParameters.rows();
}

}  // namespace covariance_functions
