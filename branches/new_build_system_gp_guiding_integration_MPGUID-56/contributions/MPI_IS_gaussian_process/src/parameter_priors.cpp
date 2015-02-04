// Copyright (c) 2014 Max Planck Society

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

double GammaPrior::neg_log_prob(const double hyperParameter) const {
  // note that hyp is encoded as log(hyp).
  // constant parts are left out for efficiency.
  double result = (k_ - 1) * (hyperParameter) - exp(hyperParameter)/(theta_);
  return result;
}

double GammaPrior::neg_log_prob_derivative(const double hyperParameter) const {
  // note that hyp is encoded as log(hyp).
  return (k_ - 1) - exp(hyperParameter)/(theta_);
}

void GammaPrior::setParameters(const Eigen::VectorXd& params) {
  // Easy to think of are the mode and the standard deviation.
  // For the computation, we need to convert to theta and k.
  theta_ = -0.5*params[0]
      + 0.5*std::sqrt(params[0]*params[0]+4*params[1]*params[1]);
  k_ = params[0]/theta_+1;
}

const Eigen::VectorXd GammaPrior::getParameters() const {
  Eigen::VectorXd result(2);
  result << (k_-1)*theta_; // convert back to the mode
  result << (k_*theta_*theta_); // convert back to the mean
  return result;
}

int GammaPrior::getParameterCount() const {
  return 2;
}

}  // namespace parameter_priors
