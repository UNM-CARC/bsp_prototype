#!/bin/bash
IFS=$'\n'; 
for i in `cat $PBS_NODEFILE`; do 
  ssh $i '
  DIR=$HOME/cdse2018/data-cdse2018/results_$HOSTNAME

  if [ ! -d "$DIR" ] ; then
    echo Moving data to results_$HOSTNAME
    mkdir $DIR
    mv /tmp/results/* $DIR
  elif [ -d "$DIR" ] ; then
    for j in $(ls /tmp/results/) ; do
      for file in $(ls /tmp/results/$j/) ; do
        if [ ! -d "$DIR/$j" ] ; then
          mkdir $DIR/$j
        fi
        #echo Copying $j/$file to $DIR/$j/$file
        cat /tmp/results/$j/$file >> $DIR/$j/$file
      done;
    done;
    rm -rf /tmp/results/
  fi'
done;

