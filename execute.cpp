#include "std.h"
#include "rope.h"
#include "sig.h"
#include "frame.h"
#include "lock.h"
#include "symbol.h"
#include "excep.h"
#include "SpmtThread.h"
#include "CertainMode.h"
#include "SpeculativeMode.h"
#include "RvpMode.h"
#include "RopeVM.h"
#include "Message.h"

#include "Loggers.h"
#include "DebugScaffold.h"
#include "interp.h"

#define VA_DOUBLE(args, sp)                     \
    if(*sig == 'D')                             \
        *(double*)sp = va_arg(args, double);    \
    else                                        \
        *(u8*)sp = va_arg(args, u8);            \
    sp+=2

#define VA_SINGLE(args, sp)                     \
    if(*sig == 'L' || *sig == '[')              \
        *sp = va_arg(args, uintptr_t);          \
    else                                        \
        if(*sig == 'F')                         \
            *(float*)sp = va_arg(args, double); \
        else                                    \
            *sp = va_arg(args, u4);             \
    sp++

#define JA_DOUBLE(args, sp)  *(u8*)sp = *args++; sp+=2
#define JA_SINGLE(args, sp)                     \
    switch(*sig) {                              \
        case 'L': case '[': case 'F':           \
            *sp = *(uintptr_t*)args;            \
            break;                              \
        case 'B': case 'Z':                     \
            *sp = *(signed char*)args;          \
            break;                              \
        case 'C':                               \
            *sp = *(unsigned short*)args;       \
            break;                              \
        case 'S':                               \
            *sp = *(signed short*)args;         \
            break;                              \
        case 'I':                               \
            *sp = *(signed int*)args;           \
            break;                              \
    }                                           \
    sp++; args++

void *executeMethodArgs(Object *ob, Class *classobj, MethodBlock *mb, ...) {
    va_list jargs;
    void *ret;

    va_start(jargs, mb);
    ret = executeMethodVaList(ob, classobj, mb, jargs);
    va_end(jargs);

    return ret;
}

void *executeMethodVaList(Object *ob, Class *classobj, MethodBlock *mb, va_list jargs) {
    void *ret;

    Thread* this_thread = threadSelf();
    SpmtThread* this_core = this_thread->get_current_core();

//     int args_count = ob ? mb->args_count - 1 : mb->args_count;
//     std::vector<uintptr_t> args(args_count);
    std::vector<uintptr_t> arguments(mb->args_count);

    uintptr_t* arg = &arguments[0];

    if (ob) {
        *arg++ = (uintptr_t)ob;
    }

    char *sig = mb->type;
    SCAN_SIG(sig, VA_DOUBLE(jargs, arg), VA_SINGLE(jargs, arg));
    ret = this_core->get_current_mode()->do_execute_method((ob ? ob : classobj), mb, arguments);

    return ret;
}

void *executeMethodList(Object *ob, Class *classobj, MethodBlock *mb, u8 *jargs) {
    void *ret;

    Thread* this_thread = threadSelf();
    SpmtThread* this_core = this_thread->get_current_core();

//     int args_count = ob ? mb->args_count - 1 : mb->args_count;
//     std::vector<uintptr_t> args(args_count);
    std::vector<uintptr_t> arguments(mb->args_count);

    uintptr_t* arg = &arguments[0];

    if (ob) {
        *arg++ = (uintptr_t)ob;
    }

    char *sig = mb->type;
    SCAN_SIG(sig, JA_DOUBLE(jargs, arg), JA_SINGLE(jargs, arg));
    ret = this_core->get_current_mode()->do_execute_method((ob ? ob : classobj), mb, arguments);

    return ret;
}
