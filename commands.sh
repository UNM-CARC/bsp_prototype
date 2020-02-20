#!/bin/bash
mkdir -p /tmp/results/
export HOST=`hostname`
export OUTFILE=`mktemp /tmp/results/${HOST}_XXXXXXXX.csv`


# LDMS environment variables for openmpi sampling
# NOTE: If you aren't using OpenMPI, change LD_PRELOAD.
export LDMS_INSTALL_PATH=/usr/local
export LDMS_MPI_PROFILER_LIB_NAME=libldms_mpi_profiler.so
export LDMS_MPI_PROFILER_PATH=$LDMS_INSTALL_PATH/lib/ovis-ldms/$LDMS_MPI_PROFILER_LIB_NAME
export LD_PRELOAD=$LDMS_MPI_PROFILER_PATH:/usr/local/lib/libmpi.so:$LD_PRELOAD
# Choose mpi functions to sample
export LDMS_SHM_MPI_FUNC_INCLUDE="MPI_Send,MPI_Recv,MPI_Barrier,MPI_Isend,MPI_Irecv,MPI_Wait"
export LDMS_SHM_MPI_PROFILER_LOG_LEVEL=1
# Name of the shared index. This should be the same in the mpi sampler configuration (shm_index=...).
export LDMS_SHM_INDEX="/ldms_shm_mpi_index"

# Launch workload
set -x
mpirun -n $PBS_NP -mca pml ucx --mca btl ^vader,tcp,openib,uct -x UCX_NET_DEVICES=mlx4_0:1 -x PBS_O_HOST -x LDMS_SHM_MPI_FUNC_INCLUDE -x PBS_JOBNAME -x LDMS_MPI_PROFILER_PATH -x LDMS_SHM_INDEX -x LDMS_SHM_MPI_PROFILER_LOG_LEVEL -x LD_PRELOAD -machinefile $PBS_NODEFILE /home/docker/bsp_prototype "$@" ${OUTFILE}

# Clean up results
mv ${OUTFILE} /results/
exit "$?"
