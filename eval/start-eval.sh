#!/usr/bin/env sh

. benchmarks

probe=0
quiet=1

if [ ! -d result ] ; then
    mkdir result
fi

for i in $DIRS
do
    echo evaluating benchmark $i
    cd $i
    make -f ../eval-it.mk
    cd ..
done

make -f result.mk
