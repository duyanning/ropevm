#ifndef OOSPMTJVM_H
#define OOSPMTJVM_H

class Thread;
class Object;
class SpmtThread;

class RopeVM
{
public:
    //    ~RopeVM();
    SpmtThread* new_spmt_thread();
    SpmtThread* create_spmt_thread();


    void output_summary();
    void dump_rope_params();    // 输出与rope相关的参数信息

    static RopeVM* instance();

    void turn_on_log();
    void turn_off_log();
    void turn_on_log_backdoor();
    void turn_off_log_backdoor();

    void turn_on_probe();
    void turn_off_probe();


    static int model;           // 1-sequential; 2-degraded rope; 3-rope

    // 以下这些features仅在模型3下有效。其他模型下皆取false。
    static bool support_invoker_execute; // 是否支持invoker execute方法
    static bool support_irrevocable;     // 是否支持不可召回方法
    static bool support_spec_safe_native; // 是否支持推测安全的native方法
    static bool support_spec_barrier;     // 是否支持推测路障
    static bool support_self_read;        // 是否支持自行读取其他线程负责的对象



    // 要开启日志功能，以下两者必须都为true
    // 用两个变量控制是为了：即便java程序中通过后门开启了日志，我们也可通过命令行或环境变量关闭日志。
    bool m_logger_enabled;      // 虚拟机是否开启日志功能
    bool m_logger_enabled_backdoor; // java程序通过后门控制虚拟机是否开启日志功能

    static bool probe_enabled;

protected:
    RopeVM();
    int get_next_id() { return m_next_id++; }

private:
    void adjust_log_state();

	std::vector<SpmtThread*> m_sts;
    pthread_mutex_t m_lock;

    static RopeVM* m_instance;
    static int m_next_id;

    // stat
public:
    //long long m_count_control_transfer;
    long long m_certain_instr_count;
    void report_stat(std::ostream& os);
};


class MethodBlock;
extern bool g_should_enable_probe(MethodBlock* mb);

#endif
