#!/bin/bash

HOME_DIR=($HOME)/validation/test_sample_analysis

RUN=1

NOISE=$Par_noise
MEAN=$Par_mean
OUTPUT=${MEAN}_${NOISE}.rtime
NOISELESS=${MEAN}_noiseless.rtime

BASE=64

for i in 1 2 4 8 16 32 64 128 256
do
	NODES=$(($BASE * $i))
	if [ $NOISE == noiseless ]
	then
		FILE=$NODES/constant_${MEAN}/constant_interval/${i}/${i}-1/${NOISE}/${NOISE}-runtime-${i}
	else
		FILE=$NODES/constant_${MEAN}/constant_interval/${i}/${i}-1/${NOISE}/noise-${NOISE}-runtime-${i}
	fi

	runtime=$(<$FILE)

	runtime=$(echo $runtime / 1000000000 | bc -l)
	
	printf '%s\t%f\n' "$NODES" "$runtime" >> tmp

done


if [ $NOISE != noiseless ]
then	
	awk 'NR==FNR{a[NR]=$2;next}{print $1,"\t",a[FNR],"\t",$2}' $NOISELESS tmp > $OUTPUT
	rm tmp
else
	mv tmp $OUTPUT
fi
