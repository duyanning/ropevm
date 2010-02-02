#ifndef ALIGN_H
#define ALIGN_H

inline
void*
aligned_base(void* addr, int align)
{
    return (void*)((uintptr_t)(addr) & ~(align - 1));
}

inline
void*
align(void* addr, int align)
{
    return (void*)(align + (((uintptr_t)addr - 1) & ~(align - 1)));
}


inline
ptrdiff_t
addr_diff(void* p1, void* p2)
{
    return (uintptr_t)p1 - (uintptr_t)p2;
}

inline
void*
addr_add(void* p, ptrdiff_t diff)
{
    return (void*)((uintptr_t)p + diff);
}

#endif // ALIGN_H
