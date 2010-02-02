#define HEADER_SIZE		sizeof(uintptr_t)
#define LOG_OBJECT_GRAIN	3
#define FLC_BIT			2

#define HDR_ADDRESS(obj) (uintptr_t*)(((char*)obj)-HEADER_SIZE)

#define clearFlcBit(obj) {                      \
	uintptr_t *hdr_addr = HDR_ADDRESS(obj); \
        *hdr_addr &= ~FLC_BIT;                  \
	MBARRIER();                             \
}

#define setFlcBit(obj) {                        \
	uintptr_t *hdr_addr = HDR_ADDRESS(obj); \
        *hdr_addr |= FLC_BIT;                   \
	MBARRIER();                             \
}

#define testFlcBit(obj) *HDR_ADDRESS(obj) & FLC_BIT
