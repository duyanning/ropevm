#include "std.h"
#include "rope.h"
#include "SpecMode.h"
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
#include "Break.h"

using namespace std;


SpecMode::SpecMode()
:
    UncertainMode("SPEC mode")
{
}

uint32_t
SpecMode::mode_read(uint32_t* addr)
{
    return m_st->m_state_buffer.read(addr);
}

void
SpecMode::mode_write(uint32_t* addr, uint32_t value)
{
    m_st->m_state_buffer.write(addr, value);
}


void
SpecMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    MINILOG(s_logger, "#" << m_st->id()
            << " (S) is to invoke method: " << new_mb);

    if (new_mb->is_synchronized()) {
        MINILOG(s_logger, "#" << m_st->id()
                << " (S) " << new_mb << "is sync method");
        m_st->halt(RunningState::halt_cannot_exec_sync_method);

        return;
    }


    if (new_mb->is_native() and not new_mb->is_rope_spec_safe()) {
        MINILOG(s_logger, "#" << m_st->id()
                << " (S) " << new_mb << "is native method");
        m_st->halt(RunningState::halt_cannot_exec_native_method);

        return;
    }


    assert(not new_mb->is_synchronized());

    frame->last_pc = pc;

    SpmtThread* target_st = target_object->get_st();

    if (target_st == m_st or new_mb->is_rope_invoker_execute()) {

        sp -= new_mb->args_count;

        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }

        invoke_impl(target_object, new_mb, &args[0],
                    m_st, pc, frame, sp, false);


    }
    else {

        // pop up arguments
        sp -= new_mb->args_count;

        // InvokeMsg的构造函数和invoke_impl取用参数都是不考虑模式的，所以我们先根据模式读出来
        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }


        MINILOG(s_logger, "#" << m_st->m_id
                << " (S) target thread is #" << target_st->id());


        // 构造推测性的invoke_msg发送给目标线程
        InvokeMsg* invoke_msg = new InvokeMsg(m_st, target_st,
                                              target_object, new_mb, &args[0],
                                              pc, frame, sp);

        m_st->send_msg(invoke_msg);


        // 推测性的return_msg在rvp方法获得推测性的返回值之后才能构造

        MethodBlock* rvp_method = ::get_rvp_method(new_mb);

        if (rvp_method == nullptr) { // 若无rvp方法就不推测执行了，停机。
            MINILOG(rvp_logger, "#" << m_st->m_id
                    << "no rvp-method for " << new_mb);
            m_st->halt(RunningState::halt_no_syn_msg);
            throw Break();
        }


        MINILOG(s_logger, "#" << m_st->m_id
                << " (S)invokes rvp-method: " << rvp_method);

        m_st->m_rvp_mode.invoke_impl(target_object,
                                              rvp_method,
                                              &args[0],
                                              m_st,
                                              pc, frame, sp, false);

        m_st->switch_to_rvp_mode();

    }
}


void
SpecMode::do_method_return(int len)
{
    assert(len == 0 || len == 1 || len == 2);
    assert(not frame->mb->is_synchronized());

    Frame* current_frame = frame;

    MINILOG(s_logger, "#" << m_st->id()
            << " (S) is to return from " << frame);


    SpmtThread* target_st = current_frame->caller;

    if (target_st == m_st) {

        sp -= len;
        uintptr_t* caller_sp = current_frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            write(caller_sp++, read(&sp[i]));
        }


        frame = current_frame->prev;
        sp = caller_sp;
        pc = current_frame->caller_pc;

        pop_frame(current_frame);
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

    }
    else {

        sp -= len;
        std::vector<uintptr_t> ret_val;
        for (int i = 0; i < len; ++i) {
            ret_val.push_back(read(&sp[i]));
        }


        // 构造推测性的return_msg（该消息只是被记录在effect中，并不真正发送给目标线程）
        ReturnMsg* return_msg = new ReturnMsg(target_st,
                                              &ret_val[0], len,
                                              current_frame->caller_pc,
                                              current_frame->prev,
                                              current_frame->caller_sp);

        pop_frame(current_frame);

        m_st->send_msg(return_msg);

        // 需要加载下一条待处理消息
        m_st->m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;
    }
}

void
SpecMode::before_signal_exception(Class *exception_class)
{
    MINILOG(s_exception_logger, "#" << m_st->id()
            << " (S) exception detected!!! " << exception_class->name());
    m_st->halt(RunningState::halt_cannot_signal_exception);

    throw Break();
}


Frame*
SpecMode::push_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                              SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                              bool is_top)

{
    Frame* new_frame = g_create_frame(m_st, object, new_mb, args, caller, caller_pc, caller_frame, caller_sp, is_top);

    MINILOG(s_push_frame_logger, "#" << m_st->id()
            << " (S) push frame " << new_frame);

    // 把创建的栈桢记录下来
    Effect* current_effect = m_st->m_current_spec_msg->get_effect();
    assert(current_effect);

    current_effect->add_to_C(new_frame);

    return new_frame;
}


void
SpecMode::pop_frame(Frame* frame)
{
    MINILOG(s_pop_frame_logger, "#" << m_st->id()
            << " (S) destroy " << (frame->pinned ? "(skipped) " : "") << "frame "
            << frame);

    Effect* current_effect = m_st->m_current_spec_msg->get_effect();
    assert(current_effect);

    if (not frame->pinned) {

        m_st->clear_frame_in_state_buffer(frame);

        // 把已销毁的栈桢从记录中去掉
        current_effect->remove_from_C(frame);

        g_destroy_frame(frame);
    }
    else {
        current_effect->add_to_R(frame);
    }
}

void
SpecMode::do_get_field(Object* target_object, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger, "#" << m_st->id()
            << " (S) is to getfield: " << fb)
            //<< " of " << target_object);


    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    if (target_st == m_st) {

        sp -= is_static ? 0 : 1;
        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }
        pc += 3;

    }
    else {
        sp -= is_static ? 0 : 1;

        // 构造推测性的get_msg发送给目标线程
        GetMsg* get_msg = new GetMsg(m_st, target_st,
                                     target_object, fb);

        MINILOG(s_logger, "#" << m_st->id()
                << " (S) add get msg to #"
                << target_st->id() << ": " << get_msg);

        m_st->send_msg(get_msg);


        // 直接从稳定内存读取，构造推测性的get_ret_msg供自己使用
        vector<uintptr_t> value;
        for (int i = 0; i < size; ++i) {
            value.push_back(addr[i]);
        }
        GetRetMsg* get_ret_msg = new GetRetMsg(target_st,
                                                     &value[0], value.size());
        m_st->launch_spec_msg(get_ret_msg);

    }

}

void
SpecMode::do_put_field(Object* target_object, FieldBlock* fb,
                              uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger, "#" << m_st->id()
            << " (S) is to put: " << fb);

    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    if (target_st == m_st) {
        sp -= size;
        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }
        sp -= is_static ? 0 : 1;
        pc += 3;
    }
    else {
        sp -= size;

        // 构造推测性的put_msg发送给目标线程
        vector<uintptr_t> val;
        for (int i = 0; i < size; ++i) {
            val.push_back(read(sp + i));
        }

        sp -= is_static ? 0 : 1;

        PutMsg* put_msg = new PutMsg(m_st, target_st,
                                     target_object, fb, &val[0]);

        MINILOG(s_logger, "#" << m_st->id()
                << " (S) add put msg to #"
                << target_st->id() << ": " << put_msg);

        m_st->send_msg(put_msg);


        // 构造推测性的put_ret_msg供自己使用
        PutRetMsg* put_ret_msg = new PutRetMsg(m_st);
        m_st->launch_spec_msg(put_ret_msg);

    }
}

void
SpecMode::do_array_load(Object* array, int index, int type_size)
{
    MINILOG(s_logger, "#" << m_st->id()
            << " (S) is to load from array");

    Object* target_object = array;

    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    int nslots = type_size > 4 ? 2 : 1; // number of slots for value

    if (target_st == m_st) {
        sp -= 2; // pop up arrayref and index
        load_from_array(sp, array, index, type_size);
        sp += nslots;

        pc += 1;

    }
    else {

        sp -= 2; // pop up arrayref and index

        // 构造推测性的aload_msg发送给目标线程
        ALoadMsg* aload_msg = new ALoadMsg(m_st, target_st,
                                                       array, type_size, index);

        MINILOG(s_logger, "#" << m_st->id()
                << " (S) add arrayload msg to #"
                << target_st->id() << ": " << aload_msg);

        m_st->send_msg(aload_msg);


        // 直接从稳定内存取数，构造推测性的arrayload_return消息并使用
        vector<uintptr_t> value;
        g_load_from_stable_array_to_c(&value[0], array, index, type_size);
        ALoadRetMsg* aload_ret_msg = new ALoadRetMsg(m_st,
                                                                       &value[0], value.size());
        m_st->launch_spec_msg(aload_ret_msg);

    }

}

void
SpecMode::do_array_store(Object* array, int index, int type_size)
{
    MINILOG(s_logger, "#" << m_st->id()
            << " (S) is to store to array");


    Object* target_object = array;

    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    //void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value


    if (target_st == m_st) {
        sp -= nslots; // pop up value
        store_to_array(sp, array, index, type_size);
        sp -= 2;                    // pop arrayref and index
        pc += 1;

    }
    else {

        sp -= nslots; // pop up value

        // 构造推测性的astore_msg发送给目标线程
        uint32_t val[2]; // at most 2 slots
        for (int i = 0; i < nslots; ++i) {
            val[i] = read(sp + i);
        }

        sp -= 2;

        AStoreMsg* astore_msg = new AStoreMsg(m_st, target_st,
                                                          array,
                                                          type_size, index, &val[0]);

        MINILOG(s_logger, "#" << m_st->id()
                << " (S) add arraystore task to #"
                << target_st->id() << ": " << astore_msg);

        m_st->send_msg(astore_msg);

        // 构造推测性的astore_ret_msg供自己使用
        AStoreRetMsg* astore_ret_msg = new AStoreRetMsg(m_st);
        m_st->launch_spec_msg(astore_ret_msg);

    }


}

void*
SpecMode::do_execute_method(Object* target_object,
                                   MethodBlock *mb,
                                   std::vector<uintptr_t>& jargs, DummyFrame* dummy)
{
    MINILOG(step_loop_in_out_logger, "#" << m_st->id()
            << " (S) to be execute java method: " << mb);

    // 大多数execute_method调用的目的都是为了在解释new指令的时候通过解
    // 释方法ClassLoader.loadClass来加载类。如果我们需要的类已经被加载，
    // 就不会调用execute_method。而且，就算现在这个类尚未加载，需要解释
    // ClassLoader.loadClass，而我们（在推测模式下）又没有资格调用
    // execute_method，只能中断new指令的解释过程，但过了一会之后，可能
    // 其他线程（在确定模式下）已经调用execute_method解释了
    // ClassLoader.loadClass，从而加载了我们没能加载的类，那么我们的
    // new指令就可以重新执行了。是让被中断的new指令不停地重试，还是等一
    // 会？前者代价太大。那么就等一会，等到什么时候（在推测模式下）重试
    // 呢？等到我们resume推测执行的时候。
    // SpmtThread::resume_suspended_spec_execution中有对应实现。
    m_st->halt(RunningState::halt_cannot_exec_method);

    throw Break();

    return 0;
}


void
SpecMode::send_msg(Message* msg)
{

    Effect* current_effect = m_st->m_current_spec_msg->get_effect();
    current_effect->msg_sent = msg;

    MINILOG(spec_msg_logger, "#" << m_st->id()
            << " send" << (g_is_async_msg(msg) ? "" : "(only record)")
            << " spec msg to "
            << "#" << msg->get_target_st()->id()
            << " " << msg);

    // 同步消息是只记录，但不真正发送出去
    if (g_is_async_msg(msg)) {
        msg->get_target_st()->add_spec_msg(msg);
    }
}


void
SpecMode::do_spec_barrier()
{
    if (RopeVM::model < 5) {    // 5以下的模型不支持推测路障
        pc +=  3;
        return;
    }

    m_st->halt(RunningState::halt_spec_barrier);

    MINILOG(spec_barrier_logger, "#" << m_st->id()
            << " halt because spec barrier");


    //pc +=  3; 不增加pc，推测模式下无法越过该路障
}
