#include "std.h"
#include "align.h"

using namespace std;

int main()
{
    int a = 0x11223344;
    cout << a << endl;

    uint8_t* p = reinterpret_cast<uint8_t*>(&a);
    for (int i = 0; i < 4; ++i) {
        cout << (void*)&p[i] << " " << hex << (int)p[i]
             << " " << aligned_base(&p[i], 4)
             << " " << align(&p[i], 4)
             << endl;
    }

    cout << dec;
    cout << addr_diff(&a, &p[3]) << endl;
    cout << addr_diff(&p[3], &a) << endl;
    //cout << &a - &p[3] << endl;
    cout << addr_add(&a, 2) << endl;

}
