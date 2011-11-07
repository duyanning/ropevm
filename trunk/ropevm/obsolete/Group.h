#ifndef GROUP_H
#define GROUP_H

class Object;
class SpmtThread;
class Thread;

class Group {
public:
    Group(Thread* thread, Object* leader, SpmtThread* core);
    Object* get_leader();
    void add(Object* obj);
    void remove(Object* obj);
    SpmtThread* get_core();     // refactor: remove
    SpmtThread* get_spmt_thread();
    Thread* get_thread();
    bool can_speculate();
private:
    Object* m_leader;
    int m_obj_count;                 // refs count
    SpmtThread* m_core;
    Thread* m_thread;
};

inline
Thread*
Group::get_thread()
{
    return m_thread;
}

inline
Object*
Group::get_leader()
{
    return m_leader;
}

inline
SpmtThread*
Group::get_core()
{
    return m_core;
}

inline
SpmtThread*
Group::get_spmt_thread()
{
    return m_core;
}

inline
bool
Group::can_speculate()
{
    return get_leader() != 0;
}
#endif // GROUP_H
