#!/bin/bash
mkdir -p /tmp/results/
export PATH=$PATH:/opt/software/linux-centos7-x86_64/gcc-8.3.1/osu-micro-benchmarks-5.6.1-gxlojmj2kwdr366txez2ko6tw75yi4o3/libexec/osu-micro-benchmarks/mpi/pt2pt
export HOST=`hostname`
if [[ $1 == "osu" ]] 
then
	#passing all but the first arg to osu_bw
	osu_bw "${@:2:$#}"
	exit "$?"
fi
export OUTFILE=`mktemp /tmp/results/${HOST}_XXXXXXXX.json`
rm ${OUTFILE}	# Let the bsp_prototype actually crate hte file if it wants it.
/home/docker/bsp_prototype "$@" ${OUTFILE}
if [ -f ${OUTFILE} ]; then
	mv ${OUTFILE} /results/
fi
exit "$?"
