#!/bin/bash
IFS=$'\n';
for i in `cat $PBS_NODEFILE`; do
  ssh $i
  "
    DIR=$JOBOUTPUT #HOME/cdse2018/data/results
    for i in \$(ls /tmp/results/) ; do
      mv /tmp/results/\$i/ \$DIR/\$i/
    done;
    rm -rf /tmp/results/
  "
done;
