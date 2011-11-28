#include <cstdarg>
#include <cmath>
#include <cassert>
#include "Math.h"

using namespace std;

JNIEXPORT jdouble JNICALL Java_Math_sqrt
  (JNIEnv *, jclass, jdouble x)
{
    return sqrt(x);
}


JNIEXPORT jdouble JNICALL Java_Math_pow
  (JNIEnv *, jclass, jdouble x, jdouble y)
{
    return pow (x, y);
}


JNIEXPORT jdouble JNICALL Java_Math_floor
  (JNIEnv *, jclass, jdouble x)
{
    return floor (x);
}
