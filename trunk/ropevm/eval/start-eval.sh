#!/usr/bin/env sh

export DIRS='health-1-1 health-1-2 em3d-1-1 em3d-1-2 em3d-1-3 tsp-1-1 power-1-1 treeadd-1-1 treeadd-2-1'
#export DIRS='health-1-1 health-1-2'
for i in $DIRS
do
    echo evaluating benchmark $i
    cd $i
    make -f ../eval-it.mk
    cd ..
done

make -f result.mk
