#include "std.h"
#include "jam.h"

#ifndef USE_FFI
#include <string.h>
//#include "../../../sig.h"
#include "sig.h"

#define RET_VOID    0
#define RET_DOUBLE  1
#define RET_LONG    2
#define RET_FLOAT   3
#define RET_DFLT    4

int nativeExtraArg(MethodBlock *mb) {
    int len = strlen(mb->type);
    if(mb->type[len-2] == ')')
        switch(mb->type[len-1]) {
            case 'V':
                return RET_VOID;
            case 'D':
                return RET_DOUBLE;
            case 'J':
                return RET_LONG;
            case 'F':
                return RET_FLOAT;
        }

    return RET_DFLT;
}

u4 *callJNIMethod(void *env, Class *classobj, char *sig, int ret_type, u4 *ostack, unsigned char *f, int args) {
    u4 *opntr = ostack + args;
    int i;

    for(i = 0; i < args; i++)
        asm volatile ("pushl %0" :: "m" (*--opntr) : "sp");

    if(classobj) {
        asm volatile ("pushl %0" :: "m" (classobj) : "sp");
	args++;
    }

    asm volatile ("pushl %0" :: "m" (env) : "sp");

    switch(ret_type) {
        case RET_VOID:
            (*(void (*)())f)();
            break;

        case RET_DOUBLE:
            *(double*)ostack = (*(double (*)())f)();
            ostack += 2;
            break;

        case RET_LONG:
            *(long long*)ostack = (*(long long (*)())f)();
            ostack += 2;
            break;

        case RET_FLOAT:
            *(float*)ostack = (*(float (*)())f)();
            ostack++;
            break;

        default:
            *ostack++ = (*(u4 (*)())f)();
            break;
    }

    asm volatile ("addl %0,%%esp" :: "r" ((args + 1) * sizeof(u4)) : "cc", "sp");
    return ostack;
}
#endif
