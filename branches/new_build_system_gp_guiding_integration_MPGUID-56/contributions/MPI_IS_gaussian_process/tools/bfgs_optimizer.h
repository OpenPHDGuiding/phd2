// Copyright (c) 2014 Max Planck Society

/*!@file
 * @author  Stephan Wenninger <swenninger@tuebingen.mpg.de>
 * @author  Edgar Klenske <eklenske@tuebingen.mpg.de>
 *
 * This file provides the BFGS Algorithm 
 * (http://en.wikipedia.org/wiki/Broyden-Fletcher-Goldfarb-Shanno_algorithm )
 * needed for the Gaussian process toolbox.
 *
 */

#ifndef GP_TOOLS_OPTIMIZER_BFGS
#define GP_TOOLS_OPTIMIZER_BFGS

#include "objective_function.h"

namespace bfgs_optimizer {
namespace bfgs_details {
/*!
 * A point in the linesearch
 *
 * Consists of the point in the function, the function value and
 * the derivative at this point and a stepsize.
 */
struct LinesearchPoint {
  double x;
  double function_value;
  Eigen::VectorXd derivative;
  double stepsize;

  LinesearchPoint() {}

  LinesearchPoint(double x,
                  double function_value,
                  const Eigen::VectorXd& derivative,
                  double stepsize)
  : x(x),
    function_value(function_value),
    derivative(derivative),
    stepsize(stepsize) {}
};

/*!
 * A struct consisting of Wolfe Powell Conditions for the linesearch.
 *
 * The linesearch stops when the conditions are met.
 * The conditions ensure that the function value decreases enough with
 * regards to the current stepsize.
 */
struct WolfePowellConditions {
  double a;
  double b;
  double c;

  WolfePowellConditions(const LinesearchPoint &p);
};


/*!@brief Result of the line search algorithm of the BFGS.
 *
 */
struct LineSearchResult {
  Eigen::VectorXd x;
  double function_value;
  Eigen::VectorXd derivative;

  //! Indicates whether the line search conditions are met or not.
  //! If the conditions are not met, the BFGS tries to refine the current 
  //! state without estimating the Hessian. If this condition is still not
  //! met in the next step, the BFGS stops. 
  bool continue_hessian_estimation;

  LineSearchResult(const Eigen::VectorXd& x,
                   double function_value,
                   const Eigen::VectorXd& derivative,
                   bool continue_hessian_estimation_)
  : x(x),
    function_value(function_value),
    derivative(derivative),
    continue_hessian_estimation(continue_hessian_estimation_) {}
};
}  // namespace bfgs_details

/*!
 * BFGS class, implements the Broyden-Fletcher-Goldfarb-Shanno Algorithm.
 *
 * Example Usage:
 * BFGS bfgs(ObjectiveFunction,
 *           number of linesearches per minimisation,
 *           [approximate hessian matrix of the objective function],
 *           [initial step length]);
 * 
 * Eigen::VectorXd initial guess;
 * Eigen::vectorXd result = bfgs.minimize(initial guess);
 *
 * The Objective Function needs to have the definition space
 * \f$ R^n \rightarrow R\f$ and need to have a defined derivative.
 */
class BFGS {
 public:
  /*!
   * BFGS ctor
   *
   * Initializes the BFGS with an approximation of the hessian matrix
   * and a first step length.
   *
   * @param[in] objective_function The objective function that should be
   *     minimized.
   * @note The lifetime of the objective function is not managed by the BFGS
   *     object.
   *
   * @param[in] number_of_linesearches The number of linesearches done per
   *     minimisation.
   * @param[in] approx_hessian_matrix A first approximation of the hessian
   *     matrix of the objective function.
   * @param[in] initial_step_length The initial steplength that is used for
   *     the linesearch.
   */
  BFGS(ObjectiveFunction* objective_function,
       int number_of_linesearches,
       Eigen::MatrixXd approx_hessian_matrix,
       double initial_step_length);

  /*!
   * BFGS ctor
   *
   * No approximation of the hessian matrix is given, the algorithm starts
   * with an appropriate-sized (with regards to the dimensionality of the
   * objective function) identity matrix.
   *
   * @param[in] objective_function The objective function that should be
   *     minimized.
   * @note The lifetime of the objective function is not managed by the BFGS
   *     object.
   *
   * @param[in] number_of_linesearches The number of linesearches done per
   *     minimisation.
   */
  BFGS(ObjectiveFunction* objective_function,
       int number_of_linesearches);

  /*!
   * BFGS Algorithm
   *
   * Starts with an initial guess of the function minimum. Then does a
   * set of linesearches to get closer and updates the approximation
   * of the hessian matrix according to the results of the linesearch.
   *
   * The BFGS update rule for the Hessian Matrix is as follows:
   * \f[
   * H = H + (ty + y^T * Hy)/ty^2 * t * t^T - 1/ty * Hy * t^T - 1/ty * t *Hy^T
   * \f]
   * where
   * \f[
   * t = linesearch_result_x - initial_guess \\
   * y = linesearch_result_derivative - initial_derivative \\
   * ty = t^T * y \\
   * Hy = H * y
   * \f]
   *
   */
  Eigen::VectorXd minimize(const Eigen::VectorXd &initial_guess);

 private:
  ObjectiveFunction* objective_function_;
  int allowed_linesearches_;
  Eigen::MatrixXd approx_hessian_matrix_;
  double step_length_;
  bool hessian_and_stepsize_initialized_;

  /*!
   * Takes care of cubic extra- and interpolation
   *
   */
  double min_cubic(double x, double func_value_delta, double s0, double s1,
                   bool extrapolate);

  /*!
   * Checks the conditions for the linesearch
   *
   * @return An int value indicating the results of checking the conditions
   */
  int check_wolfe_powell_conditions(const bfgs_details::LinesearchPoint& p,
                        const bfgs_details::WolfePowellConditions& conditions);

  /*!
   * LineSearch Algorithm
   *
   * Finds a minimum by fitting a cubic function to the objective function
   *
   */
  bfgs_details::LineSearchResult linesearch(const Eigen::VectorXd& x,
                                            double function_value,
                                            const Eigen::VectorXd& derivative,
                                            const Eigen::VectorXd& direction,
                                            double stepsize);
};
}  // namespace bfgs_optimizer

#endif /* define GP_TOOLS_OPTIMIZER_BFGS */
