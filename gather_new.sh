#!/bin/zsh
tmp="data_generator/results/job_output/"
JOBOUTPUT=${${$(pwd)%/*}%/*}"$tmp"
if [ "$(echo $PBS_NODEFILE)" != "" ] ;
then
  for i in `cat $PBS_NODEFILE`; do
    ssh $i
    '
      for i in $(ls /tmp/results/) ; do
        mv /tmp/results/$i $JOBOUTPUT/$i
      done;
      rm -rf /tmp/results/
    '
  done;
else
  for i in $(ls /tmp/results/) ; do
    mv /tmp/results/$i $JOBOUTPUT/$i
  done;
  rm -rf /tmp/results/
fi
