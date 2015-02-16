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
 * @author  Klenske <edgar.klenske@tuebingen.mpg.de>
 *
 * @brief
 * The file holds the hyperparameter priors that can be used with the GP class.
 *
 */

#ifndef PARAMETER_PRIORS_H
#define PARAMETER_PRIORS_H

#include <Eigen/Dense>
#include <vector>
#include <utility>
#include <cstdint>

namespace parameter_priors {

typedef std::pair< double, Eigen::VectorXd > DoubleVecPair;

class ParameterPrior {
protected:
  Eigen::VectorXd parameters_;

public:
  ParameterPrior() {};
  virtual ~ParameterPrior() {};

  /*!
   * Returns the prior probability of this parameter coming from the prior
   * distribution. Note that there is one prior for every hyper parameter.
   */
  virtual double neg_log_prob(const double) const = 0;

  /*!
   * Returns the derivative with respect to the hyperparameter of the prior
   * probability of this parameter coming from the prior distribution.
   */
  virtual double neg_log_prob_derivative(const double) const = 0;

  /**
   * Method to set the hyper-parameters.
   */
  virtual void setParameters(const Eigen::VectorXd& params) = 0;

  /**
   * Returns the hyper-parameters.
   */
  virtual const Eigen::VectorXd getParameters() const = 0;

  /**
   * Returns the number of hyper-parameters.
   */
  virtual int getParameterCount() const = 0;

  /**
   * Produces a clone to be able to copy the object.
   */
  virtual ParameterPrior* clone() const = 0;
};


/*
 * The GammaPrior is a prior that allows for positive values only.
 * The GammaPrior is based on the gamma distribution that is defined by
 * \f$ \frac{1}{\Gamma(k)\theta^k}x^{k-1}e^{-\frac{x}{\theta}} \f$.
 * The input to the log probability functions is also in log space.
 */
class GammaPrior : public ParameterPrior {
private:
  // The parameter names are according to Wikipedia:
  // http://en.wikipedia.org/wiki/Gamma_distribution
  double theta_;
  double k_;

public:
  GammaPrior();
  virtual ~GammaPrior() {};
  explicit GammaPrior(const Eigen::VectorXd& parameters);

  /*!
   * Returns the prior probability of this parameter coming from the prior
   * distribution. Note that there is one prior for every hyper parameter.
   */
  virtual double neg_log_prob(const double hyperParameter) const;

  /*!
   * Returns the derivative with respect to the hyperparameter of the prior
   * probability of this parameter coming from the prior distribution.
   */
  virtual double neg_log_prob_derivative(const double hyperParameter) const;

  /**
   * Method to set the prior parameters. The first element is the mode of the
   * gamma distribution, the second parameter is the standard deviation. Note
   * that the gamma distribution is not symmetric.
   */
  virtual void setParameters(const Eigen::VectorXd& params);

  /**
   * Returns the hyper-parameters.
   */
  virtual const Eigen::VectorXd getParameters() const;

  /**
   * Returns the number of hyper-parameters.
   */
  virtual int getParameterCount() const;

  /**
   * Produces a clone to be able to copy the object.
   */
  virtual ParameterPrior* clone() const {
    return new GammaPrior(*this);
  }
};
}  // namespace covariance_functions
#endif  // ifndef PARAMETER_PRIORS
