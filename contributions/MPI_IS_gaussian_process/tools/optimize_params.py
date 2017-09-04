"""
This file can help developers to optimize the parameters of the guiding algorithm.

GPOpt is used to do Bayesian optimization on the hyperparameters, the range for
optimization and the parameters to optimize are selected by setting the bounds
accordingly.

The function evaluation is done by calling a binary file GuidePerformanceEval
with the parameters as command line arguments. The return values is parsed from
stdin as a float.

The scoring algorithm tries to never degrade performance (uses the worst value
if it is negative) and averages the performance in all other cases. This way
we achieve good average performance while not degrading performance for anyone.
"""

import subprocess
import math
import numpy as np
import os
from GPyOpt.methods import BayesianOptimization

def eval_func(input):
    parameters = np.exp(input[0])
    folder = os.path.abspath(os.path.join('..','..','contributions', 'MPI_IS_gaussian_process', 'tests', 'gaussian_process'))

    # prepares command line arguments for an evaluation
    def make_args(file):
        return ['./GuidePerformanceEval', os.path.join(folder, f)] + [str(param) for param in parameters]

    # calculates the score for a specific dataset file
    def file_score(file):
        return float(subprocess.check_output(make_args(file)))

    # find all files in the directory
    files = os.walk(folder).next()[2]

    # extract all dataset files for the performance testing
    valid_files = [f for f in files if f.startswith('performance_') and f.endswith('.txt')]

    # calculate the scores for all files
    scores = [file_score(f) for f in valid_files]

    # only optimize the average if we pass regression testing
    if np.min(scores) < 0.0:
        score = np.min(scores) # make sure we pass regression tests
    else:
        score = np.mean(scores) # try to achieve good average performance

    return score

def optimize():
    """
    the parameters are:
        pontrol_gain
        min_periods_for_inference
        min_move
        SE0KLengthScale
        SE0KSignalVariance
        PKLengthScale
        PKPeriodLength
        PKSignalVariance
        SE1KLengthScale
        SE1KSignalVariance
        min_periods_for_period_estimation
        points_for_approximation
        prediction_gain
    """

    # the bounds are used to select which parameters to optimize,
    # and in which range to optimize. If the range is zero (e.g., (1.0,1.0))
    # the parameter isn't optimized at all.
    bounds = [(0.7,0.7), (2.0,2.0), (0.2,0.2), (700,700), (18,18), (10,10), (200,200), (20,20), (25,25), (10,10), (2,2), (100,100), (0.5,0.5)]
    bounds = np.log(bounds)

    # evaluate the current parameter set
    opt_params = [0.7, 2, 0.2, 700, 18, 10, 200, 20, 25, 10, 2, 100, 0.5]
    print eval_func([np.log(opt_params)])

    # the initial_design_numdata defines how many random values are tested.
    # apparently this is the most important value and should be as high as affordable.
    # num_cores and batch_size define how many evaluations run simultaneously.
    myBopt = BayesianOptimization(f=eval_func, bounds=bounds, num_cores=6, batch_size=6, maximize=True, initial_design_numdata=24)
    myBopt.run_optimization(verbosity=True, maximize=True)
    print -myBopt.fx_opt
    opt_params = np.exp(myBopt.x_opt)

    print "{:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f}".format(opt_params[0], opt_params[1], opt_params[2], opt_params[3], opt_params[4], opt_params[5], opt_params[6], opt_params[7], opt_params[8], opt_params[9], opt_params[10], opt_params[11], opt_params[12])

if __name__ == "__main__":
    optimize()
