NOISE=gaussian
NODES=4096
SCRIPT=../extract_collective_data3.py 
MEAN=100ms
BASE=64
EXTR=$(( $NODES / $BASE))

if [ $NOISE == "noiseless" ]
	then
		$SCRIPT -f $NODES/constant_${MEAN}/constant_interval/$EXTR/$EXTR-1/${NOISE}/noiseless-coll-${EXTR} --interarrival -r 0 -o ${NOISE}_${MEAN}_1proc
	else
		$SCRIPT -f $NODES/constant_${MEAN}/constant_interval/$EXTR/$EXTR-1/${NOISE}/noise-${NOISE}-coll-${EXTR} --interarrival -r 0 -o ${NOISE}_${MEAN}_1proc
fi
