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
# 注意：-z参数是为了告诉infomap，节点的编号是从0开始的
infomap event.net infomap-output/ -u -2 -N 10 --map -z
# ./Infomap -d -2 --bftree --without-iostream modified_event.net .

# 移到当前目录下
mv infomap-output/event.map .

# 输出event.map中的社区信息(即故事)至story.txt
# 同时也会输出供graphviz使用的story.gv文件
find-story.cpps

# 将故事图(story.txt)转换为infomap可以处理的.net格式(story.net)
story2net.cpps

# 用infomap在story.net中发现阶段，结果保存在infomap-output/story.map中
# 注意：这个.net中的节点的编号是从1开始的，所以不用-z参数
# 参数含义：-d有向图，-2两级，--map生成.map文件，-N 10迭代次数
infomap story.net infomap-output/ -u -2 -N 10 --map

# 用dot从story.gv生成可视化的story.pdf
dot -Tpdf story.gv -o story.pdf # 对于大程序太慢

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
