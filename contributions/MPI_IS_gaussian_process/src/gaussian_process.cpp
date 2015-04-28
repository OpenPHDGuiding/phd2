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
#include <Eigen/Cholesky>
#include <cstdint>
#include <iostream>

#include "gaussian_process.h"
#include "math_tools.h"
#include "objective_function.h"
#include "covariance_functions.h"
#include "bfgs_optimizer.h"

GP::GP() : covFunc_(0), // initialize pointer to null
  data_loc_(Eigen::VectorXd()),
  data_out_(Eigen::VectorXd()),
  gram_matrix_(Eigen::MatrixXd()),
  alpha_(Eigen::VectorXd()),
  chol_gram_matrix_(Eigen::LDLT<Eigen::MatrixXd>()),
  log_noise_sd_(-1E20), 
  optimization_mask_(Eigen::VectorXi()),
  prior_vector_(1, 0)
{ }

GP::GP(const covariance_functions::CovFunc& covFunc) :
  covFunc_(covFunc.clone()),
  data_loc_(Eigen::VectorXd()),
  data_out_(Eigen::VectorXd()),
  gram_matrix_(Eigen::MatrixXd()),
  alpha_(Eigen::VectorXd()),
  chol_gram_matrix_(Eigen::LDLT<Eigen::MatrixXd>()),
  log_noise_sd_(-1E20), 
  optimization_mask_(Eigen::VectorXi()),
  prior_vector_(covFunc.getParameterCount() + 1, 0)
{ }

GP::GP(const double noise_variance,
       const covariance_functions::CovFunc& covFunc) :
  covFunc_(covFunc.clone()),
  data_loc_(Eigen::VectorXd()),
  data_out_(Eigen::VectorXd()),
  gram_matrix_(Eigen::MatrixXd()),
  alpha_(Eigen::VectorXd()),
  chol_gram_matrix_(Eigen::LDLT<Eigen::MatrixXd>()),
  log_noise_sd_(std::log(noise_variance)),
  optimization_mask_(Eigen::VectorXi()),
  prior_vector_(covFunc.getParameterCount() + 1, 0) 
{ }

GP::~GP() {
  delete this->covFunc_; // tidy up since we are responsible for the covFunc.
  for(std::vector<parameter_priors::ParameterPrior*>::iterator it =
      prior_vector_.begin(); it != prior_vector_.end(); ++it) {
    delete *it;
    *it = 0;
  }
}

GP::GP(const GP& that) :
  covFunc_(0),
  data_loc_(that.data_loc_),
  data_out_(that.data_out_),
  gram_matrix_(that.gram_matrix_),
  gram_matrix_derivatives_(that.gram_matrix_derivatives_),
  alpha_(that.alpha_),
  chol_gram_matrix_(that.chol_gram_matrix_),
  log_noise_sd_(that.log_noise_sd_),
  optimization_mask_(that.optimization_mask_) 
{
  covFunc_ = that.covFunc_->clone();
  for(size_t i = 0; i < that.prior_vector_.size(); ++i) {
    if(that.prior_vector_[i] != 0) {
      prior_vector_.push_back(that.prior_vector_[i]->clone());
    } else {
      prior_vector_.push_back(0);
    }
  }
}


bool GP::setCovarianceFunction(const covariance_functions::CovFunc& covFunc)
{
  if(data_loc_.size() != 0 || data_out_.size() != 0 || prior_vector_.size() != 1)
    return false;
  delete covFunc_;
  covFunc_ = covFunc.clone();
  
  // to fix: we cannot do the call twice because of this. We should find a more 
  // appropriate way of testing that (all zeros maybe?)
  prior_vector_.resize(covFunc_->getParameterCount() + 1, 0);

  return true;
}

GP& GP::operator=(const GP& that) {
  if (this != &that)
  {
    covariance_functions::CovFunc* temp = covFunc_;  // store old pointer...
    covFunc_ = that.covFunc_->clone();  // ... first clone ...
    delete temp;  // ... and then delete.

    for(size_t i = 0; i < prior_vector_.size(); ++i) {
      delete prior_vector_[i];
    }
    prior_vector_.clear();
    for(size_t i = 0; i < that.prior_vector_.size(); ++i) {
      if(that.prior_vector_[i] != 0) {
        prior_vector_.push_back(that.prior_vector_[i]->clone());
      } else {
        prior_vector_.push_back(0);
      }
    }

    // copy the rest
    data_loc_ = that.data_loc_;
    data_out_ = that.data_out_;
    gram_matrix_ = that.gram_matrix_;
    gram_matrix_derivatives_ = that.gram_matrix_derivatives_;
    alpha_ = that.alpha_;
    chol_gram_matrix_ = that.chol_gram_matrix_;
    log_noise_sd_ = that.log_noise_sd_;
    optimization_mask_ = that.optimization_mask_;
  }
  return *this;
}

Eigen::VectorXd GP::drawSample(const Eigen::VectorXd& locations) const {
  return drawSample(locations,
                    math_tools::generate_normal_random_matrix(locations.rows(), locations.cols()));
}

Eigen::VectorXd GP::drawSample(const Eigen::VectorXd& locations,
                               const Eigen::VectorXd& random_vector) const {
  Eigen::MatrixXd prior_covariance;
  Eigen::MatrixXd kernel_matrix;
  Eigen::LLT<Eigen::MatrixXd> chol_kernel_matrix;
  Eigen::MatrixXd samples;

  // we need the prior covariance for both, prior and posterior samples.
  prior_covariance = covFunc_->evaluate(locations, locations).first;
  kernel_matrix = prior_covariance;

  if (gram_matrix_.cols() == 0) { // i.e. only a prior
    kernel_matrix = prior_covariance + JITTER * Eigen::MatrixXd::Identity(
      prior_covariance.rows(), prior_covariance.cols());
  } else {
    Eigen::MatrixXd mixed_covariance;
    mixed_covariance = covFunc_->evaluate(locations, data_loc_).first;
    Eigen::MatrixXd posterior_covariance;
    posterior_covariance = prior_covariance - mixed_covariance *
      (chol_gram_matrix_.solve(mixed_covariance.transpose()));
    kernel_matrix = posterior_covariance + JITTER * Eigen::MatrixXd::Identity(
                      posterior_covariance.rows(), posterior_covariance.cols());
  }
  chol_kernel_matrix = kernel_matrix.llt();

  // Draw sample: s = chol(K)*x, where x is a random vector
  samples = chol_kernel_matrix.matrixL() * random_vector;

  return samples + std::exp(log_noise_sd_) *
      math_tools::generate_normal_random_matrix(samples.rows(), samples.cols());
}

void GP::infer() {
  assert(data_loc_.rows() > 0 && "Error: the GP is not yet initialized!");

  // The data covariance matrix
  covariance_functions::MatrixStdVecPair cov_result =
                                       covFunc_->evaluate(data_loc_, data_loc_);
  Eigen::MatrixXd & data_cov = cov_result.first;

  gram_matrix_derivatives_.resize(cov_result.second.size() + 1);
  for(size_t i = 0; i < cov_result.second.size(); i++)
  {
    gram_matrix_derivatives_[i+1].swap(cov_result.second[i]);
  }
  // noise derivative first
  gram_matrix_derivatives_[0] = 2*std::exp(2*log_noise_sd_) *
                                     Eigen::MatrixXd::Identity(data_cov.rows(), data_cov.cols());

  // compute and store the Gram matrix
  gram_matrix_.swap(data_cov);
  gram_matrix_ += (std::exp(2*log_noise_sd_) + JITTER) *
                            Eigen::MatrixXd::Identity(gram_matrix_.rows(), gram_matrix_.cols());

  // compute the Cholesky decomposition of the Gram matrix
  chol_gram_matrix_ = gram_matrix_.ldlt();

  // pre-compute the alpha, which is the solution of the chol to the data.
  alpha_ = chol_gram_matrix_.solve(data_out_);
}

void GP::infer(const Eigen::VectorXd& data_loc,
               const Eigen::VectorXd& data_out) {
  data_loc_ = data_loc;
  data_out_ = data_out;
  infer(); // updates the Gram matrix and its Cholesky decomposition
}

void GP::clear() {
  gram_matrix_ = Eigen::MatrixXd();
  chol_gram_matrix_ = Eigen::LDLT<Eigen::MatrixXd>();
  data_loc_ = Eigen::VectorXd();
  data_out_ = Eigen::VectorXd();
}

GP::VectorMatrixPair GP::predict(const Eigen::VectorXd& locations) const {

  // The prior covariance matrix (evaluated on test points)
  Eigen::MatrixXd prior_cov = this->covFunc_->evaluate(
                                locations, locations).first;

  if(data_loc_.rows() == 0) { // check if the data is empty
    return std::make_pair(Eigen::VectorXd::Zero(locations.size()), prior_cov);
  } else {

    // The mixed covariance matrix (test and data points)
    Eigen::MatrixXd mixed_cov = this->covFunc_->evaluate(
                                  locations, data_loc_).first;

    return predict(prior_cov, mixed_cov);
  }
}

GP::VectorMatrixPair GP::predict(const Eigen::MatrixXd& prior_cov,
                                 const Eigen::MatrixXd& mixed_cov) const {

  Eigen::VectorXd m = mixed_cov * alpha_;

  Eigen::MatrixXd v = prior_cov - mixed_cov *
                      (chol_gram_matrix_.solve(mixed_cov.transpose()));

  return std::make_pair(m, v);
}

double GP::neg_log_posterior() const {
  double result = neg_log_likelihood();
  Eigen::VectorXd hyperParameters = getHyperParameters();
  for(size_t i = 0; i < prior_vector_.size(); ++i) {
    if(prior_vector_[i] != 0) {
      result += prior_vector_[i]->neg_log_prob(hyperParameters[i]);
    }
  }
  return result;
}

Eigen::VectorXd GP::neg_log_posterior_gradient() const {
  Eigen::VectorXd result = neg_log_likelihood_gradient();
  Eigen::VectorXd hyperParameters = getHyperParameters();
  int j = 0; // counter for the mask
  for(size_t i = 0; i < prior_vector_.size(); ++i) {
    if(optimization_mask_.size() == 0 || optimization_mask_[i] == 1) {
      if(prior_vector_[i] != 0) {
        result[j] +=
            prior_vector_[i]->neg_log_prob_derivative(hyperParameters[i]);
      }
      ++j;
    }
  }
  return result;
}

double GP::neg_log_likelihood() const {
  double result = 0;
  if(gram_matrix_.rows() > 0) {
    // Implmented according to Equation (5.8) in Rasmussen & Williams, 2006
    Eigen::MatrixXd Z = chol_gram_matrix_.matrixL();
    result = data_out_.transpose()*chol_gram_matrix_.solve(data_out_);
    result += chol_gram_matrix_.vectorD().array().log().sum();
    result += data_out_.rows()*std::log(2*M_PI);
  }
  return 0.5*result;
}

Eigen::VectorXd GP::neg_log_likelihood_gradient() const {
  assert((static_cast<size_t>(optimization_mask_.rows()) ==
          gram_matrix_derivatives_.size() || optimization_mask_.rows() == 0) &&
          "The supplied mask has to have as many elements as hyperparameters!");

  Eigen::VectorXd result;
  if(optimization_mask_.rows() > 0) {
    result = Eigen::VectorXd(optimization_mask_.array().sum());
  } else {
    result = Eigen::VectorXd(gram_matrix_derivatives_.size());
  }

  Eigen::MatrixXd beta(gram_matrix_.rows(), gram_matrix_.cols());
  int j = 0; // separate index, needed to keep track of the mask's element
  // Implmented according to Equation (5.9) in Rasmussen & Williams, 2006
  for(size_t i = 0; i < gram_matrix_derivatives_.size(); ++i) {
    // if a mask is defined, only calculate derivatives where the mask is 1
    if(optimization_mask_.rows() == 0 || optimization_mask_[i] == 1) {
      beta = chol_gram_matrix_.solve(gram_matrix_derivatives_[i]);
      result[j] = -0.5*(alpha_.transpose()*gram_matrix_derivatives_[i]*alpha_ -
                        beta.trace());
      ++j;
    }
  }
  return result;
}

void GP::setHyperParameters(const Eigen::VectorXd& hyperParameters) {
  assert(hyperParameters.rows() == covFunc_->getParameterCount() + 1 &&
         "Wrong number of hyperparameters supplied to setHyperParameters()!");
  log_noise_sd_ = hyperParameters[0];
  covFunc_->setParameters(hyperParameters.tail(hyperParameters.rows()-1));
  if(data_loc_.rows() > 0) {
    infer();
  }
}

Eigen::VectorXd GP::getHyperParameters() const {
  Eigen::VectorXd hyperParameters(covFunc_->getParameterCount() + 1);
  hyperParameters << log_noise_sd_, covFunc_->getParameters();
  return hyperParameters;
}

void GP::setCovarianceHyperParameters(const Eigen::VectorXd& hyperParameters) {
  assert(hyperParameters.rows() == covFunc_->getParameterCount() &&
         "Wrong number of hyperparameters supplied to"
         "setCovarianceHyperParameters()!");
  covFunc_->setParameters(hyperParameters);
  infer();
}

/*!
 * A wrapper objective function that uses an objective in the form of a
 * gaussian process.
 */
class GPObjective : public bfgs_optimizer::ObjectiveFunction {
private:
  GP* gp_;
  GPObjective();

public:
  GPObjective(GP* gp) : gp_(gp) {}
  virtual void evaluate(const Eigen::VectorXd& x,
                        double* function_value,
                        Eigen::VectorXd* derivative) const
  {
    gp_->setHyperParameters(gp_->unmask(x));

    *function_value = gp_->neg_log_posterior();
    *derivative = gp_->neg_log_posterior_gradient();
  }
};


Eigen::VectorXd GP::optimizeHyperParameters(int number_of_linesearches) const {

  // this way, we avoid calling setHyperParameters at enter AND exit of evaluate
  // which indirectly calls infer.
  GP this_copy(*this);
  GPObjective gpo(&this_copy);

  // optimisation
  bfgs_optimizer::BFGS bfgs(&gpo, number_of_linesearches);

  Eigen::VectorXd result =
      this_copy.unmask(bfgs.minimize(this_copy.mask(getHyperParameters())));

#if 0
  this_copy.setHyperParameters(result);


  int periodicity_index_ = 2; // FIXME!
  double current_likelihood_value = this_copy.neg_log_posterior();
  double last_likelihood_value = current_likelihood_value;

  // Trisection for lower values
  last_likelihood_value = current_likelihood_value;
  while(last_likelihood_value >= current_likelihood_value - 1E-2) {
    last_likelihood_value = current_likelihood_value;
    result(periodicity_index_) -= std::log(3);
    this_copy.setHyperParameters(result);
    current_likelihood_value = this_copy.neg_log_likelihood();
  }
  // it was too small before, so we multiply by 3 again
  result(periodicity_index_) += std::log(3);

  // Bisection for lower values
  last_likelihood_value = current_likelihood_value;
  while(last_likelihood_value >= current_likelihood_value - 1E-2) {
    last_likelihood_value = current_likelihood_value;
    result(periodicity_index_) -= std::log(2);
    this_copy.setHyperParameters(result);
    current_likelihood_value = this_copy.neg_log_likelihood();
  }
  // it was too small before, so we multiply by 2 again
  result(periodicity_index_) += std::log(2);
#endif
  return result;
}

void GP::setOptimizationMask(const Eigen::VectorXi& mask) {
  optimization_mask_ = mask;
}

void GP::clearOptimizationMask() {
  optimization_mask_ = Eigen::VectorXi();
}

Eigen::VectorXd GP::mask(const Eigen::VectorXd& original_parameters) {
  Eigen::VectorXd temp_parameters;
  if(optimization_mask_.rows() > 0) {
    temp_parameters = Eigen::VectorXd(optimization_mask_.array().sum());
  } else {
    temp_parameters = original_parameters;
  }

  int j = 0; // separate index to keep track of masked parameters
  if(optimization_mask_.rows() > 0) {
    for(int i = 0; i < optimization_mask_.rows(); ++i) {
      if(optimization_mask_[i] == 1) {
        temp_parameters[j] = original_parameters[i];
        ++j;
      }
    }
  }
  return temp_parameters;
}

Eigen::VectorXd GP::unmask(const Eigen::VectorXd& masked_parameters) {
  Eigen::VectorXd original_parameters = getHyperParameters();
  assert((optimization_mask_.rows() == 0 || optimization_mask_.rows() ==
      original_parameters.rows()) &&
      "The supplied mask has to have as many elements as hyperparameters!");
  Eigen::VectorXd temp_parameters = original_parameters;
  int j = 0; // separate index to keep track of masked parameters
  if(optimization_mask_.rows() > 0) {
    for(int i = 0; i < optimization_mask_.rows(); ++i) {
      if(optimization_mask_[i] == 1) {
        temp_parameters[i] = masked_parameters[j];
        ++j;
      }
    }
  } else {
    temp_parameters = masked_parameters;
  }
  return temp_parameters;
}

void GP::setHyperPrior(const parameter_priors::ParameterPrior& prior, const
    int index) {
  prior_vector_[index] = prior.clone();
}

/*!
 * Removes a hyperprior for a given parameter index.
 */
void GP::clearHyperPrior(int index) {
  delete prior_vector_[index];
  prior_vector_[index] = 0;
}
