#include "std.h"
#include "rope.h"
#include "lock.h"
#include "symbol.h"
#include "excep.h"
#include "SpmtThread.h"
#include "Loggers.h"
#include "Helper.h"
#include "Group.h"

static Class *ste_class, *ste_array_class, *throw_class, *vmthrow_class;
static MethodBlock *vmthrow_init_mb;
static int backtrace_offset;
static int inited = FALSE;

static Class *exceptions[MAX_EXCEPTION_ENUM];

static int exception_symbols[] = {
    EXCEPTIONS_DO(SYMBOL_NAME_ENUM)
};

void initialiseException() {
    FieldBlock *bcktrce;
    int i;

    ste_class = findSystemClass0(SYMBOL(java_lang_StackTraceElement));
    ste_array_class = findArrayClass(SYMBOL(array_java_lang_StackTraceElement));
    vmthrow_class = findSystemClass0(SYMBOL(java_lang_VMThrowable));
    throw_class = findSystemClass0(SYMBOL(java_lang_Throwable));
    bcktrce = findField(vmthrow_class, SYMBOL(backtrace), SYMBOL(sig_java_lang_Object));
    vmthrow_init_mb = findMethod(ste_class, SYMBOL(object_init),
                         SYMBOL(_java_lang_String_I_java_lang_String_java_lang_String_Z__V));

    if((bcktrce == NULL) || (vmthrow_init_mb == NULL)) {
        jam_fprintf(stderr, "Error initialising VM (initialiseException)\n");
        exitVM(1);
    }

    CLASS_CB(vmthrow_class)->flags |= VMTHROWABLE;
    backtrace_offset = bcktrce->offset;

    registerStaticClassRef(&ste_class);
    registerStaticClassRef(&ste_array_class);
    registerStaticClassRef(&vmthrow_class);
    registerStaticClassRef(&throw_class);

    /* Load and register the exceptions used within the VM.
       These are preloaded to speed up access.  The VM will
       abort if any can't be loaded */

    for(i = 0; i < MAX_EXCEPTION_ENUM; i++) {
        exceptions[i] = findSystemClass0(symbol_values[exception_symbols[i]]);
        registerStaticClassRef(&exceptions[i]);
    }

    inited = TRUE;
}

Object *exceptionOccurred() {
    //return getExecEnv()->exception;
    return threadSelf()->get_current_core()->get_current_mode()->exception;
}

void setException(Object *exp) {
    //getExecEnv()->exception = exp;
    assert(*threadSelf()->get_current_core()->get_current_mode()->get_name() == 'C');
    threadSelf()->get_current_core()->get_current_mode()->exception = exp;
}

void clearException() {
    assert(*threadSelf()->get_current_core()->get_current_mode()->get_name() == 'C');
    //ExecEnv *ee = getExecEnv();

//     if(ee->overflow) {
//         ee->overflow = FALSE;
//         //        ee->stack_end -= STACK_RED_ZONE_SIZE;
//     }
    //ee->exception = NULL;
    threadSelf()->get_current_core()->get_current_mode()->exception = 0;
}

void signalChainedExceptionClass(Class *exception, const char* message, Object *cause) {
    SpmtThread* current_core = g_get_current_core();
    std::cout << "error msg: " << message << std::endl;
    current_core->before_signal_exception(exception);

    Object *exp = allocObject(exception);
    Object *str = message == NULL ? NULL : Cstr2String(message);
    MethodBlock *init = lookupMethod(exception, SYMBOL(object_init),
                                                SYMBOL(_java_lang_String__V));
    if(exp && init) {
        executeMethod(exp, init, str);

        if(cause && !exceptionOccurred()) {
            MethodBlock *mb = lookupMethod(exception, SYMBOL(initCause),
                                           SYMBOL(_java_lang_Throwable__java_lang_Throwable));
            if(mb)
                executeMethod(exp, mb, cause);
        }
        setException(exp);
    }
}

void signalChainedExceptionName(char *excep_name, char *message, Object *cause) {
    if(!inited) {
        jam_fprintf(stderr, "Exception occurred while VM initialising.\n");
        if(message)
            jam_fprintf(stderr, "%s: %s\n", excep_name, message);
        else
            jam_fprintf(stderr, "%s\n", excep_name);
        exit(1);
    } else {
        Class *exception = findSystemClass(excep_name);

        if(!exceptionOccurred())
            signalChainedExceptionClass(exception, message, cause);
    }
}

void signalChainedExceptionEnum(int excep_enum, const char* message, Object *cause) {
    if(!inited) {
        const char *excep_name = symbol_values[exception_symbols[excep_enum]];

        jam_fprintf(stderr, "Exception occurred while VM initialising.\n");
        if(message)
            jam_fprintf(stderr, "%s: %s\n", excep_name, message);
        else
            jam_fprintf(stderr, "%s\n", excep_name);
        exit(1);
    }

    signalChainedExceptionClass(exceptions[excep_enum], message, cause);
}

void printException() {
    //ExecEnv *ee = getExecEnv();
    //Object *exception = ee->exception;
    Object *exception = threadSelf()->get_current_core()->get_current_mode()->exception;

    if(exception != NULL) {
        MethodBlock *mb = lookupMethod(exception->classobj, SYMBOL(printStackTrace),
                                                         SYMBOL(___V));
        clearException();
        executeMethod(exception, mb);

        /* If we're really low on memory we might have been able to throw
         * OutOfMemory, but then been unable to print any part of it!  In
         * this case the VM just seems to stop... */
        //if(ee->exception) {
        if(threadSelf()->get_current_core()->get_current_mode()->exception) {
            jam_fprintf(stderr, "Exception occurred while printing exception (%s)...\n",
                        //CLASS_CB(ee->exception->classobj)->name);
                            CLASS_CB(threadSelf()->get_current_core()->get_current_mode()->exception->classobj)->name);
            jam_fprintf(stderr, "Original exception was %s\n", CLASS_CB(exception->classobj)->name);
        }
    }
}

/*
  mb                 exception occured in this method
  exception          exception
  pc_pntr            points to instruction which caused the exception
 */
CodePntr
findCatchBlockInMethod(MethodBlock *mb, Class *exception, CodePntr pc_pntr)
{
    ExceptionTableEntry *table = mb->exception_table;
    int size = mb->exception_table_size;
    int pc = pc_pntr - ((CodePntr)mb->code);
    int i;

    for(i = 0; i < size; i++)
        if ((pc >= table[i].start_pc) and (pc < table[i].end_pc)) {

            /* If the catch_type is 0 it's a finally block, which matches
               any exception.  Otherwise, the thrown exception class must
               be an instance of the caught exception class to catch it */

            if (table[i].catch_type != 0) {
                Class *caught_class = resolveClass(mb->classobj, table[i].catch_type, FALSE);
                if (caught_class == NULL) {
                    clearException();
                    continue;
                }
                if (!isInstanceOf(caught_class, exception))
                    continue;
            }
            return ((CodePntr)mb->code) + table[i].handler_pc;
        }

    return NULL;
}

CodePntr
findCatchBlock(Class *exception)
{
    //Frame *frame = getExecEnv()->last_frame;
	//assert(getExecEnv()->last_frame == threadSelf()->get_current_core()->get_current_mode()->frame);
    Frame* frame = threadSelf()->get_current_core()->get_current_mode()->frame;
    CodePntr handler_pc = NULL;

    MINILOG(c_exception_logger, "#" << threadSelf()->get_current_core()->id() << " finding handler"
            << " in: " << info(frame)
            << " on: #" << frame->get_object()->get_group()->get_core()->id()
            );

    while (((handler_pc = findCatchBlockInMethod(frame->mb, exception, frame->last_pc)) == NULL)
           and (frame->prev->mb != NULL)) {

        if (frame->mb->is_synchronized()) {
            Object *sync_ob = frame->mb->access_flags & ACC_STATIC ?
                (Object*)frame->mb->classobj : (Object*)frame->lvars[0];
            objectUnlock(sync_ob);
        }
        frame = frame->prev;

        MINILOG(c_exception_logger, "#" << threadSelf()->get_current_core()->id() << " finding handler"
                << " in: " << info(frame)
                << " on: #" << frame->get_object()->get_group()->get_core()->id()
                );

    }

    //getExecEnv()->last_frame = frame;
    threadSelf()->get_current_core()->get_current_mode()->frame = frame;

    return handler_pc;
}

int mapPC2LineNo(MethodBlock *mb, CodePntr pc_pntr) {
    int pc = pc_pntr - (CodePntr) mb->code;
    int i;

    if(mb->line_no_table_size > 0) {
        for(i = mb->line_no_table_size-1; i && pc < mb->line_no_table[i].start_pc; i--);
        return mb->line_no_table[i].line_no;
    }

    return -1;
}

//Object *setStackTrace0(ExecEnv *ee, int max_depth)
Object *setStackTrace0(int max_depth)
{
    //assert(false);
    Frame* bottom;
    //Frame* last = ee->last_frame;
    SpmtThread* current_core = threadSelf()->get_current_core();
    Frame* last = current_core->get_current_mode()->frame;
    Object *array, *vmthrwble;
    uintptr_t *data;
    int depth = 0;

    if(last->prev == NULL) {
        if((array = allocTypeArray(sizeof(uintptr_t) == 4 ? T_INT : T_LONG, 0)) == NULL)
            return NULL;
        goto out2;
    }

    // for(; last->mb != NULL && isInstanceOf(vmthrow_class, last->mb->classobj);
    //       last = last->prev);
    while (last->mb != NULL and isInstanceOf(vmthrow_class, last->mb->classobj)) {
        last = last->prev;
    }

    // for(; last->mb != NULL && isInstanceOf(throw_class, last->mb->classobj);
    //       last = last->prev);
    while (last->mb != NULL and isInstanceOf(throw_class, last->mb->classobj)) {
          last = last->prev;
    }

    bottom = last;
    do {
        for(; last->mb != NULL; last = last->prev, depth++)
            if(depth == max_depth)
                goto out;
    } while((last = last->prev)->prev != NULL);

out:
    if((array = allocTypeArray(sizeof(uintptr_t) == 4 ? T_INT : T_LONG, depth*2)) == NULL)
        return NULL;

    data = (uintptr_t*)ARRAY_DATA(array);
    depth = 0;
    do {
        for(; bottom->mb != NULL; bottom = bottom->prev) {
            if(depth == max_depth)
                goto out2;

            data[depth++] = (uintptr_t)bottom->mb;
            data[depth++] = (uintptr_t)bottom->last_pc;
        }
    } while((bottom = bottom->prev)->prev != NULL);

out2:
    if((vmthrwble = allocObject(vmthrow_class)))
        INST_DATA(vmthrwble)[backtrace_offset] = (uintptr_t)array;

    return vmthrwble;
}

Object *convertStackTrace(Object *vmthrwble) {
    Object *array, *ste_array;
    int depth, i, j;
    uintptr_t *src;
    Object **dest;

    if((array = (Object *)INST_DATA(vmthrwble)[backtrace_offset]) == NULL)
        return NULL;

    src = (uintptr_t*)ARRAY_DATA(array);
    depth = ARRAY_LEN(array);

    if((ste_array = allocArray(ste_array_class, depth/2, sizeof(Object*))) == NULL)
        return NULL;

    dest = (Object**)ARRAY_DATA(ste_array);

    for(i = 0, j = 0; i < depth; j++) {
        MethodBlock *mb = (MethodBlock*)src[i++];
        CodePntr pc = (CodePntr)src[i++];
        ClassBlock *cb = CLASS_CB(mb->classobj);
        char *dot_name = slash2dots(cb->name);

        int isNative = mb->is_native();
        Object *filename = isNative ? NULL : (cb->source_file_name ?
                                             createString(cb->source_file_name) : NULL);
        Object *classname = createString(dot_name);
        Object *methodname = createString(mb->name);
        Object *ste = allocObject(ste_class);
        sysFree(dot_name);

        if(exceptionOccurred())
            return NULL;

        executeMethod(ste, vmthrow_init_mb, filename, isNative ? -1 : mapPC2LineNo(mb, pc),
                        classname, methodname, isNative);

        if(exceptionOccurred())
            return NULL;

        dest[j] = ste;
    }

    return ste_array;
}

/* GC support for marking classes referenced by a VMThrowable.
   In rare circumstances a stack backtrace may hold the only
   reference to a class */

void markVMThrowable(Object *vmthrwble, int mark, int mark_soft_refs) {
    Object *array;

    if((array = (Object *)INST_DATA(vmthrwble)[backtrace_offset]) != NULL) {
        uintptr_t *src = (uintptr_t*)ARRAY_DATA(array);
        int i, depth = ARRAY_LEN(array);

        for(i = 0; i < depth; i += 2) {
            MethodBlock *mb = (MethodBlock*)src[i];
            markObject(mb->classobj, mark, mark_soft_refs);
        }
    }
}
