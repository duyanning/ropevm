#include "std.h"
#include "Stat.h"

using namespace std;

class A {
public:

    void f1();
    STAT_DECL(
              int m_x;
              void foo();
              );
    void f2();
};

STAT_DECL\
(
 void
 A::foo()
 {
     cout << m_x << endl;
 }

 ) // STAT_CODE

int main()
{
    A a;

    int x = 9;
    cout << "x is " << x << endl;
    STAT_DECL\
        (
         int y = 3;
         int z = 4;
         );

    STAT_CODE(
              int x = 0;
              x++;
              cout << "stat x is " << x << endl;
              cout << "stat y is " << y << endl;
              cout << "stat z is " << z << endl;
              );
    cout << "x is " << x << endl;

    STAT_CODE\
        (
         a.foo();
         )
}
