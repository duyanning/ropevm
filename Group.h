#ifndef GROUP_H
#define GROUP_H

class Object;
class Core;

class Group {
public:
    Group(Object* leader, Core* core);
    Object* get_leader() { return m_leader; }
    void add(Object* obj);
    void remove(Object* obj);
    Core* get_core() { return m_core; }
private:
    Object* m_leader;
    int m_obj_count;                 // refs count
    Core* m_core;
};

#endif // GROUP_H
