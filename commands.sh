#!/bin/bash
mkdir -p /tmp/results/
export HOST=`hostname`
export OUTFILE=`mktemp /tmp/results/${HOST}_XXXXXXXX.csv`
/home/docker/bsp_prototype ${OUTFILE} "$@"
mv ${OUTFILE} /results/
exit "$?"
