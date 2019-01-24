#!/bin/bash

#TYPE=pareto     	#exponential / pareto
#TYPE2=pareto    	#expo / pareto
#MEAN=40ms

TYPE=exponential     	#exponential / pareto
TYPE2=expo    		#expo / pareto
MEAN=40ms

RUN=1

HOME_DIR=/home/omondrag/develop/model_experiments/validation

APP_NAME=${TYPE}_${MEAN}
DATA_NAME=data_${TYPE}_${MEAN}
APP_DIR=$HOME_DIR/test_model_${TYPE2}_titan1

DATA_DIR=$HOME_DIR/$DATA_NAME
NOISE0_DIR=$DATA_DIR/titan

SCRIPT=$HOME_DIR/extract_collective_data2.py


mkdir $DATA_DIR
mkdir $NOISE0_DIR

NR_FILES=1
BASE=64

for i in `seq 1 $NR_FILES`
do
	$SCRIPT -f $APP_DIR/${TYPE}_${MEAN}_${BASE}/result_coll --total_interval -o $NOISE0_DIR/$APP_NAME-total-base-${BASE}-${RUN} #64 nodes case
done


#generate total times  matrix

APP_DIR=$HOME_DIR/test_model_${TYPE2}_titan
cd $HOME_DIR

TOTAL_TIME[5]=0

for i in 64 128 256 512 1024 2048 4096 8192 16384
do
	for j in 1 2 3 #4 5
	do
		RESULT=$APP_DIR${j}/${TYPE}_${MEAN}_$i/result_coll
		echo $RESULT
		if [ -e "$RESULT" ]
		then
			TOTALTIME[$j]=$(awk 'END {printf "%lu", $5}' $RESULT)
		else
			TOTALTIME[$j]="NAN"	
		fi
	done
	
	echo "${i}	${TOTALTIME[1]}	${TOTALTIME[2]}	${TOTALTIME[3]}	${TOTALTIME[4]}	${TOTALTIME[5]}" >> $NOISE0_DIR/total_time

done

