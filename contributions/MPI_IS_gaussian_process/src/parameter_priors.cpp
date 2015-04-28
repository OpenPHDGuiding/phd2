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

/*!@file
 * @author  Edgar Klenske <edgar.klenske@tuebingen.mpg.de>
 * @author  Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de>
 *
 */

#include <Eigen/Dense>
#include <vector>
#include <cstdint>
#include <iostream>

#include "parameter_priors.h"
#include "math_tools.h"

namespace parameter_priors {

GammaPrior::GammaPrior() : theta_(0), k_(0) { }

GammaPrior::GammaPrior(const Eigen::VectorXd& parameters) : theta_(0), k_(0) {
  // need to do this with the set function since some conversion is done.
  setParameters(parameters);
}

double GammaPrior::neg_log_prob(double point) const {
  // note that hyp is encoded as log(hyp).
  // constant parts are left out for efficiency.
  return std::exp(point)/(theta_) - (k_ - 1) * (point);
}

double GammaPrior::neg_log_prob_derivative(double point) const {
  // note that hyp is encoded as log(hyp).
  return std::exp(point)/(theta_) - (k_ - 1);
}

void GammaPrior::setParameters(const Eigen::VectorXd& params) {
  // input is a 2-vector with mode and variance respectively. 
  // we have: mode = (k-1)\theta and variance = k\theta^2. 
  theta_ = -0.5*params[0]
      + 0.5*std::sqrt(params[0]*params[0]+4*params[1]);
  k_ = params[0]/theta_+1;
}

Eigen::VectorXd GammaPrior::getParameters() const {
  Eigen::VectorXd result(2);
  result << (k_-1)*theta_,      // convert back to the mode
            (k_*theta_*theta_); // convert back to the variance
  return result;
}

LogisticPrior::LogisticPrior() : center_(0), steepness_(0) { }

LogisticPrior::LogisticPrior(const Eigen::VectorXd& parameters) : center_(0), steepness_(0) {
    // need to do this with the set function since some conversion is done.
    setParameters(parameters);
}

double LogisticPrior::neg_log_prob(double hyp) const {
    // note that hyp is encoded as log(hyp)

    double prob = 1 / (1 + std::exp(-steepness_*(std::exp(hyp) - center_)));
    prob += std::numeric_limits<double>::min(); // prevent it from becoming zero.
    return -std::log(prob);
}

double LogisticPrior::neg_log_prob_derivative(double hyp) const {
    // note that hyp is encoded as log(hyp).
    double sig = 1 / (1 + std::exp(-steepness_*(std::exp(hyp) - center_)));
    double ddx_sig = sig*(1 - sig)*(steepness_*std::exp(hyp));
    return - ddx_sig / sig;
}

void LogisticPrior::setParameters(const Eigen::VectorXd& params) {
    center_ = params[0];
    steepness_ = params[1];
}

Eigen::VectorXd LogisticPrior::getParameters() const {
    Eigen::VectorXd result(2);
    result << center_, steepness_;
    return result;
}

}  // namespace parameter_priors
