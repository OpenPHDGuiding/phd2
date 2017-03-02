from pyevolve import *
import subprocess
import math
from GPyOpt.methods import BayesianOptimization
import numpy as np
import pdb

current_parameters = np.array([0.6, 0.01, 0.8, 20, 30, 10, 500, 100, 100, 25, 7])

def eval_func(input):
    parameters = np.exp(input[0])*current_parameters
    if (parameters[0] > 1.0 or parameters[1] > 1.0 or parameters[2] > 1.0):
        return -5.0

    scores = np.zeros(6)

    args = ['./GuidePerformanceEval', 'dataset03.csv']
    args.extend(map(str,parameters))
    scores[0] = float(subprocess.check_output(args))

    args = ['./GuidePerformanceEval', 'dataset04.csv']
    args.extend(map(str,parameters))
    scores[1] = float(subprocess.check_output(args))

    args = ['./GuidePerformanceEval', 'dataset05.csv']
    args.extend(map(str,parameters))
    scores[2] = float(subprocess.check_output(args))

    args = ['./GuidePerformanceEval', 'dataset06.csv']
    args.extend(map(str,parameters))
    scores[3] = float(subprocess.check_output(args))

    args = ['./GuidePerformanceEval', 'dataset07.csv']
    args.extend(map(str,parameters))
    scores[4] = float(subprocess.check_output(args))

    args = ['./GuidePerformanceEval', 'dataset08.csv']
    args.extend(map(str,parameters))
    scores[5] = float(subprocess.check_output(args))

    score = np.min(scores)

    return score

def optimize():
    bounds = [(0.5,2), (0.5,2), (0.5,2), (0.5,2), (0.5,2), (0.5,2), (0.5,2), (0.5,2), (0.5,2), (0.5,2), (0.5,2)]
    bounds = np.log(bounds)

    myBopt = BayesianOptimization(f=eval_func, bounds=bounds, num_cores=6, batch_size=6, verbosity=True, maximize=True, verbosity_model=True, initial_design_numdata=50)
    myBopt.run_optimization(verbosity=True, maximize=True)
    #print myBopt.__dict__
    print -myBopt.fx_opt
    opt_params = np.exp(myBopt.x_opt)*current_parameters

    print "{:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f} {:.2f}".format(opt_params[0], opt_params[1], opt_params[2], opt_params[3], opt_params[4], opt_params[5], opt_params[6], opt_params[7], opt_params[8], opt_params[9], opt_params[10])

if __name__ == "__main__":
    optimize()
