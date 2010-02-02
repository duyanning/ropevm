#include "std.h"
#include "Cache.h"

using namespace std;

void read_cache(Cache& cache, Word* addr)
{
    Word value = cache.read(addr);
    cout << "read: (" << addr << ") = " << value << endl;
}

void write_cache(Cache& cache, Word* addr, Word value)
{
    cache.write(addr, value);
    cout << "write: (" << addr << ") = " << value << endl;
}

void freeze(Cache& cache)
{
    cout << "freeze version: " << cache.version() << endl;
    cache.snapshot();
}

void commit_to(Cache& cache, int ver)
{
    cout << "commit to version: " << ver << endl;
    cache.commit(ver);
}

void show_cache(Cache& cache, bool integer = true)
{
    show_cache(cout, 0, cache, integer);
}

void reset_cache(Cache& cache)
{
    cout << "reset" << endl;
    cache.reset();
}

int main()
{
    Word x = 0;
    Word y = 99;
    Cache cache1;

    show_cache(cache1);
    read_cache(cache1, &x);
    show_cache(cache1);
    write_cache(cache1, &x, 1);
    show_cache(cache1);
    read_cache(cache1, &x);
    show_cache(cache1);
    freeze(cache1);
    freeze(cache1);
    freeze(cache1);
    freeze(cache1);
    show_cache(cache1);
    read_cache(cache1, &x);
    show_cache(cache1);
    write_cache(cache1, &x, 2);
    show_cache(cache1);
    freeze(cache1);
    show_cache(cache1);
    read_cache(cache1, &y);
    freeze(cache1);
    show_cache(cache1);
    write_cache(cache1, &y, 98);
    show_cache(cache1);
    freeze(cache1);
    freeze(cache1);
    show_cache(cache1);
    write_cache(cache1, &x, 3);
    show_cache(cache1);
    commit_to(cache1, 3);
    show_cache(cache1);
    commit_to(cache1, 4);
    show_cache(cache1);
    commit_to(cache1, 6);
    show_cache(cache1);
    commit_to(cache1, 8);
    show_cache(cache1);
    write_cache(cache1, &x, 14);
    write_cache(cache1, &y, 95);
    show_cache(cache1);
    reset_cache(cache1);
    show_cache(cache1);
    write_cache(cache1, &x, 14);
    write_cache(cache1, &y, 95);
    show_cache(cache1);
    freeze(cache1);
    freeze(cache1);
    freeze(cache1);
    freeze(cache1);
    freeze(cache1);
    freeze(cache1);
    freeze(cache1);
    show_cache(cache1);
    write_cache(cache1, &x, 14);
    write_cache(cache1, &y, 95);
    show_cache(cache1);
    commit_to(cache1, 0);
    show_cache(cache1);
    commit_to(cache1, 6);
    show_cache(cache1);
    commit_to(cache1, 3);
    show_cache(cache1);

    cout << "x is : " << x << endl;
    cout << "y is : " << y << endl;
}
