#ifndef OBJECT_H
#define OBJECT_H

class Class;
class Core;
class Group;

class Object {
public:

    //Object() // because using gcMalloc to alloc Object, ctor for Object has no chance to run, so we MUST call initialize
    void initialize();
    void set_group(Group* group);
    void join_group_in_other_threads(Group* group);
    Group* get_group();

    uintptr_t lock;
private:
    Group* m_group;             // set by the thread that created it
public:
    Class* classobj;
};

inline
void
Object::initialize()
{
    m_group = 0;
}



// // struct ClassBlock;
// // struct MethodBlock;

// // Class MUST NOT add data members to Object
// class Class : public Object {
// public:
//     ClassBlock* classblock() { return CLASS_CB(this); }
//     const char* name() { return classblock()->name; }
//     //    MethodBlock* method_table() { return classblock()->method_table; }
//     MethodBlock* method_table(int i) { return classblock()->method_table[i]; }
//     ConstantPool* constant_pool() { return &classblock()->constant_pool; }
// };

#endif // OBJECT_H
