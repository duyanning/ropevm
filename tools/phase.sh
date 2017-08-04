#!/usr/bin/env bash

# 从profiler产生的graph.txt中生成infomap需要的graph.net
event2net.cpps

# 为了方便调试，将对象地址替换为对象的名字

#sed -e 's/0xaf4051c8/m/g' -e 's/0xaf406418/a/g' -e 's/0xaf406468/b/g' -e 's/0xaf4064b8/c/g' -e 's/0xaf406508/d/g' event.net > modified_event.net

#BEGIN { q="'" }
# option_to_sed=option_to_sed" -e 's/"$1"/"$2"/g'"
# 上面这行中有单引号，语法通不过，要把单引号换成\x27
# option_to_sed=option_to_sed" -e \x27s/"$1"/"$2"/g\x27"
awk '
{ option_to_sed=option_to_sed" -e \x27s/"$1"/"$2"/g\x27"; }
END { 
	cmd = "sed"option_to_sed" event.net > modified_event.net";
	print cmd;
	system(cmd); }
' ref_name.txt

# 为infomap创建输出文件夹infomap-output
#if [ ! -d infomap-output ] ; then
#	mkdir infomap-output
#fi

# 用infomap在graph.net中发现社区，结果保存在infomap-output/graph.map中
#infomap graph.net infomap-output/ -N 10 --two-level --map

# 移到当前目录下
#mv infomap-output/graph.map .

