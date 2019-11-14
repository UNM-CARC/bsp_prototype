#!/bin/bash
mkdir -p /tmp/results/
export HOST=`hostname`
export OUTFILE=`mktemp /tmp/results/${HOST}_XXXXXXXX.csv`


# LDMS environment variables for openmpi sampling
# NOTE: If you aren't using OpenMPI, change LD_PRELOAD.
export LDMS_INSTALL_PATH=/usr/local
export LDMS_MPI_PROFILER_LIB_NAME=libldms_mpi_profiler.so
export LDMS_MPI_PROFILER_PATH=$LDMS_INSTALL_PATH/lib/ovis-ldms/$LDMS_MPI_PROFILER_LIB_NAME
export LD_PRELOAD=/usr/lib64/openmpi3/lib/libmpi.so:$LDMS_MPI_PROFILER_PATH

/home/docker/bsp_prototype "$@" ${OUTFILE}
mv ${OUTFILE} /results/
exit "$?"
