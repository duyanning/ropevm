#include "std.h"
#include "../MiniLogger.h"

using namespace std;

class A {
public:
    int x;
};

ostream& operator<<(ostream& os, const A& a)
{
    os << "data is: " << a.x;
    return os;
}

void log_proc(std::ostream& os, int x, double y)
{
    os << "x: " << x << endl;
    os << "y: " << y << endl;
}

int main()
{
    MiniLogger logger1(cout, true);
    MiniLogger logger2(cout, true, &logger1);

    A a;
    a.x = 3;
    MINILOG(logger1, "logger 1 " << a);

    A b;
    b.x = 4;
    MINILOG(logger2, "logger 2 " << b);

    logger1 << "x";
    cout << endl;
    //logger1 << endl;

    MINILOGPROC(logger1, log_proc, (os, 1, 2));
}
