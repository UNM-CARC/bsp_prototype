#!/bin/bash

#HOME_DIR=/home/omondrag/develop/model_experiments/real_app_test
HOME_DIR=($HOME)/data-cdse2018/experiements

RUN=1

APP=lulesh
NOISE=checkpointing
EXTRP=8

APP_NAME=${APP}_512_${NOISE}
DATA_NAME=data_${APP_NAME}_start
APP_DIR=$HOME_DIR/${APP}_512_${NOISE}/${EXTRP}/${EXTRP}

#DATA_DIR=/home/omondrag/develop/model_experiments/real_app_test/$DATA_NAME
DATA_DIR=($HOME)/data-cdse2018/experiments/$DATA_NAME
NOISE0_DIR=$DATA_DIR/${NOISE}_nocosched

#SCRIPT=/home/omondrag/develop/model_experiments/validation/extract_collective_data3.py

#For lulesh, use this
#SCRIPT=/home/omondrag/develop/model_experiments/validation/extract_collective_data2.py
SCRIPT=($HOME)/data-cdse2018/experiments/validation/extract_collective_data2.py


mkdir $DATA_DIR
mkdir $NOISE0_DIR

#NR_FILES=1280
#NR_FILES=550
NR_FILES=520 # lulesh checkpointing
BASE=512

for i in `seq 1 $NR_FILES`
do
	if [ "$NOISE" == "noiseless" ] 
		then
			$SCRIPT -f $APP_DIR-${i}/${NOISE}/${NOISE}-coll-${EXTRP} --start-time --max -o $NOISE0_DIR/$APP_NAME-start-base-${BASE}-${i} #64 nodes case
		else
			$SCRIPT -f $APP_DIR-${i}/${NOISE}/noise-${NOISE}-coll-${EXTRP} --start-time --max -o $NOISE0_DIR/$APP_NAME-start-base-${BASE}-${i} #64 nodes case
	fi
done

