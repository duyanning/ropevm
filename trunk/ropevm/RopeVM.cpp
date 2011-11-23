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
:
    m_logger_enabled(true),
    m_logger_enabled_backdoor(true)
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
        cout << "ROPEVM: too many spmt threads" << endl;
        assert(false);
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


int RopeVM::model;

void initialiseJvm(InitArgs *args)
{
    RopeVM::model = args->model;
}

void
RopeVM::report_stat(std::ostream& os)
{
    Statistic::instance()->report_stat(os);

    os << "JVM" << '\t' << "spmt thread count" << '\t' << m_spmt_threads.size()-4 << '\n';
    os << "JVM" << '\t' << "control transfer" << '\t' << m_count_control_transfer << '\n';

    for (vector<SpmtThread*>::iterator i = m_spmt_threads.begin(); i != m_spmt_threads.end(); ++i) {
        SpmtThread* st = *i;
        if (1 <= st->id() && st->id() <= 4)
            continue;
        st->report_stat(os);
    }
}

void
RopeVM::adjust_log_state()
{
    MiniLogger::disable_all_loggers = not (m_logger_enabled and
                                           m_logger_enabled_backdoor);
}


void
RopeVM::turn_on_log()
{
    m_logger_enabled = true;
    adjust_log_state();
}


void
RopeVM::turn_off_log()
{
    m_logger_enabled = false;
    adjust_log_state();
}


void
RopeVM::turn_on_log_backdoor()
{
    m_logger_enabled_backdoor = true;
    adjust_log_state();
}


void
RopeVM::turn_off_log_backdoor()
{
    m_logger_enabled_backdoor = false;
    adjust_log_state();
}
