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
 
/* Created by Stephan Wenninger <stephan.wenninger@tuebingen.mpg.de> 
 */

#include "objective_function.h"
#include <cmath>

namespace bfgs_optimizer {

ObjectiveFunction::ObjectiveFunction() {}

XSquared::XSquared() {}

void XSquared::evaluate(const Eigen::VectorXd& x,
                        double* function_value,
                        Eigen::VectorXd* derivative) const {
  double x_as_double = x(0);
  *function_value = x_as_double * x_as_double;
  (*derivative)(0) = 2 * x_as_double;
}

RosenbrockFunction::RosenbrockFunction(double a, double b)
  : a_(a),
  b_(b) {}

void RosenbrockFunction::evaluate(const Eigen::VectorXd& x_,
                                  double* function_value,
                                  Eigen::VectorXd* derivative) const {
  double x = x_(0);
  double y = x_(1);

  *function_value = std::pow(a_ - x, 2) + b_ * pow(y - x * x, 2);
  (*derivative)(0) = 4 * b_ * std::pow(x, 3) -
  4 * b_ * y * x + 2 * x -
  2 * a_;
  (*derivative)(1) = 2 * b_ * y -
  2 * b_ * pow(x, 2);
}

FunctionPointerObjective::FunctionPointerObjective(evaluate_function_type evaluate_function)
  : evaluate_function_(evaluate_function) 
{}

void FunctionPointerObjective::evaluate(const Eigen::VectorXd& x, 
                                        double* function_value, 
                                        Eigen::VectorXd* derivative) const {
  evaluate_function_(x, function_value, derivative);
}


}  // namespace bfgs_optimizer
