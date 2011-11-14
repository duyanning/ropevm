#include "std.h"
#include "rope.h"
#include "sig.h"
#include "thread.h"
#include "lock.h"
#include "hash.h"
#include "zip.h"
#include "class.h"
#include "interp.h"
#include "symbol.h"
#include "excep.h"
#include "DebugScaffold.h"
#include "SpmtThread.h"
#include "RopeVM.h"

#define PREPARE(ptr) ptr
#define SCAVENGE(ptr) FALSE
#define FOUND(ptr) ptr

static int verbose;
static char *bootpath;
static char *classpath;
static int max_cp_element_len;

/* Structures holding the boot loader classpath */
typedef struct bcp_entry {
    char *path;
    ZipFile *zip;
} BCPEntry;

static BCPEntry *bootclasspath;
static int bcp_entries;

/* Cached offsets of fields in java.lang.ref.Reference objects */
int ref_referent_offset = -1;
int ref_queue_offset;

/* Cached offset of vmdata field in java.lang.ClassLoader objects */
int ldr_vmdata_offset = -1;


static Class *loader_data_class = NULL;
static MethodBlock *ldr_new_unloader;
static int ldr_data_tbl_offset;

/* Instance of java.lang.Class for java.lang.Class */
Class *java_lang_Class = NULL;

/* Method table index of ClassLoader.loadClass - used when
   requesting a Java-level class loader to load a class.
   Cached on first use. */
static int loadClass_mtbl_idx = -1;

/* Method table index of finalizer method and ClassLoader.enqueue.
   Used by finalizer and reference handler threads */
int finalize_mtbl_idx;
int enqueue_mtbl_idx;

/* hash table containing classes loaded by the boot loader and
   internally created arrays */

#define INITSZE 1<<8
static HashTable loaded_classes;

/* Array large enough to hold all primitive classes -
 * access protected by loaded_classes hash table lock */
#define MAX_PRIM_CLASSES 9
static Class *prim_classes[MAX_PRIM_CLASSES];

/* Bytecode for stub abstract method.  If it is invoked
   we'll get an abstract method error. */
static char abstract_method[] = {(char)OPC_ABSTRACT_METHOD_ERROR};

static Class *addClassToHash(Class *classobj, Object *class_loader) {
    HashTable *table;
    Class *entry;

#define HASH(ptr) utf8Hash(CLASS_CB((Class *)ptr)->name)
#define COMPARE(ptr1, ptr2, hash1, hash2) (hash1 == hash2) && \
            CLASS_CB((Class *)ptr1)->name == CLASS_CB((Class *)ptr2)->name

    if(class_loader == NULL)
        table = &loaded_classes;
    else {
        Object *vmdata = (Object*)INST_DATA(class_loader)[ldr_vmdata_offset];

        if(vmdata == NULL) {
            objectLock(class_loader);
            vmdata = (Object*)INST_DATA(class_loader)[ldr_vmdata_offset];

            if(vmdata == NULL) {
                if((vmdata = allocObject(loader_data_class)) == NULL) {
                    objectUnlock(class_loader);
                    return NULL;
                }

                table = (HashTable*)sysMalloc(sizeof(HashTable));
                initHashTable((*table), INITSZE, TRUE);

                INST_DATA(vmdata)[ldr_data_tbl_offset] = (uintptr_t)table;
                INST_DATA(class_loader)[ldr_vmdata_offset] = (uintptr_t)vmdata;

                objectUnlock(class_loader);
            }
        }

        table = (HashTable*)INST_DATA(vmdata)[ldr_data_tbl_offset];
    }

    /* Add if absent, no scavenge, locked */
    findHashEntry((*table), classobj, entry, TRUE, FALSE, TRUE, Class*);

    return entry;
}

static void prepareClass(Class *classobj)
{
    ClassBlock *cb = CLASS_CB(classobj);

    if (cb->name == SYMBOL(java_lang_Class)) {
        java_lang_Class = classobj->classobj = classobj;
        cb->flags |= CLASS_CLASS;
    }
    else {
        if(java_lang_Class == NULL)
            findSystemClass0(SYMBOL(java_lang_Class));
        classobj->classobj = java_lang_Class;
    }
    // dynsearch
    // std::cout << "prepareClass: " << classobj->name() << "UUU" << std::endl;
    // //if (strcmp(classobj->name(), "Hello") == 0) {
    // if (std::string(classobj->name()) == "Hello") {
    // //if (*(classobj->name()) == 'H') {
    //     std::cout << "fuck" << std::endl;
    //     int x = 0;
    //     x++;
    // }
    // SpmtThread* current_core = g_current_core();
    // current_core->after_alloc_object(classobj);
}

void process_policies(unsigned char*& ptr, ConstantPool* constant_pool, int& self_policy, int& others_policy)
{
    int len = 0;
    u2 elem_count;
    READ_U2(elem_count, ptr, len);
    assert(elem_count >= 0 and elem_count <= 2);

    for (; elem_count != 0; elem_count--) {

        u2 elem_name_idx;
        READ_TYPE_INDEX(elem_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
        char* elem_name;
        elem_name = CP_UTF8(constant_pool, elem_name_idx);
        assert(elem_name == SYMBOL(self) or elem_name == SYMBOL(others));

        u1 value_category;
        READ_U1(value_category, ptr, len);
        assert(value_category == 'e');

        u2 enum_name_idx;
        READ_U2(enum_name_idx, ptr, len);
        char* enum_name;
        enum_name = CP_UTF8(constant_pool, enum_name_idx);
        assert(enum_name == SYMBOL(GroupingPolicy));

        u2 enum_value_name_idx;
        READ_U2(enum_value_name_idx, ptr, len);
        char* enum_value_name;
        enum_value_name = CP_UTF8(constant_pool, enum_value_name_idx);

        int enum_value = 0;
        if (enum_value_name == SYMBOL(UNSPECIFIED)) {
            enum_value = GP_UNSPECIFIED;
        }
        else if (enum_value_name == SYMBOL(NEW_GROUP)) {
            enum_value = GP_NEW_GROUP;
        }
        else if (enum_value_name == SYMBOL(CURRENT_GROUP)) {
            enum_value = GP_CURRENT_GROUP;
        }
        else if (enum_value_name == SYMBOL(NO_GROUP)) {
            enum_value = GP_NO_GROUP;
        }
        else {
            assert(false);
        }

        if (elem_name == SYMBOL(self)) {
            self_policy = enum_value;
        }
        else if (elem_name == SYMBOL(others)) {
            others_policy = enum_value;
        }
        else {
            assert(false);
        }

    }
}

void process_runtime_visible_annotations(void* data,
                                         ClassBlock* classblock, ConstantPool* constant_pool)
{
    unsigned char* ptr = (unsigned char*)data;
    int len = 0;                // no use
    u2 anno_count;
    READ_U2(anno_count, ptr, len);
    for (; anno_count != 0; anno_count--) {
        u2 anno_name_idx;
        char* anno_name;
        READ_TYPE_INDEX(anno_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
        anno_name = CP_UTF8(constant_pool, anno_name_idx);

        if (anno_name == SYMBOL(GroupingPolicies)) {
            process_policies(ptr, constant_pool,
                             classblock->grouping_policy_self,
                             classblock->grouping_policy_others);
        }
        else if (anno_name == SYMBOL(ClassGroupingPolicies)) {
            process_policies(ptr, constant_pool,
                             classblock->class_grouping_policy_self,
                             classblock->class_grouping_policy_others);
        }
        else {
            std::cout << "ROPEVM: unknown annotation on class " << classblock->name << std::endl;
        }
    }
}

Class *defineClass(const char* classname, char *data, int offset, int len, Object *class_loader)
{
    unsigned char *ptr = (unsigned char *)data+offset;
    int cp_count, intf_count, i;
    u2 major_version, minor_version, this_idx, super_idx;
    u2 attr_count;
    u4 magic;

    ConstantPool *constant_pool;
    ClassBlock *classblock;
    Class *classobj, *found;
    Class **interfaces;

    READ_U4(magic, ptr, len);

    if(magic != 0xcafebabe) {
       signalException(java_lang_ClassFormatError, "bad magic");
       return NULL;
    }

    READ_U2(minor_version, ptr, len);
    READ_U2(major_version, ptr, len);

    if((classobj = allocClass()) == NULL)
        return NULL;

    classblock = CLASS_CB(classobj);
    READ_U2(cp_count, ptr, len);

    constant_pool = &classblock->constant_pool;
    constant_pool->type = (u1 *)sysMalloc(cp_count);
    constant_pool->info = (ConstantPoolEntry *)
                       sysMalloc(cp_count*sizeof(ConstantPoolEntry));

    for(i = 1; i < cp_count; i++) {
        u1 tag;

        READ_U1(tag, ptr, len);
        CP_TYPE(constant_pool,i) = tag;

        switch(tag) {
           case CONSTANT_Class:
           case CONSTANT_String:
               READ_INDEX(CP_INFO(constant_pool,i), ptr, len);
               break;

           case CONSTANT_Fieldref:
           case CONSTANT_Methodref:
           case CONSTANT_NameAndType:
           case CONSTANT_InterfaceMethodref:
           {
               u2 idx1, idx2;

               READ_INDEX(idx1, ptr, len);
               READ_INDEX(idx2, ptr, len);
               CP_INFO(constant_pool,i) = (idx2<<16)+idx1;
               break;
           }

           case CONSTANT_Integer:
               READ_U4(CP_INFO(constant_pool,i), ptr, len);
               break;

           case CONSTANT_Float:
           {
               u4 val;

               READ_U4(val, ptr, len);
               CP_INFO(constant_pool,i) = FLOAT_CONST(val);
               break;
           }

           case CONSTANT_Long:
               READ_U8(*(u8 *)&(CP_INFO(constant_pool,i)), ptr, len);
               CP_TYPE(constant_pool,++i) = 0;
               break;

           case CONSTANT_Double:
               READ_DBL(*(u8 *)&(CP_INFO(constant_pool,i)), ptr, len);
               CP_TYPE(constant_pool,++i) = 0;
               break;

           case CONSTANT_Utf8:
           {
               int length;
               char *buff, *utf8;

               READ_U2(length, ptr, len);
               buff = (char*)sysMalloc(length+1);

               memcpy(buff, ptr, length);
               buff[length] = '\0';
               ptr += length;

               CP_INFO(constant_pool,i) = (uintptr_t) (utf8 = newUtf8(buff));

               if(utf8 != buff)
                   sysFree(buff);

               break;
           }

           default:
               signalException(java_lang_ClassFormatError, "bad constant pool tag");
               return NULL;
        }
    }

    /* Set count after constant pool has been initialised -- it is now
       safe to be scanned by GC */
    classblock->constant_pool_count = cp_count;

    READ_U2(classblock->access_flags, ptr, len);

    READ_TYPE_INDEX(this_idx, constant_pool, CONSTANT_Class, ptr, len);
    classblock->name = CP_UTF8(constant_pool, CP_CLASS(constant_pool, this_idx));

    if(classname && strcmp(classblock->name, classname) != 0) {
        signalException(java_lang_NoClassDefFoundError, "class file has wrong name");
        return NULL;
    }

    prepareClass(classobj);

    if(classblock->name == SYMBOL(java_lang_Object)) {
        READ_U2(super_idx, ptr, len);
        if(super_idx) {
           signalException(java_lang_ClassFormatError, "Object has super");
           return NULL;
        }
        classblock->super_name = NULL;
    } else {
        READ_TYPE_INDEX(super_idx, constant_pool, CONSTANT_Class, ptr, len);
        classblock->super_name = CP_UTF8(constant_pool, CP_CLASS(constant_pool, super_idx));
    }

    classblock->class_loader = class_loader;

    READ_U2(intf_count = classblock->interfaces_count, ptr, len);
    interfaces = classblock->interfaces =
                      (Class **)sysMalloc(intf_count * sizeof(Class *));

    memset(interfaces, 0, intf_count * sizeof(Class *));
    for(i = 0; i < intf_count; i++) {
       u2 index;
       READ_TYPE_INDEX(index, constant_pool, CONSTANT_Class, ptr, len);
       interfaces[i] = resolveClass(classobj, index, FALSE);
       if(exceptionOccurred())
           return NULL;
    }

    READ_U2(classblock->fields_count, ptr, len);
    classblock->fields = (FieldBlock *)
                     sysMalloc(classblock->fields_count * sizeof(FieldBlock));

    for(i = 0; i < classblock->fields_count; i++) {
        u2 name_idx, type_idx;

        READ_U2(classblock->fields[i].access_flags, ptr, len);
        READ_TYPE_INDEX(name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
        READ_TYPE_INDEX(type_idx, constant_pool, CONSTANT_Utf8, ptr, len);
        classblock->fields[i].name = CP_UTF8(constant_pool, name_idx);
        classblock->fields[i].type = CP_UTF8(constant_pool, type_idx);
        classblock->fields[i].annotations = NULL;
        classblock->fields[i].signature = NULL;
        classblock->fields[i].constant = 0;

        READ_U2(attr_count, ptr, len);
        for(; attr_count != 0; attr_count--) {
            u2 attr_name_idx;
            char *attr_name;
            u4 attr_length;

            READ_TYPE_INDEX(attr_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
            attr_name = CP_UTF8(constant_pool, attr_name_idx);
            READ_U4(attr_length, ptr, len);

            if(attr_name == SYMBOL(ConstantValue)) {
                READ_INDEX(classblock->fields[i].constant, ptr, len);
            } else
                if(attr_name == SYMBOL(Signature)) {
                    u2 signature_idx;
                    READ_TYPE_INDEX(signature_idx, constant_pool, CONSTANT_Utf8, ptr, len);
                    classblock->fields[i].signature = CP_UTF8(constant_pool, signature_idx);
                } else
                    if(attr_name == SYMBOL(RuntimeVisibleAnnotations)) {
                        classblock->fields[i].annotations = (AnnotationData*)sysMalloc(sizeof(AnnotationData));
                        classblock->fields[i].annotations->len = attr_length;
                        classblock->fields[i].annotations->data = (u1*)sysMalloc(attr_length);
                        memcpy(classblock->fields[i].annotations->data, ptr, attr_length);
                        ptr += attr_length;
                    } else
                        ptr += attr_length;
        }
    }

    READ_U2(classblock->methods_count, ptr, len);

    classblock->methods = (MethodBlock *)
            sysMalloc(classblock->methods_count * sizeof(MethodBlock));

    memset(classblock->methods, 0, classblock->methods_count * sizeof(MethodBlock));

    for(i = 0; i < classblock->methods_count; i++) {
        MethodBlock *method = &classblock->methods[i];
        MethodAnnotationData annos;
        u2 name_idx, type_idx;

        memset(&annos, 0, sizeof(MethodAnnotationData));

        READ_U2(method->access_flags, ptr, len);
        READ_TYPE_INDEX(name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
        READ_TYPE_INDEX(type_idx, constant_pool, CONSTANT_Utf8, ptr, len);

        method->name = CP_UTF8(constant_pool, name_idx);
        method->type = CP_UTF8(constant_pool, type_idx);

        READ_U2(attr_count, ptr, len);
        for(; attr_count != 0; attr_count--) {
            u2 attr_name_idx;
            char *attr_name;
            u4 attr_length;

            READ_TYPE_INDEX(attr_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
            READ_U4(attr_length, ptr, len);
            attr_name = CP_UTF8(constant_pool, attr_name_idx);

            if(attr_name == SYMBOL(Code)) {
                u4 code_length;
                u2 code_attr_cnt;
                int j;

                READ_U2(method->max_stack, ptr, len);
                READ_U2(method->max_locals, ptr, len);

                READ_U4(code_length, ptr, len);
                method->code = (char *)sysMalloc(code_length);
                memcpy(method->code, ptr, code_length);
                ptr += code_length;

                method->code_size = code_length;

                READ_U2(method->exception_table_size, ptr, len);
                method->exception_table = (ExceptionTableEntry *)
                sysMalloc(method->exception_table_size*sizeof(ExceptionTableEntry));

                for(j = 0; j < method->exception_table_size; j++) {
                    ExceptionTableEntry *entry = &method->exception_table[j];

                    READ_U2(entry->start_pc, ptr, len);
                    READ_U2(entry->end_pc, ptr, len);
                    READ_U2(entry->handler_pc, ptr, len);
                    READ_U2(entry->catch_type, ptr, len);
                }

                READ_U2(code_attr_cnt, ptr, len);
                for(; code_attr_cnt != 0; code_attr_cnt--) {
                    u2 attr_name_idx;
                    u4 attr_length;

                    READ_TYPE_INDEX(attr_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
                    attr_name = CP_UTF8(constant_pool, attr_name_idx);
                    READ_U4(attr_length, ptr, len);

                    if(attr_name == SYMBOL(LineNumberTable)) {
                        READ_U2(method->line_no_table_size, ptr, len);
                        method->line_no_table = (LineNoTableEntry *)
                            sysMalloc(method->line_no_table_size*sizeof(LineNoTableEntry));

                        for(j = 0; j < method->line_no_table_size; j++) {
                            LineNoTableEntry *entry = &method->line_no_table[j];

                            READ_U2(entry->start_pc, ptr, len);
                            READ_U2(entry->line_no, ptr, len);
                        }
                    } else
                        ptr += attr_length;
                }
            } else
                if(attr_name == SYMBOL(Exceptions)) {
                    int j;

                    READ_U2(method->throw_table_size, ptr, len);
                    method->throw_table = (u2 *)sysMalloc(method->throw_table_size*sizeof(u2));
                    for(j = 0; j < method->throw_table_size; j++) {
                        READ_U2(method->throw_table[j], ptr, len);
                    }
                } else
                    if(attr_name == SYMBOL(Signature)) {
                        u2 signature_idx;
                        READ_TYPE_INDEX(signature_idx, constant_pool, CONSTANT_Utf8, ptr, len);
                        method->signature = CP_UTF8(constant_pool, signature_idx);
                    } else
                        if(attr_name == SYMBOL(RuntimeVisibleAnnotations)) {
                            annos.annotations = (AnnotationData*)sysMalloc(sizeof(AnnotationData));
                            annos.annotations->len = attr_length;
                            annos.annotations->data = (u1*)sysMalloc(attr_length);
                            memcpy(annos.annotations->data, ptr, attr_length);
                            ptr += attr_length;
                        } else
                            if(attr_name == SYMBOL(RuntimeVisibleParameterAnnotations)) {
                                annos.parameters = (AnnotationData*)sysMalloc(sizeof(AnnotationData));
                                annos.parameters->len = attr_length;
                                annos.parameters->data = (u1*)sysMalloc(attr_length);
                                memcpy(annos.parameters->data, ptr, attr_length);
                                ptr += attr_length;
                            } else
                                if(attr_name == SYMBOL(AnnotationDefault)) {
                                    annos.dft_val = (AnnotationData*)sysMalloc(sizeof(AnnotationData));
                                    annos.dft_val->len = attr_length;
                                    annos.dft_val->data = (u1*)sysMalloc(attr_length);
                                    memcpy(annos.dft_val->data, ptr, attr_length);
                                    ptr += attr_length;
                                } else
                                    ptr += attr_length;
        }
        if(annos.annotations != NULL || annos.parameters != NULL
                                     || annos.dft_val != NULL) {
            method->annotations = (MethodAnnotationData*)sysMalloc(sizeof(MethodAnnotationData));
            memcpy(method->annotations, &annos, sizeof(MethodAnnotationData));
        }
    }

    READ_U2(attr_count, ptr, len);
    for(; attr_count != 0; attr_count--) {
        u2 attr_name_idx;
        char *attr_name;
        u4 attr_length;

        READ_TYPE_INDEX(attr_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
        attr_name = CP_UTF8(constant_pool, attr_name_idx);
        READ_U4(attr_length, ptr, len);

        if(attr_name == SYMBOL(SourceFile)) {
            u2 file_name_idx;
            READ_TYPE_INDEX(file_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
            classblock->source_file_name = CP_UTF8(constant_pool, file_name_idx);
        } else
            if(attr_name == SYMBOL(InnerClasses)) {
                int j, size;
                READ_U2(size, ptr, len);
                {
                    u2 inner_classes[size];
                    for(j = 0; j < size; j++) {
                        int inner, outer;
                        READ_TYPE_INDEX(inner, constant_pool, CONSTANT_Class, ptr, len);
                        READ_TYPE_INDEX(outer, constant_pool, CONSTANT_Class, ptr, len);

                        if(inner == this_idx) {
                            int inner_name_idx;

                            /* A member class doesn't have an EnclosingMethod attribute, so set
                               the enclosing class to be the same as the declaring class */
                            if(outer)
                                classblock->declaring_class = classblock->enclosing_class = outer;

                            READ_TYPE_INDEX(inner_name_idx, constant_pool, CONSTANT_Utf8, ptr, len);
                            if(inner_name_idx == 0)
                                classblock->flags |= ANONYMOUS;

                            READ_U2(classblock->inner_access_flags, ptr, len);
                        } else {
                            ptr += 4;
                            if(outer == this_idx)
                                inner_classes[classblock->inner_class_count++] = inner;
                        }
                    }

                    if(classblock->inner_class_count) {
                        classblock->inner_classes = (u2*)sysMalloc(classblock->inner_class_count*sizeof(u2));
                        memcpy(classblock->inner_classes, &inner_classes[0],
                                                          classblock->inner_class_count*sizeof(u2));
                    }
                }
            } else
                if(attr_name == SYMBOL(EnclosingMethod)) {
                    READ_TYPE_INDEX(classblock->enclosing_class, constant_pool, CONSTANT_Class, ptr, len);
                    READ_TYPE_INDEX(classblock->enclosing_method, constant_pool, CONSTANT_NameAndType, ptr, len);
                } else
                    if(attr_name == SYMBOL(Signature)) {
                        u2 signature_idx;
                        READ_TYPE_INDEX(signature_idx, constant_pool, CONSTANT_Utf8, ptr, len);
                        classblock->signature = CP_UTF8(constant_pool, signature_idx);
                    } else
                        if(attr_name == SYMBOL(Synthetic))
                            classblock->access_flags |= ACC_SYNTHETIC;
                        else
                            if(attr_name == SYMBOL(RuntimeVisibleAnnotations)) {
                                classblock->annotations = (AnnotationData*)sysMalloc(sizeof(AnnotationData));
                                classblock->annotations->len = attr_length;
                                classblock->annotations->data = (u1*)sysMalloc(attr_length);
                                memcpy(classblock->annotations->data, ptr, attr_length);
                                ptr += attr_length;

                                process_runtime_visible_annotations(classblock->annotations->data, classblock, constant_pool);

                            } else
                                ptr += attr_length;
    }

    classblock->super = super_idx ? resolveClass(classobj, super_idx, FALSE) : NULL;

    if(exceptionOccurred())
       return NULL;

    classblock->state = CLASS_LOADED;

    if((found = addClassToHash(classobj, class_loader)) != classobj) {
        classblock->flags = CLASS_CLASH;
        if(class_loader != NULL) {
            signalException(java_lang_LinkageError, "duplicate class definition");
            return NULL;
        }
        return found;
    }

    // dynsearch
    if (strcmp(classobj->name(), "Hello") == 0) {
        int x = 0;
        x++;
    }
    SpmtThread* current_spmt_thread = g_get_current_spmt_thread();
    current_spmt_thread->after_alloc_object(classobj);

    return classobj;
}

Class *createArrayClass(const char *classname, Object *class_loader) {
    ClassBlock *elem_cb, *classblock;
    Class *classobj, *found = NULL;
    int len = strlen(classname);

    if((classobj = allocClass()) == NULL)
        return NULL;

    classblock = CLASS_CB(classobj);

    classblock->name = copyUtf8(classname);
    classblock->super_name = SYMBOL(java_lang_Object);
    classblock->super = findSystemClass0(SYMBOL(java_lang_Object));
    classblock->method_table = CLASS_CB(classblock->super)->method_table;

    classblock->interfaces_count = 2;
    classblock->interfaces = (Class**)sysMalloc(2*sizeof(Class*));
    classblock->interfaces[0] = findSystemClass0(SYMBOL(java_lang_Cloneable));
    classblock->interfaces[1] = findSystemClass0(SYMBOL(java_io_Serializable));

    classblock->state = CLASS_ARRAY;

    /* Find the array element class and the dimension --
       this is used to speed up type checking (instanceof) */

    if(classname[1] == '[') {
        Class *comp_class = findArrayClassFromClassLoader(classname + 1, class_loader);

        if(comp_class == NULL)
            goto error;

        classblock->element_class = CLASS_CB(comp_class)->element_class;
        classblock->dim = CLASS_CB(comp_class)->dim + 1;
    } else {
        if(classname[1] == 'L') {
            char element_name[len-2];

            strcpy(element_name, classname + 2);
            element_name[len-3] = '\0';
            classblock->element_class = findClassFromClassLoader(element_name, class_loader);
        } else
            classblock->element_class = findPrimitiveClass(classname[1]);

        if(classblock->element_class == NULL)
            goto error;

         classblock->dim = 1;
    }

    elem_cb = CLASS_CB(classblock->element_class);

    /* The array's classloader is the loader of the element class */
    classblock->class_loader = elem_cb->class_loader;

    /* The array's visibility (i.e. public, etc.) is that of the element */
    classblock->access_flags = (elem_cb->access_flags & ~ACC_INTERFACE) |
                               ACC_FINAL | ACC_ABSTRACT;

    prepareClass(classobj);

    if((found = addClassToHash(classobj, classblock->class_loader)) == classobj) {
        if(verbose)
            jam_printf("[Created array class %s]\n", classname);
        // dynsearch
        SpmtThread* current_spmt_thread = g_get_current_spmt_thread();
        current_spmt_thread->after_alloc_object(classobj);

        return classobj;
    }

error:
    classblock->flags = CLASS_CLASH;
    return found;
}

Class *
createPrimClass(const char *classname, int index) {
    Class *classobj;
    ClassBlock *classblock;

    if((classobj = allocClass()) == NULL)
        return NULL;

    classblock = CLASS_CB(classobj);
    classblock->name = classname;
    classblock->state = CLASS_PRIM + index;
    classblock->access_flags = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;

    prepareClass(classobj);
    // dynsearch
    SpmtThread* current_spmt_thread = g_get_current_spmt_thread();
    current_spmt_thread->after_alloc_object(classobj);


    lockHashTable(loaded_classes);
    if(prim_classes[index] == NULL)
        prim_classes[index] = classobj;
    unlockHashTable(loaded_classes);

    if(verbose)
        jam_printf("[Created primitive class %s]\n", classname);

    return prim_classes[index];
}

#define MRNDA_CACHE_SZE 10

#define resizeMTable(method_table, method_table_size, miranda, count)  \
{                                                                      \
    method_table = (MethodBlock**)sysRealloc(method_table,             \
                  (method_table_size + count) * sizeof(MethodBlock*)); \
                                                                       \
    memcpy(&method_table[method_table_size], miranda,                  \
                               count * sizeof(MethodBlock*));          \
    method_table_size += count;                                        \
}

#define fillinMTable(method_table, methods, methods_count)             \
{                                                                      \
    int i;                                                             \
    for(i = 0; i < methods_count; i++, methods++) {                    \
        if((methods->access_flags & (ACC_STATIC | ACC_PRIVATE)) ||     \
               (methods->name[0] == '<'))                              \
            continue;                                                  \
        method_table[methods->method_table_index] = methods;           \
    }                                                                  \
}

void linkClass(Class *classobj) {
   static MethodBlock *obj_fnlzr_mthd = NULL;

   ClassBlock *cb = CLASS_CB(classobj);
   Class *super = (cb->access_flags & ACC_INTERFACE) ? NULL : cb->super;

   RefsOffsetsEntry *spr_rfs_offsts_tbl = NULL;
   ITableEntry *spr_imthd_tbl = NULL;
   MethodBlock **method_table = NULL;
   MethodBlock **spr_mthd_tbl = NULL;
   MethodBlock *finalizer;
   MethodBlock *mb;
   FieldBlock *fb;

   int spr_rfs_offsts_sze = 0;
   int new_methods_count = 0;
   int spr_imthd_tbl_sze = 0;
   int itbl_offset_count = 0;
   int spr_mthd_tbl_sze = 0;
   int method_table_size;
   int new_itable_count;
   int spr_obj_sze = 0;
   int refs_end_offset;
   int itbl_idx, i, j;
   int spr_flags = 0;
   int field_offset;

   if(cb->state >= CLASS_LINKED)
       return;

   objectLock((Object *)classobj);

   if(cb->state >= CLASS_LINKED)
       goto unlock;

   if(verbose)
       jam_printf("[Linking class %s]\n", cb->name);

   if(super) {
      ClassBlock *super_cb = CLASS_CB(super);
      if(super_cb->state < CLASS_LINKED)
          linkClass(super);

      spr_flags = super_cb->flags;
      spr_obj_sze = super_cb->object_size;
      spr_mthd_tbl = super_cb->method_table;
      spr_imthd_tbl = super_cb->imethod_table;
      spr_mthd_tbl_sze = super_cb->method_table_size;
      spr_imthd_tbl_sze = super_cb->imethod_table_size;
      spr_rfs_offsts_sze = super_cb->refs_offsets_size;
      spr_rfs_offsts_tbl = super_cb->refs_offsets_table;
   }

   /* prepare fields */

   field_offset = spr_obj_sze;

   for(fb = cb->fields, i = 0; i < cb->fields_count; i++,fb++) {
      if(fb->access_flags & ACC_STATIC) {
         /* init to default value */
         if((*fb->type == 'J') || (*fb->type == 'D'))
            *(long long *)&fb->static_value = 0;
         else
            fb->static_value = 0;
      } else {
         /* calc field offset */
         if((*fb->type == 'L') || (*fb->type == '['))
            fb->offset = field_offset++;
      }
      fb->classobj = classobj;
   }

   refs_end_offset = field_offset;

   for(fb = cb->fields, i = 0; i < cb->fields_count; i++,fb++)
      if(!(fb->access_flags & ACC_STATIC) &&
                 (*fb->type != 'L') && (*fb->type != '[')) {
         /* calc field offset */
         fb->offset = field_offset;
         if((*fb->type == 'J') || (*fb->type == 'D'))
             field_offset += 2;
         else
             field_offset += 1;
      }

   cb->object_size = field_offset;

   /* prepare methods */

   for(mb = cb->methods, i = 0; i < cb->methods_count; i++,mb++) {

       /* calculate argument count from signature */

       int count = 0;
       char *sig = mb->type;
       SCAN_SIG(sig, count+=2, count++);

       if(mb->access_flags & ACC_STATIC)
           mb->args_count = count;
       else
           mb->args_count = count + 1;

       mb->classobj = classobj;

       /* Set abstract method to stub */
       if(mb->access_flags & ACC_ABSTRACT) {
           mb->code_size = sizeof(abstract_method);
           mb->code = abstract_method;
       }

       if(mb->is_native()) {

           /* set up native invoker to wrapper to resolve function
              on first invocation */

           mb->native_invoker = (void *) resolveNativeWrapper;

           /* native methods have no code attribute so these aren't filled
              in at load time - as these values are used when creating frame
              set to appropriate values */

           mb->max_locals = mb->args_count;
           mb->max_stack = 0;
       }

#ifdef DIRECT
       else  {
           /* Set the bottom bit of the pointer to indicate the
              method is unprepared */
           mb->code = ((char*)mb->code) + 1;
       }
#endif

       /* Static, private or init methods aren't dynamically invoked, so
         don't stick them in the table to save space */

       if((mb->access_flags & (ACC_STATIC | ACC_PRIVATE)) || (mb->name[0] == '<'))
           continue;

       /* if it's overriding an inherited method, replace in method table */

       for(j = 0; j < spr_mthd_tbl_sze; j++)
           if(mb->name == spr_mthd_tbl[j]->name &&
                        mb->type == spr_mthd_tbl[j]->type &&
                        checkMethodAccess(spr_mthd_tbl[j], classobj)) {
               mb->method_table_index = spr_mthd_tbl[j]->method_table_index;
               break;
           }

       if(j == spr_mthd_tbl_sze)
           mb->method_table_index = spr_mthd_tbl_sze + new_methods_count++;
   }

   /* construct method table */

   method_table_size = spr_mthd_tbl_sze + new_methods_count;

   if(!(cb->access_flags & ACC_INTERFACE)) {
       method_table = (MethodBlock**)sysMalloc(method_table_size * sizeof(MethodBlock*));

       /* Copy parents method table to the start */
       memcpy(method_table, spr_mthd_tbl, spr_mthd_tbl_sze * sizeof(MethodBlock*));

       /* fill in the additional methods -- we use a
          temporary because fillinMtable alters mb */
       mb = cb->methods;
       fillinMTable(method_table, mb, cb->methods_count);
   }

   /* setup interface method table */

   /* number of interfaces implemented by this class is those implemented by
    * parent, plus number of interfaces directly implemented by this class,
    * and the total number of their superinterfaces */

   new_itable_count = cb->interfaces_count;
   for(i = 0; i < cb->interfaces_count; i++)
       new_itable_count += CLASS_CB(cb->interfaces[i])->imethod_table_size;

   cb->imethod_table_size = spr_imthd_tbl_sze + new_itable_count;
   cb->imethod_table = (ITableEntry*)sysMalloc(cb->imethod_table_size * sizeof(ITableEntry));

   /* copy parent's interface table - the offsets into the method table won't change */

   memcpy(cb->imethod_table, spr_imthd_tbl, spr_imthd_tbl_sze * sizeof(ITableEntry));

   /* now run through the extra interfaces implemented by this class,
    * fill in the interface part, and calculate the number of offsets
    * needed (i.e. the number of methods defined in the interfaces) */

   itbl_idx = spr_imthd_tbl_sze;
   for(i = 0; i < cb->interfaces_count; i++) {
       Class *intf = cb->interfaces[i];
       ClassBlock *intf_cb = CLASS_CB(intf);

       cb->imethod_table[itbl_idx++].interface = intf;
       itbl_offset_count += intf_cb->method_table_size;

       for(j = 0; j < intf_cb->imethod_table_size; j++) {
           Class *spr_intf = intf_cb->imethod_table[j].interface;

           cb->imethod_table[itbl_idx++].interface = spr_intf;
           itbl_offset_count += CLASS_CB(spr_intf)->method_table_size;
       }
   }

   /* if we're an interface all finished - offsets aren't used */

   if(!(cb->access_flags & ACC_INTERFACE)) {
       int *offsets_pntr = (int*)sysMalloc(itbl_offset_count * sizeof(int));
       int old_mtbl_size = method_table_size;
       MethodBlock *miranda[MRNDA_CACHE_SZE];
       int miranda_count = 0;
       int mtbl_idx;

       /* run through table again, this time filling in the offsets array -
        * for each new interface, run through it's methods and locate
        * each method in this classes method table */

       for(i = spr_imthd_tbl_sze; i < cb->imethod_table_size; i++) {
           ClassBlock *intf_cb = CLASS_CB(cb->imethod_table[i].interface);
           cb->imethod_table[i].offsets = offsets_pntr;

           for(j = 0; j < intf_cb->methods_count; j++) {
               MethodBlock *intf_mb = &intf_cb->methods[j];

               if((intf_mb->access_flags & (ACC_STATIC | ACC_PRIVATE)) ||
                      (intf_mb->name[0] == '<'))
                   continue;

               /* We scan backwards so that we find methods defined in sub-classes
                  before super-classes.  This ensures we find non-overridden
                  methods before the inherited non-accessible method */
               for(mtbl_idx = method_table_size - 1; mtbl_idx >= 0; mtbl_idx--)
                   if(intf_mb->name == method_table[mtbl_idx]->name &&
                           intf_mb->type == method_table[mtbl_idx]->type) {
                       *offsets_pntr++ = mtbl_idx;
                       break;
                   }

               if(mtbl_idx < 0) {

                   /* didn't find it - add a dummy abstract method (a so-called
                      miranda method).  If it's invoked we'll get an abstract
                      method error */

                   int k;
                   for(k = 0; k < miranda_count; k++)
                       if(intf_mb->name == miranda[k]->name &&
                                   intf_mb->type == miranda[k]->type)
                           break;

                   *offsets_pntr++ = method_table_size + k;

                   if(k == miranda_count) {
                       if(miranda_count == MRNDA_CACHE_SZE) {
                           resizeMTable(method_table, method_table_size, miranda, MRNDA_CACHE_SZE);
                           miranda_count = 0;
                       }
                       miranda[miranda_count++] = intf_mb;
                   }
               }
           }
       }

       if(miranda_count > 0)
           resizeMTable(method_table, method_table_size, miranda, miranda_count);

       if(old_mtbl_size != method_table_size) {
           /* We've created some abstract methods */
           int num_mirandas = method_table_size - old_mtbl_size;

           mb = (MethodBlock *) sysRealloc(cb->methods,
                   (cb->methods_count + num_mirandas) * sizeof(MethodBlock));

           /* If the realloc of the method list gave us a new pointer, the pointers
              to them in the method table are now wrong. */
           if(mb != cb->methods) {
               /*  mb will be left pointing to the end of the methods */
               cb->methods = mb;
               fillinMTable(method_table, mb, cb->methods_count);
           } else
               mb += cb->methods_count;

           cb->methods_count += num_mirandas;

           /* Now we've expanded the method list, replace pointers to
              the interface methods. */

           for(i = old_mtbl_size; i < method_table_size; i++,mb++) {
               memcpy(mb, method_table[i], sizeof(MethodBlock));
               mb->access_flags |= ACC_MIRANDA;
               mb->method_table_index = i;
               mb->classobj = classobj;
               method_table[i] = mb;
           }
       }
   }

   cb->method_table = method_table;
   cb->method_table_size = method_table_size;

   /* Handle finalizer */

   /* If this is Object find the finalize method.  All subclasses will
      have it in the same place in the method table.  Note, Object
      should always have a valid finalizer -- but check just in case */

   if(cb->super == NULL) {
       finalizer = findMethod(classobj, SYMBOL(finalize), SYMBOL(___V));
       if(finalizer && !(finalizer->access_flags & (ACC_STATIC | ACC_PRIVATE))) {
           finalize_mtbl_idx = finalizer->method_table_index;
           obj_fnlzr_mthd = finalizer;
       }
   }

   cb->flags |= spr_flags;

   /* Store the finalizer only if it's overridden Object's.  We don't
      want to finalize every object, and Object's imp is empty */

   if(super && obj_fnlzr_mthd && (finalizer =
               method_table[obj_fnlzr_mthd->method_table_index]) != obj_fnlzr_mthd)
       cb->flags |= FINALIZED;

   /* Handle reference classes */

   if(ref_referent_offset == -1 && cb->name == SYMBOL(java_lang_ref_Reference)) {
       FieldBlock *ref_fb = findField(classobj, SYMBOL(referent), SYMBOL(sig_java_lang_Object));
       FieldBlock *queue_fb = findField(classobj, SYMBOL(queue), SYMBOL(sig_java_lang_ref_ReferenceQueue));
       MethodBlock *enqueue_mb = findMethod(classobj, SYMBOL(enqueue), SYMBOL(___Z));

       if(ref_fb == NULL || queue_fb == NULL || enqueue_mb == NULL) {
           jam_fprintf(stderr, "Expected fields/methods missing in java.lang.ref.Reference\n");
           exitVM(1);
       }

       for(fb = cb->fields, i = 0; i < cb->fields_count; i++,fb++)
           if(fb->offset > ref_fb->offset)
               fb->offset--;

       ref_referent_offset = ref_fb->offset = field_offset - 1;
       enqueue_mtbl_idx = enqueue_mb->method_table_index;
       ref_queue_offset = queue_fb->offset;
       refs_end_offset--;

       cb->flags |= REFERENCE;
   }

   if(spr_flags & REFERENCE) {
       if(cb->name == SYMBOL(java_lang_ref_SoftReference))
           cb->flags |= SOFT_REFERENCE;
       else
           if(cb->name == SYMBOL(java_lang_ref_WeakReference))
               cb->flags |= WEAK_REFERENCE;
           else
               if(cb->name == SYMBOL(java_lang_ref_PhantomReference))
                   cb->flags |= PHANTOM_REFERENCE;
   }

   /* Handle class loader classes */

   if(ldr_vmdata_offset == -1 && cb->name == SYMBOL(java_lang_ClassLoader)) {
       FieldBlock *ldr_fb = findField(classobj, SYMBOL(vmdata), SYMBOL(sig_java_lang_Object));

       if(ldr_fb == NULL) {
           jam_fprintf(stderr, "Expected vmdata field missing in java.lang.ClassLoader\n");
           exitVM(1);
       }

       ldr_vmdata_offset = ldr_fb->offset;
       cb->flags |= CLASS_LOADER;
   }

   /* Construct the reference offsets list.  This is used to speed up
      scanning of an objects references during the mark phase of GC. */

   if(refs_end_offset > spr_obj_sze) {
       int new_start;

       if(spr_rfs_offsts_sze > 0 && spr_rfs_offsts_tbl[spr_rfs_offsts_sze-1].end == spr_obj_sze) {
           cb->refs_offsets_size = spr_rfs_offsts_sze;
           new_start = spr_rfs_offsts_tbl[spr_rfs_offsts_sze-1].start;
       } else {
           cb->refs_offsets_size = spr_rfs_offsts_sze + 1;
           new_start = spr_obj_sze;
      }

       cb->refs_offsets_table = (RefsOffsetsEntry*)sysMalloc(cb->refs_offsets_size * sizeof(RefsOffsetsEntry));

      memcpy(cb->refs_offsets_table, spr_rfs_offsts_tbl,
                                     spr_rfs_offsts_sze * sizeof(RefsOffsetsEntry));

      cb->refs_offsets_table[cb->refs_offsets_size-1].start = new_start;
      cb->refs_offsets_table[cb->refs_offsets_size-1].end = refs_end_offset;
   } else {
       cb->refs_offsets_size = spr_rfs_offsts_sze;
       cb->refs_offsets_table = spr_rfs_offsts_tbl;
   }

   cb->state = CLASS_LINKED;

unlock:
   objectUnlock((Object *)classobj);
}

Class *initClass(Class *classobj)
{
   ClassBlock *cb = CLASS_CB(classobj);
   ConstantPool *cp = &cb->constant_pool;
   FieldBlock *fb = cb->fields;
   MethodBlock *mb;
   Object *excep;
   int state, i;

   if(cb->state >= CLASS_INITED)
      return classobj;

   linkClass(classobj);
   objectLock((Object *)classobj);

   while(cb->state == CLASS_INITING)
      if(cb->initing_tid == threadSelf()->id) {
         objectUnlock((Object *)classobj);
         return classobj;
      } else {
          /* FALSE means this wait is non-interruptible.
             An interrupt will appear as if the initialiser
             failed (below), and clearing will lose the
             interrupt status */
          objectWait0((Object *)classobj, 0, 0, FALSE);
      }

   if(cb->state >= CLASS_INITED) {
      objectUnlock((Object *)classobj);
      return classobj;
   }

   if(cb->state == CLASS_BAD) {
       objectUnlock((Object *)classobj);
       signalException(java_lang_NoClassDefFoundError, cb->name);
       return NULL;
   }

   cb->state = CLASS_INITING;
   cb->initing_tid = threadSelf()->id;

   objectUnlock((Object *)classobj);

   if(!(cb->access_flags & ACC_INTERFACE) && cb->super
              && (CLASS_CB(cb->super)->state != CLASS_INITED)) {
      initClass(cb->super);
      if(exceptionOccurred()) {
          state = CLASS_BAD;
          goto set_state_and_notify;
      }
   }

    {
        // dynsearch
        // std::cout << "ininClass: " << classobj->name() << std::endl;
        // SpmtThread* current_core = g_current_core();
        // current_core->after_alloc_object(classobj);

    }

   /* Never used to bother with this as only static finals use it and
      the constant value's copied at compile time.  However, separate
      compilation can result in a getstatic to a (now) constant field,
      and the VM didn't initialise it... */

   for(i = 0; i < cb->fields_count; i++,fb++)
      if((fb->access_flags & ACC_STATIC) && fb->constant) {
         if((*fb->type == 'J') || (*fb->type == 'D'))
            *(u8*)&fb->static_value = *(u8*)&(CP_INFO(cp, fb->constant));
         else
            fb->static_value = resolveSingleConstant(classobj, fb->constant);
      }

   if((mb = findMethod(classobj, SYMBOL(class_init), SYMBOL(___V))) != NULL)
      executeStaticMethod(classobj, mb);

   if((excep = exceptionOccurred())) {
       Class *error, *eiie;

       clearException();

       /* Don't wrap exceptions of type java.lang.Error... */
       if((error = findSystemClass0(SYMBOL(java_lang_Error)))
                 && !isInstanceOf(error, excep->classobj)
                 && (eiie = findSystemClass(SYMBOL(java_lang_ExceptionInInitializerError)))
                 && (mb = findMethod(eiie, SYMBOL(object_init), SYMBOL(_java_lang_Throwable__V)))) {

           Object *ob = allocObject(eiie);

           if(ob != NULL) {
               executeMethod(ob, mb, excep);
               setException(ob);
           }
       } else
           setException(excep);

       state = CLASS_BAD;
   } else
       state = CLASS_INITED;

set_state_and_notify:
   objectLock((Object *)classobj);
   cb->state = state;

   objectNotifyAll((Object *)classobj);
   objectUnlock((Object *)classobj);

   return state == CLASS_BAD ? NULL : classobj;
}

char *findFileEntry(char *path, int *file_len) {
    int read_len;
    char *data;
    FILE *fd;

    if((fd = fopen(path, "r")) == NULL)
        return NULL;

    fseek(fd, 0L, SEEK_END);
    *file_len = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    data = (char *)sysMalloc(*file_len);
    read_len = fread(data, sizeof(char), *file_len, fd);
    fclose(fd);

    if(read_len == *file_len)
        return data;

    sysFree(data);
    return NULL;
}

Class *loadSystemClass(const char *classname) {
    int file_len, fname_len = strlen(classname) + 8;
    char buff[max_cp_element_len + fname_len];
    char filename[fname_len];
    Class *classobj = NULL;
    char *data = NULL;
    int i;

    filename[0] = '/';
    strcat(strcpy(&filename[1], classname), ".class");

    for(i = 0; i < bcp_entries && data == NULL; i++)
        if(bootclasspath[i].zip)
            data = findArchiveEntry(filename+1, bootclasspath[i].zip, &file_len);
        else
            data = findFileEntry(strcat(strcpy(buff, bootclasspath[i].path), filename), &file_len);

    if(data == NULL) {
        signalException(java_lang_NoClassDefFoundError, classname);
        return NULL;
    }

    classobj = defineClass(classname, data, 0, file_len, NULL);
    sysFree(data);

    if(verbose && classobj)
        jam_printf("[Loaded %s from %s]\n", classname, bootclasspath[i-1].path);

    return classobj;
}

void addInitiatingLoaderToClass(Object *class_loader, Class *classobj) {
    ClassBlock *cb = CLASS_CB(classobj);

    /* The defining class loader is automatically an initiating
       loader so don't add again */
    if(cb->class_loader != class_loader)
        addClassToHash(classobj, class_loader);
}

Class *findHashedClass(char* classname, Object *class_loader) {
    HashTable *table;
    Class *classobj;
    char *name;

    /* If the class name is not in the utf8 table it can't
       have been loaded */
    if((name = findUtf8(classname)) == NULL)
        return NULL;

    if(class_loader == NULL)
        table = &loaded_classes;
    else {
        Object *vmdata = (Object*)INST_DATA(class_loader)[ldr_vmdata_offset];

        if(vmdata == NULL)
            return NULL;

        table = (HashTable*)INST_DATA(vmdata)[ldr_data_tbl_offset];
    }

#undef HASH
#undef COMPARE
#define HASH(ptr) utf8Hash(ptr)
#define COMPARE(ptr1, ptr2, hash1, hash2) (hash1 == hash2) && \
            (ptr1 == CLASS_CB((Class *)ptr2)->name)

    /* Do not add if absent, no scavenge, locked */
    findHashEntry((*table), name, classobj, FALSE, FALSE, TRUE, Class*);

   return classobj;
}

Class *findSystemClass0(const char* classname) {
    Class *classobj = findHashedClass(const_cast<char*>(classname), NULL);

   if(classobj == NULL)
       classobj = loadSystemClass(classname);

   if(!exceptionOccurred())
       linkClass(classobj);

   return classobj;
}

Class *findSystemClass(const char *classname) {
   Class *classobj = findSystemClass0(classname);

   if(!exceptionOccurred())
       initClass(classobj);

   return classobj;
}

Class *findArrayClassFromClassLoader(const char *classname, Object *class_loader) {
    Class *classobj = findHashedClass(const_cast<char*>(classname), class_loader);

   if(classobj == NULL) {
       if((classobj = createArrayClass(classname, class_loader)) != NULL)
           addInitiatingLoaderToClass(class_loader, classobj);
   }
   return classobj;
}

Class *findPrimitiveClass(char prim_type) {
   int index;
   Class *prim;
   const char *classname;

   switch(prim_type) {
      case 'Z':
          classname = SYMBOL(boolean); index = 1;
          break;
      case 'B':
          classname = SYMBOL(byte); index = 2;
          break;
      case 'C':
          classname = SYMBOL(char); index = 3;
          break;
      case 'S':
          classname = SYMBOL(short); index = 4;
          break;
      case 'I':
          classname = SYMBOL(int); index = 5;
          break;
      case 'F':
          classname = SYMBOL(float); index = 6;
          break;
      case 'J':
          classname = SYMBOL(long); index = 7;
          break;
      case 'D':
          classname = SYMBOL(double); index = 8;
          break;
      case 'V':
          classname = SYMBOL(void); index = 0;
          break;
      default:
          signalException(java_lang_NoClassDefFoundError, NULL);
          return NULL;
          break;
   }

   prim = prim_classes[index];
   return prim ? prim : createPrimClass(classname, index);
}

Class *findNonArrayClassFromClassLoader(char *classname, Object *loader) {
    Class *classobj = findHashedClass(classname, loader);

    if(classobj == NULL) {
        char *dot_name = slash2dots(classname);

        {
            //Thread* this_thread = threadSelf();
            SpmtThread* current_spmt_thread = g_get_current_spmt_thread();
            // MINILOG0_IF(debug_scaffold::java_main_arrived,
            //             "#" << current_spmt_thread->id() << " is to load class: " << dot_name);
        }

        Object *string = createString(dot_name);
        Object *excep;

        sysFree(dot_name);
        if(string == NULL)
            return NULL;

        if(loadClass_mtbl_idx == -1) {
            MethodBlock *mb = lookupMethod(loader->classobj, SYMBOL(loadClass),
                            SYMBOL(_java_lang_String__java_lang_Class));
            if(mb == NULL)
                return NULL;

            loadClass_mtbl_idx = mb->method_table_index;
        }

        /* The public loadClass is not synchronized.
           Lock the class-loader to be thread-safe */
        objectLock(loader);
        classobj = *(Class**)executeMethod(loader,
                    CLASS_CB(loader->classobj)->method_table[loadClass_mtbl_idx], string);
        objectUnlock(loader);

        if((excep = exceptionOccurred()) || classobj == NULL) {
            clearException();
            signalChainedException(java_lang_NoClassDefFoundError, classname, excep);
            return NULL;
        }

        addInitiatingLoaderToClass(loader, classobj);

        if(verbose && (CLASS_CB(classobj)->class_loader == loader))
            jam_printf("[Loaded %s]\n", classname);
    }
    return classobj;
}


Class *findClassFromClassLoader(char *classname, Object *loader) {

    if(*classname == '[')
        return findArrayClassFromClassLoader(classname, loader);

    if(loader != NULL)
        return findNonArrayClassFromClassLoader(classname, loader);

    return findSystemClass0(classname);
}

// duyanning, do not use class loader to load class
// Class *findClassFromClassLoader(char *classname, Object *loader) {

//     if(*classname == '[')
//         return findArrayClassFromClassLoader(classname, 0);

//     return findSystemClass0(classname);
// }

Object *getSystemClassLoader() {
    Class *class_loader = findSystemClass(SYMBOL(java_lang_ClassLoader));

    if(!exceptionOccurred()) {
        MethodBlock *mb;

        if((mb = findMethod(class_loader, SYMBOL(getSystemClassLoader),
                                          SYMBOL(___java_lang_ClassLoader))) != NULL) {
            Object *system_loader = *(Object**)executeStaticMethod(class_loader, mb);

            if(!exceptionOccurred())
                return system_loader;
        }
    }
    return NULL;
}

/* gc support for marking classes */

#define ITERATE(ptr)  markRoot((Object*)ptr)

void markBootClasses() {
   int i;

   hashIterate(loaded_classes);

   for(i = 0; i < MAX_PRIM_CLASSES; i++)
       if(prim_classes[i] != NULL)
           markRoot((Object*)prim_classes[i]);
}

#undef ITERATE
#define ITERATE(ptr)  threadReference((Object**)ptr)

void threadBootClasses() {
   int i;

   hashIterateP(loaded_classes);

   for(i = 0; i < MAX_PRIM_CLASSES; i++)
       if(prim_classes[i] != NULL)
           threadReference((Object**)&prim_classes[i]);
}

#undef ITERATE
#define ITERATE(ptr)                                         \
    if(CLASS_CB((Class *)ptr)->class_loader == class_loader) \
        markObject((Object*)ptr, mark, mark_soft_refs)

void markLoaderClasses(Object *class_loader, int mark, int mark_soft_refs) {
    Object *vmdata = (Object*)INST_DATA(class_loader)[ldr_vmdata_offset];

    if(vmdata != NULL) {
        HashTable *table = (HashTable*)INST_DATA(vmdata)[ldr_data_tbl_offset];
        hashIterate((*table));
    }
}

#undef ITERATE
#define ITERATE(ptr)  threadReference((Object**)ptr)

void threadLoaderClasses(Object *class_loader) {
    Object *vmdata = (Object*)INST_DATA(class_loader)[ldr_vmdata_offset];

    if(vmdata != NULL) {
        HashTable *table = (HashTable*)INST_DATA(vmdata)[ldr_data_tbl_offset];
        hashIterateP((*table));
    }
}

void freeClassData(Class *classobj) {
    ClassBlock *cb = CLASS_CB(classobj);
    int i;

    if(IS_ARRAY(cb)) {
        gcPendingFree(cb->interfaces);
        return;
    }

    gcPendingFree((void*)cb->constant_pool.type);
    gcPendingFree(cb->constant_pool.info);
    gcPendingFree(cb->interfaces);

    for(i = 0; i < cb->fields_count; i++) {
        FieldBlock *fb = &cb->fields[i];

        if(fb->annotations != NULL) {
            gcPendingFree(fb->annotations->data);
            gcPendingFree(fb->annotations);
        }
    }

    gcPendingFree(cb->fields);

    for(i = 0; i < cb->methods_count; i++) {
        MethodBlock *mb = &cb->methods[i];

#ifdef DIRECT
        if(!((uintptr_t)mb->code & 0x3)) {
#ifdef INLINING
            if(cb->state >= CLASS_LINKED)
                freeMethodInlinedInfo(mb);
#endif
            gcPendingFree(mb->code);
        } else
            if(!(mb->access_flags & ACC_ABSTRACT))
                gcPendingFree((void*)((uintptr_t)mb->code & ~3));
#else
        if(!(mb->access_flags & ACC_ABSTRACT))
            gcPendingFree(mb->code);
#endif

        gcPendingFree(mb->exception_table);
        gcPendingFree(mb->line_no_table);
        gcPendingFree(mb->throw_table);

        if(mb->annotations != NULL) {
            if(mb->annotations->annotations != NULL) {
                gcPendingFree(mb->annotations->annotations->data);
                gcPendingFree(mb->annotations->annotations);
            }
            if(mb->annotations->parameters != NULL) {
                gcPendingFree(mb->annotations->parameters->data);
                gcPendingFree(mb->annotations->parameters);
            }
            if(mb->annotations->dft_val != NULL) {
                gcPendingFree(mb->annotations->dft_val->data);
                gcPendingFree(mb->annotations->dft_val);
            }
            gcPendingFree(mb->annotations);
        }
    }

    gcPendingFree(cb->methods);
    gcPendingFree(cb->inner_classes);

    if(cb->annotations != NULL) {
        gcPendingFree(cb->annotations->data);
        gcPendingFree(cb->annotations);
    }

   if(cb->state >= CLASS_LINKED) {
        ClassBlock *super_cb = CLASS_CB(cb->super);

        /* interfaces do not have a method table, or
            imethod table offsets */
        if(!IS_INTERFACE(cb)) {
             int spr_imthd_sze = super_cb->imethod_table_size;

            gcPendingFree(cb->method_table);
            if(cb->imethod_table_size > spr_imthd_sze)
                gcPendingFree(cb->imethod_table[spr_imthd_sze].offsets);
        }

        gcPendingFree(cb->imethod_table);

        if(cb->refs_offsets_table != super_cb->refs_offsets_table)
            gcPendingFree(cb->refs_offsets_table);
    }
}

void freeClassLoaderData(Object *class_loader) {
    Object *vmdata = (Object*)INST_DATA(class_loader)[ldr_vmdata_offset];

    if(vmdata != NULL) {
        HashTable *table = (HashTable*)INST_DATA(vmdata)[ldr_data_tbl_offset];
        gcFreeHashTable((*table));
        gcPendingFree(table);
    }
}

/* Add a library unloader object to the class loader for the
   library contained within entry.  The library has an unload
   function, which will be called from the unloader finalizer
   when the class loader is garbage collected */
void newLibraryUnloader(Object *class_loader, void *entry) {
    Object *vmdata = (Object*)INST_DATA(class_loader)[ldr_vmdata_offset];

    if(vmdata != NULL)
        executeMethod(vmdata, ldr_new_unloader, (long long)(uintptr_t)entry);
}

int parseBootClassPath(char *cp_var) {
    char *cp, *pntr, *start;
    int i, j, len, max = 0;
    struct stat info;

    cp = (char*)sysMalloc(strlen(cp_var)+1);
    strcpy(cp, cp_var);

    for(i = 0, start = pntr = cp; *pntr; pntr++) {
        if(*pntr == ':') {
            if(start != pntr) {
                *pntr = '\0';
                i++;
            }
            start = pntr+1;
        }
    }
    if(start != pntr)
        i++;

    bootclasspath = (BCPEntry *)sysMalloc(sizeof(BCPEntry)*i);

    for(j = 0, pntr = cp; i > 0; i--) {
        while(*pntr == ':')
            pntr++;

        start = pntr;
        pntr += (len = strlen(pntr))+1;

        if(stat(start, &info) == 0) {
            if(S_ISDIR(info.st_mode)) {
                bootclasspath[j].zip = NULL;
                if(len > max)
                    max = len;
            } else
                if((bootclasspath[j].zip = processArchive(start)) == NULL)
                    continue;
            bootclasspath[j++].path = start;
        }
    }

    max_cp_element_len = max;

    return bcp_entries = j;
}

void setClassPath(char *cmdlne_cp) {
    char *env;
    classpath = cmdlne_cp ? cmdlne_cp :
        ((env = getenv("CLASSPATH")) ? env : (char*)".");
}

char *getClassPath() {
    return classpath;
}

int filter(struct dirent *entry) {
    int len = strlen(entry->d_name);
    char *ext = &entry->d_name[len-4];

    return len >= 4 && (strcasecmp(ext, ".zip") == 0 ||
                        strcasecmp(ext, ".jar") == 0);
}

void scanDirForJars(char *dir) {
    int bootpathlen = strlen(bootpath) + 1;
    int dirlen = strlen(dir);
    struct dirent **namelist;
    int n;

    n = scandir(dir, &namelist, (int (*)(const dirent*))&filter, &alphasort);

    if(n >= 0) {
        while(--n >= 0) {
            char *buff;
            bootpathlen += strlen(namelist[n]->d_name) + dirlen + 2;
            buff = (char*)sysMalloc(bootpathlen);

            strcat(strcat(strcat(strcat(strcpy(buff, dir), "/"),
                                 namelist[n]->d_name), ":"), bootpath);

            sysFree(bootpath);
            bootpath = buff;
            free(namelist[n]);
        }
        free(namelist);
    }
}

void scanDirsForJars(const char *directories) {
    int dirslen = strlen(directories);
    char *pntr, *end, *dirs = (char*)sysMalloc(dirslen + 1);
    strcpy(dirs, directories);

    for(end = pntr = &dirs[dirslen]; pntr != dirs; pntr--) {
        if(*pntr == ':') {
            char *start = pntr + 1;
            if(start != end)
                scanDirForJars(start);

            *(end = pntr) = '\0';
        }
    }

    if(end != dirs)
        scanDirForJars(dirs);

    sysFree(dirs);
}

char *setBootClassPath(char *cmdlne_bcp, char bootpathopt) {
    const char* endorsed_dirs;

    if(cmdlne_bcp)
        switch(bootpathopt) {
            case 'a':
            case 'p':
                bootpath = (char*)sysMalloc(strlen(DFLT_BCP) + strlen(cmdlne_bcp) + 2);
                if(bootpathopt == 'a')
                    strcat(strcat(strcpy(bootpath, DFLT_BCP), ":"), cmdlne_bcp);
                else
                    strcat(strcat(strcpy(bootpath, cmdlne_bcp), ":"), DFLT_BCP);
                break;

            case 'c':
                bootpath = (char*)sysMalloc(strlen(JAMVM_CLASSES) + strlen(cmdlne_bcp) + 2);
                strcat(strcat(strcpy(bootpath, JAMVM_CLASSES), ":"), cmdlne_bcp);
                break;

            case 'v':
                bootpath = (char*)sysMalloc(strlen(CLASSPATH_CLASSES) + strlen(cmdlne_bcp) + 2);
                strcat(strcat(strcpy(bootpath, cmdlne_bcp), ":"), CLASSPATH_CLASSES);
                break;

            default:
                bootpath = (char*)sysMalloc(strlen(cmdlne_bcp) + 1);
                strcpy(bootpath, cmdlne_bcp);
        }
    else {
        char *env = getenv("BOOTCLASSPATH");
        char *path = env ? env : (char*)DFLT_BCP;
        bootpath = (char*)sysMalloc(strlen(path) + 1);
        strcpy(bootpath, path);
    }

    endorsed_dirs = getCommandLineProperty("java.endorsed.dirs");
    if(endorsed_dirs == NULL)
        endorsed_dirs = INSTALL_DIR"/share/jamvm/endorsed";

    scanDirsForJars(endorsed_dirs);

    return bootpath;
}

char *getBootClassPath() {
    return bootpath;
}

int bootClassPathSize() {
    return bcp_entries;
}

Object *bootClassPathResource(char *filename, int index) {
    if(index < bcp_entries) {
        /* Alloc enough space for Jar file URL -- jar:file://<path>!/<filename> */
        char buff[strlen(filename) + strlen(bootclasspath[index].path) + 14];

        if(bootclasspath[index].zip) {
            while(*filename == '/')
                filename++;

            if(!findArchiveDirEntry(filename, bootclasspath[index].zip))
                return NULL;

            sprintf(buff, "jar:file://%s!/%s", bootclasspath[index].path, filename);
        } else {
            struct stat info;

            sprintf(buff, "file://%s/%s", bootclasspath[index].path, filename);
            if(stat(&buff[7], &info) != 0 || S_ISDIR(info.st_mode))
                return NULL;
        }

        return createString(buff);
    }

    return NULL;
}

void initialiseClass(InitArgs *args) {
    char *bcp = setBootClassPath(args->bootpath, args->bootpathopt);
    FieldBlock *hashtable = NULL;

    if(!(bcp && parseBootClassPath(bcp))) {
        jam_fprintf(stderr, "bootclasspath is empty!\n");
        exitVM(1);
    }

    verbose = args->verboseclass;
    setClassPath(args->classpath);

    /* Init hash table, and create lock */
    initHashTable(loaded_classes, INITSZE, TRUE);

    loader_data_class = findSystemClass0(SYMBOL(jamvm_java_lang_VMClassLoaderData));
    registerStaticClassRef(&loader_data_class);

    if(loader_data_class != NULL) {
        ldr_new_unloader = findMethod(loader_data_class, SYMBOL(newLibraryUnloader),
                                                         SYMBOL(_J__V));
        hashtable = findField(loader_data_class, SYMBOL(hashtable), SYMBOL(I));
    }

    if(hashtable == NULL || ldr_new_unloader == NULL) {
        jam_fprintf(stderr, "Fatal error: Bad VMClassLoaderData (missing or corrupt)\n");
        exitVM(1);
    }
    ldr_data_tbl_offset = hashtable->offset;

    /* Register the address of where the java.lang.Class ref _will_ be */
    registerStaticClassRef(&java_lang_Class);
}

