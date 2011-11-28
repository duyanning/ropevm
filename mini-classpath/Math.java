// 这个类用来模仿GNU Classpath 0.97.2中的两个类
// classpath-0.97.2/java/lang/Math.java
// classpath-0.97.2/vm/reference/java/lang/VMMath.java
// 以及classpath-0.97.2/native/jni/java-lang/java_lang_VMMath.c
public final class Math
{
    // # 无论是设置LD_LIBRARY_PATH，还是用命令行选项-Djava.library.path，都不起作用。
    // # 要为native方法加载动态库，只能用System.load，并指明动态库的绝对路径。
    static
    {
        //System.loadLibrary("rope-hacked-classpath");
        System.load("/home/duyanning/work/ropevm/mini-classpath/mini-classpath.so");
    }


    public static final double PI = 3.141592653589793;


    @RopeInvokerExecute
    public static int abs(int i)
    {
        return (i < 0) ? -i : i;
    }


    @RopeInvokerExecute
    public static long abs(long l)
    {
        return (l < 0) ? -l : l;
    }


    @RopeInvokerExecute
    public static float abs(float f)
    {
        return (f <= 0) ? 0 - f : f;
    }


    @RopeInvokerExecute
    public static double abs(double d)
    {
        return (d <= 0) ? 0 - d : d;
    }



    @RopeInvokerExecute
    @RopeSpecSafe
    public static native double pow(double a, double b);


    @RopeInvokerExecute
    @RopeSpecSafe
    public static native double floor(double a);


    @RopeInvokerExecute
    @RopeSpecSafe
    public static native double sqrt(double a);
}


