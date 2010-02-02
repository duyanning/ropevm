#include "std.h"
#include "jam.h"

static int isSameRuntimePackage(Class *classobj1, Class *classobj2) {
    if(classobj1 != classobj2) {
        ClassBlock *cb1 = CLASS_CB(classobj1);
        ClassBlock *cb2 = CLASS_CB(classobj2);

        /* The class loader must match */
        if(cb1->class_loader != cb2->class_loader)
            return FALSE;
        else {
            /* And the package name */

            /* If either class is an array compare the element
               name to get rid of leading array characters (the
               class loaders are the same) */

            if(IS_ARRAY(cb1))
                cb1 = CLASS_CB(cb1->element_class);

            if(IS_ARRAY(cb2))
                cb2 = CLASS_CB(cb2->element_class);

            if(cb1 != cb2) {
                const char *ptr1 = cb1->name;
                const char *ptr2 = cb2->name;

                /* Names must match at least up to the last slash
                   in each.  Note, we do not need to check for NULLs
                   because names _must_ be different (same loader,
                   but different class). */

                while(*ptr1++ == *ptr2++);

                for(ptr1--; *ptr1 && *ptr1 != '/'; ptr1++);

                /* Didn't match to last slash in ptr1 */
                if(*ptr1)
                    return FALSE;

                for(ptr2--; *ptr2 && *ptr2 != '/'; ptr2++);

                /* Didn't match to last slash in ptr2 */
                if(*ptr2)
                    return FALSE;
            }
        }
    }
    return TRUE;
}

int checkClassAccess(Class *classobj1, Class *classobj2) {
    ClassBlock *cb1 = CLASS_CB(classobj1);

    /* We can access it if it is public */
    if(cb1->access_flags & ACC_PUBLIC)
        return TRUE;

    /* Or if they're members of the same runtime package */
    return isSameRuntimePackage(classobj1, classobj2);
}

static int checkMethodOrFieldAccess(int access_flags, Class *decl_class, Class *classobj) {

    /* Public methods and fields are always accessible */
    if(access_flags & ACC_PUBLIC)
        return TRUE;

    /* If the method or field is private, it must be declared in
       the accessing class */
    if(access_flags & ACC_PRIVATE)
        return decl_class == classobj;

    /* The method or field must be protected or package-private */

    /* If it is protected it is accessible if it is declared in the
       accessing class or in a super-class */
    if((access_flags & ACC_PROTECTED) && isSubClassOf(decl_class, classobj))
        return TRUE;

    /* Lastly protected and package-private methods/fields are accessible
       if they are in the same runtime package as the accessing class */
    return isSameRuntimePackage(decl_class, classobj);
}

int checkMethodAccess(MethodBlock *mb, Class *classobj) {
    return checkMethodOrFieldAccess(mb->access_flags, mb->classobj, classobj);
}

int checkFieldAccess(FieldBlock *fb, Class *classobj) {
    return checkMethodOrFieldAccess(fb->access_flags, fb->classobj, classobj);
}
