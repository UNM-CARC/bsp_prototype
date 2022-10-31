#!/bin/bash
module load gcc/11.2.0-otgt
module load gsl/2.7-gpnk
module load libjpeg/9d-v2zc
module load openmpi/3.1.6-t4zs

MYDIR=`pwd`
export LD_LIBRARY_PATH=$MYDIR/lib:/users/sahba/wheeler-scratch/git/rabbitmq-c/build/librabbitmq:$LD_LIBRARY_PATH

#mpirun -np 1 --bind-to core --report-bindings --map:by ppr:1:node -machinefile $PBS_NODEFILE -x UCX_WARN_UNUSED_ENV_VARS=n ./bsp_prototype -w fwq -a 10000 -b 1000 -d gaussian -t 0 -i 100 -l 1 -z 0 -v

mpirun -n 1 ./bsp_prototype -w hpcg -a 16 -b 50 -t 0 -i 5 -l 1 -z 0 -v test.csv
