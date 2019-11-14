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

# Launch ldms sampler
/opt/ldms_wheeler/start_sampler.sh
# Launch workload
/home/docker/bsp_prototype "$@" ${OUTFILE}
# Clean up results
mv ${OUTFILE} /results/
# Kill ldms sampler
kill $(ps aux | grep ldms | grep -v grep | awk '{print $2}')
exit "$?"
