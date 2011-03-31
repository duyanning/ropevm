#include "std.h"
#include "RopeVM.h"
#include "Helper.h"
#include "Statistic.h"
#include "Group.h"
#include "thread.h"

using namespace std;

RopeVM* RopeVM::m_instance = 0;
int RopeVM::m_next_core_id = 0;

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
//     for (vector<Core*>::iterator i = m_cores.begin(); i != m_cores.end(); ++i) {
//         delete (*i);
//         *i = 0;
//     }
// }

Core*
RopeVM::alloc_core()
{
    Core* core = 0;
    pthread_mutex_lock(&m_lock);
    if (m_cores.size() < 1000) {
    //if (m_cores.size() < 6) {
        core = new Core(get_next_core_id());
        m_cores.push_back(core);
        core->init();
    }
    else {
        assert(false);
        cerr << "no more cores\n";
    }
    pthread_mutex_unlock(&m_lock);
    return core;
}

Group*
RopeVM::new_group_for(Object* leader, Thread* thread)
{
    Core* core = alloc_core();
    //Thread* thread = threadSelf();

    Group* group = new Group(thread, leader, core);

    //MINILOG0("#" << core->id() << " is assigned to obj: " << leader << " class: " << type_name(leader));
    return group;
}

bool RopeVM::do_spec;

void initialiseJvm(InitArgs *args)
{
    RopeVM::do_spec = args->do_spec;
}

void
RopeVM::report_stat(std::ostream& os)
{
    Statistic::instance()->report_stat(os);

    os << "JVM" << '\t' << "core count" << '\t' << m_cores.size()-4 << '\n';
    os << "JVM" << '\t' << "control transfer" << '\t' << m_count_control_transfer << '\n';

    for (vector<Core*>::iterator i = m_cores.begin(); i != m_cores.end(); ++i) {
        Core* core = *i;
        if (1 <= core->id() && core->id() <= 4)
            continue;
        core->report_stat(os);
    }
}
