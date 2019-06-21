#!/bin/bash
 
outputdir=$1
file=$2
cache=tmp.$$
 
#rm -r $outputdir
if [ ! -d "$outputdir" ] ; then
    mkdir -p $outputdir
fi
 
echo "Getting rank IDs"
openss -batch -f $file > $cache << EOF
list -v ranks
EOF
 
for rankid in $(cat $cache | grep "^[0-9]"); do
    echo "Getting data for rank $rankid"
    openss -batch -f $file > $outputdir/$rankid << EOF
expview -v trace mpit -r $rankid -m start_time, stop_time, stddev, average -F CSV
EOF
done
 
#echo "Getting load balance info"
#openss -batch -f $file > $outputdir/loadbalance << EOF
#expview -v trace mpit -m loadbalance -v functions
#EOF
 
rm $cache
