#include "std.h"
#include "jam.h"

void *nativeStackBase() {
#ifdef __UCLIBC__
    return NULL;
#else
    pthread_attr_t attr;
    void *addr;
    size_t size;

    pthread_getattr_np(pthread_self(), &attr);
    pthread_attr_getstack(&attr, &addr, &size);

    return (char*)addr+size;
#endif
}

int nativeAvailableProcessors() {
#ifdef __UCLIBC__
    return 1;
#else
    return get_nprocs();
#endif
}

char *nativeLibPath() {
    return getenv("LD_LIBRARY_PATH");
}

void *nativeLibOpen(char *path) {
    return dlopen(path, RTLD_LAZY);
}

void nativeLibClose(void *handle) {
    dlclose(handle);
}

void *nativeLibSym(void *handle, const char *symbol) {
    return dlsym(handle, symbol);
}

char *nativeLibMapName(char *name) {
    char *buff = (char*)sysMalloc(strlen(name) + sizeof("lib.so") + 1);

   sprintf(buff, "lib%s.so", name);
   return buff;
}
