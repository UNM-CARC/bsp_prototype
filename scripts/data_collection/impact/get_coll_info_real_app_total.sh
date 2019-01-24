#!/bin/bash

HOME_DIR=/home/omondrag/develop/model_experiments/real_app_test

RUN=1

APP=lulesh
NOISE=checkpointing
EXTRP=8

APP_NAME=${APP}_512_${NOISE}
DATA_NAME=data_${APP_NAME}_total
APP_DIR=$HOME_DIR/${APP}_512_${NOISE}/${EXTRP}/${EXTRP}

DATA_DIR=/home/omondrag/develop/model_experiments/real_app_test/$DATA_NAME
NOISE0_DIR=$DATA_DIR/${NOISE}_nocosched

SCRIPT=/home/omondrag/develop/model_experiments/validation/extract_collective_data3.py


mkdir $DATA_DIR
mkdir $NOISE0_DIR

#NR_FILES=550
NR_FILES=520 #Checkpointing lulesh

#NR_FILES=1280?
#NR_FILES=520 lulesh
#NR_FILES=550 comd
#NR_FILES=1000 lj 900 lj_noiseless
BASE=512

for i in `seq 1 $NR_FILES`
do
	if [ "$NOISE" == "noiseless" ] 
		then
			$SCRIPT -f $APP_DIR-${i}/${NOISE}/${NOISE}-coll-${EXTRP} --total_interval --rank=1 -o $NOISE0_DIR/$APP_NAME-total-base-${BASE}-${i} #64 nodes case
		else
			$SCRIPT -f $APP_DIR-${i}/${NOISE}/noise-${NOISE}-coll-${EXTRP} --total_interval --rank=1 -o $NOISE0_DIR/$APP_NAME-total-base-${BASE}-${i} #64 nodes case
	fi
done

