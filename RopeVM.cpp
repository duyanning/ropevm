#include "std.h"
#include "rope.h"
#include "RopeVM.h"
#include "Helper.h"
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
    m_logger_enabled(true)//,
    //m_logger_enabled_backdoor(true)
{
    pthread_mutex_init(&m_lock, 0);

    // stat
    //m_count_control_transfer = 0;
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


bool RopeVM::probe_enabled;
bool RopeVM::graph_enabled;
bool RopeVM::event_enabled;
int RopeVM::model;
bool RopeVM::support_invoker_execute;
bool RopeVM::support_irrevocable;
bool RopeVM::support_spec_safe_native;
bool RopeVM::support_spec_barrier;
bool RopeVM::support_self_read;


void initialiseJvm(InitArgs *args)
{
    if (args->do_log) {
        RopeVM::instance()->turn_on_log();
    }
    else {
        RopeVM::instance()->turn_off_log();
    }

    if (args->do_probe) {
        RopeVM::instance()->turn_on_probe();
    }
    else {
        RopeVM::instance()->turn_off_probe();
    }

    if (args->do_graph) {
        RopeVM::instance()->turn_on_graph();
    }
    else {
        RopeVM::instance()->turn_off_graph();
    }

    if (args->do_event) {
        RopeVM::instance()->turn_on_event();
    }
    else {
        RopeVM::instance()->turn_off_event();
    }


    RopeVM::model = args->model;

    // 对象协作图只能在串行模式下生成
    if (args->do_graph) {
        RopeVM::model = 1;
    }

    // 事件历史只能在串行模式下生成
    if (args->do_event) {
        RopeVM::model = 1;
    }

    if (RopeVM::model < 3)      // 模型3以下，不支持这些特征。其值皆为false。
        return;

    RopeVM::support_invoker_execute = args->support_invoker_execute;
    RopeVM::support_irrevocable = args->support_irrevocable;
    RopeVM::support_spec_safe_native = args->support_spec_safe_native;
    RopeVM::support_spec_barrier = args->support_spec_barrier;
    RopeVM::support_self_read = args->support_self_read;
}

void
RopeVM::report_stat(std::ostream& os)
{
    //Statistic::instance()->report_stat(os);

    // os << "JVM" << '\t' << "spmt thread count" << '\t' << m_sts.size()-4 << '\n';
    // os << "JVM" << '\t' << "control transfer" << '\t' << m_count_control_transfer << '\n';

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
    // 由两个变量共同控制，只要有一个为false，就不产生日志。
    // MiniLogger::disable_all_loggers = not (m_logger_enabled and
    //                                        m_logger_enabled_backdoor);
    MiniLogger::disable_all_loggers = not m_logger_enabled;
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


// void
// RopeVM::turn_on_log_backdoor()
// {
//     m_logger_enabled_backdoor = true;
//     adjust_log_state();
// }


// void
// RopeVM::turn_off_log_backdoor()
// {
//     m_logger_enabled_backdoor = false;
//     adjust_log_state();
// }


void
RopeVM::turn_on_probe()
{
    probe_enabled = true;
}


void
RopeVM::turn_off_graph()
{
    graph_enabled = false;
}

void
RopeVM::turn_on_graph()
{
    graph_enabled = true;
}

void
RopeVM::turn_off_event()
{
    event_enabled = false;
}

void
RopeVM::turn_on_event()
{
    event_enabled = true;
}


void
RopeVM::turn_off_probe()
{
    probe_enabled = false;
}


namespace {

    template <typename T>
    void
    print_entry(const char* name, T val)
    {
        cout << setw(25) << right << name << "   " << val << "\n";
    }

}

// 变量名跟变量的显示名相同
#define PRINT_ENTRY(name) print_entry(#name, name)

void
RopeVM::dump_rope_params()
{
    // 注意：以support_打头的变量值未必等于同名的环境变量的值，它们的取值还与model的取值有关。
    // 参考函数initialiseJvm
    cout << "\n" << setw(40) << setfill('=') << "\n" << setfill(' ');
    PRINT_ENTRY(model);
    print_entry("log", m_logger_enabled);
    PRINT_ENTRY(support_invoker_execute);
    PRINT_ENTRY(support_irrevocable);
    PRINT_ENTRY(support_spec_safe_native);
    PRINT_ENTRY(support_spec_barrier);
    PRINT_ENTRY(support_self_read);
}


void
RopeVM::output_summary()
{
    dump_rope_params();
    cout<< "\n";
    cout << "statistic:\n";
    cout << "cycles: " << m_certain_instr_count << endl;
}


// 只统计程序本身的代码，而不统计那庞大但未经并行化的标准库代码
bool
g_should_enable_probe(MethodBlock* mb)
{
    if (not RopeVM::probe_enabled)
        return false;

    if (not is_client_code)
        return false;

    // if (mb and not is_app_obj(mb->classobj))
    //     return false;

    return true;
}

std::ofstream ofs_graph("graph.txt");
std::ofstream ofs_timeline("timeline.txt");
std::ofstream ofs_event("event.txt");
std::ofstream ofs_ref_name("ref_name.txt");
