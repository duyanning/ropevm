* 安装GNU Classpath 0.97.2
./configure
make
sudo make install
（GNU Classpath将被安装到默认位置，即/usr/local/classpath)
然后去
/usr/local/classpath/share/classpath
执行
sudo unzip glibj.zip

* 安装epm
RopeVM使用epm进行工程管理

* 安装RopeVM
** 下载
从仓库中签出
svn co https://ropevm.svn.sourceforge.net/svnroot/ropevm/trunk/ropevm ropevm

** 配置
根据所在位置修改config.h中的宏定义INSTALL_DIR。
根据所在位置修改env-vars中的环境变量ROPEVM_ROOT。

** 编译
执行make

** 安装
不需要安装，RopeVM是一个自包含的绿色软件。
执行
. env-vars
即可设置好。（env-vars在ropevm目录下）

为了方便使用，在.profile中添加下面这句：
. "$HOME/ropevm/env-vars"   # 假设你把ropevm放在了～下。

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
env-vars			一些环境变量，每次使用之前要 . env-vars
classes/			用于标注分组策略的java annotation
mini-classpath/		取自GNU Classpath的一些类，进行了rope特有的标注
lib/				ropevm自带的java类
tools/				一些脚本
	eb				运行ebrowse
    genlog			同ropejavac，不同的是它会把日志输出重定向到文件
    javajavac		用java运行javac编译出来的代码
    ropejavac		用ropevm运行javac编译出来的代码
    show			查看class文件的字节码
    usejavac		用javac编译
    genlog			运行ropejavac，产生日志

* 开发调试


虚拟机在解释过程中，可能会使用库中的java代码，如ClassLoader
在调试过程中看着这些java代码的字节码是很有帮助的。
这些java类位于
/usr/local/classpath/share/classpath/java/lang
这是安装classpath所产生的，可用show查看其字节码。
最好在emacs的shell中查看，方便查找。

对应的classpath的源代码位于
classpath-0,97.2/java/lang
以及
classpath-0.97.2/vm/reference/java/lang
由于这些代码的缩进，在emacs中看有些乱，用gedit看。
** gdb的设置
set env CLASSPATH=/home/duyanning/work/ropevm/classes:/home/duyanning/work/ropevm/mini-classpath:bin-javac
set env model=3
set env log=0

** epmacs的设置
M-x epm-set-gdb-dir    包含.java文件的目录
M-x epm-set-gdb-args   ropevm Health -l 3 -t 16 -s 1 -m（因ropevm的命令行较长，推荐在gdb中通过环境变量控制，而不是用命令行）

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

* 与JamVM的关系
RopeVM是在JamVM 1.5.1的基础上修改
JamVM根据配置，可以生成好几种解释器。
RopeVM所基于的是其中最简单的一种，即最幼稚的采用loop+switch的decode-and-dispatch解释器。
即配置JamVM时执行configure --disable-int-threading所得到的解释器。
也就是说，THREADED、DIRECT、INLINING都没有定义。
