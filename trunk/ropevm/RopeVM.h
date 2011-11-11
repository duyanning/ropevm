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
    //static bool do_spec;
    static int model;           // 1-sequential; 2-degraded rope; 3-rope

protected:
    RopeVM();
    int get_next_id() { return m_next_id++; }

private:
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
