#!/bin/bash -

cd $1
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
clean=$(git diff)
branch=$(git rev-parse --abbrev-ref HEAD)
_hash=$(git log -p -1 | head -n 1 | awk '{print $2}')
hash=${_hash:0:7}
if [ -n "$clean" ]; then
	hash=${hash}" unclean"
fi
hash="$branch "${hash}
echo '#define GITHASH "'$hash'"' >> ${1}/build_id.h
cd $2
libs=$(ls libraries)
folder=$(dirname ${1})
maxnamesize=0
maxversize=0
nblib=-1
manifest=${1}/manifest.h
declare -a libnames
declare -a libversions
for i in $libs; do
	if [ -f ${folder}/libraries/$i/library.properties ]; then
		nblib=$(expr $nblib + 1)
		libname=$(grep name ${folder}/libraries/$i/library.properties | cut -d= -f2)
		sz=${#libname}
		if [ $sz -gt $maxnamesize ]; then
			maxnamesize=$sz
		fi
		libver=$(grep version ${folder}/libraries/$i/library.properties | cut -d= -f2)
		sz=${#libver}
		if [ $sz -gt $maxversize ]; then
			maxversize=$sz
		fi
		libnames[${nblib}]=$libname
		libversions[${nblib}]=$libver
	fi
done
nbm1=$(expr $nblib - 1)
nblib2=$(expr $nblib + 1)
echo 'etl::string<'$maxnamesize'> libraries['$nblib2'] = {' > $manifest
for i in $(seq 0 $nbm1); do
	echo '	etl::string<'$maxnamesize'>("'${libnames[$i]}'"),' >> $manifest
done
echo '	etl::string<'$maxnamesize'>("'${libnames[$nblib]}'")' >> $manifest
echo '};' >> $manifest
echo 'etl::string<'$maxversize'> libversions['$nblib2'] = {' >> $manifest
for i in $(seq 0 $nbm1); do
	echo '	etl::string<'$maxversize'>("'${libversions[$i]}'"),' >> $manifest
done
echo '	etl::string<'$maxversize'>("'${libversions[$nblib]}'")' >> $manifest
echo '};' >> $manifest
echo '#define NBLIB '$nblib2 >> $manifest
