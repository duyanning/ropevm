#include "std.h"

/* Change floating point precision to double (64-bit) from
 * the extended (80-bit) Linux default. */

void setDoublePrecision() {
#ifdef __CYGWIN__
    // cygwin下没有fpu_control.h这个头文件
#else
    fpu_control_t cw;

    _FPU_GETCW(cw);
    cw &= ~_FPU_EXTENDED;
    cw |= _FPU_DOUBLE;
    _FPU_SETCW(cw);
#endif
}

void initialisePlatform() {
    setDoublePrecision();
}
