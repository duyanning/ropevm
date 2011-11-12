#include <iostream>
#include <cstddef>
#include <cassert>
#include "align.h"

// 一个虚假的类，仅仅为了测试两个成员函数：read,write
// 测试通过后，将read和write的实现复制过去。
class Mode {
public:
    uint32_t mode_read(uint32_t* addr)
    {
        return *addr;
    }
    void mode_write(uint32_t* addr, uint32_t value)
    {
        *addr = value;
    }

    template <class T> T read(T* addr);
    template <class T, class U> void write(T* addr, U value);

};

// 读出1字节，无论如何这1字节都包含在作为整体读出的4个字节之中。然而，
// 读出2字节，可能你读出的4字节只包含了这2字节中的1字节，你还要再读一个
// 4字节，从中取出另一个1字节，然后将这两个1字节拼接起来。
template <class T>
T
Mode::read(T* addr)
{
    if (sizeof (T) >= sizeof (uint32_t)) {
        assert(sizeof (T) == 4 || sizeof (T) == 8);
        T v;
        uint32_t* dst = reinterpret_cast<uint32_t*>(&v);
        uint32_t* src = reinterpret_cast<uint32_t*>(addr);
        for (size_t i = 0; i < sizeof (T) / sizeof (uint32_t); i++) {
            //*dst++ = *src++;
            *dst++ = mode_read(src++);
        }
        return v;
    }
    else if (sizeof (T) == 1) {
        void* base = aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *(uint32_t*)base;
        uint32_t v = mode_read((uint32_t*)base);
        v >>= addr_diff(addr, base) * 8;
        return (T)v;
    }
    else if (sizeof (T) == 2) {
        void* base = aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *(uint32_t*)base;
        uint32_t v = mode_read((uint32_t*)base);
        v >>= addr_diff(addr, base) * 8;

        ptrdiff_t offset = addr_diff(addr, base);
        if (offset == 3) {
            // 读出下一个4字节
            //uint32_t v2 = *((uint32_t*)base + 1);
            uint32_t v2 = mode_read((uint32_t*)base+1);
            v2 <<= 8;
            v2 &= 0xffff;

            v |= v2;

        }

        return (T)v;
    }
    else {
        assert(false);
    }
}

// 对于写2字节，先读出4字节，如果要写的2字节完全包含在这4字节中，就改写这4字节中的相应部分，然后将4字节写回。如果要写的2字节中的1字节在下一个4字节中，那么再读出下一个4字节，改写其中的1字节，写回。
template <class T, class U>
void
Mode::write(T* addr, U value)
{
    //assert(sizeof (T) <= sizeof (U));

    if (sizeof (T) >= sizeof (uint32_t)) {
        assert(sizeof (T) == 4 || sizeof (T) == 8);
        T v = value;
        uint32_t* dst = reinterpret_cast<uint32_t*>(addr);
        uint32_t* src = reinterpret_cast<uint32_t*>(&v);
        for (size_t i = 0; i < sizeof (T) / sizeof (uint32_t); i++) {
            //*dst++ = *src++;
            mode_write(dst++, *src++);
        }
    }
    else if (sizeof (T) == 1) {
        uint32_t* base = (uint32_t*)aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *base;
        uint32_t v = mode_read(base);

        void* p = addr_add(&v, addr_diff(addr, base));
        *(T*)p = value;         // 改动4字节中的2字节（或其中1的字节）
        //*base = v;              // 写回
        mode_write(base, v);

    }
    else if (sizeof (T) == 2) {
        uint16_t val = value;

        uint32_t* base = (uint32_t*)aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *base;     // 读出4字节
        uint32_t v = mode_read(base);

        void* p = addr_add(&v, addr_diff(addr, base)); // 要写的2字节在4字节中的位置
        *(T*)p = val;           // 改动4字节中的2字节（或其中1的字节）
        //*base = v;              // 写回
        mode_write(base, v);

        ptrdiff_t offset = addr_diff(addr, base);
        if (offset == 3) {
            //uint32_t v2 = *(base + 1); // 读出另一个4字节
            uint32_t v2 = mode_read(base + 1);
            v2 &= 0xffffff00;
            val >>= 8;
            v2 |= val;
            //*(base+1) = v2;     // 写回
            mode_write(base+1, v2);
        }
    }
    else {
        assert(false);
    }
}


using namespace std;

int main()
{
    Mode m;
    unsigned char v[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    cout << "one byte" << endl;
    for (int i = 0; i < 15; ++i) {
        cout << hex << (int)m.read(v+i) << endl;
    }
    cout << "two bytes" << endl;
    for (int i = 0; i < 14; ++i) {
        cout << hex << m.read((short*)(v+i)) << endl;
    }

    cout << "write one byte" << endl;
    m.write(v, 0xef);
    m.write(v+1, 0xde);
    m.write(v+2, 0xcd);
    m.write(v+3, 0xbc);
    m.write(v+4, 0xab);
    m.write(v+5, 0x9a);
    m.write(v+6, 0x89);
    for (auto a : v) {
        cout << hex << (int)a << endl;
    }
    cout << "write two byte" << endl;
    m.write(v, 0xef);
    m.write((short*)(v+3), 0x4321);
    m.write((short*)(v+5), 0x8765);
    for (auto a : v) {
        cout << hex << (int)a << endl;
    }
}
