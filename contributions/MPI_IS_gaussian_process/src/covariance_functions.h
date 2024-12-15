/*
 * Copyright 2014-2017, Max Planck Society.
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

/**
 * @file
 * @date      2014-2017
 * @copyright Max Planck Society
 *
 * @author    Edgar D. Klenske <edgar.klenske@tuebingen.mpg.de>
 * @author    Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de>
 * @author    Raffi Enficiaud <raffi.enficiaud@tuebingen.mpg.de>
 *
 * @brief     The file holds the covariance functions that can be used with the GP class.
 */

#ifndef COVARIANCE_FUNCTIONS_H
#define COVARIANCE_FUNCTIONS_H

#include <Eigen/Dense>
#include <vector>
#include <list>
#include <utility>
#include <cstdint>

namespace covariance_functions
{
/*!@brief Base class definition for covariance functions
 */
class CovFunc
{
public:
    CovFunc() { }
    virtual ~CovFunc() { }

    /*!
     * Evaluates the covariance function, caches the quantities that are needed
     * to calculate gradient and Hessian.
     */
    virtual Eigen::MatrixXd evaluate(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2) = 0;

    //! Method to set the hyper-parameters.
    virtual void setParameters(const Eigen::VectorXd& params) = 0;
    virtual void setExtraParameters(const Eigen::VectorXd& params) = 0;

    //! Returns the hyper-parameters.
    virtual const Eigen::VectorXd& getParameters() const = 0;
    virtual const Eigen::VectorXd& getExtraParameters() const = 0;

    //! Returns the number of hyper-parameters.
    virtual int getParameterCount() const = 0;
    virtual int getExtraParameterCount() const = 0;

    //! Produces a clone to be able to copy the object.
    virtual CovFunc *clone() const = 0;
};

/*!
 * The function computes a combined covariance function. It is a periodic
 * covariance function with an additional square exponential. This
 * combination makes it possible to learn a signal that consists of both
 * periodic and aperiodic parts.
 *
 * Square Exponential Component:
 * @f[
 * k _{\textsc{se}}(t,t';\theta_\textsc{se},\ell_\textsc{se}) =
 * \theta_\textsc{se} \cdot
 * \exp\left(-\frac{(t-t')^2}{2\ell_\textsc{se}^{2}}\right)
 * @f]
 *
 * Periodic Component:
 * @f[
 * k_\textsc{p}(t,t';\theta_\textsc{p},\ell_\textsc{p},\lambda) =
 * \theta_\textsc{p} \cdot
 * \exp\left(-\frac{2\sin^2\left(\frac{\pi}{\lambda}
 * (t-t')\right)}{\ell_\textsc{p}^2}\right)
 * @f]
 *
 * Kernel Combination:
 * @f[
 * k _\textsc{c}(t,t';\theta_\textsc{se},\ell_\textsc{se},\theta_\textsc{p},
 * \ell_\textsc{p},\lambda) =
 * k_{\textsc{se}}(t,t';\theta_\textsc{se},\ell_\textsc{se})
 * +
 * k_\textsc{p}(t,t';\theta_\textsc{p},\ell_\textsc{p},\lambda)
 * @f]
 */
class PeriodicSquareExponential : public CovFunc
{
private:
    Eigen::VectorXd hyperParameters;
    Eigen::VectorXd extraParameters;

public:
    PeriodicSquareExponential();
    explicit PeriodicSquareExponential(const Eigen::VectorXd& hyperParameters);

    /*!
     * Evaluates the covariance function, caches the quantities that are needed
     * to calculate gradient and Hessian.
     */
    Eigen::MatrixXd evaluate(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2);

    //! Method to set the hyper-parameters.
    void setParameters(const Eigen::VectorXd& params);
    void setExtraParameters(const Eigen::VectorXd& params);

    //! Returns the hyper-parameters.
    const Eigen::VectorXd& getParameters() const;
    const Eigen::VectorXd& getExtraParameters() const;

    //! Returns the number of hyper-parameters.
    int getParameterCount() const;
    int getExtraParameterCount() const;

    /**
     * Produces a clone to be able to copy the object.
     */
    virtual CovFunc *clone() const { return new PeriodicSquareExponential(*this); }
};

/*!
 * The function computes a combined covariance function. It is a periodic
 * covariance function with two additional square exponential components.
 * This combination makes it possible to learn a signal that consists of
 * periodic parts, long-range aperiodic parts and small-range deformations.
 *
 * Square Exponential Component:
 * @f[
 * k _{\textsc{se}}(t,t';\theta_\textsc{se},\ell_\textsc{se}) =
 * \theta_\textsc{se} \cdot
 * \exp\left(-\frac{(t-t')^2}{2\ell_\textsc{se}^{2}}\right)
 * @f]
 *
 * Periodic Component:
 * @f[
 * k_\textsc{p}(t,t';\theta_\textsc{p},\ell_\textsc{p},\lambda) =
 * \theta_\textsc{p} \cdot
 * \exp\left(-\frac{2\sin^2\left(\frac{\pi}{\lambda}
 * (t-t')\right)}{\ell_\textsc{p}^2}\right)
 * @f]
 *
 * Kernel Combination:
 * @f[
 * k _\textsc{c}(t,t';\theta_\textsc{se},\ell_\textsc{se},\theta_\textsc{p},
 * \ell_\textsc{p},\lambda) =
 * k_{\textsc{se},1}(t,t';\theta_{\textsc{se},1},\ell_{\textsc{se},1})
 * +
 * k_\textsc{p}(t,t';\theta_\textsc{p},\ell_\textsc{p},\lambda)
 * +
 * k_{\textsc{se},2}(t,t';\theta_{\textsc{se},2},\ell_{\textsc{se},2})
 * @f]
 */
class PeriodicSquareExponential2 : public CovFunc
{
private:
    Eigen::VectorXd hyperParameters;
    Eigen::VectorXd extraParameters;

public:
    PeriodicSquareExponential2();
    explicit PeriodicSquareExponential2(const Eigen::VectorXd& hyperParameters);

    Eigen::MatrixXd evaluate(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2);

    //! Method to set the hyper-parameters.
    void setParameters(const Eigen::VectorXd& params);
    void setExtraParameters(const Eigen::VectorXd& params);

    //! Returns the hyper-parameters.
    const Eigen::VectorXd& getParameters() const;
    const Eigen::VectorXd& getExtraParameters() const;

    //! Returns the number of hyper-parameters.
    int getParameterCount() const;
    int getExtraParameterCount() const;

    /**
     * Produces a clone to be able to copy the object.
     */
    virtual CovFunc *clone() const { return new PeriodicSquareExponential2(*this); }
};
} // namespace covariance_functions
#endif // ifndef COVARIANCE_FUNCTIONS_H
