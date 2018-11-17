#!/usr/bin/env bash

# 从profiler产生的event.txt中生成infomap需要的event.net
# 同时也会输出供graphviz使用的event.gv文件
event2net.cpps

# 用dot从event.gv生成可视化的event.pdf
# dot -Tpdf event.gv -o event.pdf # 对于大程序太慢


# 为infomap创建输出文件夹infomap-output
if [ ! -d infomap-output ] ; then
	mkdir infomap-output
fi

# 用infomap在event.net中发现社区，结果保存在infomap-output/event.map中
infomap event.net infomap-output/ -d -2 -N 10 --two-level --map
# ./Infomap -d -2 --bftree --without-iostream modified_event.net .

# 移到当前目录下
mv infomap-output/event.map .

find-story.cpps

story2net.cpps
# 用infomap在story.net中发现阶段，结果保存在infomap-output/story.map中
infomap story.net infomap-output/ -d -2 -N 10 --two-level --map
mv infomap-output/story.map .


# 为了方便调试，将对象地址替换为对象的名字
if [ -s ref_name.txt ] ; then
    replace.sh event.net ref_name.txt ;
    replace.sh event.map ref_name.txt ;
fi

# 复制到vmware的共享文件夹，方便windows host上处理(不需要了，用wmware的共享文件夹，还不如装个samba)
# cp modified_event.net /mnt/hgfs/vmware-dir
# cp event.pdf /mnt/hgfs/vmware-dir
# cp event.dot /mnt/hgfs/vmware-dir
# cp event.gv /mnt/hgfs/vmware-dir
