#include "std.h"
#include "jam.h"

char implements(Class *classobj, Class *test) {
    ClassBlock *test_cb = CLASS_CB(test);
    int i;

    for(i = 0; i < test_cb->interfaces_count; i++)
        if((classobj == test_cb->interfaces[i]) ||
                      implements(classobj, test_cb->interfaces[i]))
            return TRUE;

    if(test_cb->super)
        return implements(classobj, test_cb->super);

    return FALSE;
}

char isSubClassOf(Class *classobj, Class *test) {
    for(; test != NULL && test != classobj; test = CLASS_CB(test)->super);
    return test != NULL;
}

char isInstOfArray0(Class *array_class, Class *test_elem, int test_dim) {
    ClassBlock *array_cb = CLASS_CB(array_class);
    Class *array_elem = array_cb->element_class;

    if(test_dim == array_cb->dim)
        return isInstanceOf(array_elem, test_elem);

    if(test_dim > array_cb->dim)
        return IS_INTERFACE(CLASS_CB(array_elem)) ?
                     implements(array_elem, array_class) :
                     (array_elem == array_cb->super);

    return FALSE;
}

char isInstOfArray(Class *classobj, Class *test) {
    ClassBlock *test_cb = CLASS_CB(test);

    if(!IS_ARRAY(CLASS_CB(classobj)))
        return classobj == test_cb->super;

    return isInstOfArray0(classobj, test_cb->element_class, test_cb->dim);
}

char isInstanceOf(Class *classobj, Class *test) {
    if(classobj == test)
        return TRUE;

    if(IS_INTERFACE(CLASS_CB(classobj)))
        return implements(classobj, test);
    else
        if(IS_ARRAY(CLASS_CB(test)))
            return isInstOfArray(classobj, test);
        else
            return isSubClassOf(classobj, test);
}

char arrayStoreCheck(Class *array_class, Class *test) {
    ClassBlock *test_cb = CLASS_CB(test);

    if(!IS_ARRAY(test_cb))
        return isInstOfArray0(array_class, test, 1);

    return isInstOfArray0(array_class, test_cb->element_class, test_cb->dim + 1);
}
