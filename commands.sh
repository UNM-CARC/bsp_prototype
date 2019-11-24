#!/bin/bash
mkdir -p /tmp/results/
export HOST=`hostname`
export OUTFILE=`mktemp /tmp/results/${HOST}_XXXXXXXX.json`
rm ${OUTFILE}	# Let the bsp_prototype actually crate hte file if it wants it.
/home/docker/bsp_prototype "$@" ${OUTFILE}
if [ -f ${OUTFILE} ]; then
	mv ${OUTFILE} /results/
fi
exit "$?"
