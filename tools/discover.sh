#!/usr/bin/env bash

# 从profiler产生的graph.txt中生成infomap需要的graph.net
graph2net.cpps

# 为infomap创建输出文件夹infomap-output
if [ ! -d infomap-output ] ; then
	mkdir infomap-output
fi

# 用infomap在graph.net中发现社区，结果保存在infomap-output/graph.map中
infomap graph.net infomap-output/ -N 10 --two-level --map

# 移到当前目录下
mv infomap-output/graph.map .

find-leaders.cpps
