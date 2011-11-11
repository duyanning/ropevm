安装classpath 0.93 (为编译这个需安装jikes)
安装jamvm 1.51

自己安装的这些都须安装在/usr/local下

=======================================
当前目录.也必须加入PATH，因为
javajavac和ropejavac
都要 . cmd
cmd在各个test目录下

运行. env-vars
然后在各个测试目录下执行usejavac进行编译。
然后用执行javajavac和ropejavac运行。

=========================
export spec=1
export log=1

命令行参数
-Xspec:<on/off>
-Xlog:<on/off>
================================================================
ropeclasses 用于标注分组策略的java annotation
env-vars 一些环境变量，每次使用之前要 . env-vars
tools 一些脚本
	eb 运行ebrowse
    genlog 同ropejavac，不同的是它会把日志输出重定向到文件
    javajavac 用java运行javac编译出来的代码
    ropejavac 用ropevm运行javac编译出来的代码
    show 查看class文件的字节码
    usejavac 用javac编译

用法：
在mytest下，运行usejavac进行编译，然后运行ropejavac运行，可跟javajavac的运行结果进行对比。
================================================================
epmacs的设置
当前目录 包含.java文件的目录
命令行参数 -cp bin-javac -Xspec:off -Xlog:off Hello

