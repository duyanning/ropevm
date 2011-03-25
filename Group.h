#ifndef GROUP_H
#define GROUP_H

class Object;
class Core;
class Thread;

class Group {
public:
    Group(Thread* thread, Object* leader, Core* core);
    Object* get_leader();
    void add(Object* obj);
    void remove(Object* obj);
    Core* get_core();
    Thread* get_thread();
    bool can_speculate();
private:
    Object* m_leader;
    int m_obj_count;                 // refs count
    Core* m_core;
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
Core*
Group::get_core()
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
