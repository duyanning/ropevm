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
#include "Group.h"
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

    SpmtThread* target_spmt_thread = target_object->get_group()->get_spmt_thread();

    if (target_spmt_thread == m_spmt_thread) {

        sp -= new_mb->args_count;

        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }

        Frame* new_frame = create_frame(target_object, new_mb, &args[0], m_spmt_thread, pc, frame, sp);

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
        InvokeMsg* msg = new InvokeMsg(m_spmt_thread,
                                       target_object,
                                       new_mb,
                                       &args[0]);

        m_spmt_thread->send_spec_msg(target_spmt_thread, msg);




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

    MINILOG(s_logger,
            "#" << m_spmt_thread->id() << " (S) is to return from " << *frame);

    // // top frame' caller is native code, so we sleep
    // if (frame->is_top_frame()) {
    //     MINILOG(s_logger,
    //             "#" << m_spmt_thread->id() << " (S) " << *frame->mb << " is a top frame");
    //     m_spmt_thread->sleep();
    //     return;
    // }



    Object* target_object = frame->calling_object;
    SpmtThread* target_spmt_thread = target_object->get_group()->get_spmt_thread();

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
        ReturnMsg* msg = new ReturnMsg(current_frame->get_object(),
                                       target_object,
                                       &ret_val[0], len);

        // record spec msg sent in current effect
        m_spmt_thread->current_spec_msg()->get_effect()->msg_sent = msg;

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
        // m_spmt_thread->m_states_buffer.clear(frame->lvars, frame->lvars + frame->mb->max_locals);
        // m_spmt_thread->m_states_buffer.clear(frame->ostack_base, frame->ostack_base + frame->mb->max_stack);
        m_spmt_thread->clear_frame_in_states_buffer(frame);
        //delete frame;
        Mode::destroy_frame(frame);
    }
}

void
SpeculativeMode::do_get_field(Object* source_object, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_spmt_thread->id() << " (S) is to getfield: " << *fb
            //            << " of " << static_cast<void*>(target_object));
            << " of " << source_object);

    Object* current_object = frame->get_object();

    Group* current_group = current_object->get_group();
    Group* source_group = source_object->get_group();

    if (source_group == current_group) {

        sp -= is_static ? 0 : 1;
        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }
        pc += 3;

    }
    else {

        m_spmt_thread->snapshot(1==1);

        // read all at one time, to avoid data change between two reading.
        vector<uintptr_t> val;
        for (int i = 0; i < size; ++i) {
            val.push_back(addr[i]);
        }

        sp -= is_static ? 0 : 1;

        GetMsg* msg = new GetMsg(m_spmt_thread, source_object, fb, &val[0], size, frame, sp, pc);
        m_spmt_thread->add_message_to_be_verified(msg);

        for (int i = 0; i < size; ++i) {
            write(sp, val[i]);
            sp++;
        }
        pc += 3;

    }

}

void
SpeculativeMode::do_put_field(Object* target_object, FieldBlock* fb,
                              uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_spmt_thread->id() << " (S) is to putfield: " << *fb);


    Object* current_object = frame->get_object();

    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    if (target_group == current_group) {

        sp -= size;
        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }
        sp -= is_static ? 0 : 1;
        pc += 3;

    }
    else {

        m_spmt_thread->snapshot(1==1);

        if (target_group->can_speculate()) {

            SpmtThread* target_core = target_group->get_core();
            sp -= size;

            vector<uintptr_t> val;
            for (int i = 0; i < size; ++i) {
                val.push_back(read(sp + i));
            }

            sp -= is_static ? 0 : 1;
            pc += 3;

            MINILOG(s_logger,
                     "#" << m_spmt_thread->id() << " (S) target object has a core #"
                     << target_core->id());

            PutMsg* msg = new PutMsg(m_spmt_thread, target_object, fb, addr, &val[0], size, is_static);

            MINILOG(s_logger,
                     "#" << m_spmt_thread->id() << " (S) add putfield task to #"
                    << target_core->id() << ": " << *msg);

            m_spmt_thread->send_spec_msg(target_core, msg);
        }
        else {

            MINILOG(s_logger,
                    "#" << m_spmt_thread->id() << " target object is coreless");
        }

    }
}

void
SpeculativeMode::do_array_load(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_spmt_thread->id() << " (S) is to load from array");

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value
    assert(array != get_group()->get_leader()); // array never can be leader

    Object* source_object = array;
    Group* source_group = source_object->get_group();
    Object* current_object = frame->get_object();
    Group* current_group = current_object->get_group();

    if (source_group == current_group) {

        sp -= 2; // pop up arrayref and index
        load_from_array(sp, addr, type_size);
        sp += nslots;

        pc += 1;

    }
    else {

        m_spmt_thread->snapshot(1==1);

        // read all at one time, to avoid data change between two reading.
        uint8_t val[8]; // longest value is 8 bytes
        std::copy((uint8_t*)addr, (uint8_t*)addr + type_size, val); // get from certain memory

        sp -= 2;

        ArrayLoadMsg* msg = new ArrayLoadMsg(m_spmt_thread, array, index, &val[0], type_size, frame, sp, pc);
        m_spmt_thread->add_message_to_be_verified(msg);

        load_array_from_no_cache_mem(sp, &val[0], type_size);
        sp += nslots;

        pc += 1;
    }

}

void
SpeculativeMode::do_array_store(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_spmt_thread->id() << " (S) is to store to array");

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value
    assert(array != get_group()->get_leader()); // array never can be leader

    Object* target_object = array;
    Group* target_group = target_object->get_group();
    Object* current_object = frame->get_object();
    Group* current_group = current_object->get_group();

    if (target_group == current_group) {

        sp -= nslots; // pop up value
        store_to_array(sp, addr, type_size);
        sp -= 2;                    // pop arrayref and index

        pc += 1;

    }
    else {

        m_spmt_thread->snapshot(true);

        if (target_group->can_speculate()) {      // target_object has a core
            SpmtThread* target_core = target_group->get_core();

            sp -= nslots; // pop up value

            uint32_t val[2]; // at most 2 slots
            for (int i = 0; i < nslots; ++i) {
                val[i] = read(sp + i);
            }

            sp -= 2;
            pc += 1;

            MINILOG(s_logger,
                     "#" << m_spmt_thread->id() << " (S) target object has a core #"
                     << target_core->id());

            ArrayStoreMsg* msg =
                new ArrayStoreMsg(m_spmt_thread, target_object, index, &val[0], nslots, type_size);

            MINILOG(s_logger,
                     "#" << m_spmt_thread->id() << " (S) add arraystore task to #"
                    << target_core->id() << ": " << *msg);

            m_spmt_thread->send_spec_msg(target_core, msg);
        }
        else {                  // target_object is a coreless object
            MINILOG(s_logger,
                    "#" << m_spmt_thread->id() << " target object is coreless");
        }

        // AckMsg* ack = new AckMsg;
        // m_spmt_thread->add_message_to_be_verified(ack);
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
