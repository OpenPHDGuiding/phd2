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
 * @brief     Provides mathematical tools needed for the Gaussian process toolbox.
 */

#ifndef GP_MATH_TOOLS_H
#define GP_MATH_TOOLS_H

#define MINIMAL_THETA 1e-7

#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>
#include <limits>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cmath>

// M_PI not part of the standard
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

namespace math_tools
{
/*!
 * The squareDistance is the pairwise distance between all points of the passed
 * vectors. I.e. it generates a symmetric matrix when two vectors are passed.
 *
 * The first dimension (rows) is the dimensionality of the input space, the
 * second dimension (columns) is the number of datapoints. The number of
 * rows must be identical.

  @param a
  a Matrix of size Dxn

  @param b
  a Matrix of size Dxm

  @result
  a Matrix of size nxm containing all pairwise squared distances
*/
Eigen::MatrixXd squareDistance(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

/*!
 * Single-input version of squaredDistance. For single inputs, it is assumed
 * that the pairwise distance matrix should be computed between the passed
 * vector itself.
 */
Eigen::MatrixXd squareDistance(const Eigen::MatrixXd& a);

/*!
 * Generates a uniformly distributed random matrix of values between 0 and 1.
 */
Eigen::MatrixXd generate_uniform_random_matrix_0_1(const size_t n, const size_t m);

/*!
 * Apply the Box-Muller transform, which transforms uniform random samples
 * to Gaussian distributed random samples.
 */
Eigen::MatrixXd box_muller(const Eigen::VectorXd& vRand);

/*!
 * Generates normal random samples. First it gets some uniform random samples
 * and then uses the Box-Muller transform to get normal samples out of it
 */
Eigen::MatrixXd generate_normal_random_matrix(const size_t n, const size_t m);

/*!
 * Checks if a value is NaN by comparing it with itself.
 *
 * The rationale is that NaN is the only value that does not evaluate to true
 * when comparing it with itself.
 * Taken from:
 * http://stackoverflow.com/questions/3437085/check-nan-number
 * http://stackoverflow.com/questions/570669/checking-if-a-double-or-float-is-nan-in-c
 */
inline bool isNaN(double x)
{
    return x != x;
}

static const double NaN = std::numeric_limits<double>::quiet_NaN();

/*!
 * Checks if a double is infinite via std::numeric_limits<double> functions.
 *
 * Taken from:
 * http://bytes.com/topic/c/answers/588254-how-check-double-inf-nan#post2309761
 */
inline bool isInf(double x)
{
    return x == std::numeric_limits<double>::infinity() || x == -std::numeric_limits<double>::infinity();
}

/*!
 * Calculates the spectrum of a data vector.
 *
 * Does pre-and postprocessing:
 * - The data is zero-padded until the desired resolution is reached.
 * - ditfft2 is called to compute the FFT.
 * - The frequencies from the padding are removed.
 * - The constant coefficient is removed.
 * - A list of frequencies is generated.
 */
std::pair<Eigen::VectorXd, Eigen::VectorXd> compute_spectrum(Eigen::VectorXd& data, int N = 0);

/*!
 * Computes a Hamming window (used to reduce spectral leakage of subsequent DFT).
 */
Eigen::VectorXd hamming_window(int N);

/*!
 * Computes the standard deviation of a vector... which is not part of Eigen.
 */
double stdandard_deviation(Eigen::VectorXd& input);

} // namespace math_tools

#endif // define GP_MATH_TOOLS_H
