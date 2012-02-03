#!/usr/bin/env sh

# if [ ! -f $SPEEDUPS ] ; then
# fi

rm -rf result/*

for dir in health-1-1 health-1-2 em3d-1-1 em3d-1-2 em3d-1-3 bh-1-1 power-1-1 treeadd-1-1 treeadd-2-1 tsp-1-1
#for dir in health-1-1 health-1-2
do
    echo evaluating benchmark $dir

    cd $dir

    usejavac


    export model=1
    ropejavac > output.txt
    tail -n 1 output.txt | awk '{ printf "%s" , $2 ; }' > cycles.txt

    . vmparams
    ropejavac > output.txt
    tail -n 1 output.txt | awk '{ printf " %s\n" , $2 ; }' >> cycles.txt

    awk '{ printf "%s\t%f\n" , prog , $1 / $2 ; }' prog="$dir" cycles.txt > speedup.txt

    cat speedup.txt >> ../result/speedups.txt
    mv profile.txt ../result/$dir-profile.txt

    cd ..
done
