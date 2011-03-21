#ifndef OBJECT_H
#define OBJECT_H

class Class;
class Core;
class Group;

class Object {
public:
    // because using gcMalloc to alloc Object, ctor for Object has no chance to run
    //Object()
    void init()
    {
        m_group = 0;
    }

    void set_group(Group* group) { m_group = group; }
    Group* get_group() { return m_group; }

    uintptr_t lock;
private:
    Group* m_group;
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
