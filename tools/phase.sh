#!/usr/bin/env bash

# 将事件历史(event.txt)转换为infomap可以处理的.net格式(event.net)的复杂网络。
# 同时也会输出供graphviz使用的event.gv文件
event2net.cpps

# 用dot从event.gv生成可视化的event.pdf
dot -Tpdf event.gv -o event.pdf # 对于大程序太慢


# 为infomap创建输出文件夹infomap-output
if [ ! -d infomap-output ] ; then
	mkdir infomap-output
fi

# 用infomap在event.net中发现社区，结果保存在infomap-output/event.map中
# infomap提供的可执行文件是Infomap（首字母大写），我为了便于输入，搞了个符号链接infomap。可以放在tools下，也可以放在~/bin下。
infomap event.net infomap-output/ -d -2 -N 10 --two-level --map
# ./Infomap -d -2 --bftree --without-iostream modified_event.net .

# 移到当前目录下
mv infomap-output/event.map .

# 输出event.map中的社区信息(即故事)至story.txt
find-story.cpps

# 将故事图(story.txt)转换为infomap可以处理的.net格式(story.net)
story2net.cpps

# 用infomap在story.net中发现阶段，结果保存在infomap-output/story.map中
infomap story.net infomap-output/ -d -2 -N 10 --two-level --map

# 移到当前目录下
mv infomap-output/story.map .

# 输出story.map中的社区信息(即阶段)至phase.txt
find-phase.cpps

# 将phase.txt中的阶段起止位置转换为源代码中的行号
map-phase-to-program.cpps


# 本来到此为止
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
