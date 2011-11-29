#ifndef OBJECT_H
#define OBJECT_H

class Class;
class SpmtThread;

class Object {
public:

    //Object() // because using gcMalloc to alloc Object, ctor for Object has no chance to run, so we MUST call initialize
    void initialize();

    SpmtThread* get_st(); // 根据不同的java线程调用它而返回不同的spmt线程。
    void set_st(SpmtThread* st); // 创建对象的java线程为其分配的spmt线程
    void join_st_in_other_threads(SpmtThread* st); // 其他java线程为其分配的spmt线程。
    uintptr_t lock;
private:
    SpmtThread* m_st;             // set by the thread that created it
public:
    Class* classobj;
};

inline
void
Object::initialize()
{
    m_st = 0;
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
