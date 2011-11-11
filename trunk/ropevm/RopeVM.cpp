#include "std.h"
#include "rope.h"
#include "RopeVM.h"
#include "Helper.h"
#include "Statistic.h"
#include "thread.h"
#include "SpmtThread.h"

using namespace std;

RopeVM* RopeVM::m_instance = 0;
int RopeVM::m_next_id = 0;

RopeVM*
RopeVM::instance()
{
    if (m_instance == 0) {
        m_instance = new RopeVM;
    }

    return m_instance;
}

RopeVM::RopeVM()
{
    pthread_mutex_init(&m_lock, 0);

    // stat
    m_count_control_transfer = 0;
}

// RopeVM::~RopeVM()
// {
//     //    std::cout << "total cores: " << m_cores.size() << std::endl;
//     for (vector<SpmtThread*>::iterator i = m_cores.begin(); i != m_cores.end(); ++i) {
//         delete (*i);
//         *i = 0;
//     }
// }

SpmtThread*
RopeVM::new_spmt_thread()
{
    SpmtThread* spmt_thread = 0;
    pthread_mutex_lock(&m_lock);
    if (m_spmt_threads.size() < 1000) {
        spmt_thread = new SpmtThread(get_next_id());
        m_spmt_threads.push_back(spmt_thread);
    }
    else {
        assert(false);
        cerr << "no more cores\n";
    }
    pthread_mutex_unlock(&m_lock);
    return spmt_thread;
}


SpmtThread*
RopeVM::create_spmt_thread()
{
    SpmtThread* spmt_thread = new_spmt_thread();

    // 为spmt_thread对象配上os线程(S_threadStart中调用spmt_thread->drive_loop)
    // 但目前是单os线程实现方式，让一个spmt线程运行drive_loop的时候带动其他spmt线程。
    //os_api_create_thread(SpmtThread::S_threadStart, spmt_thread);

    return spmt_thread;
}


//bool RopeVM::do_spec;
int RopeVM::model;

void initialiseJvm(InitArgs *args)
{
    //RopeVM::do_spec = args->do_spec;
    RopeVM::model = args->model;
}

void
RopeVM::report_stat(std::ostream& os)
{
    Statistic::instance()->report_stat(os);

    os << "JVM" << '\t' << "core count" << '\t' << m_spmt_threads.size()-4 << '\n';
    os << "JVM" << '\t' << "control transfer" << '\t' << m_count_control_transfer << '\n';

    for (vector<SpmtThread*>::iterator i = m_spmt_threads.begin(); i != m_spmt_threads.end(); ++i) {
        SpmtThread* core = *i;
        if (1 <= core->id() && core->id() <= 4)
            continue;
        core->report_stat(os);
    }
}
