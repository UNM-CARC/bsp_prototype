#!/bin/bash
mkdir -p /tmp/results/
export PATH=$PATH:/opt/software/linux-centos7-x86_64/gcc-8.3.1/osu-micro-benchmarks-5.6.1-gxlojmj2kwdr366txez2ko6tw75yi4o3/libexec/osu-micro-benchmarks/mpi/pt2pt
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
/home/docker/bsp_prototype "$@" ${OUTFILE}
rm ${OUTFILE}# Let the bsp_prototype actually crate hte file i  f it wants it.
/home/docker/bsp_prototype "$@" ${OUTFILE}
if [ -f ${OUTFILE}  ]; then
    mv ${OUTFILE} /results/
fi
exit "$?"
