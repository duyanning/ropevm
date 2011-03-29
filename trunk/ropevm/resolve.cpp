#include "std.h"
#include "jam.h"
#include "symbol.h"
#include "excep.h"

MethodBlock *findMethod(Class *classobj, const char* methodname, const char* type) {
   ClassBlock *cb = CLASS_CB(classobj);
   MethodBlock *mb = cb->methods;
   int i;

   for(i = 0; i < cb->methods_count; i++,mb++)
       if(mb->name == methodname && mb->type == type)
          return mb;

   return NULL;
}

/* As a Java program can't have two fields with the same name but different types,
   we used to give up if we found a field with the right name but wrong type.
   However, obfuscators rename fields, breaking this optimisation.
*/
FieldBlock *findField(Class *classobj, const char *fieldname, const char *type) {
    ClassBlock *cb = CLASS_CB(classobj);
    FieldBlock *fb = cb->fields;
    int i;

    for(i = 0; i < cb->fields_count; i++,fb++)
        if(fb->name == fieldname && fb->type == type)
            return fb;

    return NULL;
}

MethodBlock *lookupMethod(Class *classobj, const char* methodname, const char* type) {
    MethodBlock *mb;

    if((mb = findMethod(classobj, methodname, type)))
       return mb;

    if(CLASS_CB(classobj)->super)
        return lookupMethod(CLASS_CB(classobj)->super, methodname, type);

    return NULL;
}

FieldBlock *lookupField(Class *classobj, const char *fieldname, const char* fieldtype) {
    ClassBlock *cb;
    FieldBlock *fb;
    int i;

    if((fb = findField(classobj, fieldname, fieldtype)) != NULL)
        return fb;

    cb = CLASS_CB(classobj);
    i = cb->super ? CLASS_CB(cb->super)->imethod_table_size : 0;

    for(; i < cb->imethod_table_size; i++) {
        Class *intf = cb->imethod_table[i].interface;
        if((fb = findField(intf, fieldname, fieldtype)) != NULL)
            return fb;
    }

    if(cb->super)
        return lookupField(cb->super, fieldname, fieldtype);

    return NULL;
}

Class *resolveClass(Class *classobj, int cp_index, int init) {
    ConstantPool *cp = &(CLASS_CB(classobj)->constant_pool);
    Class *resolved_class = NULL;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_ResolvedClass:
            resolved_class = (Class *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_Class: {
            char *classname;
            int name_idx = CP_CLASS(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_Class)
                goto retry;

            classname = CP_UTF8(cp, name_idx);
            // if (strcmp(classname, "Wolf") == 0) {
            //     int x = 0;
            //     x++;
            // }
            resolved_class = findClassFromClass(classname, classobj);

            /* If we can't find the class an exception will already have
               been thrown */

            if(resolved_class == NULL)
                return NULL;

            if(!checkClassAccess(resolved_class, classobj)) {
                signalException(java_lang_IllegalAccessError, "class is not accessible");
                return NULL;
            }

            CP_TYPE(cp, cp_index) = CONSTANT_Locked;
            MBARRIER();
            CP_INFO(cp, cp_index) = (uintptr_t)resolved_class;
            MBARRIER();
            CP_TYPE(cp, cp_index) = CONSTANT_ResolvedClass;

            break;
        }
    }

    if(init)
        initClass(resolved_class);

    return resolved_class;
}

MethodBlock *resolveMethod(Class *classobj, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(classobj)->constant_pool);
    MethodBlock *mb = NULL;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            mb = (MethodBlock *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_Methodref: {
            Class *resolved_class;
            ClassBlock *resolved_cb;
            char *methodname, *methodtype;
            int cl_idx = CP_METHOD_CLASS(cp, cp_index);
            int name_type_idx = CP_METHOD_NAME_TYPE(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_Methodref)
                goto retry;

            methodname = CP_UTF8(cp, CP_NAME_TYPE_NAME(cp, name_type_idx));

            methodtype = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, name_type_idx));
            resolved_class = resolveClass(classobj, cl_idx, FALSE);
            resolved_cb = CLASS_CB(resolved_class);

            if(exceptionOccurred())
                return NULL;

            if(resolved_cb->access_flags & ACC_INTERFACE) {
                signalException(java_lang_IncompatibleClassChangeError, NULL);
                return NULL;
            }

            mb = lookupMethod(resolved_class, methodname, methodtype);

            if(mb) {
                if((mb->access_flags & ACC_ABSTRACT) &&
                       !(resolved_cb->access_flags & ACC_ABSTRACT)) {
                    signalException(java_lang_AbstractMethodError, methodname);
                    return NULL;
                }

                if(!checkMethodAccess(mb, classobj)) {
                    signalException(java_lang_IllegalAccessError, "method is not accessible");
                    return NULL;
                }

                if(initClass(mb->classobj) == NULL)
                    return NULL;

                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)mb;
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_Resolved;
            } else
                signalException(java_lang_NoSuchMethodError, methodname);

            break;
        }
    }

    return mb;
}

MethodBlock *resolveInterfaceMethod(Class *classobj, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(classobj)->constant_pool);
    MethodBlock *mb = NULL;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            mb = (MethodBlock *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_InterfaceMethodref: {
            Class *resolved_class;
            char *methodname, *methodtype;
            int cl_idx = CP_METHOD_CLASS(cp, cp_index);
            int name_type_idx = CP_METHOD_NAME_TYPE(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_InterfaceMethodref)
                goto retry;

            methodname = CP_UTF8(cp, CP_NAME_TYPE_NAME(cp, name_type_idx));
            methodtype = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, name_type_idx));
            resolved_class = resolveClass(classobj, cl_idx, FALSE);

            if(exceptionOccurred())
                return NULL;

            if(!(CLASS_CB(resolved_class)->access_flags & ACC_INTERFACE)) {
                signalException(java_lang_IncompatibleClassChangeError, NULL);
                return NULL;
            }

            mb = lookupMethod(resolved_class, methodname, methodtype);
            if(mb == NULL) {
                ClassBlock *cb = CLASS_CB(resolved_class);
                int i;

                for(i = 0; mb == NULL && (i < cb->imethod_table_size); i++) {
                    Class *intf = cb->imethod_table[i].interface;
                    mb = findMethod(intf, methodname, methodtype);
                }
            }

            if(mb) {
                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)mb;
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_Resolved;
            } else
                signalException(java_lang_NoSuchMethodError, methodname);

            break;
        }
    }

    return mb;
}

FieldBlock *resolveField(Class *classobj, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(classobj)->constant_pool);
    FieldBlock *fb = NULL;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            fb = (FieldBlock *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_Fieldref: {
            Class *resolved_class;
            char *fieldname, *fieldtype;
            int cl_idx = CP_FIELD_CLASS(cp, cp_index);
            int name_type_idx = CP_FIELD_NAME_TYPE(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_Fieldref)
                goto retry;

            fieldname = CP_UTF8(cp, CP_NAME_TYPE_NAME(cp, name_type_idx));
            fieldtype = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, name_type_idx));
            resolved_class = resolveClass(classobj, cl_idx, FALSE);

            if(exceptionOccurred())
                return NULL;

            fb = lookupField(resolved_class, fieldname, fieldtype);

            if(fb) {
                if(!checkFieldAccess(fb, classobj)) {
                    signalException(java_lang_IllegalAccessError, "field is not accessible");
                    return NULL;
                }

                if(initClass(fb->classobj) == NULL)
                    return NULL;

                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)fb;
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_Resolved;
            } else
                signalException(java_lang_NoSuchFieldError, fieldname);

            break;
        }
    }

    return fb;
}

uintptr_t resolveSingleConstant(Class *classobj, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(classobj)->constant_pool);

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Class:
            resolveClass(classobj, cp_index, FALSE);
            break;

        case CONSTANT_String: {
            Object *string;
            int idx = CP_STRING(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_String)
                goto retry;

            string = createString(CP_UTF8(cp, idx));

            if(string) {
                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)findInternedString(string);
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_ResolvedString;
            }

            break;
        }

        default:
            break;
    }

    return CP_INFO(cp, cp_index);
}

MethodBlock *lookupVirtualMethod(Object *ob, MethodBlock *mb) {
    ClassBlock *cb = CLASS_CB(ob->classobj);
    int mtbl_idx = mb->method_table_index;

    if(mb->access_flags & ACC_PRIVATE)
        return mb;

    if(CLASS_CB(mb->classobj)->access_flags & ACC_INTERFACE) {
        int i;

        for(i = 0; (i < cb->imethod_table_size) &&
                   (mb->classobj != cb->imethod_table[i].interface); i++);

        if(i == cb->imethod_table_size) {
            signalException(java_lang_IncompatibleClassChangeError,
                            "unimplemented interface");
            return NULL;
        }
        mtbl_idx = cb->imethod_table[i].offsets[mtbl_idx];
    }

    mb = cb->method_table[mtbl_idx];

    if(mb->access_flags & ACC_ABSTRACT) {
        signalException(java_lang_AbstractMethodError, mb->name);
        return NULL;
    }

    return mb;
}

MethodBlock* get_rvp_method(MethodBlock* complete)
{
    using namespace std;

    MethodBlock* rvpmb;

    std::string method_name;

    if (strcmp(complete->name, "<init>") != 0) {
        method_name += "_p_slice_for_";
        method_name += complete->name;
    }
    else {
        method_name = "_p_slice_for_ctor";
    }

    // note: lookupMethod compares pointers, so we use newUtf8
    rvpmb = lookupMethod(complete->classobj, copyUtf8(method_name.c_str()), complete->type);

    if (rvpmb == 0) {
        MINILOG0("WARNING: no rvpet for " << *complete << ", use complete method");
        //assert(simplified);
        return complete;
    }
    return rvpmb;
}
