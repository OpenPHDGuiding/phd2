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
 * @brief
 * The file holds the covariance functions that can be used with the GP class.
 *
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

    typedef std::pair< Eigen::MatrixXd, std::vector< Eigen::MatrixXd> > MatrixStdVecPair;

    enum paramIndices { LengthScalePIndex,
                        PeriodLengthPIndex,
                        SignalVariancePIndex,
                        LengthScaleSEIndex,
                        SignalVarianceSEIndex,
                        TauIndex
                      };

    /*!@brief Base class definition for covariance functions
     */
    class CovFunc
    {
    public:
        CovFunc() {}
        virtual ~CovFunc() {}

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
        virtual CovFunc* clone() const = 0;
    };

    /*!
     * The function computes the combined Kernel k_p * k_se and their derivatives.
     *
     * The following equations show how the kernel is computed and which
     * abbreviations are used to compute subexpressions and the derivatives.
     * Periodic Kernel
     * @f[
     * k_p = svP * \exp( -2 * (\sin^2(\pi/periodLength * (t-t') / lengthScaleP^2)))
     *     = svP * \exp( -2 * (\sin(\pi/periodLength * (t-t') / lengthScaleP))^2)
     *     = svP * \exp( -2 * (\sin(P1) / lengthscaleP)^2)
     *     = svP * \exp( -2 * S1^2)
     *     = svP * \exp( -2 * Q1)
     *     = K1
     * @f]
     *
     * @f[
     * svP = signalVarianceP^2
     * @f]
     *
     * Squared Exponential Kernel
     * @f[
     * k_se = \exp ( -1 * (t- t')^2 / (2 * lengthScaleSE^2))
     *      = \exp (-1/2 * (t-t')^2 / lengthScaleSE^2)
     *      = \exp (-1/2 * E2)
     *      = K2
     * @f]
     *
     * Derivatives (.* is elementwise multiplication of matrices)
     * @f[
     * D1 = 4 * K1 .* Q1 .* K2
     * D2 = 4/lengthScaleP * K1 .* S1 .* cos(P1) .* P1 .* K2
     * D3 = 2 * K1 .* K2
     * D4 = K2 .* E2 .* K1
     * @f]
     *
     * The Matlab implementation has different cases for y=="diag" and a case where
     * y is not given. These cases are never reached, though. So they are left out
     * at first.
     *
     * This is a combined SE + Per kernel function */
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
         virtual CovFunc* clone() const
         {
             return new PeriodicSquareExponential(*this);
         }
     };


    /* This is a combined SE + Per + SE kernel function */
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
        virtual CovFunc* clone() const
        {
            return new PeriodicSquareExponential2(*this);
        }
    };
}  // namespace covariance_functions
#endif  // ifndef COVARIANCE_FUNCTIONS_H
