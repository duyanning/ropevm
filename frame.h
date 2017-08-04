#ifndef FRAME_H
#define FRAME_H


class Frame {
public:
    CodePntr last_pc;
    uintptr_t* lvars;
    uintptr_t* ostack_base;
    MethodBlock *mb;

    Object* object;             // 本来这个变量是为了给没有this的静态方法使用，指向相应的类对象（静态方法属于类对象）。后来非静态方法也用这个，而不用参数中的this。
    SpmtThread* owner;          // 执行方法的线程（又名executor）。因为invokerExecuting方法的存在，executor并不一定就是object所属的线程，它也可能等于invoker。

    SpmtThread* caller;         // 调用方法的线程（又名invoker）。
    CodePntr caller_pc;         // 发起该方法时，caller线程的pc。
    Frame *prev;                // 发起该方法时，caller线程的frame。（叫caller_frame更好）
    uintptr_t* caller_sp;       // 发起该方法时，caller线程的sp。


    bool pinned;
    bool is_top;

    intmax_t frame_no;          // frame的编号(用于日志)


    // 该栈桢在创建其的effect的C集合中的位置，为方便在栈桢销毁时快速地
    // 从effect的C集合中删除。但是，当栈桢被销毁时，创建其的effect可能
    // 已经因为提交被销毁了，也就是说该effect中的C也随之销毁了。在这种
    // 情况下，iter_in_C_of_effect已经变成无效的，不能使用。
    std::list<Frame*>::iterator iter_in_C_of_effect;
    bool is_iter_in_current_effect;

    unsigned long long c; // just for debug
    int magic; // just for debug
    int xxx;

    const char* _name_;

    std::vector<Object*> * lrefs;

    Frame(int lvars_size, int ostack_size);
    ~Frame();
    Object* get_object();
    bool is_top_frame();
    bool is_dummy() { return object == 0; }
private:
    Frame(const Frame&);
};


class DummyFrame : public Frame {
public:
    DummyFrame();
};


/* typedef struct jni_frame { */
/*    Object **next_ref; */
/*    Object **lrefs; */
/*    uintptr_t *ostack; */
/*    MethodBlock *mb; */
/*    struct frame *prev; */
/* } JNIFrame; */

typedef Frame JNIFrame;

std::ostream& operator<<(std::ostream& os, const Frame* f);

inline
bool is_rvp_frame(Frame* frame)
{
    return frame->owner == 0;
}


// class Frame;
// class MethodBlock;
// class Object;

Frame* g_create_frame(SpmtThread* owner, Object* object, MethodBlock* new_mb, uintptr_t* args,
                      SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                      bool is_top);

void g_destroy_frame(Frame* frame);


Frame* create_dummy_frame(Frame* caller_frame);


#endif // FRAME_H
