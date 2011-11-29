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
    SpmtThread* st = 0;
    pthread_mutex_lock(&m_lock);
    if (m_sts.size() < 1000) {
        st = new SpmtThread(get_next_id());
        m_sts.push_back(st);
    }
    else {
        cout << "ROPEVM: too many spmt threads" << endl;
        assert(false);
    }
    pthread_mutex_unlock(&m_lock);
    return st;
}


SpmtThread*
RopeVM::create_spmt_thread()
{
    SpmtThread* st = new_spmt_thread();

    // 为st对象配上os线程(S_threadStart中调用st->drive_loop)
    // 但目前是单os线程实现方式，让一个spmt线程运行drive_loop的时候带动其他spmt线程。
    //os_api_create_thread(SpmtThread::S_threadStart, st);

    return st;
}


int RopeVM::model;
bool RopeVM::support_invoker_execute;
bool RopeVM::support_spec_safe_native;
bool RopeVM::support_spec_barrier;
bool RopeVM::support_self_read;


void initialiseJvm(InitArgs *args)
{
    RopeVM::model = args->model;
    if (RopeVM::model < 3)      // 模型3以下，不支持这些特征。其值皆为false。
        return;

    RopeVM::support_invoker_execute = args->support_invoker_execute;
    RopeVM::support_spec_safe_native = args->support_spec_safe_native;
    RopeVM::support_spec_barrier = args->support_spec_barrier;
    RopeVM::support_self_read = args->support_self_read;
}

void
RopeVM::report_stat(std::ostream& os)
{
    Statistic::instance()->report_stat(os);

    os << "JVM" << '\t' << "spmt thread count" << '\t' << m_sts.size()-4 << '\n';
    os << "JVM" << '\t' << "control transfer" << '\t' << m_count_control_transfer << '\n';

    for (vector<SpmtThread*>::iterator i = m_sts.begin(); i != m_sts.end(); ++i) {
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
