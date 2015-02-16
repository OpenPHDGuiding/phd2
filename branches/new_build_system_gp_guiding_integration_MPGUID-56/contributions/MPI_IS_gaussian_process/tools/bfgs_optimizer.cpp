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
 
/* Created by Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de> and 
 * Edgar Klenske <edgar.klenske@tuebingen.mpg.de>
 */

#include "bfgs_optimizer.h"

#include <algorithm>
#include <cmath>
#include <iostream>


#include "objective_function.h"
#include "math_tools.h"

namespace bfgs_optimizer {
namespace bfgs_details {
const double wolfe_powell_rho = 0;
const double wolfe_powell_sig = 0.5; // manually tuned for longer jumps

WolfePowellConditions::WolfePowellConditions(const LinesearchPoint& p) {
  a = wolfe_powell_rho * p.stepsize;
  b = p.function_value;
  c = -wolfe_powell_sig * p.stepsize;
}
}  // namespace bfgs_details

const double bfgs_interpolate_limit = 0.1;
const double bfgs_extrapolate_limit = 5.0;
const int    max_func_evals_per_linesearch = 10;

BFGS::BFGS(ObjectiveFunction* objective_function,
           int number_of_linesearches,
           Eigen::MatrixXd approx_hessian,
           double initial_step_length)
  : objective_function_(objective_function),
    allowed_linesearches_(number_of_linesearches),
    approx_hessian_matrix_(approx_hessian),
    step_length_(initial_step_length),
    hessian_and_stepsize_initialized_(true) {}


BFGS::BFGS(ObjectiveFunction* objective_function,
           int number_of_linesearches)
  : objective_function_(objective_function),
    allowed_linesearches_(number_of_linesearches),
    hessian_and_stepsize_initialized_(false) {}

double BFGS::min_cubic(double x, double func_value_delta, double s0, double s1,
                        bool extrapolate) {
    // Cubic interpolation
  double A = -6 * func_value_delta + 3 * (s0 + s1) * x;
  double B =  3 * func_value_delta - (2 * s0 + s1) * x;
  double z = 0.0;
  bool z_is_valid_double = true;

  /*
   * Computation is safe-guarded in order to prevent negative sqrts and
   * division by 0.0
   */
  if (B < 0) {
    if (std::abs(s0 - s1) < 1e-15) {  // Too close to 0.0?
      z_is_valid_double = false;
    } else {
      z = s0 * x / (s0 - s1);
    }
  } else {
    double sqrt_term = B * B - A * s0 * x;
    if (sqrt_term < 0.0) {
      z_is_valid_double = false;
    } else {
      double division_term = B + std::sqrt(sqrt_term);
      if (division_term < 1e-15) {
        z_is_valid_double = false;
      } else {
        z = -s0 * x * x / division_term;
        /*
         * Computation of z:
         * z = -s0 * x * x / (B + sqrt(B * B - A * s0 * x))
         */
      }
    }
  }

  z_is_valid_double = !math_tools::isNaN(z) &&
                      !math_tools::isInf(z) &&
                      z_is_valid_double;

  if (extrapolate) {
    if (!z_is_valid_double ||
        z < x ||
        z > x * bfgs_extrapolate_limit) {
      // Fix bad z
      z = x * bfgs_extrapolate_limit;
    }
      // Extrapolate by at least the interpolate limit
      z = std::max(z, (1 + bfgs_interpolate_limit) * x);
    } else {  // interpolate
      if (!z_is_valid_double ||
          z < 0 ||
          z > x) {
        // Fix bad z
        z = x/2;
      }
      /*
       * Result has to be away from the boundaries by at least
       * the interpolate limit
       */
      z = std::min(std::max(z, bfgs_interpolate_limit * x),
                   (1 - bfgs_interpolate_limit) * x);
  }
  return z;
}

int BFGS::check_wolfe_powell_conditions(const bfgs_details::LinesearchPoint& p,
        const bfgs_details::WolfePowellConditions& conditions) {
  int result;
  if (p.function_value > conditions.a * p.x + conditions.b) {
    // sufficient decrease condition not met
    result = (conditions.a > 0) ? -1 : -2;
  } else {
    if (p.stepsize < -conditions.c) {
      // curvature condition not met
      result = 0;
    } else if (p.stepsize > conditions.c) {
      // curvature condition not met
      result = 1;
    } else {
      // everything OK, line search successful
      result = 2;
    }
  }
  return result;
}

bfgs_details::LineSearchResult BFGS::linesearch(const Eigen::VectorXd& x,
                                        double function_value,
                                        const Eigen::VectorXd& derivative,
                                        const Eigen::VectorXd& direction,
                                        double stepsize) {
  // Set up the points
  bfgs_details::LinesearchPoint p0(0.0, function_value, derivative, stepsize);
  bfgs_details::LinesearchPoint p1 = p0;
  bfgs_details::LinesearchPoint p2;
  bfgs_details::LinesearchPoint p3;
  p3.x = this->step_length_;
  bool extrapolate = true;

  int j = 0;

  // The conditions are relative to the starting point p0
  bfgs_details::WolfePowellConditions conditions(p0);

  // Set up the variables that are passed to the evaluation function
  double function_value_objective_function;
  Eigen::VectorXd partial_derivative(x.size());

  // extrapolating as long as necessary
  while (true) {
    j++;

    this->objective_function_->evaluate(x + p3.x * direction,
                                        &function_value_objective_function,
                                        &partial_derivative);

    p3.function_value = function_value_objective_function;
    p3.derivative     = partial_derivative;
    p3.stepsize       = p3.derivative.transpose() * direction;

    // Are we done yet?
    if (check_wolfe_powell_conditions(p3, conditions) != 0 ||
        j >= max_func_evals_per_linesearch) {
      break;
    }
    p0 = p1;
    p1 = p3;
    p3.x = p0.x + min_cubic(p1.x - p0.x,
                            p1.function_value - p0.function_value,
                            p0.stepsize,
                            p1.stepsize,
                            extrapolate);
  }

  // interpolate as long as necessary
  while (true) {
    // p2 is now the best point so far
    p2 = (p1.function_value > p3.function_value) ? p3 : p1;

    if (check_wolfe_powell_conditions(p2, conditions) > 1 ||
        j >= max_func_evals_per_linesearch ) {
      break;
    }

    p2.x = p1.x + min_cubic(p3.x - p1.x,
                            p3.function_value - p1.function_value,
                            p1.stepsize,
                            p3.stepsize,
                            !extrapolate);
    j++;
    this->objective_function_->evaluate(x + p2.x * direction,
                                        &function_value_objective_function,
                                        &partial_derivative);

    p2.function_value = function_value_objective_function;
    p2.derivative     = partial_derivative;
    p2.stepsize       = p2.derivative.transpose() * direction;

    if ((check_wolfe_powell_conditions(p2, conditions) > -1 &&
         p2.stepsize > 0) ||
        check_wolfe_powell_conditions(p2, conditions) < -1) {
      p3 = p2;
    } else {
      p1 = p2;
    }
  }

  this->step_length_ = p2.x;
  return bfgs_details::LineSearchResult(x + p2.x * direction,
                                        p2.function_value,
                                        p2.derivative,
                                        check_wolfe_powell_conditions(p2, conditions) == 2);
}

Eigen::VectorXd BFGS::minimize(const Eigen::VectorXd& initial_guess) {

  double function_value;

  Eigen::VectorXd derivative(initial_guess.size());
  this->objective_function_->evaluate(initial_guess,
                                      &function_value,
                                      &derivative);

  Eigen::VectorXd x(initial_guess);

  // Initialize a first approximation of the hessian matrix
  if (!hessian_and_stepsize_initialized_) {
    int dimensions = initial_guess.size();
    approx_hessian_matrix_ = Eigen::MatrixXd::Identity(dimensions,
                                                       dimensions);
  }

  assert(initial_guess.size() == derivative.rows() &&
         "The size of the initial guess has to match the size of the derivative!");
  Eigen::VectorXd start_direction = -approx_hessian_matrix_ * derivative;
  double initial_step_size = start_direction.transpose() * derivative;

  // Initialize step length
  if (!hessian_and_stepsize_initialized_) {
    step_length_ = -1/(initial_step_size - 1);
    hessian_and_stepsize_initialized_ = true;
  }

  int i = 0;
  bool previous_line_search_condition = false;

  while (i < allowed_linesearches_) {
    bfgs_details::LineSearchResult result = linesearch(x,
                                                       function_value,
                                                       derivative,
                                                       start_direction,
                                                       initial_step_size);

    if(result.continue_hessian_estimation && result.x != x)
    {
      Eigen::VectorXd t  = result.x - x;
      Eigen::VectorXd y  = result.derivative - derivative;
      double ty          = t.transpose() * y;
      Eigen::VectorXd Hy = approx_hessian_matrix_  * y;

      /*
       * BFGS update rule:
       * H = H + (ty+y'*Hy)/ty^2*t*t' - 1/ty*Hy*t' - 1/ty*t*Hy'
       */
      approx_hessian_matrix_ = approx_hessian_matrix_
      + (ty + y.transpose() * Hy)/std::pow(ty, 2)
      * t * t.transpose()
      - 1/ty * Hy * t.transpose()
      - 1/ty * t * Hy.transpose();

      previous_line_search_condition = true;
    }
    else
    {
      if(!previous_line_search_condition)
        break;
      previous_line_search_condition = false;
    }

    // update direction and points
    start_direction   = - approx_hessian_matrix_ * result.derivative;

    // This seems to be best practice (by Carls Rasmussen):
    // don't try the unit step but try this instead:
    initial_step_size = start_direction.transpose() * result.derivative;
    x = result.x;
    // convergence condition
    if(fabs(function_value - result.function_value) < 1E-60) {
      break;
    }
    function_value = result.function_value;
    derivative     = result.derivative;
    i++;
  }
  return x;
}

}  // namespace bfgs_optimizer
