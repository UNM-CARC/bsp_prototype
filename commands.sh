#!/bin/bash
source /home/docker/env.sh

# Establish filename suffix
export OUTFILE="${APPLICATION_OUTPUT_DIR}/bsp_simulator_output_$(date +%m-%d-%y_%H:%M:%S).json"

#########################################
#
#  Setup LDMS envinroment
#
#########################################

# LDMS environment variables for openmpi sampling
# NOTE: If you aren't using OpenMPI, change LD_PRELOAD.
export LD_LIBRARY_PATH=/usr/local/lib/ovis-lib:/usr/local/lib/ovis-ldms:${LD_LIBRARY_PATH}
export LDMS_INSTALL_PATH=/usr/local
export LDMS_MPI_PROFILER_LIB_NAME=libldms_mpi_profiler.so
export LDMS_MPI_PROFILER_PATH=$LDMS_INSTALL_PATH/lib/ovis-ldms/$LDMS_MPI_PROFILER_LIB_NAME
export LD_PRELOAD=$LDMS_MPI_PROFILER_PATH:/usr/local/lib/libmpi.so:$LD_PRELOAD
# Choose mpi functions to sample
export LDMS_SHM_MPI_FUNC_INCLUDE="MPI_Send,MPI_Recv,MPI_Barrier,MPI_Isend,MPI_Irecv,MPI_Wait"
export LDMS_SHM_MPI_PROFILER_LOG_LEVEL=1
# Name of the shared index. This should be the same in the mpi sampler configuration (shm_index=...).
export LDMS_SHM_INDEX="/ldms_shm_mpi_index"

#########################################

# Launch the LDMS sampler on all nodes
#echo STARTING SAMPLERS
#for i in $(cat $PBS_NODEFILE | uniq); do
#  ssh $i "source /etc/profile; module load openssl-1.1.1b-gcc-4.8.5-obdqvnl; module load singularity-3.2.1-gcc-4.8.5-ulix7vo; singularity exec -B /etc/hostname -B ${OUTPUT_DIR} ${CONTAINER_IMAGE_DIR}/bsp_prototype.sif /opt/ldms_wheeler/start_sampler.sh"
#done;
#echo SAMPLERS SHOULD HAVE STARTED

########################################
#
# Layer 2 Container environment setup
#
########################################

# Same as above but with part of the environment shoved into the command
export OMPI_MCA_orte_launch_agent="${CONTAINER_IMAGE_DIR}/bsp_prototype/home/docker/ompi_launch.sh"

########################################
#
# Launch the application in L2 context
#
########################################
TIMER=$((-1*$(date +%s)))
mpirun --map-by ppr:1:node -n $(( $SLURM_NNODES )) -x CONTAINER_IMAGE_DIR -x LDMS_SHM_MPI_FUNC_INCLUDE -x SLURM_TACC_JOBNAME -x LDMS_MPI_PROFILER_PATH -x LDMS_SHM_INDEX -x LDMS_SHM_MPI_PROFILER_LOG_LEVEL -x LD_PRELOAD --hostfile /hostfile.txt /home/docker/bsp_prototype "$@" ${OUTFILE}
echo Time taken to do workload is $(( $(date +%s) + ${TIMER} ))

# Kill LDMS sampler on all nodes
#
#for i in $(cat $PBS_NODEFILE | uniq); do
#  echo ssh $i "kill \$(ps aux | grep ldmsd | grep -v grep | awk "\'"{print \$2}"\'")"
#  ssh $i "kill \$(ps aux | grep ldmsd | grep -v grep | awk "\'"{print \$2}"\'")"
#done;

exit "$?"
