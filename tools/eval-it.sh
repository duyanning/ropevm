#!/usr/bin/env sh

usejavac

export model=1
rm -f output.txt
ropejavac > output.txt
tail -n 1 output.txt | awk '{ printf "%s" , $2 ; }' > cycles.txt

if [ -s vmparams ] ; then
    . vmparams ;
fi

rm -f output.txt
ropejavac > output.txt
tail -n 1 output.txt | awk '{ printf " %s\n" , $2 ; }' >> cycles.txt

awk '{ printf "%f\n" , $1 / $2 ; }' cycles.txt > speedup.txt
