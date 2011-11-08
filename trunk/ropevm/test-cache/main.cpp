#include "std.h"
#include "StatesBuffer.h"

using namespace std;

void read_buf(StatesBuffer& buf, Word* addr)
{
    Word value = buf.read(addr);
    cout << "read: (" << addr << ") = " << value << endl;
}

void write_buf(StatesBuffer& buf, Word* addr, Word value)
{
    buf.write(addr, value);
    cout << "write: (" << addr << ") = " << value << endl;
}

void freeze(StatesBuffer& buf)
{
    cout << "freeze version: " << buf.version() << endl;
    buf.freeze();
}

void commit_to(StatesBuffer& buf, int ver)
{
    cout << "commit to version: " << ver << endl;
    buf.commit(ver);
}

void show_buf(StatesBuffer& buf, bool integer = true)
{
    show_buffer(cout, 0, buf, integer);
}

void reset_buf(StatesBuffer& buf)
{
    cout << "reset" << endl;
    buf.reset();
}

int main()
{
    Word x = 0;
    Word y = 99;
    StatesBuffer buf1;

    show_buf(buf1);
    read_buf(buf1, &x);
    show_buf(buf1);
    write_buf(buf1, &x, 1);
    show_buf(buf1);
    read_buf(buf1, &x);
    show_buf(buf1);
    freeze(buf1);
    freeze(buf1);
    freeze(buf1);
    freeze(buf1);
    show_buf(buf1);
    read_buf(buf1, &x);
    show_buf(buf1);
    write_buf(buf1, &x, 2);
    show_buf(buf1);
    freeze(buf1);
    show_buf(buf1);
    read_buf(buf1, &y);
    freeze(buf1);
    show_buf(buf1);
    write_buf(buf1, &y, 98);
    show_buf(buf1);
    freeze(buf1);
    freeze(buf1);
    show_buf(buf1);
    write_buf(buf1, &x, 3);
    show_buf(buf1);
    commit_to(buf1, 3);
    show_buf(buf1);
    commit_to(buf1, 4);
    show_buf(buf1);
    commit_to(buf1, 6);
    show_buf(buf1);
    commit_to(buf1, 8);
    show_buf(buf1);
    write_buf(buf1, &x, 14);
    write_buf(buf1, &y, 95);
    show_buf(buf1);
    reset_buf(buf1);
    show_buf(buf1);
    write_buf(buf1, &x, 14);
    write_buf(buf1, &y, 95);
    show_buf(buf1);
    freeze(buf1);
    freeze(buf1);
    freeze(buf1);
    freeze(buf1);
    freeze(buf1);
    freeze(buf1);
    freeze(buf1);
    show_buf(buf1);
    write_buf(buf1, &x, 14);
    write_buf(buf1, &y, 95);
    show_buf(buf1);
    commit_to(buf1, 0);
    show_buf(buf1);
    commit_to(buf1, 6);
    show_buf(buf1);
    commit_to(buf1, 3);
    show_buf(buf1);

    cout << "x is : " << x << endl;
    cout << "y is : " << y << endl;
}
