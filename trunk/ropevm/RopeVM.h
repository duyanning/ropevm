#ifndef OOSPMTJVM_H
#define OOSPMTJVM_H

#include "SpmtThread.h"

class Thread;
class Object;

class RopeVM
{
public:
    //    ~RopeVM();
    SpmtThread* alloc_core();
    Group* new_group_for(Object* leader, Thread* thread);

    static RopeVM* instance();
    static bool do_spec;

protected:
    RopeVM();
    int get_next_core_id() { return m_next_core_id++; }

private:
	std::vector<SpmtThread*> m_cores;
    SpmtThread* m_currentCore;
    pthread_mutex_t m_lock;

    static RopeVM* m_instance;
    static int m_next_core_id;

    // stat
public:
    long long m_count_control_transfer;
    void report_stat(std::ostream& os);
};

#endif
