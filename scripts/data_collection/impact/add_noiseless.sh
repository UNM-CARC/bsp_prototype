APP=lulesh
NOISE=gaussian
NOISELESS_FILE=/home/omondrag/develop/model_experiments/real_app_test/${APP}_512_noiseless_files/${APP}-noiseless.rtime
NOISE_FILE=/home/omondrag/develop/model_experiments/real_app_test/${APP}_512_${NOISE}_files/${APP}-${NOISE}.rtime
TOTAL_FILE=total_time

awk 'NR==FNR{a[NR]=$2;next}{print $1,"\t",a[FNR],"\t",$2,"\t",$3,"\t",$4}' $NOISELESS_FILE $NOISE_FILE > $TOTAL_FILE
