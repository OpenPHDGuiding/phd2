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

#include "math_tools.h"
#include <stdexcept>
#include <cmath>
#include <cstdint>

namespace math_tools
{

Eigen::MatrixXd squareDistance(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b)
{
    int aRows = a.rows(); // bRows must be identical to aRows
    int aCols = a.cols();
    int bCols = b.cols();

    Eigen::MatrixXd am(aRows, aCols); // mean-corrected a
    Eigen::MatrixXd bm(aRows, bCols); // mean-corrected b
    // final result, aCols x bCols
    Eigen::MatrixXd result(aCols, bCols);

    Eigen::VectorXd mean(aRows);

    /*  If the two Matrices have the same address, it means the function was
        called from the overloaded version, and thus the mean only has to be
        computed once.
     */
    if (&a == &b) // Same address?
    {
        mean = a.rowwise().mean();
        am = a.colwise() - mean;
        bm = am;
    }
    else
    {
        if (aRows != b.rows())
        {
            throw std::runtime_error("Matrix dimension incorrect.");
        }

        mean = static_cast<double>(aCols) / (aCols + bCols) * a.rowwise().mean() +
            static_cast<double>(bCols) / (bCols + aCols) * b.rowwise().mean();

        // The mean of the two Matrices is subtracted beforehand, because the
        // squared error is independent of the mean and this makes the squares
        // smaller.
        am = a.colwise() - mean;
        bm = b.colwise() - mean;
    }

    // The square distance calculation (a - b)^2 is calculated as a^2 - 2*ab * b^2
    // (using the binomial formula) because of numerical stability.

    // fast version
    return ((am.array().square().colwise().sum().transpose().rowwise().replicate(bCols).matrix() +
             bm.array().square().colwise().sum().colwise().replicate(aCols).matrix()) -
            2 * (am.transpose()) * bm)
        .array()
        .max(0);

    /* // verbose version
    Eigen::MatrixXd a_square =
        am.array().square().colwise().sum().transpose().rowwise().replicate(bCols);

    Eigen::MatrixXd b_square =
        bm.array().square().colwise().sum().colwise().replicate(aCols);

    Eigen::MatrixXd twoab = 2 * (am.transpose()) * bm;

    return ((a_square.matrix() + b_square.matrix()) - twoab).array().max(0);
    */
}

Eigen::MatrixXd squareDistance(const Eigen::MatrixXd& a)
{
    return squareDistance(a, a);
}

Eigen::MatrixXd generate_uniform_random_matrix_0_1(const size_t n, const size_t m)
{
    Eigen::MatrixXd result = Eigen::MatrixXd(n, m);
    result.setRandom(); // here we get uniform random values between -1 and 1
    Eigen::MatrixXd temp = result.array() + 1; // shift to positive
    result = temp / 2.0; // divide to obtain 0/1 interval
    // prevent numerical problems by enforcing a particular interval
    result = result.array().max(1e-10); // eliminate too small values
    result = result.array().min(1.0); // eliminiate too large values
    return result;
}

Eigen::MatrixXd box_muller(const Eigen::VectorXd& vRand)
{
    size_t n = vRand.rows();
    size_t m = n / 2; // Box-Muller transforms pairs of numbers

    Eigen::ArrayXd rand1 = vRand.head(m);
    Eigen::ArrayXd rand2 = vRand.tail(m);

    /* Implemented according to
     * http://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
     */

    // enforce interval to avoid numerical issues
    rand1 = rand1.max(1e-10);
    rand1 = rand1.min(1.0);

    rand1 = -2 * rand1.log();
    rand1 = rand1.sqrt(); // this is an amplitude

    rand2 = rand2 * 2 * M_PI; // this is a random angle in the complex plane

    Eigen::MatrixXd result(2 * m, 1);
    Eigen::MatrixXd res1 = (rand1 * rand2.cos()).matrix(); // first elements
    Eigen::MatrixXd res2 = (rand1 * rand2.sin()).matrix(); // second elements
    result << res1, res2; // combine the pairs into one vector

    return result;
}

Eigen::MatrixXd generate_normal_random_matrix(const size_t n, const size_t m)
{
    // if n*m is odd, we need one random number extra!
    // therefore, we have to round up here.
    size_t N = static_cast<size_t>(std::ceil(n * m / 2.0));

    Eigen::MatrixXd result(2 * N, 1);
    // push random samples through the Box-Muller transform
    result = box_muller(generate_uniform_random_matrix_0_1(2 * N, 1));
    result.conservativeResize(n, m);
    return result;
}

std::pair<Eigen::VectorXd, Eigen::VectorXd> compute_spectrum(Eigen::VectorXd& data, int N)
{

    int N_data = data.rows();

    if (N < N_data)
    {
        N = N_data;
    }
    N = static_cast<int>(std::pow(2, std::ceil(std::log(N) / std::log(2)))); // map to nearest power of 2

    Eigen::VectorXd padded_data = Eigen::VectorXd::Zero(N);
    padded_data.head(N_data) = data;

    Eigen::FFT<double> fft;

    // initialize the double vector from Eigen vector. This works by initializing
    // with two pointers: 1) the first element of the data, 2) the last element of the data
    std::vector<double> vec_data(padded_data.data(), padded_data.data() + padded_data.rows() * padded_data.cols());
    std::vector<std::complex<double>> vec_result;
    fft.fwd(vec_result, vec_data); // this is the forward-FFT, from time domain to Fourier domain

    // map back to Eigen vector with the address of the first element and the size
    Eigen::VectorXcd result = Eigen::Map<Eigen::VectorXcd>(&vec_result[0], vec_result.size());

    // the low_index is the lowest useful frequency, depending on the number of actual datapoints
    int low_index = static_cast<int>(std::ceil(static_cast<double>(N) / static_cast<double>(N_data)));

    // prepare amplitudes and frequencies, don't return frequencies introduced by padding
    Eigen::VectorXd spectrum = result.segment(low_index, N / 2 - low_index + 1).array().abs().pow(2);
    Eigen::VectorXd frequencies = Eigen::VectorXd::LinSpaced(N / 2 - low_index + 1, low_index, N / 2);
    frequencies /= N;

    return std::make_pair(spectrum, frequencies);
}

Eigen::VectorXd hamming_window(int N)
{
    double alpha = 0.54;
    double beta = 0.46;

    Eigen::VectorXd range = Eigen::VectorXd::LinSpaced(N, 0, 1);
    Eigen::VectorXd window = alpha - beta * (2 * M_PI * range.array()).cos();

    return window;
}

double stdandard_deviation(Eigen::VectorXd& input)
{
    Eigen::ArrayXd centered = input.array() - input.array().mean();
    return std::sqrt(centered.pow(2).sum() / (centered.size() - 1));
}

} // namespace math_tools
