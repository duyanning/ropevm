#include "std.h"
#include "OoSpmtJvm.h"
#include "Helper.h"
#include "Statistic.h"

using namespace std;

OoSpmtJvm* OoSpmtJvm::m_instance = 0;
int OoSpmtJvm::m_next_core_id = 0;

OoSpmtJvm*
OoSpmtJvm::instance()
{
    if (m_instance == 0) {
        m_instance = new OoSpmtJvm;
    }

    return m_instance;
}

OoSpmtJvm::OoSpmtJvm()
{
    pthread_mutex_init(&m_lock, 0);

    // stat
    m_count_control_transfer = 0;
}

// OoSpmtJvm::~OoSpmtJvm()
// {
//     //    std::cout << "total cores: " << m_cores.size() << std::endl;
//     for (vector<Core*>::iterator i = m_cores.begin(); i != m_cores.end(); ++i) {
//         delete (*i);
//         *i = 0;
//     }
// }

Core*
OoSpmtJvm::alloc_core()
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

Core*
OoSpmtJvm::core_for_object(Object* obj)
{
    if (obj == 0) return 0;
    return obj->get_core();
}

Core*
OoSpmtJvm::alloc_core_for_object(Object* object)
{
    Core* core = alloc_core();
    if (core) {
        core->set_owner(object);
    }
    MINILOG0("#" << core->id() << " is assigned to obj: " << object << " class: " << type_name(object));
    return core;
}

bool OoSpmtJvm::do_spec;

void initialiseJvm(InitArgs *args)
{
    OoSpmtJvm::do_spec = args->do_spec;
}

void
OoSpmtJvm::report_stat(std::ostream& os)
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
