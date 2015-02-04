// Copyright (c) 2014 Max Planck Society

/*!@file
 * @author  Edgar Klenske <eklenske@tuebingen.mpg.de>
 *
 * @date    2014-12-03
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
