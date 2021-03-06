#include "std.h"
#include "rope.h"

#include "Group.h"

#include "SpmtThread.h"

Group::Group(Thread* thread, Object* leader, SpmtThread* core)
    :
    m_leader(leader),
    m_thread(thread),
    m_core(core)
{
    assert(core);
    m_core->set_group(this);
}

void
Group::add(Object*)
{
    m_obj_count++;
}

void
Group::remove(Object* obj)
{
    assert(obj);

    obj->set_group(0);
    m_obj_count--;
}


