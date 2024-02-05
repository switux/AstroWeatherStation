#!/bin/bash -

project=$(basename $1)
if [ -z "$1" -o "$1" != "AstroWeatherStation" ]; then
	exit
fi

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
build=${dat}_${seq}
echo '#define BUILD_DATE "'$build'"' > ${1}/build_seq.h
echo '#define BUILD_DATE "'$build'"'
