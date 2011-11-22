* 安装
安装classpath 0.93 (为编译这个需安装jikes)
安装jamvm 1.51

自己安装的这些都须安装在/usr/local下

* 运行
当前目录.也必须加入PATH，因为
javajavac和ropejavac
都要 . cmd
cmd在各个test目录下

运行. env-vars
然后在各个测试目录下执行usejavac进行编译。
然后用执行javajavac和ropejavac运行。

* 三种执行模型
支持三种执行模型：1-串行模型；2-退化的rope模型；3-rope模型。
1-串行模型：唯一的初始spmt线程负责所有的对象。
2-退化的rope模型：多个spmt线程，各自负责不同的一组对象。但是spmt线程失去确定控制后不推测执行。
3-rope模型：多个spmt线程，各自负责不同的一组对象。spmt线程失去确定控制后推测执行。

从模型2开始，消息就登上了舞台。bug可能因为消息传递而引起。


* 环境变量与命令行参数
model用来选择执行模型，log控制日志开关。

环境变量
export model=1
export log=1

命令行参数
-Xmodel:<1/2/3>
-Xlog:<on/off>

* 目录结构
ropeclasses 用于标注分组策略的java annotation
env-vars 一些环境变量，每次使用之前要 . env-vars
tools 一些脚本
	eb 运行ebrowse
    genlog 同ropejavac，不同的是它会把日志输出重定向到文件
    javajavac 用java运行javac编译出来的代码
    ropejavac 用ropevm运行javac编译出来的代码
    show 查看class文件的字节码
    usejavac 用javac编译
    genlog运行ropejavac，产生日志。

日志文件名：
log-model-1.txt
log-model-2.txt
log-model-3.txt

* 开发调试
epmacs的设置
当前目录 包含.java文件的目录
命令行参数 -cp bin-javac -Xmodel:3 -Xlog:off Health -l 3 -t 16 -s 1 -m
或
-cp bin-javac Hello

-cp bin-javac -Xmodel:4 -Xlog:off Em3d -n 7 -d 3 -m -p -i 100

-cp bin-javac -Xmodel:3 -Xlog:off Em3d -n 5 -d 3 -m -p -i 1

虚拟机在解释过程中，可能会使用库中的java代码，如ClassLoader
在调试过程中看着这些java代码的字节码是很有帮助的。
这些java类位于
/usr/local/classpath/share/classpath/java/lang
可用show查看其字节码。
最好在emacs的shell中查看，方便查找。为了在emacs的shell中使用show，需要先 . env-vars
对应的classpath的源代码位于classpath-0,97.2/java/lang
由于这些代码的缩进，在emacs中看有些乱，用gedit看。
* 一些工具
class editor
http://classeditor.sourceforge.net/

按照其安装帮助，解压后得一目录，在该目录中运行一次：
java -jar ce.jar
以便创建相关配置文件。
然后在~/bin中新建一脚本ce，内容如下
#!/usr/bin/env sh
java -jar ~/ce2.23/ce.jar
以后运行ce即可。




