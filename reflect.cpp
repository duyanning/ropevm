#include "std.h"
#include "rope.h"
#include "frame.h"
#include "lock.h"
#include "class.h"
#include "symbol.h"
#include "excep.h"
#include "SpmtThread.h"


static char inited = FALSE;

static Class *class_array_class, *cons_array_class, *cons_reflect_class, *method_array_class;
static Class *method_reflect_class, *field_array_class, *field_reflect_class;
static MethodBlock *cons_init_mb, *method_init_mb, *field_init_mb;
static int cons_slot_offset, method_slot_offset, field_slot_offset;
static int cons_class_offset, method_class_offset, field_class_offset;

static int initReflection() {
    FieldBlock *cons_slot_fb, *mthd_slot_fb, *fld_slot_fb;
    FieldBlock *cons_class_fb, *mthd_class_fb, *fld_class_fb;

    class_array_class = findArrayClass(SYMBOL(array_java_lang_Class));
    cons_array_class = findArrayClass(SYMBOL(array_java_lang_reflect_Constructor));
    cons_reflect_class = findSystemClass(SYMBOL(java_lang_reflect_Constructor));
    method_array_class = findArrayClass(SYMBOL(array_java_lang_reflect_Method));
    method_reflect_class = findSystemClass(SYMBOL(java_lang_reflect_Method));
    field_array_class = findArrayClass(SYMBOL(array_java_lang_reflect_Field));
    field_reflect_class = findSystemClass(SYMBOL(java_lang_reflect_Field));

    if(!cons_array_class || !cons_reflect_class || !method_array_class ||
          !method_reflect_class || !field_array_class || !field_reflect_class)
        return FALSE;

    cons_init_mb = findMethod(cons_reflect_class, SYMBOL(object_init),
                              SYMBOL(_java_lang_Class_array_java_lang_Class_array_java_lang_Class_I__V));

    method_init_mb = findMethod(method_reflect_class, SYMBOL(object_init), SYMBOL(
         _java_lang_Class_array_java_lang_Class_array_java_lang_Class_java_lang_Class_java_lang_String_I__V));

    field_init_mb = findMethod(field_reflect_class, SYMBOL(object_init),
                               SYMBOL(_java_lang_Class_java_lang_Class_java_lang_String_I__V));

    cons_slot_fb = findField(cons_reflect_class, SYMBOL(slot), SYMBOL(I));
    mthd_slot_fb = findField(method_reflect_class, SYMBOL(slot), SYMBOL(I));
    fld_slot_fb = findField(field_reflect_class, SYMBOL(slot), SYMBOL(I));
    cons_class_fb = findField(cons_reflect_class, SYMBOL(declaringClass), SYMBOL(sig_java_lang_Class));
    mthd_class_fb = findField(method_reflect_class, SYMBOL(declaringClass), SYMBOL(sig_java_lang_Class));
    fld_class_fb = findField(field_reflect_class, SYMBOL(declaringClass), SYMBOL(sig_java_lang_Class));

    if(!cons_init_mb || ! method_init_mb || !field_init_mb ||
           !cons_slot_fb || !mthd_slot_fb || !fld_slot_fb ||
           !cons_class_fb || !mthd_class_fb || !fld_class_fb) {
        /* Find Field/Method doesn't throw an exception... */
        signalException(java_lang_InternalError, "Expected field/method doesn't exist");
        return FALSE;
    }

    cons_slot_offset = cons_slot_fb->offset;
    method_slot_offset = mthd_slot_fb->offset;
    field_slot_offset = fld_slot_fb->offset;
    cons_class_offset = cons_class_fb->offset;
    method_class_offset = mthd_class_fb->offset;
    field_class_offset = fld_class_fb->offset;

    registerStaticClassRefLocked(&class_array_class);
    registerStaticClassRefLocked(&cons_array_class);
    registerStaticClassRefLocked(&method_array_class);
    registerStaticClassRefLocked(&field_array_class);
    registerStaticClassRefLocked(&cons_reflect_class);
    registerStaticClassRefLocked(&method_reflect_class);
    registerStaticClassRefLocked(&field_reflect_class);

    return inited = TRUE;
}

Class *convertSigElement2Class(char **sig_pntr, Class *declaring_class) {
    char *sig = *sig_pntr;
    Class *classobj;

    switch(*sig) {
        case '[': {
            char next;
            while(*++sig == '[');
            if(*sig == 'L')
                while(*++sig != ';');
            next = *++sig;
            *sig = '\0';
            classobj = findArrayClassFromClass(*sig_pntr, declaring_class);
            *sig = next;
            break;
        }

        case 'L':
            while(*++sig != ';');
            *sig++ = '\0';
            classobj = findClassFromClass((*sig_pntr)+1, declaring_class);
            break;

        default:
            classobj = findPrimitiveClass(*sig++);
            break;
    }
    *sig_pntr = sig;
    return classobj;
}

Object *convertSig2ClassArray(char **sig_pntr, Class *declaring_class) {
    char *sig = *sig_pntr;
    int no_params, i = 0;
    Class **params;
    Object *array;

    for(no_params = 0; *++sig != ')'; no_params++) {
        if(*sig == '[')
            while(*++sig == '[');
        if(*sig == 'L')
            while(*++sig != ';');
    }

    if((array = allocArray(class_array_class, no_params, sizeof(Class*))) == NULL)
        return NULL;

    params = (Class**)ARRAY_DATA(array);

    *sig_pntr += 1;
    while(**sig_pntr != ')')
        if((params[i++] = convertSigElement2Class(sig_pntr, declaring_class)) == NULL)
            return NULL;

    return array;
}

Object *getExceptionTypes(MethodBlock *mb) {
    int i;
    Object *array;
    Class **excps;

    if((array = allocArray(class_array_class, mb->throw_table_size, sizeof(Class*))) == NULL)
        return NULL;

    excps = (Class**)ARRAY_DATA(array);

    for(i = 0; i < mb->throw_table_size; i++)
        if((excps[i] = resolveClass(mb->classobj, mb->throw_table[i], FALSE)) == NULL)
            return NULL;

    return array;
}

Object *createConstructorObject(MethodBlock *mb) {
    Object *reflect_ob;

    if((reflect_ob = allocObject(cons_reflect_class))) {
        char *signature = (char*)sysMalloc(strlen(mb->type) + 1);
        char *sig = signature;
        Object *classes, *exceps;

        strcpy(sig, mb->type);
        classes = convertSig2ClassArray(&sig, mb->classobj);
        exceps = getExceptionTypes(mb);
        sysFree(signature);

        if((classes == NULL) || (exceps == NULL))
            return NULL;

        DummyFrame dummy;
        executeMethod(&dummy, reflect_ob, cons_init_mb, mb->classobj, classes, exceps,
                      mb - CLASS_CB(mb->classobj)->methods);
    }

    return reflect_ob;
}

Object *getClassConstructors(Class *classobj, int publi) {
    ClassBlock *cb = CLASS_CB(classobj);
    Object *array, **cons;
    int count = 0;
    int i, j;

    if(!inited && !initReflection())
        return NULL;

    for(i = 0; i < cb->methods_count; i++) {
        MethodBlock *mb = &cb->methods[i];
        if((mb->name == SYMBOL(object_init)) && (!publi || (mb->access_flags & ACC_PUBLIC)))
            count++;
    }

    if((array = allocArray(cons_array_class, count, sizeof(Object*))) == NULL)
        return NULL;
    cons = (Object**)ARRAY_DATA(array);

    for(i = 0, j = 0; j < count; i++) {
        MethodBlock *mb = &cb->methods[i];

        if((mb->name == SYMBOL(object_init)) && (!publi || (mb->access_flags & ACC_PUBLIC)))
            if((cons[j++] = createConstructorObject(mb)) == NULL)
                return NULL;
    }

    return array;
}

Object *createMethodObject(MethodBlock *mb) {
    Object *reflect_ob;

    if((reflect_ob = allocObject(method_reflect_class))) {
        char *signature = (char*)sysMalloc(strlen(mb->type) + 1);
        char *sig = signature;
        Object *classes, *exceps, *name;
        Class *ret;

        strcpy(sig, mb->type);
        classes = convertSig2ClassArray(&sig, mb->classobj);
        exceps = getExceptionTypes(mb);
        name = createString(mb->name);

        sig++;
        ret = convertSigElement2Class(&sig, mb->classobj);
        sysFree(signature);

        if((classes == NULL) || (exceps == NULL) || (name == NULL) || (ret == NULL))
            return NULL;

        DummyFrame dummy;
        executeMethod(&dummy, reflect_ob, method_init_mb, mb->classobj, classes, exceps, ret, name,
                      mb - CLASS_CB(mb->classobj)->methods);
    }

    return reflect_ob;
}

Object *getClassMethods(Class *classobj, int publi) {
    ClassBlock *cb = CLASS_CB(classobj);
    Object *array, **methods;
    int count = 0;
    int i, j;

    if(!inited && !initReflection())
        return NULL;

    for(i = 0; i < cb->methods_count; i++) {
        MethodBlock *mb = &cb->methods[i];
        if((mb->name[0] != '<') && (!publi || (mb->access_flags & ACC_PUBLIC))
                                && ((mb->access_flags & ACC_MIRANDA) == 0))
            count++;
    }

    if((array = allocArray(method_array_class, count, sizeof(Object*))) == NULL)
        return NULL;
    methods = (Object**)ARRAY_DATA(array);

    for(i = 0, j = 0; j < count; i++) {
        MethodBlock *mb = &cb->methods[i];

        if((mb->name[0] != '<') && (!publi || (mb->access_flags & ACC_PUBLIC))
                                && ((mb->access_flags & ACC_MIRANDA) == 0))

            if((methods[j++] = createMethodObject(mb)) == NULL)
                return NULL;
    }

    return array;
}

Object *createFieldObject(FieldBlock *fb) {
    Object *reflect_ob;

    if((reflect_ob = allocObject(field_reflect_class))) {
        char *signature = (char*)sysMalloc(strlen(fb->type) + 1);
        char *sig = signature;
        Object *name;
        Class *type;

        strcpy(signature, fb->type);
        type = convertSigElement2Class(&sig, fb->classobj);
        sysFree(signature);
        name = createString(fb->name);

        if((type == NULL) || (name == NULL))
            return NULL;

        DummyFrame dummy;
        executeMethod(&dummy, reflect_ob, field_init_mb, fb->classobj, type, name,
                      fb - CLASS_CB(fb->classobj)->fields);
    }

    return reflect_ob;
}

Object *getClassFields(Class *classobj, int publi) {
    ClassBlock *cb = CLASS_CB(classobj);
    Object *array, **fields;
    int count = 0;
    int i, j;

    if(!inited && !initReflection())
        return NULL;

    if(!publi)
        count = cb->fields_count;
    else
        for(i = 0; i < cb->fields_count; i++)
            if(cb->fields[i].access_flags & ACC_PUBLIC)
                count++;

    if((array = allocArray(field_array_class, count, sizeof(Object*))) == NULL)
        return NULL;
    fields = (Object**)ARRAY_DATA(array);

    for(i = 0, j = 0; j < count; i++) {
        FieldBlock *fb = &cb->fields[i];

        if(!publi || (fb->access_flags & ACC_PUBLIC))
            if((fields[j++] = createFieldObject(fb)) == NULL)
                return NULL;
    }

    return array;
}

Object *getClassInterfaces(Class *classobj) {
    ClassBlock *cb = CLASS_CB(classobj);
    Object *array;

    if(!inited && !initReflection())
        return NULL;

    if((array = allocArray(class_array_class, cb->interfaces_count, sizeof(Class*))) == NULL)
        return NULL;

    memcpy(ARRAY_DATA(array), cb->interfaces, cb->interfaces_count * sizeof(Class*));
    return array;
}

Object *getClassClasses(Class *classobj, int publi) {
    ClassBlock *cb = CLASS_CB(classobj);
    int i, j, count = 0;
    Class **classes;
    Object *array;

    if(!inited && !initReflection())
        return NULL;

    for(i = 0; i < cb->inner_class_count; i++) {
        Class *iclass;
        if((iclass = resolveClass(classobj, cb->inner_classes[i], FALSE)) == NULL)
            return NULL;
        if(!publi || (CLASS_CB(iclass)->inner_access_flags & ACC_PUBLIC))
            count++;
    }

    if((array = allocArray(class_array_class, count, sizeof(Class*))) == NULL)
        return NULL;

    classes = (Class**)ARRAY_DATA(array);
    for(i = 0, j = 0; j < count; i++) {
        Class *iclass = resolveClass(classobj, cb->inner_classes[i], FALSE);
        if(!publi || (CLASS_CB(iclass)->inner_access_flags & ACC_PUBLIC))
            classes[j++] = iclass;
    }

    return array;
}

Class *getDeclaringClass(Class *classobj) {
    ClassBlock *cb = CLASS_CB(classobj);
    return cb->declaring_class ? resolveClass(classobj, cb->declaring_class, FALSE) : NULL;
}

Class *getEnclosingClass(Class *classobj) {
    ClassBlock *cb = CLASS_CB(classobj);
    return cb->enclosing_class ? resolveClass(classobj, cb->enclosing_class, FALSE) : NULL;
}

MethodBlock *getEnclosingMethod(Class *classobj) {
    Class *enclosing_class = getEnclosingClass(classobj);

    if(enclosing_class != NULL) {
        ClassBlock *cb = CLASS_CB(classobj);

        if(cb->enclosing_method) {
            ConstantPool *cp = &cb->constant_pool;
            char *methodname = CP_UTF8(cp, CP_NAME_TYPE_NAME(cp, cb->enclosing_method));
            char *methodtype = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, cb->enclosing_method));
            MethodBlock *mb = findMethod(enclosing_class, methodname, methodtype);

            if(mb != NULL)
                return mb;

            /* The "reference implementation" throws an InternalError if a method
               with the name and type cannot be found in the enclosing class */
            signalException(java_lang_InternalError, "Enclosing method doesn't exist");
        }
    }

    return NULL;
}

Object *getEnclosingMethodObject(Class *classobj) {
    MethodBlock *mb = getEnclosingMethod(classobj);

    if(mb != NULL && mb->name == SYMBOL(object_init))
        return createMethodObject(mb);

    return NULL;
}

Object *getEnclosingConstructorObject(Class *classobj) {
    MethodBlock *mb = getEnclosingMethod(classobj);

    if(mb != NULL && mb->name == SYMBOL(object_init))
        return createConstructorObject(mb);

    return NULL;
}

static char anno_inited = FALSE;

static Class *enum_class, *map_class, *anno_inv_class, *obj_array_class;
static Class *anno_array_class, *dbl_anno_array_class;
static MethodBlock *map_init_mb, *map_put_mb, *anno_create_mb, *enum_valueof_mb;

static int initAnnotation() {
    enum_class = findSystemClass("java/lang/Enum");
    map_class = findSystemClass("java/util/HashMap");
    anno_inv_class = findSystemClass("sun/reflect/annotation/AnnotationInvocationHandler");

    obj_array_class = findArrayClass("[Ljava/lang/Object;");
    anno_array_class = findArrayClass("[Ljava/lang/annotation/Annotation;");
    dbl_anno_array_class = findArrayClass("[[Ljava/lang/annotation/Annotation;");

    if(!enum_class || !map_class || !anno_inv_class || !obj_array_class
                   || !anno_array_class || !dbl_anno_array_class)
        return FALSE;

    map_init_mb = findMethod(map_class, SYMBOL(object_init), SYMBOL(___V));
    map_put_mb = findMethod(map_class, SYMBOL(put),
                            newUtf8("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"));

    anno_create_mb = findMethod(anno_inv_class, newUtf8("create"),
                                newUtf8("(Ljava/lang/Class;Ljava/util/Map;)Ljava/lang/annotation/Annotation;"));

    enum_valueof_mb = findMethod(enum_class, newUtf8("valueOf"),
                                 newUtf8("(Ljava/lang/Class;Ljava/lang/String;)Ljava/lang/Enum;"));

    if(!map_init_mb || !map_put_mb || !anno_create_mb || !enum_valueof_mb) {
        /* FindMethod doesn't throw an exception... */
        signalException(java_lang_InternalError, "Expected field/method doesn't exist");
        return FALSE;
    }

    registerStaticClassRefLocked(&enum_class);
    registerStaticClassRefLocked(&map_class);
    registerStaticClassRefLocked(&anno_inv_class);
    registerStaticClassRefLocked(&obj_array_class);
    registerStaticClassRefLocked(&anno_array_class);
    registerStaticClassRefLocked(&dbl_anno_array_class);

    return anno_inited = TRUE;
}

Class *findClassFromSignature(char *type_name, Class *classobj) {
    Class *type_class;
    char *name, *pntr;

    name = pntr = (char*)sysMalloc(strlen(type_name));
    strcpy(name, type_name);

    type_class = convertSigElement2Class(&pntr, classobj);
    sysFree(name);

    return type_class;
}

/* Forward declarations */
Object *createWrapperObject(int prim_type_no, uintptr_t *pntr);
Object *parseAnnotation(Class *classobj, u1 **data_ptr, int *data_len);

Object *parseElementValue(Class *classobj, u1 **data_ptr, int *data_len) {
    ClassBlock *cb = CLASS_CB(classobj);
    ConstantPool *cp = &cb->constant_pool;
    char tag;

    READ_U1(tag, *data_ptr, *data_len);

    switch(tag) {
        default: {
            int cp_tag = CONSTANT_Integer;
            int prim_type_no = 0;
            int const_val_idx;

            switch(tag) {
                case 'Z':
                    prim_type_no = 1;
                    break;
                case 'B':
                    prim_type_no = 2;
                    break;
                case 'C':
                    prim_type_no = 3;
                    break;
                case 'S':
                    prim_type_no = 4;
                    break;
                case 'I':
                    prim_type_no = 5;
                    break;
                case 'F':
                    cp_tag = CONSTANT_Float;
                    prim_type_no = 6;
                    break;
                case 'J':
                    cp_tag = CONSTANT_Long;
                    prim_type_no = 7;
                    break;
                case 'D':
                    cp_tag = CONSTANT_Double;
                    prim_type_no = 8;
                    break;
            }
            READ_TYPE_INDEX(const_val_idx, cp, cp_tag, *data_ptr, *data_len);
            return createWrapperObject(prim_type_no, &CP_INFO(cp, const_val_idx));
        }

        case 's': {
            int const_str_idx;
            READ_TYPE_INDEX(const_str_idx, cp, CONSTANT_Utf8, *data_ptr, *data_len);
            return createString(CP_UTF8(cp, const_str_idx));
        }

        case 'e': {
            int type_name_idx, const_name_idx;
            Object *const_name, *enum_obj;
            Class *type_class;

            READ_TYPE_INDEX(type_name_idx, cp, CONSTANT_Utf8, *data_ptr, *data_len);
            READ_TYPE_INDEX(const_name_idx, cp, CONSTANT_Utf8, *data_ptr, *data_len);
            type_class = findClassFromSignature(CP_UTF8(cp, type_name_idx), classobj);
            const_name = createString(CP_UTF8(cp, const_name_idx));

            if(type_class == NULL || const_name == NULL)
                return NULL;

            DummyFrame dummy;
            enum_obj = *(Object**)executeStaticMethod(&dummy, enum_class, enum_valueof_mb, type_class, const_name);
            if(exceptionOccurred())
                return NULL;

            return enum_obj;
        }

        case 'c': {
            int class_info_idx;
            READ_TYPE_INDEX(class_info_idx, cp, CONSTANT_Utf8, *data_ptr, *data_len);
            return findClassFromSignature(CP_UTF8(cp, class_info_idx), classobj);
        }

        case '@':
            return parseAnnotation(classobj, data_ptr, data_len);

        case '[': {
            Object *array;
            Object **array_data;
            int i, num_values;

            READ_U2(num_values, *data_ptr, *data_len);
            if((array = allocArray(obj_array_class, num_values, sizeof(Object*))) == NULL)
                return NULL;

            array_data = (Object**)ARRAY_DATA(array);
            for(i = 0; i < num_values; i++)
                if((array_data[i] = parseElementValue(classobj, data_ptr, data_len)) == NULL)
                    return NULL;

            return array;
        }
    }
}

Object *parseAnnotation(Class *classobj, u1 **data_ptr, int *data_len) {
    ClassBlock *cb = CLASS_CB(classobj);
    ConstantPool *cp = &cb->constant_pool;
    Object *map, *anno;
    int no_value_pairs;
    Class *type_class;
    int type_idx;
    int i;

    if((map = allocObject(map_class)) == NULL)
        return NULL;

    {
        DummyFrame dummy;
        executeMethod(&dummy, map, map_init_mb);
    }
    if(exceptionOccurred())
        return NULL;

    READ_TYPE_INDEX(type_idx, cp, CONSTANT_Utf8, *data_ptr, *data_len);
    if((type_class = findClassFromSignature(CP_UTF8(cp, type_idx), classobj)) == NULL)
        return NULL;

    READ_U2(no_value_pairs, *data_ptr, *data_len);

    for(i = 0; i < no_value_pairs; i++) {
        Object *element_name, *element_value;
        int element_name_idx;

        READ_TYPE_INDEX(element_name_idx, cp, CONSTANT_Utf8, *data_ptr, *data_len);

        element_name = createString(CP_UTF8(cp, element_name_idx));
        element_value = parseElementValue(classobj, data_ptr, data_len);
        if(element_name == NULL || element_value == NULL)
            return NULL;

        DummyFrame dummy;
        executeMethod(&dummy, map, map_put_mb, element_name, element_value);
        if(exceptionOccurred())
            return NULL;
    }

    DummyFrame dummy;
    anno = *(Object**)executeStaticMethod(&dummy, anno_inv_class, anno_create_mb, type_class, map);
    if(exceptionOccurred())
        return NULL;

    return anno;
}

Object *parseAnnotations(Class *classobj, AnnotationData *annotations) {
    if(!anno_inited && !initAnnotation())
        return NULL;

    if(annotations == NULL)
        return allocArray(anno_array_class, 0, sizeof(Object*));
    else {
        u1 *data_ptr = annotations->data;
        int data_len = annotations->len;
        Object **array_data;
        Object *array;
        int no_annos;
        int i;

        READ_U2(no_annos, data_ptr, data_len);
        if((array = allocArray(anno_array_class, no_annos, sizeof(Object*))) == NULL)
            return NULL;

        array_data = (Object**)ARRAY_DATA(array);
        for(i = 0; i < no_annos; i++)
            if((array_data[i] = parseAnnotation(classobj, &data_ptr, &data_len)) == NULL)
                return NULL;

        return array;
    }
}

Object *getClassAnnotations(Class *classobj) {
    return parseAnnotations(classobj, CLASS_CB(classobj)->annotations);
}

Object *getFieldAnnotations(FieldBlock *fb) {
    return parseAnnotations(fb->classobj, fb->annotations);
}

Object *getMethodAnnotations(MethodBlock *mb) {
    return parseAnnotations(mb->classobj, mb->annotations == NULL ? NULL : mb->annotations->annotations);
}

Object *getMethodParameterAnnotations(MethodBlock *mb) {
    if(!anno_inited && !initAnnotation())
        return NULL;

    if(mb->annotations == NULL || mb->annotations->parameters == NULL)
        return allocArray(dbl_anno_array_class, 0, sizeof(Object*));
    else {
        u1 *data_ptr = mb->annotations->parameters->data;
        int data_len = mb->annotations->parameters->len;
        Object **outer_array_data;
        Object *outer_array;
        int no_params, i;

        READ_U1(no_params, data_ptr, data_len);
        if((outer_array = allocArray(dbl_anno_array_class, no_params, sizeof(Object*))) == NULL)
            return NULL;

        outer_array_data = (Object**)ARRAY_DATA(outer_array);
        for(i = 0; i < no_params; i++) {
            Object **inner_array_data;
            Object *inner_array;
            int no_annos, j;

            READ_U2(no_annos, data_ptr, data_len);
            if((inner_array = allocArray(anno_array_class, no_annos, sizeof(Object*))) == NULL)
                return NULL;

            inner_array_data = (Object**)ARRAY_DATA(inner_array);
            for(j = 0; j < no_annos; j++)
                if((inner_array_data[j] = parseAnnotation(mb->classobj, &data_ptr, &data_len)) == NULL)
                    return NULL;

            outer_array_data[i] = inner_array;
        }
        return outer_array;
    }
}

Object *getMethodDefaultValue(MethodBlock *mb) {
    if(!anno_inited && !initAnnotation())
        return NULL;

    if(mb->annotations == NULL || mb->annotations->dft_val == NULL)
        return NULL;
    else {
        u1 *data = mb->annotations->dft_val->data;
        int len = mb->annotations->dft_val->len;

        return parseElementValue(mb->classobj, &data, &len);
    }
}

int getWrapperPrimTypeIndex(Object *arg) {
    if(arg != NULL)  {
        ClassBlock *cb = CLASS_CB(arg->classobj);

        if(cb->name == SYMBOL(java_lang_Boolean))
            return 1;

        if(cb->super_name == SYMBOL(java_lang_Number)) {
            if(cb->name == SYMBOL(java_lang_Byte))
                return 2;
            if(cb->name == SYMBOL(java_lang_Character))
                return 3;
            if(cb->name == SYMBOL(java_lang_Short))
                return 4;
            if(cb->name == SYMBOL(java_lang_Integer))
                return 5;
            if(cb->name == SYMBOL(java_lang_Float))
                return 6;
            if(cb->name == SYMBOL(java_lang_Long))
                return 7;
            if(cb->name == SYMBOL(java_lang_Double))
                return 8;
        }
    }

    return 0;
}

Object *createWrapperObject(int prim_type_no, uintptr_t *pntr) {
    static const char *wrapper_suffix[] = {"Boolean", "Byte", "Character", "Short",
                                    "Integer", "Float", "Long", "Double"};
    char wrapper_name[20] = "java/lang/";
    Object *wrapper = NULL;

    if(prim_type_no > 0 /* void */) {
        Class *wrapper_type;

        strncpy(&wrapper_name[10], wrapper_suffix[prim_type_no - 1], 10);
        if((wrapper_type = findSystemClass(wrapper_name)) &&
                 (wrapper = allocObject(wrapper_type))) {
            INST_DATA(wrapper)[0] = pntr[0];
            if(prim_type_no > 6)      /* i.e. long or double */
                INST_DATA(wrapper)[1] = pntr[1];
        }
    }
    return wrapper;
}

Object *getReflectReturnObject(Class *type, uintptr_t *pntr) {
    ClassBlock *type_cb = CLASS_CB(type);

    return IS_PRIMITIVE(type_cb) ? createWrapperObject(type_cb->state - CLASS_PRIM, pntr)
                                 : (Object*)*pntr;
}

uintptr_t *widenPrimitiveValue(int src_idx, int dest_idx, uintptr_t *src, uintptr_t *dest) {

#define err 0
#define U4 1
#define U8 2
#define I2F 3
#define I2D 4
#define I2J 5
#define J2F 6
#define J2D 7
#define F2D 8

    static char conv_table[9][8] = {
        /*  bool byte char shrt int  flt long  dbl             */
           {err, err, err, err, err, err, err, err},  /* !prim */
           {U4,  err, err, err, err, err, err, err},  /* bool  */
           {err, U4,  err, U4,  U4,  I2F, I2J, I2D},  /* byte  */
           {err, err, U4,  err, U4,  I2F, I2J, I2D},  /* char  */
           {err, err, err, U4,  U4,  I2F, I2J, I2D},  /* short */
           {err, err, err, err, U4,  I2F, I2J, I2D},  /* int   */
           {err, err, err, err, err, U4,  err, F2D},  /* float */
           {err, err, err, err, err, J2F, U8,  J2D},  /* long  */
           {err, err, err, err, err, err, err, U8 }}; /* dbl   */

    static void *handlers[] = {&&illegal_arg, &&u4, &&u8, &&i2f, &&i2d, &&i2j, &&j2f, &&j2d, &&f2d};

    int handler = conv_table[src_idx][dest_idx - 1];
    goto *handlers[handler];

u4:
    *dest = *src;
    return dest + 1;
u8:
    *(u8*)dest = *(u8*)src;
    return dest + 2;
i2f:
    *(float*)dest = (float)*(int*)src;
    return dest + 1;
i2d:
    *(double*)dest = (double)*(int*)src;
    return dest + 2;
i2j:
    *(long long*)dest = (long long)*((int*)src);
    return dest + 2;
j2f:
    *(float*)dest = (float)*(long long*)src;
    return dest + 1;
j2d:
    *(double*)dest = (double)*(long long*)src;
    return dest + 2;
f2d:
    *(double*)dest = (double)*(float*)src;
    return dest + 2;

illegal_arg:
    return NULL;
}

uintptr_t *unwrapAndWidenObject(Class *type, Object *arg, uintptr_t *pntr) {
    ClassBlock *type_cb = CLASS_CB(type);

    if(IS_PRIMITIVE(type_cb)) {
        int formal_idx = getPrimTypeIndex(type_cb);
        int actual_idx = getWrapperPrimTypeIndex(arg);
        uintptr_t *data = INST_DATA(arg);

        return widenPrimitiveValue(actual_idx, formal_idx, data, pntr);
    }

    if((arg == NULL) || isInstanceOf(type, arg->classobj)) {
        *pntr++ = (uintptr_t) arg;
        return pntr;
    }

    return NULL;
}

Object*
invoke(DummyFrame* dummy, Object* ob, MethodBlock* mb, Object* arg_array, Object* param_types, int check_access)
{
    Object **args = (Object**)ARRAY_DATA(arg_array);
    // Class **types = (Object**)ARRAY_DATA(param_types);
    Class **types = (Class**)ARRAY_DATA(param_types);
    int args_len = arg_array ? ARRAY_LEN(arg_array) : 0;
    int types_len = ARRAY_LEN(param_types);

    int i;

    Object *excep;

    if(check_access) {
        Class *caller = getCallerCallerClass();
        if(!checkClassAccess(mb->classobj, caller) || !checkMethodAccess(mb, caller)) {
            signalException(java_lang_IllegalAccessException, "method is not accessible");
            return NULL;
        }
    }

    if(args_len != types_len) {
        signalException(java_lang_IllegalArgumentException, "wrong number of args");
        return NULL;
    }

    void* ret;

    Thread* this_thread = threadSelf();
    SpmtThread* this_st = this_thread->get_current_spmt_thread();

    std::vector<uintptr_t> arguments(mb->args_count);
    uintptr_t* arg = &arguments[0];


    // 准备好参数
    // this是方法的第一个参数（静态方法的参数中没有this）
    if (ob)
        *arg++ = (uintptr_t)ob;

    // 其他参数
    for(i = 0; i < args_len; i++)
        if((arg = unwrapAndWidenObject(*types++, *args++, arg)) == NULL) {
            assert(false);      // todo
            signalException(java_lang_IllegalArgumentException, "arg type mismatch");
            return NULL;
        }

    assert(ob);
    ret = this_st->get_current_mode()->do_execute_method(ob, mb, arguments, dummy);


    if((excep = exceptionOccurred())) {
        Object *ite_excep;
        MethodBlock *init;
        Class *ite_class;

        clearException();
        ite_class = findSystemClass("java/lang/reflect/InvocationTargetException");

        if(!exceptionOccurred() && (ite_excep = allocObject(ite_class)) &&
                        (init = lookupMethod(ite_class, SYMBOL(object_init), SYMBOL(_java_lang_Throwable__V)))) {
            DummyFrame dummy;
            executeMethod(&dummy, ite_excep, init, excep);
            setException(ite_excep);
        }
        return NULL;
    }

    return (Object*)ret;
}

/* Reflection access from JNI */

Object *createReflectConstructorObject(MethodBlock *mb) {
    if(!inited && !initReflection())
        return NULL;

    return createConstructorObject(mb);
}

Object *createReflectMethodObject(MethodBlock *mb) {
    if(!inited && !initReflection())
        return NULL;

    return createMethodObject(mb);
}

Object *createReflectFieldObject(FieldBlock *fb) {
    if(!inited && !initReflection())
        return NULL;

    return createFieldObject(fb);
}

MethodBlock *mbFromReflectObject(Object *reflect_ob) {
    int slot = reflect_ob->classobj == cons_reflect_class ? cons_slot_offset : method_slot_offset;
    int classobj = reflect_ob->classobj == cons_reflect_class ? cons_class_offset : method_class_offset;
    Class *decl_class = (Class*)INST_DATA(reflect_ob)[classobj];

    return &(CLASS_CB(decl_class)->methods[INST_DATA(reflect_ob)[slot]]);
}

FieldBlock *fbFromReflectObject(Object *reflect_ob) {
    Class *decl_class = (Class*)INST_DATA(reflect_ob)[field_class_offset];
    return &(CLASS_CB(decl_class)->fields[INST_DATA(reflect_ob)[field_slot_offset]]);
}

/* Needed for stack walking */

Class *getReflectMethodClass() {
    return method_reflect_class;
}
