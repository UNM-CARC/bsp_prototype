#!/bin/bash
#scl enable rh-python36 bash
#module load mpi/openmpi3-x86_64
mkdir -p /tmp/results/
echo "Running with commands $@"
TMPFILE=`mktemp -d /tmp/results/{PBS_JOBNAME}-${PBS_JOBID}.XXXXXX`
/opt/bsp_prototype/scripts/app_gen/app_gen_metrics -f ${TMPFILE} "$@"
cp ${TMPFILE} /results/
rm ${TMPFILE}
