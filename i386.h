#define OS_ARCH "i386"

#define HANDLER_TABLE_T static const void
#define DOUBLE_1_BITS 0x3ff0000000000000LL

#define READ_DBL(v,p,l)	v = ((u8)p[0]<<56)|((u8)p[1]<<48)|((u8)p[2]<<40) \
                            |((u8)p[3]<<32)|((u8)p[4]<<24)|((u8)p[5]<<16) \
                            |((u8)p[6]<<8)|(u8)p[7]; p+=8

extern void setDoublePrecision();
#define FPU_HACK setDoublePrecision()

#define COMPARE_AND_SWAP(addr, old_val, new_val)    \
({                                                  \
    char result;                                    \
    __asm__ __volatile__ ("                         \
        lock;                                       \
        cmpxchgl %4, %1;                            \
        sete %0"                                    \
    : "=q" (result), "=m" (*addr)                   \
    : "m" (*addr), "a" (old_val), "r" (new_val)     \
    : "memory");                                    \
    result;                                         \
})

#ifdef USE_CMPXCHG8B
#define COMPARE_AND_SWAP_64(addr, old_val, new_val) \
({                                                  \
    int ov_hi = old_val >> 32;                      \
    int ov_lo = old_val & 0xffffffff;               \
    int nv_hi = new_val >> 32;                      \
    int nv_lo = new_val & 0xffffffff;               \
    char result;                                    \
    __asm__ __volatile__ ("                         \
        lock;                                       \
        cmpxchg8b %1;                               \
        sete %0"                                    \
    : "=q" (result)                                 \
    : "m" (*addr), "d" (ov_hi), "a" (ov_lo),        \
      "c" (nv_hi), "b" (nv_lo)                      \
    : "memory");                                    \
    result;                                         \
})
#endif

#define FLUSH_CACHE(addr, length)

#define LOCKWORD_READ(addr) *addr
#define LOCKWORD_WRITE(addr, value) *addr = value
#define LOCKWORD_COMPARE_AND_SWAP(addr, old_val, new_val) \
        COMPARE_AND_SWAP(addr, old_val, new_val)

#define UNLOCK_MBARRIER() __asm__ __volatile__ ("" ::: "memory")
#define JMM_LOCK_MBARRIER() __asm__ __volatile__ ("" ::: "memory")
#define JMM_UNLOCK_MBARRIER() __asm__ __volatile__ ("" ::: "memory")
#define MBARRIER() __asm__ __volatile__ ("lock; addl $0,0(%%esp)" ::: "memory")
