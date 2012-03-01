// 注意：因为推测线程的行为难以追踪，所以应由确定线程来执行后门操作。
// 为了保证后门能被确定线程执行，必须在同步路障后调用。
// 详见：inter.cpp中后门函数的实现。
public class RopeVMBackdoor {
    // 开关统计
    public static void turn_on_probe()
    {
    }

    public static void turn_off_probe()
    {
    }

    // 开关日志
    public static void turn_on_log()
    {
    }

    public static void turn_off_log()
    {
    }

}
