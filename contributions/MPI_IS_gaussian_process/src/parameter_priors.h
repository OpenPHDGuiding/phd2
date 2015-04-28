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


/*! Base class for encoding a prior distribution.
 */
class ParameterPrior {
public:
  ParameterPrior() {}
  virtual ~ParameterPrior() {}

  /*! Returns the negative logarithm of the probability at the given point.
   */
  virtual double neg_log_prob(double point) const = 0;

  /*! Returns the derivative of the negative logarithm probability at the given input point.
   */
  virtual double neg_log_prob_derivative(double point) const = 0;

  /*! Sets the hyper-parameters.
   * 
   * The way this class is parametrized is specific to the real instance of the
   * prior. However it naturally corresponds to the getParameter method. 
   */
  virtual void setParameters(const Eigen::VectorXd& params) = 0;

  /*! Returns the parameters of the prior.
   */
  virtual Eigen::VectorXd getParameters() const = 0;

  /*! Returns the number of parameters.
   */
  virtual int getParameterCount() const = 0;

  /**
   * Returns a copy of this instance.
   */
  virtual ParameterPrior* clone() const = 0;
};


/*! Gamma prior distribution.
 *
 * This prior allows for positive values only. It is encoded as a classical
 * gamma distribution, defined as
 * @f[ 
 * \frac{x^{k-1} e^{-\frac{x}{\theta}} }{ \Gamma(k)\theta^k }
 * @f]
 *
 * Hence the parameters of this distribution are @f$\theta@f$ and @f$k@f$. 
 * Those parameters are fully qualified by giving as input parameters the mode and
 * standard deviation.
 *
 * The input to the log probability functions is also in log space.
 */
class GammaPrior : public ParameterPrior {
private:
  // The parameter names are according to Wikipedia:
  // http://en.wikipedia.org/wiki/Gamma_distribution
  double theta_;
  double k_;
  GammaPrior();

public:
  
  virtual ~GammaPrior() {}
  explicit GammaPrior(const Eigen::VectorXd& parameters);

  virtual double neg_log_prob(double hyperParameter) const;
  virtual double neg_log_prob_derivative(double hyperParameter) const;

  /**
   * Method to set the prior parameters. The first element is the mode of the
   * gamma distribution, the second parameter is the variance. Note
   * that the gamma distribution is not symmetric.
   */
  virtual void setParameters(const Eigen::VectorXd& params);

  //! The parameters are the mode and the variance. 
  virtual Eigen::VectorXd getParameters() const;

  /*! Returns the number of parameters.
   */
  virtual int getParameterCount() const {
    return 2;
  }

  virtual ParameterPrior* clone() const {
    return new GammaPrior(*this);
  }
};

/*! Logistic Prior, think of it as a soft constraint
*
* This prior allows for anything, but the probability on one 
* side of the center drops to zero logarithmically.
* @f[
* \text{prior}(x,c,s) = \frac{1}{1+\text{exp}(-s(x-c)}
* @f]
*
* Since this prior also needs support for negative numbers in s,
* the parameters are given as regular numbers. Only the hyperparameter
* is in log domain.
*/
class LogisticPrior : public ParameterPrior {
private:
    double center_;
    double steepness_;
    LogisticPrior();

public:

    virtual ~LogisticPrior() {}
    explicit LogisticPrior(const Eigen::VectorXd& parameters);

    virtual double neg_log_prob(double hyperParameter) const;
    virtual double neg_log_prob_derivative(double hyperParameter) const;

    /**
    * Method to set the prior parameters. The first element is the center
    * of the logistic function, the second element is the steepness.
    */
    virtual void setParameters(const Eigen::VectorXd& params);

    //! The parameters are the center and the steepness.
    virtual Eigen::VectorXd getParameters() const;

    // Returns the number of parameters.
    virtual int getParameterCount() const {
        return 2;
    }

    virtual ParameterPrior* clone() const {
        return new LogisticPrior(*this);
    }
};
}  // namespace parameter_priors
#endif  // ifndef PARAMETER_PRIORS
