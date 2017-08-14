#!/usr/bin/env bash

# 这个脚本接受两个参数，第一个是文件，在该文件中进行替换。
# 第二个也是一个一个文件，该文件由两列构成。第一列是被替换的字符串，第二列是替换成的字符串。

# 利用sed进行替换，利用awk搜集、构造sed的命令行参数，在awk最后的动作中调用sed


awk -v src="$1" '
BEGIN {
      tgt="modified_"src
}
{
# option_to_sed=option_to_sed" -e 's/"$1"/"$2"/g'"
# 上面这行中有单引号，语法通不过，要把单引号换成\x27
    option_to_sed=option_to_sed" -e \x27s/"$1"/"$2"/g\x27";
}
END { 
	cmd = "sed"option_to_sed" "src" > "tgt;
	#print cmd;
	system(cmd);
}
' $2

