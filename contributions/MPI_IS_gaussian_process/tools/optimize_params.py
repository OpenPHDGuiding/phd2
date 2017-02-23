from pyevolve import *
import subprocess
import math

def eval_func(parameters):
    output = subprocess.check_output(["./GuidePerformanceEval",
    "{}".format(math.exp(parameters[0])),
    "{}".format(math.exp(parameters[1])),
    "{}".format(math.exp(parameters[2])),
    "{}".format(math.exp(parameters[3])),
    "{}".format(math.exp(parameters[4])),
    "{}".format(math.exp(parameters[5])),
    "{}".format(math.exp(parameters[6])),
    "{}".format(math.exp(parameters[7])),
    "{}".format(math.exp(parameters[8])),
    "{}".format(math.exp(parameters[9])),
    "{}".format(math.exp(parameters[10])),
    ])
    score = float(output)
    return 10 + score

def optimize():
    # Genome instance
    genome = G1DList.G1DList(11)
    #genome.setParams(rangemin=-4.6, rangemax=0.1) # 0.01 ... 1
    #genome.setParams(rangemin=4.6, rangemax=8.5) # 100 ... 5000
    #genome.setParams(rangemin=0.0, rangemax=4.6) # 1 ... 100
    genome.setParams(rangemin=-0.7, rangemax=0.7) # 0.5 ... 2

    # Change the initializator to Real values
    genome.initializator.set(Initializators.G1DListInitializatorReal)

    # Change the mutator to Gaussian Mutator
    genome.mutator.set(Mutators.G1DListMutatorRealGaussian)

    # The evaluator function (objective function)
    genome.evaluator.set(eval_func)

    # Genetic Algorithm Instance
    ga = GSimpleGA.GSimpleGA(genome)
    ga.selector.set(Selectors.GRouletteWheel)
    ga.nGenerations = 2
    ga.setPopulationSize(100)
    ga.setMultiProcessing(True)

    # Do the evolution
    ga.evolve(1)

    # Best individual
    print math.exp(ga.bestIndividual()[0]), math.exp(ga.bestIndividual()[1]), math.exp(ga.bestIndividual()[2]), math.exp(ga.bestIndividual()[3]), math.exp(ga.bestIndividual()[4]), math.exp(ga.bestIndividual()[5]), math.exp(ga.bestIndividual()[6]), math.exp(ga.bestIndividual()[7]), math.exp(ga.bestIndividual()[8]), math.exp(ga.bestIndividual()[9]), math.exp(ga.bestIndividual()[10])

if __name__ == "__main__":
    optimize()
