#include "std.h"
#include "jam.h"

#include "thread.h"
#include "Group.h"
#include "Core.h"

Group*
Object::get_group()
{
    assert(m_group);

    if (m_group->get_thread() == threadSelf()) {
        return m_group;
    }

    Thread* thread = threadSelf();
    Group* group = thread->group_of(this);
    if (group)
        return group;

    // first time seen by this Java thread
    group = thread->current_core()->assign_group_for(this);
    join_group_in_other_threads(group);

    return group;
}

// Object::get_group()
// {
//     if (m_group and m_group->get_thread() == threadSelf()) {
//         return m_group;
//     }

//     // object shared by several Java threads
//     Thread* thread = threadSelf();
//     Group* group = thread->group_of(this);
//     if (group)
//         return group;

//     // first time seen by this Java thread
//     group = thread->current_core()->assign_group_for(this);
//     thread->register_object_group(this, group);

//     return group;
// }

// Group*
// Object::get_group()
// {
//     //assert(m_group);

//     if (m_group)
//         return m_group;

//     // object shared by several Java threads
//     Thread* thread = threadSelf();
//     Group* group = thread->group_of(this);
//     return group;
// }

void
Object::set_group(Group* group)
{
    assert(m_group == 0);
    m_group = group;
}

void
Object::join_group_in_other_threads(Group* group)
{
    assert(m_group);

    // object shared by several Java threads
    Thread* thread = threadSelf();
    thread->register_object_group(this, group);
}

