#!/bin/bash
IFS=$'\n'; 
for i in `cat $PBS_NODEFILE`; do 
  ssh $i '
  DIR=$HOME/cdse2018/data/results

  # NEED to check for existence of sub-directories! #
  if [ ! -d "$DIR" ] ; then
    #echo Moving data to results_$HOSTNAME
    mkdir $DIR
    for i in $(ls /tmp/results/) ; do
      mkdir $DIR/$i/$HOSTNAME
      mv /tmp/results/$i/* $DIR/$i/$HOSTNAME/
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

