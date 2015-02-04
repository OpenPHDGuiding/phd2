// Copyright (c) 2014 Max Planck Society

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
