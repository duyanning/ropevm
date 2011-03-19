#include "std.h"
#include "jam.h"
#include "RvpMode.h"
#include "Core.h"


#include "lock.h"

#include "Message.h"
#include "OoSpmtJvm.h"
#include "interp.h"
#include "Loggers.h"
#include "frame.h"
#include "DebugScaffold.h"
#include "Helper.h"

using namespace std;

RvpMode::RvpMode()
:
    UncertainMode("Rvp mode")
{
}

inline
bool addr_is_between(void* addr_low, void* addr, void* addr_high)
{
    return addr_low <= addr && addr < addr_high;
}

inline
bool addr_is_in_frame(void* addr, Frame* frame)
{
    return addr_is_between(frame->lvars, addr, frame->lvars + frame->mb->max_locals)
        || addr_is_between(frame->ostack_base, addr, frame->ostack_base + frame->mb->max_stack);
}

uint32_t
RvpMode::mode_read(uint32_t* addr)
{
//     if (frame->calling_object == 0) {
//         if (addr_is_in_frame(addr, frame)) {
//             return *addr;
//         }
//         else {
//             //assert(false);
//             return m_core->m_rvpbuf.read(addr);
//         }
//     }
//     else {
//         return m_core->m_rvpbuf.read(addr);
//     }
    return m_core->m_rvpbuf.read(addr);
}

void
RvpMode::mode_write(uint32_t* addr, uint32_t value)
{
//     if (frame->calling_object == 0) {
//         if (addr_is_in_frame(addr, frame)) {
//             *addr = value;
//         }
//         else {
//             //assert(false);
//             m_core->m_rvpbuf.write(addr, value);
//         }
//     }
//     else {
//         m_core->m_rvpbuf.write(addr, value);
//     }
    m_core->m_rvpbuf.write(addr, value);
}

void
RvpMode::before_alloc_object()
{
    //MINILOG(r_new_logger, "#" << m_core->id() << " (R) hits NEW " << classobj->name());
    m_core->halt();
    throw DeepBreak();
}

void
RvpMode::after_alloc_object(Object* obj)
{
    assert(false);              // should not get here
}

void
RvpMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    if (intercept_vm_backdoor(target_object, new_mb)) return;

    log_when_invoke_return(true, m_user, frame->mb, target_object, new_mb);

    if (is_priviledged(new_mb)) {
        MINILOG(r_logger,
                "#" << m_core->id() << " (R) is to invoke native/sync method: " << *new_mb);
        m_core->halt();
        return;
    }

    sp -= new_mb->args_count;

    uintptr_t* arg1 = sp;

    std::vector<uintptr_t> args;
    for (int i = 0; i < new_mb->args_count; ++i) {
        args.push_back(read(&sp[i]));
    }

    MethodBlock* simplified_mb = get_rvp_method(new_mb);

    Frame* new_frame =
        create_frame(target_object, simplified_mb, frame, 0, &args[0], sp, pc);

    new_frame->is_certain = false;

    MINILOG_IF(debug_scaffold::java_main_arrived,
               r_frame_logger,
               "#" << m_core->id()
               << " (R) enter "
               << new_frame->mb->full_name()
               << "  <---  "
               << frame->mb->full_name()
               );

    //frame->last_pc = pc;
    sp = (uintptr_t*)new_frame->ostack_base;
    frame = new_frame;
    pc = (CodePntr)frame->mb->code;
}

void
RvpMode::do_method_return(int len)
{
    assert(len == 0 || len == 1 || len == 2);
    assert(not is_priviledged(frame->mb));

    if (frame->mb->is_synchronized()) {
        MINILOG(r_logger,
                 "#" << m_core->id() << " (S) is to return from sync method: " << frame->mb);
        m_core->halt();
        return;
    }

    log_when_invoke_return(false, frame->calling_object, frame->prev->mb, m_user, frame->mb);

    MINILOG_IF(debug_scaffold::java_main_arrived,
               r_frame_logger,
               "#" << m_core->id()
               << " (R) leave "
               << frame->mb->full_name()
               << "  --->  "
               << frame->prev->mb->full_name()
               );

    //{{{ just for debug
    if (m_core->m_id == 6 && strcmp(frame->mb->name, "_p_slice_for_makeUniqueNeighbors") == 0) {
        int x = 0;
        x++;
    }
    //{{{ just for debug

    Object* target_object = frame->prev->get_object();

    if (is_rvp_frame(frame->prev) || not target_object->get_core()) {
        sp -= len;
        uintptr_t* caller_sp = frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            write(caller_sp++, read(&sp[i]));
        }

        Frame* current_frame = frame;
        frame = current_frame->prev;
        sp = caller_sp;
        pc = current_frame->caller_pc;

        assert(is_sp_ok(sp, frame));
        assert(is_pc_ok(pc, frame->mb));


        destroy_frame(current_frame);
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    }
    else if (m_core->is_owner_or_subsidiary(target_object)) { // should quit RVP mode


        //{{{ just for debug
        if (m_core->m_id == 0
            && strcmp(frame->mb->name, "createVillage") == 0
            && strcmp(frame->prev->mb->name, "main") == 0) {
            int x = 0;
            x++;
        }
        //{{{ just for debug

        sp -= len;

        std::vector<uintptr_t> rv;
        uintptr_t* caller_sp = frame->caller_sp;

        for (int i = 0; i < len; ++i) {
            m_core->m_speculative_mode.write(caller_sp++, read(&sp[i]));
            rv.push_back(read(&sp[i]));
        }

        // ReturnMsg* msg = new ReturnMsg(frame->mb, frame->prev, &rv[0], len,
        //                                frame->caller_sp, frame->caller_pc, 0);
        ReturnMsg* msg = new ReturnMsg(frame->object, frame->mb, frame->prev,
                                       frame->calling_object, &rv[0], len,
                                       frame->caller_sp, frame->caller_pc);

        m_core->add_message_to_be_verified(msg);

        Frame* current_frame = frame;
        frame = frame->prev;
        sp = caller_sp;
        pc = current_frame->caller_pc;
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

        assert(is_sp_ok(sp, frame));
        assert(is_pc_ok(pc, frame->mb));

        m_core->leave_rvp_mode(target_object);

        // MINILOG(r_destroy_frame_logger, "#" << m_core->id()
        //         << " free rvpframe" << *current_frame);

        destroy_frame(current_frame);
    }
    else {                      // has core
        //assert(false);
        m_core->change_to_speculative_mode();
        destroy_frame(frame);
        m_core->get_speculative_mode()->load_next_task();
    }
}

void
RvpMode::before_signal_exception(Class *exception_class)
{
    MINILOG(r_exception_logger, "#" << m_core->id()
            << " (R) exception detected!!! " << exception_class->name());
    m_core->halt();
    throw DeepBreak();
}

void
RvpMode::do_throw_exception()
{
    assert(false);
}

void
RvpMode::step()
{
    Message* msg = m_core->get_certain_message();
    if (msg) {
        // m_core->enter_certain_mode();
        // m_core->verify_and_commit(msg);
        process_certain_message(msg);
        return;
    }
    //else
    exec_an_instr();
}

void
RvpMode::enter_execution()
{

}

void
RvpMode::leave_execution()
{

}

void
RvpMode::destroy_frame(Frame* frame)
{
//     MINILOG(p_destroy_frame_logger, "#" << m_core->id()
//             << " (R) destroy frame for " << *frame->mb);

    // m_core->m_rvpbuf.clear(frame->lvars, frame->lvars + frame->mb->max_locals);
    // m_core->m_rvpbuf.clear(frame->ostack_base, frame->ostack_base + frame->mb->max_stack);
    m_core->clear_frame_in_rvpbuf(frame);

    if (is_rvp_frame(frame)) {
        MINILOG(r_destroy_frame_logger, "#" << m_core->id()
                << " (R) destroy frame " << frame << " for " << *frame->mb);
        MINILOG(r_destroy_frame_logger, "#" << m_core->id()
                << " free rvpframe+" << *frame);
        // if (frame->xxx == 999)
        //     cout << "xxx999" << endl;

        //{{{ just for debug
        //assert(frame->c != 23507);
        //{{{ just for debug
        //delete frame;
        Mode::destroy_frame(frame);
    }
}

void
RvpMode::do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    sp -= is_static ? 0 : 1;

    for (int i = 0; i < size; ++i) {
        write(sp, read(addr + i));
        sp++;
    }

    pc += 3;
}

void
RvpMode::do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    sp -= size;
    for (int i = 0; i < size; ++i) {
        write(addr + i, read(sp + i));
    }

    sp -= is_static ? 0 : 1;
    pc += 3;
}

void
RvpMode::do_array_load(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    sp -= 2;                    // pop arrayref and index

    void* addr = array_elem_addr(array, index, type_size);
    load_from_array(sp, addr, type_size);
    sp += type_size > 4 ? 2 : 1;

    pc += 1;
}

void
RvpMode::do_array_store(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    sp -= type_size > 4 ? 2 : 1; // pop up value

    void* addr = array_elem_addr(array, index, type_size);
    store_to_array(sp, addr, type_size);

    sp -= 2;                    // pop arrayref and index

    pc += 1;
}

void*
RvpMode::do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs)
{
    MINILOG(step_loop_in_out_logger, "#" << m_core->id()
            << " (R) throw-> to be execute java method: " << *mb);
    m_core->halt();

    throw NestedStepLoop(m_core->id(), mb);

    return 0;
}

void
RvpMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(r_invoke_return_logger, is_invoke, m_core->id(), "(R)",
                      caller, caller_mb, callee, callee_mb);
}