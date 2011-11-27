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
