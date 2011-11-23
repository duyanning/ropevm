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

    static RopeVM* instance();

    void turn_on_log();
    void turn_off_log();
    void turn_on_log_backdoor();
    void turn_off_log_backdoor();

    static int model;           // 1-sequential; 2-degraded rope; 3-rope


    bool m_logger_enabled;      // 虚拟机是否开启日志功能
    bool m_logger_enabled_backdoor; // java程序通过后门控制虚拟机是否开启日志功能
    // 要开启日志功能，以上两者必须都为true

protected:
    RopeVM();
    int get_next_id() { return m_next_id++; }

private:
    void adjust_log_state();

	std::vector<SpmtThread*> m_spmt_threads;
    pthread_mutex_t m_lock;

    static RopeVM* m_instance;
    static int m_next_id;

    // stat
public:
    long long m_count_control_transfer;
    void report_stat(std::ostream& os);
};

#endif
