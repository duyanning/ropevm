#include "std.h"
#include "rope.h"

#ifndef USE_FFI
#include <string.h>
//#include "../../../sig.h"
#include "sig.h"

// 本文件已根据jamvm-1.5.4进行了更新。解决了调用gcc 4.7编译的GNU
// Classpath中native代码时出现的返回值不对的问题。
#define RET_VOID    0
#define RET_DOUBLE  1
#define RET_LONG    2
#define RET_FLOAT   3
#define RET_BYTE    4
#define RET_CHAR    5
#define RET_SHORT   6
#define RET_DFLT    7

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
            case 'B':
            case 'Z':
                return RET_BYTE;
            case 'C':
                return RET_CHAR;
            case 'S':
                return RET_SHORT;
        }

    return RET_DFLT;
}

// env		传递给native code编写的函数，作为参数
// classobj	传递给native code编写的函数，作为参数
// sig		方法签名（未用）
// ret_type	返回值类型
// ostack	传递给native code编写的函数的其他参数，此时就静静躺在native方法的ostack中。（是native方法的调用者复制到native方法的ostack中的）
// f		指向native code的起始地址
// args		native方法的参数数量
// 当调用结束后，返回值就静静地躺在ostack的最底下。
u4 *callJNIMethod(void* env, Class* classobj, char* sig, int ret_type, u4* ostack, unsigned char* f, int args)
{
    u4 *opntr = ostack + args;
    int i;

    // 为调用native code编写的函数做准备：将函数的参数压入native栈。嵌入汇编中的sp为native机器的sp

    // 结合一个例子来看：native方法的C++原型：jdouble Java_Math_sqrt(JNIEnv* env, jclass cls, jdouble x);

    // 例子：x入native栈
    for(i = 0; i < args; i++)
        asm volatile ("pushl %0" :: "m" (*--opntr) : "sp");

    // 例子：cls入native栈
    if(classobj) {
        asm volatile ("pushl %0" :: "m" (classobj) : "sp");
        args++;
    }

    // 例子：env入native栈
    asm volatile ("pushl %0" :: "m" (env) : "sp");


    // 下边真正调用native code编写的代码时，不传递任何参数。因为可能会
    // 碰到的native有无穷多，不可能事先在C/C++中声明其原型。只能采用这
    // 种方式。
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

        case RET_BYTE:
            *ostack++ = (*(signed char (*)())f)();
            break;

        case RET_CHAR:
            *ostack++ = (*(unsigned short (*)())f)();
            break;

        case RET_SHORT:
            *ostack++ = (*(signed short (*)())f)();
            break;

        default:
            *ostack++ = (*(u4 (*)())f)();
            break;
    }


    // 调用native code编写的函数结束，清理native栈
    asm volatile ("addl %0,%%esp" :: "r" ((args + 1) * sizeof(u4)) : "cc", "sp");



    // 此时，返回值就静静地躺在ostack的最底下。
    return ostack;
}
#endif
