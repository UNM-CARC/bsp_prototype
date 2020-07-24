#!/bin/bash

module load gsl-2.5-gcc-7.3.0-i7icadp
module load openmpi-3.1.3-gcc-7.3.0-6javta3

export LD_LIBRARY_PATH=/users/soheilaj/bsp_prototype/lib:/users/sahba/wheeler-scratch/git/rabbitmq-c/build/librabbitmq:$LD_LIBRARY_PATH

#mpirun -np 1 --bind-to core --report-bindings --map:by ppr:1:node -machinefile $PBS_NODEFILE -x UCX_WARN_UNUSED_ENV_VARS=n ./bsp_prototype -w fwq -a 10000 -b 1000 -d gaussian -t 0 -i 100 -l 1 -z 0 -v

./bsp_prototype -w hpcg -a 16 -b 50 -t 0 -i 5 -l 1 -z 0 -v test.csv
