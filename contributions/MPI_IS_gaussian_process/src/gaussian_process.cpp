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
 * @brief     The GP class implements the Gaussian Process functionality.
 */

#include <cstdint>

#include "gaussian_process.h"
#include "math_tools.h"
#include "covariance_functions.h"

// A functor for special orderings
struct covariance_ordering
{
    covariance_ordering(Eigen::VectorXd const& cov) : covariance_(cov) { }
    bool operator()(int a, int b) const { return covariance_[a] > covariance_[b]; }
    Eigen::VectorXd const& covariance_;
};

GP::GP()
    : covFunc_(nullptr), // initialize pointer to null
      covFuncProj_(nullptr), // initialize pointer to null
      data_loc_(Eigen::VectorXd()), data_out_(Eigen::VectorXd()), data_var_(Eigen::VectorXd()), gram_matrix_(Eigen::MatrixXd()),
      alpha_(Eigen::VectorXd()), chol_gram_matrix_(Eigen::LDLT<Eigen::MatrixXd>()), log_noise_sd_(-1E20),
      use_explicit_trend_(false), feature_vectors_(Eigen::MatrixXd()), feature_matrix_(Eigen::MatrixXd()),
      chol_feature_matrix_(Eigen::LDLT<Eigen::MatrixXd>()), beta_(Eigen::VectorXd())
{
}

GP::GP(const covariance_functions::CovFunc& covFunc)
    : covFunc_(covFunc.clone()), covFuncProj_(nullptr), data_loc_(Eigen::VectorXd()), data_out_(Eigen::VectorXd()),
      data_var_(Eigen::VectorXd()), gram_matrix_(Eigen::MatrixXd()), alpha_(Eigen::VectorXd()),
      chol_gram_matrix_(Eigen::LDLT<Eigen::MatrixXd>()), log_noise_sd_(-1E20), use_explicit_trend_(false),
      feature_vectors_(Eigen::MatrixXd()), feature_matrix_(Eigen::MatrixXd()),
      chol_feature_matrix_(Eigen::LDLT<Eigen::MatrixXd>()), beta_(Eigen::VectorXd())
{
}

GP::GP(const double noise_variance, const covariance_functions::CovFunc& covFunc)
    : covFunc_(covFunc.clone()), covFuncProj_(nullptr), data_loc_(Eigen::VectorXd()), data_out_(Eigen::VectorXd()),
      data_var_(Eigen::VectorXd()), gram_matrix_(Eigen::MatrixXd()), alpha_(Eigen::VectorXd()),
      chol_gram_matrix_(Eigen::LDLT<Eigen::MatrixXd>()), log_noise_sd_(std::log(noise_variance)), use_explicit_trend_(false),
      feature_vectors_(Eigen::MatrixXd()), feature_matrix_(Eigen::MatrixXd()),
      chol_feature_matrix_(Eigen::LDLT<Eigen::MatrixXd>()), beta_(Eigen::VectorXd())
{
}

GP::~GP()
{
    delete this->covFunc_; // tidy up since we are responsible for the covFunc.
    delete this->covFuncProj_; // tidy up since we are responsible for the covFuncProj.
}

GP::GP(const GP& that)
    : covFunc_(nullptr), // initialize to nullptr, clone later
      covFuncProj_(nullptr), // initialize to nullptr, clone later
      data_loc_(that.data_loc_), data_out_(that.data_out_), data_var_(that.data_var_), gram_matrix_(that.gram_matrix_),
      alpha_(that.alpha_), chol_gram_matrix_(that.chol_gram_matrix_), log_noise_sd_(that.log_noise_sd_),
      use_explicit_trend_(that.use_explicit_trend_), feature_vectors_(that.feature_vectors_),
      feature_matrix_(that.feature_matrix_), chol_feature_matrix_(that.chol_feature_matrix_), beta_(that.beta_)
{
    covFunc_ = that.covFunc_->clone();
    covFuncProj_ = that.covFuncProj_->clone();
}

bool GP::setCovarianceFunction(const covariance_functions::CovFunc& covFunc)
{
    // can only set the covariance function if training dataset is empty
    if (data_loc_.size() != 0 || data_out_.size() != 0)
        return false;
    delete covFunc_; // initialized to zero, so delete is safe
    covFunc_ = covFunc.clone();

    return true;
}

void GP::enableOutputProjection(const covariance_functions::CovFunc& covFuncProj)
{
    delete covFuncProj_; // initialized to zero, so delete is safe
    covFuncProj_ = covFuncProj.clone();
}

void GP::disableOutputProjection()
{
    delete covFuncProj_; // initialized to zero, so delete is safe
    covFuncProj_ = nullptr;
}

GP& GP::operator=(const GP& that)
{
    if (this != &that)
    {
        covariance_functions::CovFunc *temp = covFunc_; // store old pointer...
        covFunc_ = that.covFunc_->clone(); // ... first clone ...
        delete temp; // ... and then delete.

        // copy the rest
        data_loc_ = that.data_loc_;
        data_out_ = that.data_out_;
        data_var_ = that.data_var_;
        gram_matrix_ = that.gram_matrix_;
        alpha_ = that.alpha_;
        chol_gram_matrix_ = that.chol_gram_matrix_;
        log_noise_sd_ = that.log_noise_sd_;
    }
    return *this;
}

Eigen::VectorXd GP::drawSample(const Eigen::VectorXd& locations) const
{
    return drawSample(locations, math_tools::generate_normal_random_matrix(locations.rows(), locations.cols()));
}

Eigen::VectorXd GP::drawSample(const Eigen::VectorXd& locations, const Eigen::VectorXd& random_vector) const
{
    Eigen::MatrixXd prior_covariance;
    Eigen::MatrixXd kernel_matrix;
    Eigen::LLT<Eigen::MatrixXd> chol_kernel_matrix;
    Eigen::MatrixXd samples;

    // we need the prior covariance for both, prior and posterior samples.
    prior_covariance = covFunc_->evaluate(locations, locations);
    kernel_matrix = prior_covariance;

    if (gram_matrix_.cols() == 0) // no data, i.e. only a prior
    {
        kernel_matrix = prior_covariance + JITTER * Eigen::MatrixXd::Identity(prior_covariance.rows(), prior_covariance.cols());
    }
    else // we have some data
    {
        Eigen::MatrixXd mixed_covariance;
        mixed_covariance = covFunc_->evaluate(locations, data_loc_);
        Eigen::MatrixXd posterior_covariance;
        posterior_covariance = prior_covariance - mixed_covariance * (chol_gram_matrix_.solve(mixed_covariance.transpose()));
        kernel_matrix =
            posterior_covariance + JITTER * Eigen::MatrixXd::Identity(posterior_covariance.rows(), posterior_covariance.cols());
    }
    chol_kernel_matrix = kernel_matrix.llt();

    // Draw sample: s = chol(K)*x, where x is a random vector
    samples = chol_kernel_matrix.matrixL() * random_vector;

    // add the measurement noise on return
    return samples + std::exp(log_noise_sd_) * math_tools::generate_normal_random_matrix(samples.rows(), samples.cols());
}

void GP::infer()
{
    assert(data_loc_.rows() > 0 && "Error: the GP is not yet initialized!");

    // The data covariance matrix
    Eigen::MatrixXd data_cov = covFunc_->evaluate(data_loc_, data_loc_);

    // swapping in the Gram matrix is faster than directly assigning it
    gram_matrix_.swap(data_cov); // store the new data_cov as gram matrix
    if (data_var_.rows() == 0) // homoscedastic
    {
        gram_matrix_ +=
            (std::exp(2 * log_noise_sd_) + JITTER) * Eigen::MatrixXd::Identity(gram_matrix_.rows(), gram_matrix_.cols());
    }
    else // heteroscedastic
    {
        gram_matrix_ += data_var_.asDiagonal();
    }

    // compute the Cholesky decomposition of the Gram matrix
    chol_gram_matrix_ = gram_matrix_.ldlt();

    // pre-compute the alpha, which is the solution of the chol to the data
    alpha_ = chol_gram_matrix_.solve(data_out_);

    if (use_explicit_trend_)
    {
        feature_vectors_ = Eigen::MatrixXd(2, data_loc_.rows());
        // precompute necessary matrices for the explicit trend function
        feature_vectors_.row(0) = Eigen::MatrixXd::Ones(1, data_loc_.rows()); // instead of pow(0)
        feature_vectors_.row(1) = data_loc_.array(); // instead of pow(1)

        feature_matrix_ = feature_vectors_ * chol_gram_matrix_.solve(feature_vectors_.transpose());
        chol_feature_matrix_ = feature_matrix_.ldlt();

        beta_ = chol_feature_matrix_.solve(feature_vectors_) * alpha_;
    }
}

void GP::infer(const Eigen::VectorXd& data_loc, const Eigen::VectorXd& data_out,
               const Eigen::VectorXd& data_var /* = EigenVectorXd() */)
{
    data_loc_ = data_loc;
    data_out_ = data_out;
    if (data_var.rows() > 0)
    {
        data_var_ = data_var;
    }
    infer(); // updates the Gram matrix and its Cholesky decomposition
}

void GP::inferSD(const Eigen::VectorXd& data_loc, const Eigen::VectorXd& data_out, const int n,
                 const Eigen::VectorXd& data_var /* = EigenVectorXd() */,
                 const double prediction_point /*= std::numeric_limits<double>::quiet_NaN()*/)
{
    Eigen::VectorXd covariance;

    Eigen::VectorXd prediction_loc(1);
    if (math_tools::isNaN(prediction_point))
    {
        // if none given, use the last datapoint as prediction reference
        prediction_loc = data_loc.tail(1);
    }
    else
    {
        prediction_loc << prediction_point;
    }

    // calculate covariance between data and prediction point for point selection
    covariance = covFunc_->evaluate(data_loc, prediction_loc);

    // generate index vector
    std::vector<int> index(covariance.size(), 0);
    for (size_t i = 0; i != index.size(); i++)
    {
        index[i] = i;
    }

    // sort indices with respect to covariance value
    std::sort(index.begin(), index.end(), covariance_ordering(covariance));

    bool use_var = data_var.rows() > 0; // true means heteroscedastic noise

    if (n < data_loc.rows())
    {
        std::vector<double> loc_arr(n);
        std::vector<double> out_arr(n);
        std::vector<double> var_arr(n);

        for (int i = 0; i < n; ++i)
        {
            loc_arr[i] = data_loc[index[i]];
            out_arr[i] = data_out[index[i]];
            if (use_var)
            {
                var_arr[i] = data_var[index[i]];
            }
        }

        data_loc_ = Eigen::Map<Eigen::VectorXd>(loc_arr.data(), n, 1);
        data_out_ = Eigen::Map<Eigen::VectorXd>(out_arr.data(), n, 1);
        if (use_var)
        {
            data_var_ = Eigen::Map<Eigen::VectorXd>(var_arr.data(), n, 1);
        }
    }
    else // we can use all points and don't neet to select
    {
        data_loc_ = data_loc;
        data_out_ = data_out;
        if (use_var)
        {
            data_var_ = data_var;
        }
    }
    infer();
}

void GP::clearData()
{
    gram_matrix_ = Eigen::MatrixXd();
    chol_gram_matrix_ = Eigen::LDLT<Eigen::MatrixXd>();
    data_loc_ = Eigen::VectorXd();
    data_out_ = Eigen::VectorXd();
}

Eigen::VectorXd GP::predict(const Eigen::VectorXd& locations, Eigen::VectorXd *variances /*=nullptr*/) const
{
    // The prior covariance matrix (evaluated on test points)
    Eigen::MatrixXd prior_cov = covFunc_->evaluate(locations, locations);

    if (data_loc_.rows() == 0) // check if the data is empty
    {
        if (variances != nullptr)
        {
            (*variances) = prior_cov.diagonal();
        }
        return Eigen::VectorXd::Zero(locations.size());
    }
    else
    {
        // Calculate mixed covariance matrix (test and data points)
        Eigen::MatrixXd mixed_cov = covFunc_->evaluate(locations, data_loc_);

        // Calculate feature matrix for linear feature
        Eigen::MatrixXd phi(2, locations.rows());
        if (use_explicit_trend_)
        {
            phi.row(0) = Eigen::MatrixXd::Ones(1, locations.rows()); // instead of pow(0)
            phi.row(1) = locations.array(); // instead of pow(1)

            return predict(prior_cov, mixed_cov, phi, variances);
        }
        return predict(prior_cov, mixed_cov, Eigen::MatrixXd(), variances);
    }
}

Eigen::VectorXd GP::predictProjected(const Eigen::VectorXd& locations, Eigen::VectorXd *variances /*=nullptr*/) const
{
    // use the suitable covariance function, depending on whether an
    // output projection is used or not.
    covariance_functions::CovFunc *covFunc = nullptr;
    if (covFuncProj_ == nullptr)
    {
        covFunc = covFunc_;
    }
    else
    {
        covFunc = covFuncProj_;
    }

    assert(covFunc != nullptr);

    // The prior covariance matrix (evaluated on test points)
    Eigen::MatrixXd prior_cov = covFunc->evaluate(locations, locations);

    if (data_loc_.rows() == 0) // check if the data is empty
    {
        if (variances != nullptr)
        {
            (*variances) = prior_cov.diagonal();
        }
        return Eigen::VectorXd::Zero(locations.size());
    }
    else
    {
        // The mixed covariance matrix (test and data points)
        Eigen::MatrixXd mixed_cov = covFunc->evaluate(locations, data_loc_);

        Eigen::MatrixXd phi(2, locations.rows());
        if (use_explicit_trend_)
        {
            // calculate the feature vectors for linear regression
            phi.row(0) = Eigen::MatrixXd::Ones(1, locations.rows()); // instead of pow(0)
            phi.row(1) = locations.array(); // instead of pow(1)

            return predict(prior_cov, mixed_cov, phi, variances);
        }
        return predict(prior_cov, mixed_cov, Eigen::MatrixXd(), variances);
    }
}

Eigen::VectorXd GP::predict(const Eigen::MatrixXd& prior_cov, const Eigen::MatrixXd& mixed_cov,
                            const Eigen::MatrixXd& phi /*=Eigen::MatrixXd()*/, Eigen::VectorXd *variances /*=nullptr*/) const
{

    // calculate GP mean from precomputed alpha vector
    Eigen::VectorXd m = mixed_cov * alpha_;

    // precompute K^{-1} * mixed_cov
    Eigen::MatrixXd gamma = chol_gram_matrix_.solve(mixed_cov.transpose());

    Eigen::MatrixXd R;

    // include fixed-features in the calculations
    if (use_explicit_trend_)
    {
        R = phi - feature_vectors_ * gamma;

        m += R.transpose() * beta_;
    }

    if (variances != nullptr)
    {
        // calculate GP variance
        Eigen::MatrixXd v = prior_cov - mixed_cov * gamma;

        // include fixed-features in the calculations
        if (use_explicit_trend_)
        {
            assert(R.size() > 0);
            v += R.transpose() * chol_feature_matrix_.solve(R);
        }

        (*variances) = v.diagonal();
    }
    return m;
}

void GP::setHyperParameters(const Eigen::VectorXd& hyperParameters)
{
    assert(hyperParameters.rows() == covFunc_->getParameterCount() + covFunc_->getExtraParameterCount() + 1 &&
           "Wrong number of hyperparameters supplied to setHyperParameters()!");
    log_noise_sd_ = hyperParameters[0];
    covFunc_->setParameters(hyperParameters.segment(1, covFunc_->getParameterCount()));
    covFunc_->setExtraParameters(hyperParameters.tail(covFunc_->getExtraParameterCount()));
    if (data_loc_.rows() > 0)
    {
        infer();
    }

    // if the projection kernel is set, set the parameters there as well.
    if (covFuncProj_ != nullptr)
    {
        covFuncProj_->setParameters(hyperParameters.segment(1, covFunc_->getParameterCount()));
        covFuncProj_->setExtraParameters(hyperParameters.tail(covFunc_->getExtraParameterCount()));
    }
}

Eigen::VectorXd GP::getHyperParameters() const
{
    Eigen::VectorXd hyperParameters(covFunc_->getParameterCount() + covFunc_->getExtraParameterCount() + 1);
    hyperParameters << log_noise_sd_, covFunc_->getParameters(), covFunc_->getExtraParameters();
    return hyperParameters;
}

void GP::enableExplicitTrend()
{
    use_explicit_trend_ = true;
}

void GP::disableExplicitTrend()
{
    use_explicit_trend_ = false;
}
