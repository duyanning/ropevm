#include "std.h"
#include "jam.h"

#include "Group.h"

#include "Core.h"

Group::Group(Object* leader, Core* core)
    :
    m_leader(leader),
    m_core(core)
{
    assert(leader);
    assert(core);

    m_leader->set_group(this);
    m_obj_count++;

    m_core->set_group(this);
}

void
Group::add(Object* obj)
{
    assert(obj);
    assert(obj != m_leader);

    obj->set_group(this);
    m_obj_count++;
}

void
Group::remove(Object* obj)
{
    assert(obj);

    obj->set_group(0);
    m_obj_count--;
}


