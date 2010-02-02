#include "std.h"

/* Change floating point precision to double (64-bit) from
 * the extended (80-bit) Linux default. */

void setDoublePrecision() {
    fpu_control_t cw;

    _FPU_GETCW(cw);
    cw &= ~_FPU_EXTENDED;
    cw |= _FPU_DOUBLE;
    _FPU_SETCW(cw);
}

void initialisePlatform() {
    setDoublePrecision();
}
