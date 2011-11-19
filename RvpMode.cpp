#include "std.h"
#include "rope.h"
#include "RvpMode.h"
#include "SpmtThread.h"

#include "lock.h"

#include "Message.h"
#include "RopeVM.h"
#include "interp.h"
#include "Loggers.h"
#include "frame.h"
#include "DebugScaffold.h"
#include "Helper.h"
#include "Break.h"

using namespace std;

RvpMode::RvpMode()
:
    UncertainMode("RVP mode")
{
}

bool addr_is_between(void* addr_low, void* addr, void* addr_high)
{
    return addr_low <= addr && addr < addr_high;
}

bool addr_is_in_frame(void* addr, Frame* frame)
{
    return addr_is_between(frame->lvars, addr, frame->lvars + frame->mb->max_locals)
        or addr_is_between(frame->ostack_base, addr, frame->ostack_base + frame->mb->max_stack);
}

uint32_t
RvpMode::mode_read(uint32_t* addr)
{
    return m_spmt_thread->m_rvp_buffer.read(addr);
}

void
RvpMode::mode_write(uint32_t* addr, uint32_t value)
{
    m_spmt_thread->m_rvp_buffer.write(addr, value);
}

void
RvpMode::before_alloc_object()
{
    MINILOG(r_new_logger, "#" << m_spmt_thread->id() << " (R) hits NEW ");
    m_spmt_thread->halt(RunningState::halt_cannot_alloc_object);

    throw Break();
}

void
RvpMode::after_alloc_object(Object* obj)
{
    assert(false);              // should not get here
}

void
RvpMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    //if (intercept_vm_backdoor(target_object, new_mb)) return;

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    assert(target_spmt_thread != m_spmt_thread); // 目前暂不考虑对象重入

    if (is_priviledged(new_mb)) {
        MINILOG(r_logger, "#" << m_spmt_thread->id()
                << " (R) is to invoke native/sync method: " << new_mb);
        m_spmt_thread->halt(RunningState::halt_cannot_exec_priviledged_method);
        return;
    }

    sp -= new_mb->args_count;

    std::vector<uintptr_t> args;
    for (int i = 0; i < new_mb->args_count; ++i) {
        args.push_back(read(&sp[i]));
    }

    MethodBlock* rvp_mb = get_rvp_method(new_mb);
    assert(rvp_mb);

    // if (rvp_method == nullptr) { // 若无rvp方法就不推测执行了，停机。
    //     m_spmt_thread->halt();
    //     return;
    // }


    // MINILOG_IF(debug_scaffold::java_main_arrived,
    //            r_frame_logger,
    //            "#" << m_spmt_thread->id()
    //            << " (R) enter "
    //            << new_frame->mb->full_name()
    //            << "  <---  "
    //            << frame->mb->full_name()
    //            );

    // 除了最外层的rvp栈帧，里层的rvp栈帧的caller都为0。
    invoke_impl(target_object, rvp_mb, &args[0], 0, pc, frame, sp, false);
}


void
RvpMode::do_method_return(int len)
{
    assert(len == 0 || len == 1 || len == 2);
    assert(not is_priviledged(frame->mb));

    // MINILOG_IF(debug_scaffold::java_main_arrived,
    //            r_frame_logger,
    //            "#" << m_spmt_thread->id()
    //            << " (R) leave "
    //            << frame->mb->full_name()
    //            << "  --->  "
    //            << frame->prev->mb->full_name()
    //            );

    Frame* current_frame = frame;


    /*
      if 是从最顶层的rvp栈帧返回(最顶层的rvp栈帧的caller不为0)
          构造返回值消息，退出rvp模式，转入推测模式
      else
          继续在rvp模式下工作
     */

    if (frame->caller == 0) {
        sp -= len;
        uintptr_t* caller_sp = frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            write(caller_sp++, read(&sp[i]));
        }

        Frame* current_frame = frame;
        frame = current_frame->prev;
        sp = caller_sp;
        pc = current_frame->caller_pc;

        destroy_frame(current_frame);
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    }
    else { // 从最外层的rvp栈帧返回

        sp -= len;

        // 获得推测性的返回值（现在rvp方法预测出来的返回值就静静地躺在当前栈帧的ostack中）
        std::vector<uintptr_t> rv;

        for (int i = 0; i < len; ++i) {
            rv.push_back(read(&sp[i]));
        }

        // 根据该返回值构造推测性的return_msg
        ReturnMsg* return_msg = new ReturnMsg(current_frame->caller,
                                              &rv[0], len,
                                              current_frame->caller_pc,
                                              current_frame->prev,
                                              current_frame->caller_sp);

        m_spmt_thread->m_rvp_buffer.clear();

        destroy_frame(current_frame);

        // MINILOG0("#" << m_spmt_thread->id() << " leave RVP mode");

        m_spmt_thread->switch_to_speculative_mode();
        m_spmt_thread->launch_spec_msg(return_msg);

    }
}


void
RvpMode::before_signal_exception(Class *exception_class)
{
    MINILOG(r_exception_logger, "#" << m_spmt_thread->id()
            << " (R) exception detected!!! " << exception_class->name());
    m_spmt_thread->halt(RunningState::halt_cannot_signal_exception);
    throw Break();
}


Frame*
RvpMode::create_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                      SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                      bool is_top)
{
    //Frame* new_frame = g_create_frame(m_spmt_thread, object, new_mb, args, caller, caller_pc, caller_frame, caller_sp);
    // rvp模式不会抛出异常，栈帧的owner没用，我们让它为0，表示这是个rvp栈帧。用于调试。
    Frame* new_frame = g_create_frame(0, object, new_mb, args, caller, caller_pc, caller_frame, caller_sp, is_top);

    // record new_frame in V of effect
    return new_frame;
}


void
RvpMode::destroy_frame(Frame* frame)
{
    //assert(is_rvp_frame(frame));
    //     MINILOG(p_destroy_frame_logger, "#" << m_spmt_thread->id()
    //             << " (R) destroy frame for " << *frame->mb);

    m_spmt_thread->clear_frame_in_rvp_buffer(frame);

    MINILOG(r_destroy_frame_logger, "#" << m_spmt_thread->id()
            << " (R) destroy frame " << frame);

    g_destroy_frame(frame);
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
    sp -= 2;                    // pop arrayref and index

    load_from_array(sp, array, index, type_size);
    sp += type_size > 4 ? 2 : 1;

    pc += 1;
}

void
RvpMode::do_array_store(Object* array, int index, int type_size)
{
    sp -= type_size > 4 ? 2 : 1; // pop up value

    store_to_array(sp, array, index, type_size);

    sp -= 2;                    // pop arrayref and index

    pc += 1;
}

void*
RvpMode::do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs, DummyFrame* dummy)
{
    MINILOG(step_loop_in_out_logger, "#" << m_spmt_thread->id()
            << " (R) to be execute java method: " << mb);

    assert(false);
    //m_spmt_thread->halt(RunningState::halt_cannot_exec_method);
    //m_spmt_thread->halt();

    throw Break();

    return 0;
}

void
RvpMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(r_invoke_return_logger, is_invoke, m_spmt_thread->id(), "(R)",
                      caller, caller_mb, callee, callee_mb);
}


void
RvpMode::invoke_impl(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                     SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                     bool is_top)
{
    Frame* new_frame = create_frame(target_object,
                                    new_mb,
                                    args,
                                    caller,
                                    caller_pc,
                                    caller_frame,
                                    caller_sp,
                                    is_top);

    pc = (CodePntr)new_frame->mb->code;
    frame = new_frame;
    sp = (uintptr_t*)new_frame->ostack_base;
}
