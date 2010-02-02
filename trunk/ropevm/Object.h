#ifndef OBJECT_H
#define OBJECT_H

class Class;
class Core;

class Object {
public:
    // because using gcMalloc to alloc Object, ctor for Object has no chance to run
    //Object()
    void init()
    {
        m_core = 0;
        m_is_owner = false;
    }

    void set_core(Core* core) { m_core = core; }
    Core* get_core() { return m_core; }
    void set_is_owner(bool is) { m_is_owner = is; }
    bool is_owner() { return m_is_owner; }

    uintptr_t lock;
private:
    Core* m_core;
    bool m_is_owner;              // am I owner of the core?
public:
    Class* classobj;
};


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
