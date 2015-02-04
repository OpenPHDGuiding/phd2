// Copyright (c) 2014 Max Planck Society

/*!@file
 * @author  Stephan Wenninger <swenninger@tuebingen.mpg.de>
 *
 * @brief Provides an interface for calling scalar functions with defined derivative.
 */

#ifndef GP_TOOLS_OBJECTIVE_FUNCTION_H
#define GP_TOOLS_OBJECTIVE_FUNCTION_H

#include <utility>       // for std::pair
#include <Eigen/Dense>


namespace bfgs_optimizer {

/*!
 * ObjectiveFunction provides an interface for calling scalar functions
 * \f$ R^n -> R \f$ with defined derivative.
 *
 * The functions are required to implement the evaluate method which returns
 * the function value and the partial derivatives at a given point x.
 *
 * Example:
 * Subclass should implement \f$ f(x,y) = x^2 + y^2 \f$.
 * Then the evaluate functions takes a vector \f$(x, y)\f$ and returns a pair of
 * \f$f(x,y)\f$ and \f$(frac{\partial f}{\partial x}, frac{\partial f}
 * {\partial y} )\f$, which in this case would be:
 * \f$(x^2 + y^2) and \f$(2x, 2y)
 */
class ObjectiveFunction {
 public:
  typedef std::pair<double, Eigen::VectorXd> ValueAndDerivative;
  ObjectiveFunction();
  virtual ~ObjectiveFunction() {}
  /*!
   * Evaluates the function at given point x
   *
   * @param[in] x the vector of points we want to evaluate the function at
   * @param[out] function_value the function value at the point x
   * @param[out] derivative the partial derivatives
   * @note The caller of the evaluation function needs to provide an
   *     appropriate sized Vector as derivative (same size as the input
   *     vector x)
   */
  virtual void evaluate(const Eigen::VectorXd& x,
                        double* function_value,
                        Eigen::VectorXd* derivative) const = 0;
};

/*!
 *  Computes \f$f(x) = x^2\f$ and its derivative \f$f'(x) = 2x\f$
 */
class XSquared : public ObjectiveFunction {
 public:
  XSquared();
  ~XSquared() {}
  void evaluate(const Eigen::VectorXd& x,
                double* function_value,
                Eigen::VectorXd* derivative) const;
};

/*!
 * Rosenbrock Function \f$f(x,y) = (a-x)^2 + b (y - x^2)^2\f$
 *
 * The parameters are set to \f$a=1\f$ and \f$b=100\f$ as stated by
 * http://en.wikipedia.org/wiki/Rosenbrock_function
 *
 * The derivatives are computed as follows
 * \f[
 *   \frac{\partial f}{\partial x} = 4bx^3 - 4byx + 2x - 2a \\
 *   \frac{\partial f}{\partial y} = 2by - 2bx^2
 * \f]
 */
class RosenbrockFunction : public ObjectiveFunction {
 public:
  RosenbrockFunction(double a, double b);
  ~RosenbrockFunction() {}
  void evaluate(const Eigen::VectorXd& x_,
                double* function_value,
                Eigen::VectorXd* derivative) const;

 private:
  double a_;  // Usually a_ = 1;
  double b_;  // Usually b_ = 100;
};

/*!
 * A wrapper objective function that uses an objective in the form of a
 * function pointer.
 */
class FunctionPointerObjective : public ObjectiveFunction {
public:

  typedef void (*evaluate_function_type)(const Eigen::VectorXd&, double*, Eigen::VectorXd*);

  FunctionPointerObjective(evaluate_function_type evaluate_function);

  virtual void evaluate(const Eigen::VectorXd& x, 
                        double* function_value,
                        Eigen::VectorXd* derivative) const;

private:
  
  FunctionPointerObjective();
  evaluate_function_type evaluate_function_;
};


}  // namespace bfgs_optimizer

#endif  // GP_TOOLS_OBJECTIVE_FUNCTION_H
