typedef struct vm_method {
    const char* methodname;
    uintptr_t* (*method)(Class *, MethodBlock *, uintptr_t *);
} VMMethod;

typedef struct vm_class {
    const char* classname;
    VMMethod* methods;
} VMClass;

extern VMClass native_methods[];
