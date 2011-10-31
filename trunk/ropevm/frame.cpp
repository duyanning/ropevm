#include "std.h"
#include "rope.h"
#include "thread.h"
#include "SpmtThread.h"

using namespace std;

/* The function getCallerFrame() is used in the code that does
   security related stack-walking.  It guards against invocation
   via reflection.  These frames must be skipped, else it will
   appear that the caller was loaded by the boot loader. */

Frame *getCallerFrame(Frame *last)
{

loop:
    /* Skip the top frame, and check if the
       previous is a dummy frame */
    if((last = last->prev)->mb == NULL) {

        /* Skip the dummy frame, and check if
         * we're at the top of the stack */
        if((last = last->prev)->prev == NULL)
            return NULL;

        /* Check if we were invoked via reflection */
        if(last->mb->classobj == getReflectMethodClass()) {

            /* There will be two frames for invoke.  Skip
               the first, and jump back.  This also handles
               recursive invocation via reflection. */

            last = last->prev;
            goto loop;
        }
    }
    return last;
}

Class *getCallerCallerClass()
{
    //Frame *last = getExecEnv()->last_frame->prev;
	//assert(getExecEnv()->last_frame->prev == threadSelf()->current_core()->get_current_mode()->frame->prev);
    Frame* last = threadSelf()->get_current_core()->get_current_mode()->frame->prev;

    if((last->mb == NULL && (last = last->prev)->prev == NULL) ||
             (last = getCallerFrame(last)) == NULL)
        return NULL;

    return last->mb->classobj;
}

unsigned long long call_count = 0; // just for debug

Frame::Frame(int lvars_size, int ostack_size)
// :
//     lvars(new uintptr_t[lvars_size != 0 ? lvars_size : 4]),
//     ostack_base(new uintptr_t[ostack_size != 0 ? ostack_size : 4])
{
    //assert(lvars_size);
    //assert(ostack_size);
    lvars = new uintptr_t[lvars_size];
    ostack_base = new uintptr_t[ostack_size];

    last_pc = 0;
    mb = 0;
    prev = 0;
    _name_ = 0;
    object = 0;
    calling_object = 0;
    lrefs = 0;
    pinned = false;
    is_task_frame = false;
    is_certain = true;
    caller_sp = 0;
    caller_pc = 0;

    //{{{ just for debug
    call_count++;
    c = call_count;
    magic = 1978;
    if (call_count == 23808) {
        int x = 0;
        x++;
    }
    //}}} just for debug
    xxx = 0;
}

Frame::~Frame()
{
    // if (xxx == 999) {
    //     cout << "#" << g_current_core()->id() << " free xxx999 " << *this << endl;
    // }

    delete[] lvars;
    delete[] ostack_base;
    delete lrefs;

    //{{{ just for debug
    lvars = 0;
    ostack_base = 0;
    mb = 0;
    prev = 0;
    caller_pc = 0;
    caller_sp = 0;
    calling_object = 0;
    magic = 2009;
    object = 0;
    //}}} just for debug
    xxx = 0;

}

Object*
Frame::get_object()
{
    return object;
}

bool
Frame::is_top_frame()
{
    return prev->mb == 0;
}

std::ostream&
operator<<(std::ostream& os, const Frame& f)
{
    os << *f.mb
       << " " << &f
       << " obj: " << f.object
       << " prev: " << f.prev
       << " prev_obj: " << f.prev->object
       << " calling_obj: " << f.calling_object
       << " ["
       << f.lvars << ", " << f.lvars + f.mb->max_locals
       << ")"
       << " ["
       << f.ostack_base << ", " << f.ostack_base + f.mb->max_stack << ") "
       << ")";

    return os;
}

void copy_args_to_params(uintptr_t* arg, uintptr_t* param, int count)
{
    std::copy(arg, arg + count, param);
}

Frame*
create_dummy_frame(Frame* caller_frame)
{
    Frame* dummy_frame = new Frame(5, 20);
    dummy_frame->prev = caller_frame;
    dummy_frame->_name_ = "dummy frame";
    return dummy_frame;
}

Frame*
g_create_frame(Object* object, MethodBlock* new_mb, Frame* caller_prev, Object* calling_object, uintptr_t* args, uintptr_t* caller_sp, CodePntr caller_pc)
{
    assert(new_mb);

    u2 lvars_size = new_mb->max_locals;
    u2 ostack_size = new_mb->max_stack;

    if (new_mb->max_stack == 0 && new_mb->is_native()) {
        ostack_size = std::max(new_mb->max_locals, (u2)4);
    }

    lvars_size = lvars_size != 0 ? lvars_size : 4;
    ostack_size = ostack_size != 0 ? ostack_size : 4;

    Frame* new_frame = new_frame = new Frame(lvars_size, ostack_size);

    new_frame->object = object;
    new_frame->mb = new_mb;
    new_frame->prev = caller_prev;
    new_frame->calling_object = calling_object;
    new_frame->caller_sp = caller_sp;
    new_frame->caller_pc = caller_pc;

    if (args)
        copy_args_to_params(args, new_frame->lvars, new_mb->args_count);

    //{{{ just for debug
    // if (g_current_core()->id() == 9) {
    //     cout << "#9 create frame " << *new_frame << endl;
    // }
    //}}} just for debug

    // if (new_frame->mb && is_rvp_frame(new_frame)) {
    //     cout << "#" << g_current_core()->id() << " create rvpframe " << *new_frame << endl;
    // }

    // if (new_frame->mb && is_rvp_frame(new_frame) && new_frame->xxx == 999) {
    //     cout << "#" << g_current_core()->id() << " create xxx999 " << *new_frame << endl;
    // }
    return new_frame;
}
