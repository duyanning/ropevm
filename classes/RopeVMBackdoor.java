// 注意：因为推测线程的行为难以追踪，所以应由确定线程来执行后门操作。
// 故而，所有后门操作都暗含着一个推测路障。（该推测路障不受虚拟机是否开启推测路障功能的影响）
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

    // 在对象地址与名字之间建立联系
    public static void register_obj(Object obj, String name)
    //public static void register_obj()
    {
        
    }
}
