#!/usr/bin/env sh

usejavac

export model=1
ropejavac > output.txt
tail -n 1 output.txt | awk '{ printf "%s" , $2 ; }' > cycles.txt

. vmparams
ropejavac > output.txt
tail -n 1 output.txt | awk '{ printf " %s\n" , $2 ; }' >> cycles.txt

awk '{ printf "%f\n" , $1 / $2 ; }' cycles.txt > speedup.txt
