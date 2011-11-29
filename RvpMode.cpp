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
    return m_st->m_rvp_buffer.read(addr);
}

void
RvpMode::mode_write(uint32_t* addr, uint32_t value)
{
    m_st->m_rvp_buffer.write(addr, value);
}

void
RvpMode::before_alloc_object()
{
    MINILOG(r_new_logger, "#" << m_st->id() << " (R) hits NEW ");
    m_st->halt(RunningState::halt_cannot_alloc_object);

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
    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    assert(target_st != m_st); // 目前暂不考虑对象重入

    if (new_mb->is_synchronized()) {
        MINILOG(s_logger, "#" << m_st->id()
                << " (R) " << new_mb << "is sync method");
        m_st->halt(RunningState::halt_cannot_exec_sync_method);

        return;
    }


    if (new_mb->is_native() and not new_mb->is_rope_spec_safe()) {
        MINILOG(s_logger, "#" << m_st->id()
                << " (R) " << new_mb << "is native method");
        m_st->halt(RunningState::halt_cannot_exec_native_method);

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
    //     m_st->halt();
    //     return;
    // }


    // 除了最外层的rvp栈帧，里层的rvp栈帧的caller都为0。
    invoke_impl(target_object, rvp_mb, &args[0], 0, pc, frame, sp, false);
}


void
RvpMode::do_method_return(int len)
{
    assert(len == 0 || len == 1 || len == 2);
    assert(not frame->mb->is_native());
    assert(not frame->mb->is_synchronized());

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

        pop_frame(current_frame);
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

        m_st->m_rvp_buffer.clear();

        pop_frame(current_frame);

        // MINILOG0("#" << m_st->id() << " leave RVP mode");

        m_st->switch_to_speculative_mode();
        m_st->launch_spec_msg(return_msg);

    }
}


void
RvpMode::before_signal_exception(Class *exception_class)
{
    MINILOG(r_exception_logger, "#" << m_st->id()
            << " (R) exception detected!!! " << exception_class->name());
    m_st->halt(RunningState::halt_cannot_signal_exception);
    throw Break();
}


Frame*
RvpMode::push_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                      SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                      bool is_top)
{
    // rvp模式不会抛出异常，栈帧的owner没用，我们让它为0，表示这是个rvp栈帧。用于调试。
    Frame* new_frame = g_create_frame(0, object, new_mb, args, caller, caller_pc, caller_frame, caller_sp, is_top);

    // record new_frame in V of effect
    return new_frame;
}


void
RvpMode::pop_frame(Frame* frame)
{
    m_st->clear_frame_in_rvp_buffer(frame);

    MINILOG(r_pop_frame_logger, "#" << m_st->id()
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
    MINILOG(step_loop_in_out_logger, "#" << m_st->id()
            << " (R) to be execute java method: " << mb);

    assert(false);              // 产生的rvp方法中不应该遇到这种情况
    //m_st->halt(RunningState::halt_cannot_exec_method);

    throw Break();

    return 0;
}


void
RvpMode::do_spec_barrier()
{
    assert(false);              // rvp方法中不该有推测路障
    m_st->halt(RunningState::halt_spec_barrier);
}
