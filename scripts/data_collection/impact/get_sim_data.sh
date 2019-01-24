#!/bin/bash

APP=lulesh
NR_DIRS=11
RUNS=1
NOISE=gaussian
FILE=all-${APP}-${NOISE}.rtime
FILE_STATS=${APP}-${NOISE}.rtime
BASE_SIM=64

if [ -e $FILE ]
	then
	rm $FILE
fi

if [ -e $FILE_STATS ]
	then
	rm $FILE_STATS
fi

#Get runtimes

for k in 8 16 32 64 128 256 512
do
	unset $runtime
	nr_runtime=0
        EXTR=$k
	NODES=$(($BASE_SIM*$EXTR))
	for i in `seq 1 $NR_DIRS`
	do
        	for j in `seq 1 $RUNS`
        	do
			runtime[1]=$NODES
			y=$(($i-1))
                	z=$(($RUNS*$y))
                	x=$(($z+$j))
                	#echo " x is $x"
			if [ $NOISE == "noiseless" ]
			then
				RES=${APP}_$i/${EXTR}/${EXTR}-${j}/${NOISE}/${NOISE}-runtime-${EXTR} 
        		else
				RES=${APP}_$i/${EXTR}/${EXTR}-${j}/${NOISE}/noise-${NOISE}-runtime-${EXTR} 
			fi	
			

			if [ -e $RES ] 
				then
					while read line; do runtime[i+1]="$line"; done < $RES
					nr_runtime=$(($nr_runtime + 1))
			fi
		done
	done
	rt=$(($nr_runtime+1))
	printf "%s\t" "${runtime[@]:1:$rt}" >> $FILE
	printf "\n" >> $FILE
	#printf "%s\n" "${runtime[*]}" >> $FILE


	MIN=0
	MAX=0
	MEAN=0
	SUM=0


	for l in `seq 1 $nr_runtime`
	do
		SUM=$(($SUM+${runtime[$l+1]}))
	
		if [ "${runtime[$l+1]}" -lt "$MIN" ]
		then
			MIN=$((runtime[$l+1]))
		fi
		
		if [ "$MIN" -eq 0 ]
		then
			MIN=$((runtime[$l+1]))
		fi

		if [ "${runtime[$l+1]}" -gt "$MAX" ]
		then
			MAX=$((runtime[$l+1]))
		fi
	
	done

	MEAN=$(($SUM/$nr_runtime))

	MIN_S=$(echo $MIN / 1000000000 | bc -l)
	MEAN_S=$(echo $MEAN / 1000000000 | bc -l)
	MAX_S=$(echo $MAX / 1000000000 | bc -l)

	printf '%s\t%f\t%f\t%f\n' "$NODES" "$MIN_S" "$MEAN_S" "$MAX_S" >> $FILE_STATS
	
	#echo "${runtime[@]}" > $FILE
	#echo ${runtime[*]} > $FILE
done


#Get collectives data for base case


EXTR=8
NR_DIRS=11
RUNS=50


mkdir $APP
mkdir $APP/$EXTR

for i in `seq 1 $NR_DIRS`
do
	for j in `seq 1 $RUNS`
	do
		y=$(($i-1))
		z=$(($RUNS*$y))
		x=$(($z+$j))
		echo " x is $x"
		cp -r ${APP}_$i/${EXTR}/${EXTR}-${j} $APP/$EXTR/${EXTR}-${x}
	done

done


 
#while read line; do echo -e "$line\n"; done < $RES
#comd_1/64/64-1/predata/noise-predata-runtime-64 

