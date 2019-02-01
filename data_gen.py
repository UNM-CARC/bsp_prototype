import os
import subprocess        as sp
import numpy             as np
#import matplotlib.pyplot as plt
import sys
#from   pylab import

#def parseResults(results, run, dist):

def buildArgs(distribution, interMean, interStdDev, iterations, nodes):
    #DISTRIBUTION = ["gaussian","exponential","flat","pareto","constant"]
    MPI     = ["mpirun", "-np"]
    DIR     = dir_path = os.path.dirname(os.path.realpath(__file__))
    CODE    = DIR + "/scripts/app_gen"
    PROGRAM = [CODE + "/app_gen_metrics"]
    TMP     = ["/tmp/results"]
    args    = MPI
    args   += [str(nodes)]
    args   += PROGRAM
    args   += [str(interMean)]
    args   += [str(interStdDev)]
    args   += ["" + distribution + ""]
    args   += [str(iterations)]
    return args

def mvTmp():
    DIR  = dir_path = os.path.dirname(os.path.realpath(__file__))
    try:
        print(DIR + "/gather.sh")
        sp.check_call([DIR + "/gather.sh"])
    except sp.CalledProcessError:
        pass # handle errors in the called executable
    except OSError:
        pass # executable not found

#def plotData():

def runProgram(nodes):
    ITERATIONS   = 1000
    DISTRIBUTION = ["exponential", "pareto"]
    MEAN         = [60, 80, 90, 120, 140, 160]
    STDDEV       = [10, 30, 50]

    data         = np.zeros((len(DISTRIBUTION), len(MEAN), len(STDDEV)))
    program      = []
    for dist in range(0, len(DISTRIBUTION)):
        for imean in range(0, len(MEAN)):
            for istdd in range(0, len(STDDEV)):
                program = buildArgs(DISTRIBUTION[dist], MEAN[imean], STDDEV[istdd], ITERATIONS, nodes)
                #print(program)
                data[dist][imean][istdd] = sp.check_output(program)
                #print(data[dist][imean][istdd])
    #mvTmp()
def is_intstring(s):
    try:
        int(s)
        return True
    except ValueError:
        return False


def main(argv):
    MAX_NODES = 32
    nodes     = 1
    for arg in sys.argv[1:]:
        if not is_intstring(arg):
            sys.exit("All arguments must be integers. Exit.")
        else:
            nodes = int(arg)

    if nodes > 0 and nodes < MAX_NODES:
        runProgram(nodes)
    else:
        print("Error, please use a number between 0 and " + str(MAX_NODES) + "\n")

if __name__ == '__main__':
    main(sys.argv[1:])
