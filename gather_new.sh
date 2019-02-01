#!/bin/bash
IFS=$'\n';
JOBOUTPUT="${${$(pwd)%/*}%/*}/results/job_output/"
for i in `cat $PBS_NODEFILE`; do
  ssh $i
  '
    for i in $(ls /tmp/results/) ; do
      mv /tmp/results/$i/ $JOBOUTPUT/$i/
    done;
    rm -rf /tmp/results/
  '
done;
