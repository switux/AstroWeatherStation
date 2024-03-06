#!/bin/bash -

dat=$(date +"%Y%m%d")
if [ -f "$1/$dat.seq" ]; then
	seq=$(cat "$1/$dat.seq")
	seq=$(expr $seq + 1 )
else
	seq=1
fi
rm -f $1/*.seq
echo $seq > $1/${dat}.seq
seq=$(printf "%03d" $seq)
build=${dat}.${seq}
echo '#define BUILD_ID "'$build'"' > ${1}/build_id.h
