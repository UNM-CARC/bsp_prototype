#!/bin/bash
mkdir -p /tmp/results/
#OSU is not automatically added to the path!
osu_path=$(find /opt/software/linux-centos7-ivybridge/gcc-8.3.1/ -name pt2pt | head -n 1)
export PATH="${osu_path}/":$PATH
export PYTHONPATH="/root/.local/lib/python3.6/site-packages":$PYTHONPATH
export HOST=`hostname`
RABBIT_SCRIPT="/home/docker/rabit_functions.py"
if [[ $1 == "osu" ]] 
then
	#passing all but the first arg to osu_bw
	osu_bw "${@:2:$#}"
	exit "$?"

elif [[ $1 == 'rabbit' ]]
then
	PYTHON="/usr/bin/python3"
	export PYTHONPATH=/opt/bsp_prototype/pip_files/lib/python3.6/site-packages:$PYTHONPATH
	if [[ $2 == 'server' ]]
	then
    	rabbitmq-server
	else
		$PYTHON $RABBIT_SCRIPT "${@:2:$#}"
	fi
	exit "$?"

fi
export OUTFILE=`mktemp /tmp/results/${HOST}_XXXXXXXX.json`
rm ${OUTFILE}	# Let the bsp_prototype actually crate hte file if it wants it.
/home/docker/bsp_prototype "$@" ${OUTFILE}
if [ -f ${OUTFILE} ]; then
	mv ${OUTFILE} /results/
fi
exit "$?"
