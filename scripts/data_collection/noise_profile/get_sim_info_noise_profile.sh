#!/bin/bash

HOME_DIR=/home/omondrag/develop/model_experiments/validation/test_sample_analysis

RUN=1
MEAN=$Par_duration
NOISE=$Par_noise

NR_FILES=1
BASE=64
NODES=16384

EXTR=$(( $NODES/$BASE ))

APP_NAME=constant_${MEAN}_${NOISE}_${NODES}
DATA_NAME=${MEAN}

DATA_HOME=$HOME_DIR/data_${NODES}

DATA_DIR=$DATA_HOME/$DATA_NAME
NOISE0_DIR=$DATA_DIR/${NOISE}

SCRIPT=/home/omondrag/develop/model_experiments/validation/extract_collective_data3.py

if [ -e $DATA_HOME ]; 
then
	echo "data home $DATA_HOME"
else
	mkdir $DATA_HOME
fi

if [ -e $DATA_DIR ]; 
then
	echo "data dir $DATA_DIR"
else
	mkdir $DATA_DIR
fi


mkdir $NOISE0_DIR

for i in `seq 1 $NR_FILES`
do
	APP_DIR=$HOME_DIR/$NODES/constant_${MEAN}/constant_interval/$EXTR/$EXTR-${i}/${NOISE}
	
	if [ "$NOISE" = "noiseless" ]; then
		$SCRIPT -f $APP_DIR/noiseless-coll-$EXTR --start-time --max -o $NOISE0_DIR/$APP_NAME-start-allranks-base-${BASE}-${i} # noiseless
	else
		$SCRIPT -f $APP_DIR/noise-${NOISE}-coll-$EXTR --start-time --max -o $NOISE0_DIR/$APP_NAME-start-allranks-base-${BASE}-${i} # 
	fi

done

