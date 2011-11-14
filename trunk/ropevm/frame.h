#ifndef FRAME_H
#define FRAME_H


class Frame {
public:
    CodePntr last_pc;
    uintptr_t* lvars;
    uintptr_t* ostack_base;
    MethodBlock *mb;

    Frame *prev;         // caller_frame
    uintptr_t* caller_sp;
    CodePntr caller_pc;

    unsigned long long c; // just for debug
    int magic; // just for debug
    int xxx;

    const char* _name_;

    bool pinned;
    Object* object;             // 这个主要是为了给没有this的静态方法使用
    SpmtThread* caller;
    SpmtThread* owner;

    std::vector<Object*> * lrefs;

    Frame(int lvars_size, int ostack_size);
    ~Frame();
    Object* get_object();
    bool is_top_frame();
    bool is_dummy() { return object == 0; }
    //{{{ just for debug
    bool is_alive() { return magic == 1978; }
    bool is_dead() { return magic == 2009; }
    bool is_bad() { return magic != 1978 && magic != 2009; }
private:
    Frame(const Frame&);
    Frame& operator=(const Frame&);
};

/* typedef struct jni_frame { */
/*    Object **next_ref; */
/*    Object **lrefs; */
/*    uintptr_t *ostack; */
/*    MethodBlock *mb; */
/*    struct frame *prev; */
/* } JNIFrame; */

typedef Frame JNIFrame;

std::ostream& operator<<(std::ostream& os, const Frame& f);

inline
bool is_rvp_frame(Frame* frame)
{
    return frame->owner == 0;
}

#endif // FRAME_H
