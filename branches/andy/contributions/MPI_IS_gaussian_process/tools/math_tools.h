// Copyright (c) 2014 Max Planck Society


/*!@file
 * @author  Stephan Wenninger <swenninger@tuebingen.mpg.de>
 * @author  Edgar Klenske <eklenske@tuebingen.mpg.de>
 *
 * @date    2014-09-14
 *
 * @detail
 *  This file provides mathematical tools needed for the Gaussian process toolbox.
 * 
 */


#ifndef GP_MATH_TOOLS_H
#define GP_MATH_TOOLS_H

#define MINIMAL_THETA 1e-7

#include <Eigen/Dense>
#include <limits>
#include <vector>
#include <stdexcept>
#include <cstdint>

namespace math_tools {
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
Eigen::MatrixXd squareDistance(
    const Eigen::MatrixXd& a, 
    const Eigen::MatrixXd& b);

/*!
 * Single-input version of squaredDistance. For single inputs, it is assumed
 * that the pairwise distance matrix should be computed between the passed
 * vector itself.
 */
Eigen::MatrixXd squareDistance(const Eigen::MatrixXd& a);

Eigen::MatrixXd generate_random_sequence(int d, int n);
Eigen::MatrixXd generate_random_sequence(
    int n, 
    Eigen::VectorXd x,
    Eigen::VectorXd t);

/*!
 * function M = exp_map (mu, E)
 * % Computes exponential map on a sphere
 * %
 * % many thanks to Soren Hauberg!
 * %
 * % Philipp Hennig, September 2012
 */
Eigen::MatrixXd exp_map(const Eigen::VectorXd& mu, const Eigen::MatrixXd& E);

Eigen::MatrixXd generate_uniform_random_matrix_0_1(
    const size_t n,
    const size_t m);

Eigen::MatrixXd box_muller(const Eigen::VectorXd &vRand);

/*!
 * Generates normal random samples. First it gets some uniform random samples
 * and then uses the Box-Muller transform to get normal samples out of it
 */
Eigen::MatrixXd generate_normal_random_matrix(
    const size_t n,
    const size_t m);


//! Returns a single random double drawn from the standard normal distribution
double generate_normal_random_double();

/*!
 * Checks if a value is NaN by comparing it with itself.
 *
 * The rationale is that NaN is the only value that does not evaluate to true 
 * when comparing it with itself.
 * Taken from:
 * http://stackoverflow.com/questions/3437085/check-nan-number
 * http://stackoverflow.com/questions/570669/checking-if-a-double-or-float-is-nan-in-c
 */
inline bool isNaN(double x) {
  return x != x;
}

/*!
 * Checks if a double is infinite via std::numeric_limits<double> functions.
 *
 * Taken from:
 * http://bytes.com/topic/c/answers/588254-how-check-double-inf-nan#post2309761
 */
inline bool isInf(double x) {
  return x == std::numeric_limits<double>::infinity() ||
         x == -std::numeric_limits<double>::infinity();
}

}  // namespace math_tools

#endif  // define GP_MATH_TOOLS_H
