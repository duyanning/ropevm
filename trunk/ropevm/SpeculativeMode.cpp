#include "std.h"
#include "rope.h"
#include "SpeculativeMode.h"
#include "SpmtThread.h"

#include "lock.h"

#include "Message.h"
#include "RopeVM.h"
#include "interp.h"
#include "MiniLogger.h"
#include "Loggers.h"
#include "Snapshot.h"
#include "Helper.h"
#include "frame.h"
#include "Effect.h"

using namespace std;


SpeculativeMode::SpeculativeMode()
:
    UncertainMode("Speculative mode")
{
}

uint32_t
SpeculativeMode::mode_read(uint32_t* addr)
{
    return m_spmt_thread->m_state_buffer.read(addr);
}

void
SpeculativeMode::mode_write(uint32_t* addr, uint32_t value)
{
    m_spmt_thread->m_state_buffer.write(addr, value);
}


// void
// SpeculativeMode::step()
// {
//     assert(RopeVM::do_spec);

//     Message* msg = m_spmt_thread->get_certain_msg();
//     if (msg) {
//         process_certain_message(msg);
//         return;
//     }

//     if (m_spmt_thread->m_is_waiting_for_task) {
//         MINILOG(task_load_logger, "#" << m_spmt_thread->id() << " is waiting for task");
//         //assert(false);

//         // if (not m_spmt_thread->from_certain_to_spec)
//         //     m_spmt_thread->snapshot();
//         m_spmt_thread->process_next_spec_msg();

//         // if (not m_spmt_thread->m_is_waiting_for_task) {
//         //     exec_an_instr();
//         // }
//     }

//     if (not m_spmt_thread->is_halt())
//         exec_an_instr();
// }


void
SpeculativeMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    if (intercept_vm_backdoor(target_object, new_mb)) return;

    //log_when_invoke_return(true, m_user, frame->mb, target_object, new_mb);

    MINILOG(s_logger, "#" << m_spmt_thread->id()
            << " (S) is to invoke method: " << *new_mb);

    if (is_priviledged(new_mb)) {
        MINILOG(s_logger, "#" << m_spmt_thread->id()
                << " (S) " << *new_mb << "is native/sync method");
        m_spmt_thread->sleep();
        return;
    }

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();

    if (target_spmt_thread == m_spmt_thread) {

        sp -= new_mb->args_count;

        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }

        Frame* new_frame = create_frame(target_object, new_mb, &args[0],
                                        m_spmt_thread, pc, frame, sp);

        frame->last_pc = pc;

        frame = new_frame;
        sp = (uintptr_t*)frame->ostack_base;
        pc = (CodePntr)frame->mb->code;

    }
    else {

        // pop up arguments
        sp -= new_mb->args_count;

        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }


        MINILOG(s_logger, "#" << m_spmt_thread->m_id
                << " (S) target thread is #" << target_spmt_thread->id());


        // construct speculative message and send
        InvokeMsg* msg = new InvokeMsg(m_spmt_thread, target_spmt_thread,
                                       target_object, new_mb, &args[0],
                                       pc, frame, sp);

        m_spmt_thread->send_msg(msg);




        // invoke rvp-method
        MethodBlock* rvp_method = ::get_rvp_method(new_mb);

        MINILOG(s_logger, "#" << m_spmt_thread->m_id
                << " (S)invokes rvp-method: " << *rvp_method);

        Frame* rvp_frame = m_spmt_thread->m_rvp_mode.create_frame(target_object,
                                                                  rvp_method,
                                                                  &args[0],
                                                                  0,
                                                                  pc,
                                                                  frame,
                                                                  sp);

        m_spmt_thread->m_rvp_mode.pc = (CodePntr)rvp_method->code;
        m_spmt_thread->m_rvp_mode.frame = rvp_frame;
        m_spmt_thread->m_rvp_mode.sp = rvp_frame->ostack_base;

        m_spmt_thread->switch_to_rvp_mode();

    }
}


void
SpeculativeMode::do_method_return(int len)
{
    assert(len == 0 || len == 1 || len == 2);
    assert(not is_priviledged(frame->mb));

    Frame* current_frame = frame;

    MINILOG(s_logger, "#" << m_spmt_thread->id()
            << " (S) is to return from " << *frame);



    SpmtThread* target_spmt_thread = current_frame->caller;

    if (target_spmt_thread == m_spmt_thread) {

        sp -= len;
        uintptr_t* caller_sp = current_frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            write(caller_sp++, read(&sp[i]));
        }


        frame = current_frame->prev;
        sp = caller_sp;
        pc = current_frame->caller_pc;

        destroy_frame(current_frame);
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

    }
    else {

        sp -= len;
        std::vector<uintptr_t> ret_val;
        for (int i = 0; i < len; ++i) {
            ret_val.push_back(read(&sp[i]));
        }

        // construct speculative message
        ReturnMsg* msg = new ReturnMsg(target_spmt_thread,
                                       &ret_val[0], len,
                                       current_frame->caller_pc,
                                       current_frame->prev,
                                       current_frame->caller_sp);

        // record spec msg sent in current effect
        m_spmt_thread->m_current_spec_msg->get_effect()->msg_sent = msg;

        destroy_frame(current_frame);

        m_spmt_thread->launch_next_spec_msg();

    }
}

void
SpeculativeMode::before_signal_exception(Class *exception_class)
{
    MINILOG(s_exception_logger, "#" << m_spmt_thread->id()
            << " (S) exception detected!!! " << exception_class->name());
    m_spmt_thread->sleep();
    throw DeepBreak();
}

void
SpeculativeMode::do_throw_exception()
{
    assert(false);
}


Frame*
SpeculativeMode::create_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                              SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp)

{
    Frame* new_frame = g_create_frame(object, new_mb, args, caller, caller_pc, caller_frame, caller_sp);
    // record new_frame in effect
    return new_frame;
}


void
SpeculativeMode::destroy_frame(Frame* frame)
{
    if (not frame->pinned) {
        MINILOG(s_destroy_frame_logger, "#" << m_spmt_thread->id()
                << " (S) destroy frame " << frame << " for " << *frame->mb);
        if (m_spmt_thread->m_id == 5 && strcmp(frame->mb->name, "simulate") == 0) {
            int x = 0;
            x++;
        }
        // m_spmt_thread->m_state_buffer.clear(frame->lvars, frame->lvars + frame->mb->max_locals);
        // m_spmt_thread->m_state_buffer.clear(frame->ostack_base, frame->ostack_base + frame->mb->max_stack);
        m_spmt_thread->clear_frame_in_states_buffer(frame);
        //delete frame;
        Mode::destroy_frame(frame);
    }
}

void
SpeculativeMode::do_get_field(Object* target_object, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger, "#" << m_spmt_thread->id()
            << " (S) is to getfield: " << *fb
            //            << " of " << static_cast<void*>(target_object));
            << " of " << target_object);


    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    if (target_spmt_thread == m_spmt_thread) {

        sp -= is_static ? 0 : 1;
        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }
        pc += 3;

    }
    else {
        sp -= is_static ? 0 : 1;

        GetMsg* get_msg = new GetMsg(m_spmt_thread, target_spmt_thread,
                                     target_object, fb);

        MINILOG(s_logger, "#" << m_spmt_thread->id()
                << " (S) add get msg to #"
                << target_spmt_thread->id() << ": " << *get_msg);

        m_spmt_thread->send_msg(get_msg);


        // 直接从稳定内存取数，构造推测性的get_return消息并使用
        vector<uintptr_t> value;
        for (int i = 0; i < size; ++i) {
            value.push_back(addr[i]);
        }
        GetReturnMsg* get_return_msg = new GetReturnMsg(target_spmt_thread,
                                                        &value[0], value.size());
        m_spmt_thread->launch_spec_msg(get_return_msg);

    }

}

void
SpeculativeMode::do_put_field(Object* target_object, FieldBlock* fb,
                              uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger, "#" << m_spmt_thread->id()
            << " (S) is to put: " << *fb);

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    if (target_spmt_thread == m_spmt_thread) {
        sp -= size;
        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }
        sp -= is_static ? 0 : 1;
        pc += 3;
    }
    else {
        sp -= size;

        vector<uintptr_t> val;
        for (int i = 0; i < size; ++i) {
            val.push_back(read(sp + i));
        }

        sp -= is_static ? 0 : 1;

        PutMsg* put_msg = new PutMsg(m_spmt_thread, target_spmt_thread,
                                     target_object, fb, &val[0]);

        MINILOG(s_logger, "#" << m_spmt_thread->id()
                << " (S) add put msg to #"
                << target_spmt_thread->id() << ": " << *put_msg);

        m_spmt_thread->send_msg(put_msg);


        // 构造推测性的put_return消息并使用
        PutReturnMsg* put_return_msg = new PutReturnMsg(m_spmt_thread);
        m_spmt_thread->launch_spec_msg(put_return_msg);

    }
}

void
SpeculativeMode::do_array_load(Object* array, int index, int type_size)
{
    MINILOG(s_logger, "#" << m_spmt_thread->id()
            << " (S) is to load from array");

    Object* target_object = array;

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);


    //void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value

    if (target_spmt_thread == m_spmt_thread) {
        sp -= 2; // pop up arrayref and index
        load_from_array(sp, array, index, type_size);
        sp += nslots;

        pc += 1;

    }
    else {

        sp -= 2; // pop up arrayref and index
        ArrayLoadMsg* msg = new ArrayLoadMsg(m_spmt_thread, target_spmt_thread,
                                             array, type_size, index);

        MINILOG(s_logger, "#" << m_spmt_thread->id()
                << " (S) add arrayload msg to #"
                << target_spmt_thread->id() << ": " << *msg);

        m_spmt_thread->send_msg(msg);


        // 直接从稳定内存取数，构造推测性的arrayload_return消息并使用
        vector<uintptr_t> value;
        g_load_from_stable_array_to_c(&value[0], array, index, type_size);
        ArrayLoadReturnMsg* arrayload_return_msg = new ArrayLoadReturnMsg(m_spmt_thread,
                                                                          &value[0], value.size());
        m_spmt_thread->launch_spec_msg(arrayload_return_msg);

    }

}

void
SpeculativeMode::do_array_store(Object* array, int index, int type_size)
{
    MINILOG(s_logger, "#" << m_spmt_thread->id()
            << " (S) is to store to array");


    Object* target_object = array;

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    //void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value


    if (target_spmt_thread == m_spmt_thread) {
        sp -= nslots; // pop up value
        store_to_array(sp, array, index, type_size);
        sp -= 2;                    // pop arrayref and index
        pc += 1;

    }
    else {

        sp -= nslots; // pop up value

        uint32_t val[2]; // at most 2 slots
        for (int i = 0; i < nslots; ++i) {
            val[i] = read(sp + i);
        }

        sp -= 2;

        ArrayStoreMsg* msg = new ArrayStoreMsg(m_spmt_thread, target_spmt_thread,
                                               array,
                                               type_size, index, &val[0]);

        MINILOG(s_logger, "#" << m_spmt_thread->id()
                << " (S) add arraystore task to #"
                << target_spmt_thread->id() << ": " << *msg);

        m_spmt_thread->send_msg(msg);

        ArrayStoreReturnMsg* array_store_return_msg = new ArrayStoreReturnMsg(m_spmt_thread);
        m_spmt_thread->launch_spec_msg(array_store_return_msg);

    }


}

void*
SpeculativeMode::do_execute_method(Object* target_object,
                                   MethodBlock *mb,
                                   std::vector<uintptr_t>& jargs)
{
    MINILOG(step_loop_in_out_logger, "#" << m_spmt_thread->id()
            << " (S) throw-> to be execute java method: " << *mb);
    m_spmt_thread->sleep();

    throw DeepBreak();

    return 0;
}




void
SpeculativeMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(s_invoke_return_logger, is_invoke, m_spmt_thread->id(), "(S)",
                      caller, caller_mb, callee, callee_mb);
}


void
SpeculativeMode::send_msg(Message* msg)
{
    Effect* current_effect = m_spmt_thread->m_current_spec_msg->get_effect();
    current_effect->msg_sent = msg;

    if (g_is_async_msg(msg)) {
        msg->get_target_spmt_thread()->add_spec_msg(msg);
    }
}


void
SpeculativeMode::invoke_impl(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                             SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp)
{
    Frame* new_frame = create_frame(target_object,
                                    new_mb,
                                    args,
                                    caller,
                                    caller_pc,
                                    caller_frame,
                                    caller_sp);

    pc = (CodePntr)new_frame->mb->code;
    frame = new_frame;
    sp = (uintptr_t*)new_frame->ostack_base;
}
