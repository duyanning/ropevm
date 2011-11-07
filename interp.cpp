#include "std.h"
#include "rope.h"
#include "thread.h"
#include "lock.h"
#include "interp.h"
#include "excep.h"
#include "symbol.h"
#include "SpmtThread.h"
#include "RopeVM.h"
#include "interp-indirect.h"
#include "Helper.h"
#include "Statistic.h"
#include "DebugScaffold.h"
#include "frame.h"

// uintptr_t* g_drive_loop()
// {
//     Thread* this_thread = threadSelf();

//     uintptr_t* result;
//     result = this_thread->drive_loop();

//     return result;
// }


#define THROW_EXCEPTION(excep_name, message)    \
    {                                           \
        frame->last_pc = pc;                    \
        signalException(excep_name, message);   \
        throw_exception;                        \
    }


void
Mode::fetch_and_interpret_an_instruction()
{
    assert(frame);
    //assert(frame->is_alive());
    assert(not frame->is_dead());
    assert(not frame->is_bad());

    // speculative (pc, frame, sp) only serves owner
    //assert(is_speculative_mode() ? (frame->get_object()->get_group() == get_group()) : true);
    // rvp (pc, frame, sp) only serves others
    //assert(is_rvp_mode() ? frame->get_object() != m_spmt_thread->m_owner : true);

    MethodBlock* new_mb;
    Class *new_class;
    uintptr_t *arg1;
    int len;

    assert(frame->mb);
    MethodBlock* mb = frame->mb;
    int c = frame->c;
    uintptr_t* lvars = frame->lvars;
    ConstantPool* cp = mb->classobj->constant_pool();

    assert(is_sp_ok(sp, frame));
    assert(is_pc_ok(pc, mb));

    //{{{ statistic
    if (debug_scaffold::java_main_arrived
        && is_certain_mode()
        && is_app_obj(mb->classobj)
        ) {
        m_spmt_thread->m_count_certain_instr++;
        Statistic::instance()->probe_instr_exec(*pc);
    }

    if (debug_scaffold::java_main_arrived
        && is_speculative_mode()
        && is_app_obj(mb->classobj)
        ) {
        m_spmt_thread->m_count_spec_instr++;
    }

    if (debug_scaffold::java_main_arrived
        && is_rvp_mode()
        && is_app_obj(mb->classobj)
        ) {
        m_spmt_thread->m_count_rvp_instr++;
    }
    //}}} statistic

    //{{{ just for debug
    if (debug_scaffold::java_main_arrived
        && m_spmt_thread->id() == 7
        //&& strcmp(frame->mb->name, "computeNewValue") == 0
        ) {
        int x = 0;
        x++;
    }
    //{{{ just for debug

    //{{{ just for debug
    if (m_spmt_thread->m_id == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug


    assert(mb == frame->mb);


    switch (*pc) {
    default:
        assert(false);
    case OPC_ICONST_M1:
        write(sp, -1);
        sp++;
        pc += 1;
        break;
    case OPC_ACONST_NULL:
    case OPC_ICONST_0:
    case OPC_FCONST_0:
        write(sp, 0);
        sp++;
        pc += 1;
        break;
    case OPC_ICONST_1:
        write(sp, 1);
        sp++;
        pc += 1;
        break;
    case OPC_ICONST_2:
        write(sp, 2);
        sp++;
        pc += 1;
        break;
    case OPC_ICONST_3:
        write(sp, 3);
        sp++;
        pc += 1;
        break;
    case OPC_ICONST_4:
        write(sp, 4);
        sp++;
        pc += 1;
        break;
    case OPC_ICONST_5:
        write(sp, 5);
        sp++;
        pc += 1;
        break;
    case OPC_FCONST_1:
        write(sp, FLOAT_1_BITS);
        sp++;
        pc += 1;
        break;
    case OPC_FCONST_2:
        write(sp, FLOAT_2_BITS);
        sp++;
        pc += 1;
        break;
    case OPC_SIPUSH:
        write(sp, READ_S2_OP(pc));
        sp++;
        pc += 3;
        break;
    case OPC_BIPUSH:
        write(sp, READ_S1_OP(pc));
        sp++;
        pc += 2;
        break;
    case OPC_LDC_QUICK:
        write(sp, CP_INFO(cp, READ_U1_OP(pc)));
        sp++;
        pc += 2;
        break;
    case OPC_LDC_W_QUICK:
        write(sp, CP_INFO(cp, READ_U2_OP(pc)));
        sp++;
        pc += 3;
        break;
    case OPC_ILOAD:
    case OPC_FLOAD:
    case OPC_ALOAD:
        write(sp, read(&lvars[READ_U1_OP(pc)]));
        sp++;
        pc += 2;
        break;
    // case OPC_ALOAD_THIS:
    //     if (pc[1] == OPC_GETFIELD_QUICK) {
    //         assert(false);
    //         pc[0] = OPC_GETFIELD_THIS;
    //         pc += 0;
    //         break;
    //     }
    case OPC_ALOAD_0:
    case OPC_ILOAD_0:
    case OPC_FLOAD_0:
        write(sp, read(&lvars[0]));
        sp++;
        pc += 1;
        break;
    case OPC_ILOAD_1:
    case OPC_FLOAD_1:
    case OPC_ALOAD_1:
        write(sp, read(&lvars[1]));
        sp++;
        pc += 1;
        break;
    case OPC_ILOAD_2:
    case OPC_FLOAD_2:
    case OPC_ALOAD_2:
        write(sp, read(&lvars[2]));
        sp++;
        pc += 1;
        break;
    case OPC_ILOAD_3:
    case OPC_FLOAD_3:
    case OPC_ALOAD_3:
        write(sp, read(&lvars[3]));
        sp++;
        pc += 1;
        break;
    case OPC_ISTORE:
    case OPC_FSTORE:
    case OPC_ASTORE:
        write(&lvars[READ_U1_OP(pc)], read(--sp));
        pc += 2;
        break;
    case OPC_ISTORE_0:
    case OPC_ASTORE_0:
    case OPC_FSTORE_0:
        write(&lvars[0], read(--sp));
        pc += 1;
        break;
    case OPC_ISTORE_1:
    case OPC_ASTORE_1:
    case OPC_FSTORE_1:
        write(&lvars[1], read(--sp));
        pc += 1;
        break;
    case OPC_ISTORE_2:
    case OPC_ASTORE_2:
    case OPC_FSTORE_2:
        write(&lvars[2], read(--sp));
        pc += 1;
        break;
    case OPC_ISTORE_3:
    case OPC_ASTORE_3:
    case OPC_FSTORE_3:
        write(&lvars[3], read(--sp));
        pc += 1;
        break;
    case OPC_IADD:
        sp -= 2;
        write(sp, (int)read(&sp[0]) + (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_ISUB:
        sp -= 2;
        write(sp, (int)read(&sp[0]) - (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_IMUL:
        sp -= 2;
        write(sp, (int)read(&sp[0]) * (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_IDIV:
        ZERO_DIVISOR_CHECK((int) read(&sp[-1]));
        sp -= 2;
        write(sp, (int)read(&sp[0]) / (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_IREM:
        ZERO_DIVISOR_CHECK((int) read(&sp[-1]));
        sp -= 2;
        write(sp, (int)read(&sp[0]) % (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_IAND:
        sp -= 2;
        write(sp, (int)read(&sp[0]) & (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_IOR:
        sp -= 2;
        write(sp, (int)read(&sp[0]) | (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_IXOR:
        sp -= 2;
        write(sp, (int)read(&sp[0]) ^ (int)read(&sp[1]));
        sp++;
        pc += 1;
        break;
    case OPC_INEG: {
        int v = (int) read(--sp);
        write(sp, -v);
        sp++;
        pc += 1;
        break;
    }
    case OPC_ISHL:
        sp -= 2;
        write(sp, (int)read(&sp[0]) << (read(&sp[1]) & 0x1f));
        sp++;
        pc += 1;
        break;
    case OPC_ISHR:
        sp -= 2;
        write(sp, (int)read(&sp[0]) >> (read(&sp[1]) & 0x1f));
        sp++;
        pc += 1;
        break;
    case OPC_IUSHR:
        sp -= 2;
        write(sp, (unsigned int)read(&sp[0]) >> (read(&sp[1]) & 0x1f));
        sp++;
        pc += 1;
        break;
    case OPC_IF_ACMPEQ:
    case OPC_IF_ICMPEQ:
        sp -= 2;
        pc += ((int) read(&sp[0]) == (int) read(&sp[1])) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IF_ACMPNE:
    case OPC_IF_ICMPNE:
        sp -= 2;
        pc += ((int) read(&sp[0]) != (int) read(&sp[1])) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IF_ICMPLT:
        sp -= 2;
        pc += ((int) read(&sp[0]) < (int) read(&sp[1])) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IF_ICMPGE:
        sp -= 2;
        pc += ((int) read(&sp[0]) >= (int) read(&sp[1])) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IF_ICMPGT:
        sp -= 2;
        pc += ((int) read(&sp[0]) > (int) read(&sp[1])) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IF_ICMPLE:
        sp -= 2;
        pc += ((int) read(&sp[0]) <= (int) read(&sp[1])) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IFNE:
    case OPC_IFNONNULL:
        pc += ((int) read(--sp) != 0) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IFEQ:
    case OPC_IFNULL:
        pc += ((int) read(--sp) == 0) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IFLT:
        pc += ((int)read(--sp) < 0) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IFGE:
        pc += ((int) read(--sp) >= 0) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IFGT:
        pc += ((int) read(--sp) > 0) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IFLE:
        pc += ((int) read(--sp) <= 0) ? READ_S2_OP(pc) : 3;
        break;
    case OPC_IINC:
        write(&lvars[READ_U1_OP(pc)], read(&lvars[READ_U1_OP(pc)]) + READ_S1_OP(pc + 1));
        pc += 3;
        break;
    case OPC_POP:
        sp--;
        pc += 1;
        break;
    case OPC_POP2:
        sp -= 2;
        pc += 1;
        break;
    case OPC_DUP:
        write(sp, read(&sp[-1]));
        sp++;
        pc += 1;
        break;
    case OPC_IRETURN:
    case OPC_ARETURN:
    case OPC_FRETURN: {
        len = 1;
    }
        //goto methodReturn;
        method_return;
    case OPC_RETURN:
        len = 0;
        //goto methodReturn;
        method_return;
    // case OPC_GETFIELD_THIS: {
    //     //*sp = INST_DATA(self)[pc[2]];
    //     Object* self = (Object*)lvars[0];
    //     write(sp, read(&INST_DATA(self)[pc[2]]));
    //     sp++;
    //     pc += 4;
    // }
    //     break;
    case OPC_NOP:
        pc += 1;
        break;
    case OPC_LCONST_0:
    case OPC_DCONST_0:
        write((u8*)sp, 0);
        sp += 2;
        pc += 1;
        break;
    case OPC_DCONST_1:
        write((u8*)sp, DOUBLE_1_BITS);
        sp += 2;
        pc += 1;
        break;
    case OPC_LCONST_1:
        write((u8*)sp, 1);
        sp += 2;
        pc += 1;
        break;
    case OPC_LDC2_W:
        write((u8*)sp, CP_LONG(cp, READ_U2_OP(pc)));
        sp += 2;
        pc += 3;
        break;
    case OPC_LLOAD:
    case OPC_DLOAD:
        write((u8*)sp, read((u8 *) (&lvars[READ_U1_OP(pc)])));
        sp += 2;
        pc += 2;
        break;
    case OPC_LLOAD_0:
    case OPC_DLOAD_0:
        write((u8 *) sp, read((u8 *) (&lvars[0])));
        sp += 2;
        pc += 1;
        break;
    case OPC_LLOAD_1:
    case OPC_DLOAD_1:
        write((u8 *) sp, read((u8 *) (&lvars[1])));
        sp += 2;
        pc += 1;
        break;
    case OPC_LLOAD_2:
    case OPC_DLOAD_2:
        write((u8 *) sp, read((u8 *) (&lvars[2])));
        sp += 2;
        pc += 1;
        break;
    case OPC_LLOAD_3:
    case OPC_DLOAD_3:
        write((u8 *) sp, read((u8 *) (&lvars[3])));
        sp += 2;
        pc += 1;
        break;
    case OPC_LSTORE:
    case OPC_DSTORE:
        sp -= 2;
        write((u8 *) (&lvars[READ_U1_OP(pc)]), read((u8 *) sp));
        pc += 2;
        break;
    case OPC_LSTORE_0:
    case OPC_DSTORE_0:
        sp -= 2;
        write((u8 *) (&lvars[0]), read((u8 *) sp));
        pc += 1;
        break;
    case OPC_LSTORE_1:
    case OPC_DSTORE_1:
        sp -= 2;
        write((u8 *) (&lvars[1]), read((u8 *) sp));
        pc += 1;
        break;
    case OPC_LSTORE_2:
    case OPC_DSTORE_2:
        sp -= 2;
        write((u8 *) (&lvars[2]), read((u8 *) sp));
        pc += 1;
        break;
    case OPC_LSTORE_3:
    case OPC_DSTORE_3:
        sp -= 2;
        write((u8 *) (&lvars[3]), read((u8 *) sp));
        pc += 1;
        break;
    case OPC_IALOAD:
    case OPC_FALOAD: {
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 1);
        Object *array = (Object *) read(sp - 2);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(sp, ((int *) ARRAY_DATA(array))[idx]);
        // sp++;
        // pc += 1;
        do_array_load(array, idx, 4);
        break;
    }
    case OPC_AALOAD: {
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 1);
        Object *array = (Object *) read(sp - 2);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(sp, ((uintptr_t *) ARRAY_DATA(array))[idx]);
        // sp++;
        // pc += 1;
        do_array_load(array, idx, 4);
        break;
    }
    case OPC_BALOAD: {
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 1);
        Object *array = (Object *) read(sp - 2);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(sp, ((signed char*) ARRAY_DATA(array))[idx]);
        // sp++;
        // pc += 1;
        do_array_load(array, idx, 1);
        break;
    }
    case OPC_CALOAD: {
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 1);
        Object *array = (Object *) read(sp - 2);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(sp, ((unsigned short*) ARRAY_DATA(array))[idx]);
        // sp++;
        // pc += 1;
        do_array_load(array, idx, 2);
        break;
    }
    case OPC_SALOAD: {
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 1);
        Object *array = (Object *) read(sp - 2);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(sp, ((short*) ARRAY_DATA(array))[idx]);
        // sp++;
        // pc += 1;
        do_array_load(array, idx, 2);
        break;
    }
    case OPC_LALOAD: {
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 1);
        Object *array = (Object *) read(sp - 2);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write((u8 *) sp, ((u8 *) ARRAY_DATA(array))[idx]);
        // sp += 2;
        // pc += 1;
        do_array_load(array, idx, 8);
        break;
    }
    case OPC_DALOAD: {
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 1);
        Object *array = (Object *) read(sp - 2);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write((u8 *) sp, ((u8 *) ARRAY_DATA(array))[idx]);
        // sp += 2;
        // pc += 1;
        do_array_load(array, idx, 8);
        break;
    }
    case OPC_IASTORE:
    case OPC_FASTORE: {
        // int val = read(--sp);
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 2);
        Object *array = (Object *) read(sp - 3);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(&((int *) ARRAY_DATA(array))[idx], val);
        // pc += 1;
        do_array_store(array, idx, 4);
        break;
    }
    case OPC_BASTORE: {
        // int val = read(--sp);
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 2);
        Object *array = (Object *) read(sp - 3);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(&((char *) ARRAY_DATA(array))[idx], val);
        // pc += 1;
        do_array_store(array, idx, 1);
        break;
    };
    case OPC_CASTORE:
    case OPC_SASTORE: {
        // int val = read(--sp);
        // int idx = read(--sp);
        // Object *array = (Object *) read(--sp);
        int idx = read(sp - 2);
        Object *array = (Object *) read(sp - 3);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(&((short *) ARRAY_DATA(array))[idx], val);
        // pc += 1;
        do_array_store(array, idx, 2);
        break;
    };
    case OPC_AASTORE: {
        // Object *obj = (Object *) read(--sp);
        // int idx = read(--sp);
        // Object *array = (Object *) read( --sp);
        Object *obj = (Object *) read(sp - 1);
        int idx = read(sp - 2);
        Object *array = (Object *) read(sp - 3);

        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        if ((obj != NULL)
            && !arrayStoreCheck(array->classobj, obj->classobj))
            THROW_EXCEPTION(java_lang_ArrayStoreException, NULL);
        // write(&((Object**) ARRAY_DATA(array))[idx], obj);
        // pc += 1;
        do_array_store(array, idx, 4);
        break;
    }
    case OPC_LASTORE:
    case OPC_DASTORE: {
        // int idx = read(&sp[-3]);
        // Object *array = (Object *) read(&sp[-4]);
        // sp -= 4;
        int idx = read(sp - 3);
        Object *array = (Object *) read(sp - 4);
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        // write(&((u8 *) ARRAY_DATA(array))[idx], read((u8 *) & sp[2]));
        // pc += 1;
        do_array_store(array, idx, 8);
        break;
    }
    case OPC_DUP_X1: {
        uintptr_t word1 = read(&sp[-1]);
        uintptr_t word2 = read(&sp[-2]);
        write(&sp[-2], word1);
        write(&sp[-1], word2);
        write(sp++, word1);
        pc += 1;
        break;
    }
    case OPC_DUP_X2: {
        uintptr_t word1 = read(&sp[-1]);
        uintptr_t word2 = read(&sp[-2]);
        uintptr_t word3 = read(&sp[-3]);
        write(&sp[-3], word1);
        write(&sp[-2], word3);
        write(&sp[-1], word2);
        write(sp++, word1);
        pc += 1;
        break;
    }
    case OPC_DUP2: {
        write(&sp[0], read(&sp[-2]));
        write(&sp[1], read(&sp[-1]));
        sp += 2;
        pc += 1;
        break;
    }
    case OPC_DUP2_X1: {
        uintptr_t word1 = read(&sp[-1]);
        uintptr_t word2 = read(&sp[-2]);
        uintptr_t word3 = read(&sp[-3]);
        write(&sp[-3], word2);
        write(&sp[-2], word1);
        write(&sp[-1], word3);
        write(&sp[0], word2);
        write(&sp[1], word1);
        sp += 2;
        pc += 1;
        break;
    }
    case OPC_DUP2_X2: {
        uintptr_t word1 = read(&sp[-1]);
        uintptr_t word2 = read(&sp[-2]);
        uintptr_t word3 = read(&sp[-3]);
        uintptr_t word4 = read(&sp[-4]);
        write(&sp[-4], word2);
        write(&sp[-3], word1);
        write(&sp[-2], word4);
        write(&sp[-1], word3);
        write(&sp[0], word2);
        write(&sp[1], word1);
        sp += 2;
        pc += 1;
        break;
    }
    case OPC_SWAP: {
        uintptr_t word1 = read(&sp[-1]);
        write(&sp[-1], read(&sp[-2]));
        write(&sp[-2], word1);
        pc += 1;
        break;
    }
    case OPC_FADD:
        write((float *) &sp[-sizeof(float) / 4 * 2],
              read((float *) &sp[-sizeof(float) / 4 * 2]) +
              read((float *) &sp[-sizeof(float) / 4]));
        sp -= sizeof(float) / 4;
        pc += 1;
        break;
    case OPC_DADD:
        write((double *) &sp[-sizeof(double) / 4 * 2],
              read((double *) &sp[-sizeof(double) / 4 * 2]) +
              read((double *) &sp[-sizeof(double) / 4]));
        sp -= sizeof(double) / 4;
        pc += 1;
        break;
    case OPC_FSUB:
        write((float *) &sp[-sizeof(float) / 4 * 2],
              read((float *) &sp[-sizeof(float) / 4 * 2]) -
              read((float *) &sp[-sizeof(float) / 4]));
        sp -= sizeof(float) / 4;
        pc += 1;
        break;
    case OPC_DSUB:
        write((double *) &sp[-sizeof(double) / 4 * 2],
              read((double *) &sp[-sizeof(double) / 4 * 2]) -
              read((double *) &sp[-sizeof(double) / 4]));
        sp -= sizeof(double) / 4;
        pc += 1;
        break;
    case OPC_FMUL:
        write((float *) &sp[-sizeof(float) / 4 * 2],
              read((float *) &sp[-sizeof(float) / 4 * 2]) *
              read((float *) &sp[-sizeof(float) / 4]));
        sp -= sizeof(float) / 4;
        pc += 1;
        break;
    case OPC_DMUL:
        write((double *) &sp[-sizeof(double) / 4 * 2],
              read((double *) &sp[-sizeof(double) / 4 * 2]) *
              read((double *) &sp[-sizeof(double) / 4]));
        sp -= sizeof(double) / 4;
        pc += 1;
        break;
    case OPC_FDIV:
        write((float *) &sp[-sizeof(float) / 4 * 2],
              read((float *) &sp[-sizeof(float) / 4 * 2]) /
              read((float *) &sp[-sizeof(float) / 4]));
        sp -= sizeof(float) / 4;
        pc += 1;
        break;
    case OPC_DDIV:
        write((double *) &sp[-sizeof(double) / 4 * 2],
              read((double *) &sp[-sizeof(double) / 4 * 2]) /
              read((double *) &sp[-sizeof(double) / 4]));
        sp -= sizeof(double) / 4;
        pc += 1;
        break;
    case OPC_LADD:
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) +
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LSUB:
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) -
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LMUL:
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) *
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LDIV:
        ZERO_DIVISOR_CHECK(read((u8 *) & sp[-2]));
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) /
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LREM:
        ZERO_DIVISOR_CHECK(read((u8 *) & sp[-2]));
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) %
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LAND:
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) &
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LOR:
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) |
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LXOR:
        write((long long *) &sp[-sizeof(long long) / 4 * 2],
              read((long long *) &sp[-sizeof(long long) / 4 * 2]) ^
              read((long long *) &sp[-sizeof(long long) / 4]));
        sp -= sizeof(long long) / 4;
        pc += 1;
        break;
    case OPC_LSHL: {
        int shift = read(--sp) & 0x3f;
        write((long long *) &sp[-2], read((long long *) &sp[-2]) << shift);
        pc += 1;
        break;
    }
    case OPC_LSHR: {
        int shift = read(--sp) & 0x3f;
        write((long long *) &sp[-2],
              read((long long *) &sp[-2]) >> shift);
        pc += 1;
        break;
    };
    case OPC_LUSHR: {
        int shift = read(--sp) & 0x3f;
        write((unsigned long long *) &sp[-2],
              read((unsigned long long *) &sp[-2]) >> shift);
        pc += 1;
        break;
    };
    case OPC_FREM: {
        float v2 = read((float *)&sp[-1]);
        float v1 = read((float *)&sp[-2]);
        write((float *) &sp[-2], fmod(v1, v2));
        sp -= 1;
        pc += 1;
        break;
    }
    case OPC_DREM: {
        double v2 = read((double *)&sp[-2]);
        double v1 = read((double *)&sp[-4]);
        write((double *) &sp[-4], fmod(v1, v2));
        sp -= 2;
        pc += 1;
        break;
    }
    case OPC_LNEG:
        write((long long *) &sp[-sizeof(long long) / 4],
              -read((long long *) &sp[-sizeof(long long) / 4]));
        pc += 1;
        break;
    case OPC_FNEG:
        write((float *) &sp[-sizeof(float) / 4],
              -read((float *) &sp[-sizeof(float) / 4]));
        pc += 1;
        break;
    case OPC_DNEG:
        write((double *) &sp[-sizeof(double) / 4],
              -read((double *) &sp[-sizeof(double) / 4]));
        pc += 1; break;
    case OPC_I2L:
        sp -= 1;
        write((u8 *) sp, (int) read(sp));
        sp += 2;
        pc += 1;
        break;
    case OPC_L2I: {
        long long l;
        sp -= 2;
        l = read((long long *) sp);
        write(sp, (int) l);
        sp++;
        pc += 1;
        break;
    }
    case OPC_I2F:
        sp -= 1;
        write((float *) sp, (float) (int) read(sp));
        sp += sizeof(float) / 4;
        pc += 1;
        break;
    case OPC_I2D:
        sp -= 1;
        write((double *) sp, (double) (int) read(sp));
        sp += sizeof(double) / 4;
        pc += 1;
        break;
    case OPC_L2F: {
        long long v;
        sp -= sizeof(long long) / 4;
        v = read((long long *) sp);
        write((float *) sp,(float) v);
        sp += sizeof(float) / 4;
        pc += 1;
        break;
    }
    case OPC_L2D: {
        long long v;
        sp -= sizeof(long long) / 4;
        v = read((long long *) sp);
        write((double *) sp, (double) v);
        sp += sizeof(double) / 4;
        pc += 1;
        break;
    }
    case OPC_F2D: {
        float v;
        sp -= sizeof(float) / 4;
        v = read((float *) sp);
        write((double *) sp, (double) v);
        sp += sizeof(double) / 4;
        pc += 1;
        break;
    }
    case OPC_D2F: {
        double v;
        sp -= sizeof(double) / 4;
        v = read((double *) sp);
        write((float *) sp,(float) v);
        sp += sizeof(float) / 4;
        pc += 1;
        break;
    }
    case OPC_F2I: {
        int res;
        float value;
        sp -= sizeof(float) / 4;
        value = read((float *) sp);
        if (value >= (float) INT_MAX)
            res = INT_MAX;
        else
            if (value <= (float) INT_MIN)
                res = INT_MIN;
            else
                if (value != value)
                    res = 0;
                else
                    res = (int) value;
        write(sp, res);
        sp++;
        pc += 1;
        break;
    }
    case OPC_D2I: {
        int res;
        double value;
        sp -= sizeof(double) / 4;
        value = read((double *) sp);
        if (value >= (double) INT_MAX)
            res = INT_MAX;
        else
            if (value <= (double) INT_MIN)
                res = INT_MIN;
            else
                if (value != value)
                    res = 0;
                else
                    res = (int) value;
        write(sp, res);
        sp++;
        pc += 1;
        break;
    }
    case OPC_F2L: {
        long long res;
        float value;
        sp -= sizeof(float) / 4;
        value = read((float *) sp);
        if (value >= (float) LLONG_MAX)
            res = LLONG_MAX;
        else
            if (value <= (float) LLONG_MIN)
                res = LLONG_MIN;
            else
                if (value != value)
                    res = 0;
                else
                    res = (long long) value;
        write((u8 *) sp, res);
        sp += 2;
        pc += 1;
        break;
    }
    case OPC_D2L: {
        long long res;
        double value;
        sp -= sizeof(double) / 4;
        value = read((double *) sp);
        if (value >= (double) LLONG_MAX)
            res = LLONG_MAX;
        else
            if (value <= (double) LLONG_MIN)
                res = LLONG_MIN;
            else
                if (value != value)
                    res = 0;
                else
                    res = (long long) value;
        write((u8 *) sp, res);
        sp += 2;
        pc += 1;
        break;
    }
    case OPC_I2B: {
        signed char v = read(--sp) & 0xff;
        write(sp, v);
        sp++;
        pc += 1;
        break;
    }
    case OPC_I2C: {
        int v = read(--sp) & 0xffff;
        write(sp, v);
        sp++;
        pc += 1;
        break;
    }
    case OPC_I2S: {
        signed short v = read(--sp) & 0xffff;
        write(sp, (int) v);
        sp++;
        pc += 1;
        break;
    }
    case OPC_LCMP: {
        long long v2 = read((long long *) &sp[-2]);
        long long v1 = read((long long *) &sp[-4]);
        write(&sp[-4], (v1 == v2) ? 0 : ((v1 < v2) ? -1 : 1));
        sp -= 3;
        pc += 1;
        break;
    };
    case OPC_DCMPG: {
        int res;
        double v1, v2;
        sp -= sizeof(double) / 4;
        v2 = read((double *) sp);
        sp -= sizeof(double) / 4;
        v1 = read((double *) sp);
        if (v1 == v2)
            res = 0;
        else
            if (v1 < v2)
                res = -1;
            else
                if (v1 > v2)
                    res = 1;
                else
                    res = 1;
        write(sp, res);
        sp++;
        pc += 1;
        break;
    }
    case OPC_DCMPL: {
        int res;
        double v1, v2;
        sp -= sizeof(double) / 4;
        v2 = read((double *) sp);
        sp -= sizeof(double) / 4;
        v1 = read((double *) sp);
        if (v1 == v2)
            res = 0;
        else
            if (v1 < v2)
                res = -1;
            else
                if (v1 > v2)
                    res = 1;
                else
                    res = -1;
        write(sp, res);
        sp++;
        pc += 1;
        break;
    }
    case OPC_FCMPG: {
        int res;
        float v1, v2;
        sp -= sizeof(float) / 4;
        v2 = read((float *) sp);
        sp -= sizeof(float) / 4;
        v1 = read((float *) sp);
        if (v1 == v2)
            res = 0;
        else
            if (v1 < v2)
                res = -1;
            else
                if (v1 > v2)
                    res = 1;
                else
                    res = 1;
        write(sp, res);
        sp++;
        pc += 1;
        break;
    }
    case OPC_FCMPL: {
        int res;
        float v1, v2;
        sp -= sizeof(float) / 4;
        v2 = read((float *) sp);
        sp -= sizeof(float) / 4;
        v1 = read((float *) sp);
        if (v1 == v2)
            res = 0;
        else
            if (v1 < v2)
                res = -1;
            else
                if (v1 > v2)
                    res = 1;
                else
                    res = -1;
        write(sp, res);
        sp++;
        pc += 1;
        break;
    }
    case OPC_GOTO:
        pc += READ_S2_OP(pc);
        break;
    case OPC_JSR:
        write(sp++, (uintptr_t) pc);
        pc += READ_S2_OP(pc);
        break;
    case OPC_RET:
        //pc = (CodePntr) lvars[READ_U1_OP(pc)];
        pc = (CodePntr) read(&lvars[READ_U1_OP(pc)]);
        pc += 3;
        break;
    case OPC_LRETURN:
    case OPC_DRETURN: {
//         sp -= 2;
//         Frame* caller = frame->prev;
//         *(u8*)caller->osp = *(u8*)sp;
//         caller->osp += 2;
        len = 2;
    }
        //goto methodReturn;
        method_return;
    case OPC_ARRAYLENGTH: {
        //Object * array = (Object *) * --sp;
        //*sp = ARRAY_LEN(array);

        Object* array = (Object*)read(sp - 1);
        NULL_POINTER_CHECK(array);

        sp--;
        write(sp, ARRAY_LEN(array));
        sp++;
        pc += 1;
        //do_array_length(array);
        break;
    }
    case OPC_ATHROW: {
        //Object * obj = (Object *) sp[-1];
        Object * obj = (Object *) read(&sp[-1]);
        frame->last_pc = pc;
        NULL_POINTER_CHECK(obj);
        exception = obj;
        throw_exception;
        //        goto throwException;
    }
    case OPC_NEWARRAY: {
        int type = READ_U1_OP(pc);
        //int count = *--sp;
        int count = read(--sp);
        Object * obj;

        frame->last_pc = pc;
        if ((obj = allocTypeArray(type, count)) == NULL)
            throw_exception;
//             goto throwException;

        //*sp = (uintptr_t)obj;
        write(sp, (uintptr_t)obj);
        sp++;
        pc += 2;
        break;
    }
    case OPC_MONITORENTER: {
        //Object * obj = (Object *) * --sp;
        Object * obj = (Object *) read(--sp);
        NULL_POINTER_CHECK(obj);
        objectLock(obj);
        pc += 1;
        break;
    }
    case OPC_MONITOREXIT: {
        //Object * obj = (Object *) * --sp;
        Object * obj = (Object *) read(--sp);
        NULL_POINTER_CHECK(obj);
        objectUnlock(obj);
        pc += 1;
        break;
    }
    case OPC_LDC: {
        frame->last_pc = pc;
        resolveSingleConstant(mb->classobj, READ_U1_OP(pc));
        if (exception)
            throw_exception;
            //goto throwException;
        pc[0] = OPC_LDC_QUICK;
        break;
    }
    case OPC_LDC_W: {
        frame->last_pc = pc;
        resolveSingleConstant(mb->classobj, READ_U2_OP(pc));
        if (exception)
            throw_exception;
        //goto throwException;
        pc[0] = OPC_LDC_W_QUICK;
        break;
    }
    // case OPC_ALOAD_0: {
    //     if (mb->access_flags & ACC_STATIC)
    //         pc[0] = OPC_ILOAD_0;
    //     else
    //         pc[0] = OPC_ALOAD_THIS;
    //     break;
    // }
    case OPC_TABLESWITCH: {
        int *aligned_pc = (int *)((uintptr_t) (pc + 4) & ~0x3);
        int deflt = ntohl(aligned_pc[0]);
        int low = ntohl(aligned_pc[1]);
        int high = ntohl(aligned_pc[2]);
        int index = read(--sp);
        pc += (index < low
               || index >
               high) ? deflt : ntohl(aligned_pc[index - low + 3]);
        break;
    }
    case OPC_LOOKUPSWITCH: {
        int *aligned_pc = (int *)((uintptr_t) (pc + 4) & ~0x3);
        int deflt = ntohl(aligned_pc[0]);
        int npairs = ntohl(aligned_pc[1]);
        int key = read(--sp);
        int i;
        for (i = 2;
             (i < npairs * 2 + 2) && (key != (int)ntohl(aligned_pc[i]));
             i += 2);
        pc += i == npairs * 2 + 2 ? deflt : ntohl(aligned_pc[i + 1]);
        break;
    }
    case OPC_GETFIELD: {
        int idx;
        FieldBlock * fb;
        idx = READ_U2_OP(pc);
        if (pc[0] != OPC_GETFIELD)
            break;

        frame->last_pc = pc;
        fb = resolveField(mb->classobj, idx);
        if (exception)
            throw_exception;

        pc[0] = OPC_GETFIELD_QUICK_W;
//         if (fb->offset > 255)
//             pc[0] = OPC_GETFIELD_QUICK_W;
//         else {
//             pc[0] = OPC_LOCK;
//             MBARRIER();
//             pc[1] = fb->offset;
//             MBARRIER();
//             pc[0] = ((*fb->type == 'J') || (*fb->type == 'D') ?
//                      OPC_GETFIELD2_QUICK : OPC_GETFIELD_QUICK);
//         }
        break;
    }
    case OPC_GETFIELD_QUICK_W: {
        FieldBlock* fb = ((FieldBlock*)CP_INFO(cp, READ_U2_OP(pc)));
        //Object* obj = (Object*)read(--sp);
        Object* obj = (Object*)read(sp - 1);
        uintptr_t* addr;

        NULL_POINTER_CHECK(obj);
        addr = &(INST_DATA(obj)[fb->offset]);

        if ((*fb->type == 'J') || (*fb->type == 'D')) {
            do_get_field(obj, fb, addr, 2);
            break;
        }
        else {
            do_get_field(obj, fb, addr, 1);
            break;
        }
    }
//     case OPC_GETFIELD_QUICK: {
//         assert(false);
//         Object *obj = (Object *) read(--sp);
//         NULL_POINTER_CHECK(obj);
//         uintptr_t* addr = &INST_DATA(obj)[READ_U1_OP(pc)];
//         do_get_field(obj, addr, 1, 3);
//         break;
//     }
//     case OPC_GETFIELD2_QUICK: {
//         assert(false);
//         Object * obj = (Object *) read(--sp);
//         NULL_POINTER_CHECK(obj);
//         uintptr_t* addr = &INST_DATA(obj)[READ_U1_OP(pc)];
//         do_get_field(obj, addr, 2, 3);
//         break;
//     }
    case OPC_PUTFIELD: {
        int idx;
        FieldBlock * fb;
        idx = READ_U2_OP(pc);
        if (pc[0] != OPC_PUTFIELD)
            break;

        frame->last_pc = pc;
        fb = resolveField(mb->classobj, idx);

        if (exception)
            throw_exception;

        pc[0] = OPC_PUTFIELD_QUICK_W;
//         if (fb->offset > 255)
//             pc[0] = OPC_PUTFIELD_QUICK_W;
//         else {
//             pc[0] = OPC_LOCK;
//             MBARRIER();
//             pc[1] = fb->offset;
//             MBARRIER();
//             pc[0] = ((*fb->type == 'J') || (*fb->type == 'D') ?
//                      OPC_PUTFIELD2_QUICK : OPC_PUTFIELD_QUICK);
//         }
        break;
    }
    case OPC_PUTFIELD_QUICK_W: {
        FieldBlock *fb = ((FieldBlock *) CP_INFO(cp, READ_U2_OP(pc)));
        if ((*fb->type == 'J') || (*fb->type == 'D')) {
            Object *obj = (Object *) read(&sp[-3]);
            NULL_POINTER_CHECK(obj);
            uintptr_t* addr = &INST_DATA(obj)[fb->offset];
            do_put_field(obj, fb, addr, 2);
        }
        else {
            Object *obj = (Object *) read(&sp[-2]);
            NULL_POINTER_CHECK(obj);
            uintptr_t* addr = &INST_DATA(obj)[fb->offset];
            do_put_field(obj, fb, addr, 1);
        }
        break;
    }
//     case OPC_PUTFIELD_QUICK: {
//         assert(false);
//         Object *obj = (Object *) read(&sp[-2]);
//         NULL_POINTER_CHECK(obj);
//         uintptr_t* addr = &INST_DATA(obj)[READ_U1_OP(pc)];
//         do_put_field(obj, addr, 1, 3);
//         break;
//     }
//     case OPC_PUTFIELD2_QUICK: {
//         assert(false);
//         Object *obj = (Object *) read(&sp[-3]);
//         NULL_POINTER_CHECK(obj);
//         uintptr_t* addr = &INST_DATA(obj)[READ_U1_OP(pc)];
//         do_put_field(obj, addr, 2, 3);
//         break;
//     }

    case OPC_GETSTATIC: {
        FieldBlock * fb;
        frame->last_pc = pc;
        fb = resolveField(mb->classobj, READ_U2_OP(pc));
        if (exception)
            throw_exception;
        //goto throwException;
        if ((*fb->type == 'J') || (*fb->type == 'D'))
            pc[0] = OPC_GETSTATIC2_QUICK;
        else
            pc[0] = OPC_GETSTATIC_QUICK;
        break;
    }
    case OPC_GETSTATIC_QUICK: {
        FieldBlock* fb = ((FieldBlock*)CP_INFO(cp, READ_U2_OP(pc)));
        uintptr_t* addr = &fb->static_value;
        do_get_field(fb->classobj, fb, addr, 1, true);
        break;
    }
    case OPC_GETSTATIC2_QUICK: {
        FieldBlock* fb = ((FieldBlock*)CP_INFO(cp, READ_U2_OP(pc)));
        uintptr_t* addr = &fb->static_value;
        do_get_field(fb->classobj, fb, addr, 2, true);
        break;
    }
    case OPC_PUTSTATIC: {
        FieldBlock * fb;
        frame->last_pc = pc;
        fb = resolveField(mb->classobj, READ_U2_OP(pc));
        //assert(fb->offset == 0);
        if (exception)
            throw_exception;
        //goto throwException;
        if ((*fb->type == 'J') || (*fb->type == 'D'))
            pc[0] = OPC_PUTSTATIC2_QUICK;
        else
            pc[0] = OPC_PUTSTATIC_QUICK;
        break;
    }
    case OPC_PUTSTATIC_QUICK: {
        FieldBlock *fb = ((FieldBlock *) CP_INFO(cp, READ_U2_OP(pc)));
        uintptr_t* addr = &fb->static_value;
        do_put_field(fb->classobj, fb, addr, 1, true);
        break;
    }
    case OPC_PUTSTATIC2_QUICK: {
        FieldBlock *fb = ((FieldBlock *) CP_INFO(cp, READ_U2_OP(pc)));
        uintptr_t* addr = &fb->static_value;
        do_put_field(fb->classobj, fb, addr, 2, true);
        break;
    }

    case OPC_INVOKEINTERFACE: {
        frame->last_pc = pc;
        new_mb = resolveInterfaceMethod(mb->classobj, READ_U2_OP(pc));

        if (exception)
            throw_exception;
        //goto throwException;

        if (CLASS_CB(new_mb->classobj)->access_flags & ACC_INTERFACE)
            pc[0] = OPC_INVOKEINTERFACE_QUICK;
        else {
            pc[3] = pc[4] = OPC_NOP;
            pc[0] = OPC_INVOKEVIRTUAL;
        }

        break;

    }

    case OPC_INVOKEINTERFACE_QUICK: {
        int mtbl_idx;
        ClassBlock * cb;
        int cache = READ_U1_OP(pc + 3);

        new_mb = (MethodBlock *) CP_INFO(cp, READ_U2_OP(pc));
        arg1 = sp - new_mb->args_count;

        //NULL_POINTER_CHECK(*arg1);
        NULL_POINTER_CHECK(read(arg1));

        //cb = CLASS_CB(new_class = (*(Object**) arg1)->classobj);
        cb = CLASS_CB(new_class = (read((Object**)arg1))->classobj);

        if ((cache >= cb->imethod_table_size) ||
            (new_mb->classobj != cb->imethod_table[cache].interface)) {
            for (cache = 0; (cache < cb->imethod_table_size) &&
                     (new_mb->classobj != cb->imethod_table[cache].interface); cache++);

            if (cache == cb->imethod_table_size)
                THROW_EXCEPTION(java_lang_IncompatibleClassChangeError,
                                "unimplemented interface");

            READ_U1_OP(pc + 3) = cache;
        }

        mtbl_idx = cb->imethod_table[cache].offsets[new_mb->method_table_index];
        new_mb = cb->method_table[mtbl_idx];

        //        goto invokeMethod;
        //invokeMethod(new_mb, small_mb, arg1, ret);
        //break;
        invoke_method;
    }

	case OPC_INVOKEVIRTUAL: {
        int idx;
        idx = READ_U2_OP(pc);
        if (pc[0] != OPC_INVOKEVIRTUAL)
            break;
        frame->last_pc = pc;
        new_mb = resolveMethod(mb->classobj, idx);
        if (exception)
            throw_exception;

        // soot generate invokevirtual for private method
        if (new_mb->is_private()) {
            pc[0] = OPC_INVOKESPECIAL;
            break;
        }

        if ((new_mb->args_count < 256) && (new_mb->method_table_index < 256)) {
            pc[0] = OPC_LOCK;
            MBARRIER();
            pc[1] = new_mb->method_table_index;
            pc[2] = new_mb->args_count;
            MBARRIER();
            pc[0] = OPC_INVOKEVIRTUAL_QUICK; // OPC_INVOKEVIRTUAL_QUICK的解释过程：从this找到method_table，即vtable
        }
        else
            pc[0] = OPC_INVOKEVIRTUAL_QUICK_W;

        break;
    }

	case OPC_INVOKEVIRTUAL_QUICK: {
        arg1 = sp - READ_U1_OP(pc + 1);
        //NULL_POINTER_CHECK(*arg1);
        NULL_POINTER_CHECK(read(arg1));
        //new_class = (*(Object **) arg1)->classobj;
        new_class = (read((Object **) arg1))->classobj;
        new_mb = CLASS_CB(new_class)->method_table[READ_U1_OP(pc)];
        invoke_method;
    }

	case OPC_INVOKEVIRTUAL_QUICK_W: {
        new_mb = ((MethodBlock *) CP_INFO(cp, READ_U2_OP(pc)));
        arg1 = sp - (new_mb->args_count);
//         NULL_POINTER_CHECK(*arg1);
//         new_class = (*(Object **) arg1)->classobj;
        NULL_POINTER_CHECK(read(arg1));
        new_class = (read((Object **) arg1))->classobj;
        new_mb = CLASS_CB(new_class)->method_table[new_mb->method_table_index];
        invoke_method;
    }


    case OPC_INVOKESPECIAL: {
        int idx;
        idx = READ_U2_OP(pc);
        if (pc[0] != OPC_INVOKESPECIAL)
            break;

        frame->last_pc = pc;
        new_mb = resolveMethod(mb->classobj, idx);

        if (exception)
            throw_exception;
        //goto throwException;

        /* Check if invoking a super method... */
        if ((CLASS_CB(mb->classobj)->access_flags & ACC_SUPER) &&
            ((new_mb->access_flags & ACC_PRIVATE) == 0) && (new_mb->name[0] != '<')) {
            pc[0] = OPC_LOCK;
            MBARRIER();
            pc[1] = new_mb->method_table_index >> 8;
            pc[2] = new_mb->method_table_index & 0xff;
            MBARRIER();
            pc[0] = OPC_INVOKESUPER_QUICK;
        }
        else
            pc[0] = OPC_INVOKENONVIRTUAL_QUICK;

        break;
    }

    case OPC_INVOKESUPER_QUICK: {
        new_mb = CLASS_CB(CLASS_CB(mb->classobj)->super)->method_table[READ_U2_OP(pc)];
        arg1 = sp - (new_mb->args_count);
        //NULL_POINTER_CHECK(*arg1);
        NULL_POINTER_CHECK(read(arg1));
        //        goto invokeMethod;
        //invokeMethod(new_mb, small_mb, arg1, ret);
        //break;
        invoke_method;

    }
    case OPC_INVOKENONVIRTUAL_QUICK: {
        new_mb = RESOLVED_METHOD(pc);
        arg1 = sp - (new_mb->args_count);
        //NULL_POINTER_CHECK(*arg1);
        NULL_POINTER_CHECK(read(arg1));
        //        goto invokeMethod;
        //        invokeMethod(new_mb, small_mb, arg1, ret);
        //break;
        invoke_method;

    }

    case OPC_INVOKESTATIC: {
        frame->last_pc = pc;
        new_mb = resolveMethod(mb->classobj, READ_U2_OP(pc));

        if (exception)
            throw_exception;

        pc[0] = OPC_INVOKESTATIC_QUICK;
        break;

    }

	case OPC_INVOKESTATIC_QUICK: {
        new_mb = ((MethodBlock *) CP_INFO(cp, READ_U2_OP(pc)));
        //arg1 = sp - new_mb->args_count;
        arg1 = (uintptr_t*)&new_mb->classobj;
        //invoke_method;
        return do_invoke_method(new_mb->classobj, new_mb);
        break;
    }


    case OPC_ANEWARRAY: {
        frame->last_pc = pc;
        resolveClass(mb->classobj, READ_U2_OP(pc), FALSE);

        if (exception)
            throw_exception;
        //goto throwException;

        pc[0] = OPC_ANEWARRAY_QUICK;
        break;
    }
    case OPC_CHECKCAST: {
        frame->last_pc = pc;
        resolveClass(mb->classobj, READ_U2_OP(pc), FALSE);

        if (exception)
            throw_exception;
        //goto throwException;

        pc[0] = OPC_CHECKCAST_QUICK;
        break;
    }
    case OPC_INSTANCEOF: {
        frame->last_pc = pc;
        resolveClass(mb->classobj, READ_U2_OP(pc), FALSE);

        if (exception)
            throw_exception;
        //goto throwException;

        pc[0] = OPC_INSTANCEOF_QUICK;
        break;
    }
    case OPC_MULTIANEWARRAY: {
        frame->last_pc = pc;
        resolveClass(mb->classobj, READ_U2_OP(pc), FALSE);

        if (exception)
            throw_exception;
        //goto throwException;
        pc[0] = OPC_MULTIANEWARRAY_QUICK;
        break;
    }
    case OPC_NEW: {
        Class * classobj;
        ClassBlock * cb;

        frame->last_pc = pc;
        classobj = resolveClass(mb->classobj, READ_U2_OP(pc), TRUE);

        if (exception)
            throw_exception;
        //goto throwException;

        cb = CLASS_CB(classobj);
        if (cb->access_flags & (ACC_INTERFACE | ACC_ABSTRACT)) {
            signalException(java_lang_InstantiationError, cb->name);
            throw_exception;
            //goto throwException;
        }

        pc[0] = OPC_NEW_QUICK;
        break;
    }

    case OPC_WIDE: {
        int opcode = pc[1];
        switch (opcode) {
        case OPC_ILOAD:
        case OPC_FLOAD:
        case OPC_ALOAD:
            write(sp++, read(&lvars[READ_U2_OP(pc + 1)]));
            pc += 4;
            break;
        case OPC_LLOAD:
        case OPC_DLOAD:
            write((u8 *) sp, read((u8 *) (&lvars[READ_U2_OP(pc + 1)])));
            sp += 2;
            pc += 4;
            break;
        case OPC_ISTORE:
        case OPC_FSTORE:
        case OPC_ASTORE:
            write(&lvars[READ_U2_OP(pc + 1)], read(--sp));
            pc += 4;
            break;
        case OPC_LSTORE:
        case OPC_DSTORE:
            sp -= 2;
            write((u8 *) (&lvars[READ_U2_OP(pc + 1)]), read((u8 *) sp));
            pc += 4;
            break;
        case OPC_RET:
            pc = (unsigned char *) read(&lvars[READ_U2_OP((pc + 1))]);
            break;
        case OPC_IINC: {
            uintptr_t v = read(&lvars[READ_U2_OP(pc + 1)]);
            write(&lvars[READ_U2_OP(pc + 1)], v + READ_S2_OP(pc + 3));
            pc += 6;
        }
            break;
        }
        break;
    }

    case OPC_GOTO_W: {
        pc += READ_S4_OP(pc);
        break;
    }
    case OPC_JSR_W: {
        write(sp, (uintptr_t) pc + 2);
        sp++;
        pc += READ_S4_OP(pc);
        break;
    }
    case OPC_LOCK: {
        break;
    }
    case OPC_NEW_QUICK: {
        //before_alloc_object();

        Class* classobj = RESOLVED_CLASS(pc);
        Object* obj;

        frame->last_pc = pc;
        if ((obj = allocObject(classobj)) == NULL)
            throw_exception;

        //after_alloc_object(obj);

        write(sp, (uintptr_t)obj);
        sp++;
        pc += 3;

        return;
    }
    case OPC_ANEWARRAY_QUICK: {
        Class* classobj = RESOLVED_CLASS(pc);
        const char* name = CLASS_CB(classobj)->name;
        int count = read(--sp);
        Class* array_class;
        char* ac_name;
        Object* obj;

        frame->last_pc = pc;

        if (count < 0) {
            signalException(java_lang_NegativeArraySizeException, NULL);
            throw_exception;
            //goto throwException;
        }

        ac_name = (char*) sysMalloc(strlen(name) + 4);

        if (name[0] == '[')
            strcat(strcpy(ac_name, "["), name);
        else
            strcat(strcat(strcpy(ac_name, "[L"), name), ";");

        array_class = findArrayClassFromClass(ac_name, mb->classobj);
        free(ac_name);

        if (exception)
            throw_exception;
        //goto throwException;

        if ((obj = allocArray(array_class, count, sizeof(Object *))) == NULL)
            throw_exception;
        //goto throwException;

        write(sp, (uintptr_t)obj);
        sp++;
        pc += 3;
        break;
    }
    case OPC_CHECKCAST_QUICK: {
        Class* classobj = RESOLVED_CLASS(pc);
        Object* obj = (Object *) read(&sp[-1]);

        if ((obj != NULL) && !isInstanceOf(classobj,obj->classobj))
            THROW_EXCEPTION(java_lang_ClassCastException, CLASS_CB(obj->classobj)->name);

        pc += 3;
        break;
    }
    case OPC_INSTANCEOF_QUICK: {
        Class* classobj = RESOLVED_CLASS(pc);
        Object * obj = (Object *) read(&sp[-1]);

        if (obj != NULL)
            write(&sp[-1], isInstanceOf(classobj, obj->classobj));

        pc += 3;
        break;
    }
    case OPC_MULTIANEWARRAY_QUICK: {
        Class* classobj = RESOLVED_CLASS(pc);
        int i, dim = READ_U1_OP(pc + 2);
        Object * obj;

        sp -= dim;
        frame->last_pc = pc;

        std::vector<intptr_t> counts(dim);
        for (i = 0; i < dim; i++)
            if ((counts[i] = (intptr_t) read(&sp[i])) < 0) {
                signalException(java_lang_NegativeArraySizeException, NULL);
                throw_exception;
                //goto throwException;
            }

        if ((obj = allocMultiArray(classobj, dim, &counts[0])) == NULL)
            throw_exception;
        //goto throwException;

        write(sp, (uintptr_t)obj);
        sp++;
        pc += 4;
        break;
    }
        /* Special bytecode which forms the body of an abstract method.
           If it is invoked it'll throw an abstract method exception. */

    case OPC_ABSTRACT_METHOD_ERROR: {
        /* As the method has been invoked, a frame will exist for
           the abstract method itself.  Pop this to get the correct
           exception stack trace. */
        assert(false);
        //ee->last_frame = frame->prev;
        frame = frame->prev;

        /* Throw the exception */
        signalException(java_lang_AbstractMethodError, mb->name);
        throw_exception;
        //goto throwException;
    }

    } // switch

    //return 0;
}



void initialiseInterpreter(InitArgs * args)
{
}
