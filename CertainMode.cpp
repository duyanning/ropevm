#include "std.h"
#include "rope.h"
#include "CertainMode.h"

#include "lock.h"

#include "Message.h"
#include "SpmtThread.h"
#include "RopeVM.h"
#include "interp.h"
#include "Helper.h"
#include "Loggers.h"
#include "DebugScaffold.h"
#include "Snapshot.h"
#include "frame.h"

using namespace std;

CertainMode::CertainMode()
:
    Mode("CERT mode")
{
}

uint32_t
CertainMode::mode_read(uint32_t* addr)
{
    return *addr;
}

void
CertainMode::mode_write(uint32_t* addr, uint32_t value)
{
    *addr = value;
}

void
CertainMode::step()
{
    fetch_and_interpret_an_instruction();
}

// DO NOT forget function 'invoke' in reflect.cpp!!!
void*
CertainMode::do_execute_method(Object* target_object,
                               MethodBlock* new_mb,
                               std::vector<uintptr_t>& jargs, DummyFrame* dummy)
{
    assert(frame);              // see also: initialiseJavaStack

    // save (pc, frame, sp) of outer drive_loop
    CodePntr old_pc = pc;
    Frame* old_frame = frame;
    uintptr_t* old_sp = sp;


    // dummy frame is used to receive RV of top_frame
    // dummy->prev is current frame
    //Frame* dummy = create_dummy_frame(frame);
    dummy->prev = frame;

    void *ret;
    ret = dummy->ostack_base;

    //????frame->last_pc = pc;

    assert(target_object);

    SpmtThread* target_st = target_object->get_st();

    assert(target_st->get_thread() == g_get_current_thread());

    MINILOG(top_method_logger, "#" << m_st->m_id
            << " top-invoke " << "#" << target_st->m_id << " " << new_mb);

    if (target_st == m_st or new_mb->is_rope_invoker_execute()) {

        // 对top method的调用是从native代码中发出，所以caller的pc设为0
        // dummy frame作为顶级方法的上级，用来接收top frame的返回值
        invoke_impl(target_object, new_mb, &jargs[0],
                    m_st, 0, dummy, dummy->ostack_base, true);


    }
    else {
        MINILOG(inter_st_logger, "#" << m_st->m_id
                << " inter-invoke(top) " << "#" << target_st->m_id << " " << new_mb);

        // 构造确定性的invoke_msg发送给目标线程
        InvokeMsg* invoke_msg = new InvokeMsg(m_st, target_st,
                                              target_object, new_mb, &jargs[0],
                                              0, dummy, dummy->ostack_base,
                                              true);


        //frame->last_pc = pc;

        m_st->send_msg(invoke_msg);

        // 虽然进入了推测模式，但没有异步消息，推测执行无法继续。注意，
        //因为对本条指令的解释过程还没结束，使不能推测执行该指令后边的
        //指令的。
        m_st->m_spec_mode.pc = 0;
        m_st->m_spec_mode.frame = 0;
        m_st->m_spec_mode.sp = 0;
        m_st->halt(RunningState::halt_no_asyn_msg);

    }


    // 进入内层drive_loop
    m_st->drive_loop();
    // 已从内层drive_loop退出


    //assert(m_st->m_mode->is_certain_mode());

    // restore (pc, frame, sp) of outer drive_loop
    pc = old_pc;
    frame = old_frame;
    sp = old_sp;


    return ret;
}

void
CertainMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    assert(target_object);

    MINILOG_IF((m_st->m_id == 0 or m_st->m_id >= 5), order_logger, "invoke " << new_mb);

    frame->last_pc = pc;

    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    if (target_st == m_st or new_mb->is_rope_invoker_execute()) {

        sp -= new_mb->args_count;
        invoke_impl(target_object, new_mb, sp,
                    m_st, pc, frame, sp,
                    false);

    }
    else {
        MINILOG(inter_st_logger, "#" << m_st->m_id
                << " inter-invoke " << "#" << target_st->m_id << " " << new_mb);

        // pop up arguments
        sp -= new_mb->args_count;

        // 构造确定性的invoke_msg发送给目标线程
        InvokeMsg* invoke_msg = new InvokeMsg(m_st, target_st,
                                              target_object, new_mb, sp,
                                              pc, frame, sp);


        m_st->send_msg(invoke_msg);

        if (RopeVM::model < 3) // 模型3以上才支持推测执行。
            return;

        MethodBlock* rvp_method = get_rvp_method(new_mb);

        if (rvp_method == nullptr) { // 若无rvp方法就不推测执行了，停机。
            MINILOG(rvp_logger, "#" << m_st->m_id
                    << "no rvp-method for " << new_mb);

            m_st->halt(RunningState::halt_no_syn_msg);
            return;
        }

        // MINILOG0("#" << m_st->m_id
        //          << " (C)invokes rvp-method: " << *rvp_method);

        m_st->m_rvp_mode.invoke_impl(target_object,
                                     rvp_method,
                                     sp,
                                     m_st,
                                     pc, frame, sp,
                                     false);

        m_st->switch_to_rvp_mode();
    }

}

void
CertainMode::do_method_return(int len)
{
    assert(len == 0 || len == 1 || len == 2);

    Frame* current_frame = frame; // 因为后边的处理可能会改变frame，所以我们先保存一下

    MINILOG_IF((m_st->m_id == 0 or m_st->m_id >= 5), order_logger, "return " << current_frame->mb);

    // 给同步方法解锁
    if (current_frame->mb->is_synchronized()) {
        assert(current_frame->get_object() == current_frame->mb->classobj
               or current_frame->get_object() == (Object*)current_frame->lvars[0]);
        Object* sync_ob = current_frame->get_object();
        objectUnlock(sync_ob);
    }


    SpmtThread* target_st = current_frame->caller;
    assert(target_st->m_thread == m_st->m_thread);


    MINILOG_IF(current_frame->is_top_frame(), top_method_logger, "#" << m_st->m_id
               << " top-return " << "#" << target_st->m_id << " " << current_frame->mb);


    /*
    if 目标线程是当前线程
        返回值写入上级frame的ostack（if 是从top frame返回，上级frame为dummy frame，并结束drive_loop）
    else
        构造return确定消息（if 是从top frame返回，则带有top标志）
    */


    if (target_st == m_st) {

        uintptr_t* rv = sp - len;   // 返回值在ostack中占用了几个slot，我们让rv指向其起始位置
        sp -= len;

        // 将返回值写入主调方法的ostack
        uintptr_t* caller_sp = current_frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            *caller_sp++ = rv[i];
        }


        if (current_frame->is_top_frame()) {

            m_st->signal_quit_drive_loop();

        }
        else {                  // top frame之上是无代码的dummy frame，这些对于dummy frame都无意义。
            pc = current_frame->caller_pc;
            frame = current_frame->prev;
            sp = caller_sp;

            pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
        }

        pop_frame(current_frame);

    }
    else {

        MINILOG(inter_st_logger, "#" << m_st->m_id
                << " inter-return" << (current_frame->is_top_frame()? "(top) " : " ") << "#" << target_st->m_id << " " << current_frame->mb);


        uintptr_t* rv = sp - len;
        sp -= len;

        assert(not current_frame->is_top_frame() or current_frame->caller_pc == 0);

        // 构造确定性的return_msg发送给目标线程
        ReturnMsg* return_msg = new ReturnMsg(target_st,
                                              rv, len,
                                              current_frame->caller_pc,
                                              current_frame->prev,
                                              current_frame->caller_sp,
                                              current_frame->is_top_frame());

        pop_frame(current_frame);

        m_st->send_msg(return_msg);

        m_st->m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;
    }

}


void
CertainMode::before_signal_exception(Class *exception_class)
{
    MINILOG(c_exception_logger, "#" << m_st->id() << " (C) before signal exception "
            << exception_class->name() << " in: " << info(frame));
    // do nothing
}


Frame*
CertainMode::push_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                          SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                          bool is_top)
{
    Frame* new_frame = g_create_frame(m_st, object, new_mb, args,
                                      caller, caller_pc, caller_frame, caller_sp,
                                      is_top);

    return new_frame;
}

void
CertainMode::pop_frame(Frame* frame)
{
    MINILOG_IF(debug_scaffold::java_main_arrived && is_app_obj(frame->mb->classobj),
               c_pop_frame_logger, "#" << m_st->id()
               << " (C) destroy frame " << frame);

    g_destroy_frame(frame);
}

/*
before and after do_get_field

getfield
 _____             _____
| obj |           | val |
|     |<--sp =>   |     |<--sp
|     |           |     |

getstatic
 _____             _____
|     |<--sp      | val |
|     |      =>   |     |<--sp
|     |           |     |

 */
void
CertainMode::do_get_field(Object* target_object, FieldBlock* fb,
                          uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG_IF((m_st->m_id == 0 or m_st->m_id >= 5), order_logger, "get " << fb);


    if (RopeVM::support_self_read) { // 若支持自行read，就无所谓目标对象是否由本线程负责了。
        sp -= is_static ? 0 : 1;
        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }
        pc += 3;

        return;
    }

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
        // 构造确定性的get_msg发送给目标线程
        GetMsg* get_msg = new GetMsg(m_st, target_st, target_object, fb);

        m_st->halt(RunningState::halt_worthless_to_execute);

        m_st->send_msg(get_msg);

    }

}

/*
before and after do_put_field

putfield
 _____             _____
| obj |           | obj |<--sp
| val |      =>   | val |
|     |<--sp      |     |

putstatic
 _____             _____
| val |           | val |<--sp
|     |<--sp =>   |     |
|     |           |     |

 */

// size表示占几个ostack位置。
// 就算字段的类型大小只有一个字节，但在存储时，总是有四个或八个字节的位置保留给它。数组则不然，数组是紧致存储的。
void
CertainMode::do_put_field(Object* target_object, FieldBlock* fb,
                          uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG_IF((m_st->m_id == 0 or m_st->m_id >= 5), order_logger, "put " << fb);

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

        // 构造确定性的put_msg发送给目标线程
        PutMsg* put_msg = new PutMsg(m_st, target_st, target_object, fb, sp);

        sp -= is_static ? 0 : 1;

        m_st->halt(RunningState::halt_worthless_to_execute);

        m_st->send_msg(put_msg);
    }

}

void
CertainMode::do_array_load(Object* array, int index, int type_size)
{

    MINILOG_IF((m_st->m_id == 0 or m_st->m_id >= 5), order_logger, "arrayload " << index << ", " << type_size);

    Object* target_object = array;

    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    int nslots = type_size > 4 ? 2 : 1; // number of slots for value

    if (RopeVM::support_self_read) { // 若支持自行read，就无所谓目标对象是否由本线程负责了。
        sp -= 2; // pop up arrayref and index
        load_from_array(sp, array, index, type_size);
        sp += nslots;
        pc += 1;

        return;
    }

    if (target_st == m_st) {
        sp -= 2; // pop up arrayref and index
        load_from_array(sp, array, index, type_size);
        sp += nslots;
        pc += 1;
    }
    else {
        sp -= 2; // pop up arrayref and index

        // 构造确定性的aload_msg发送给目标线程
        ALoadMsg* aload_msg = new ALoadMsg(m_st, target_st,
                                                       array, type_size, index);

        m_st->halt(RunningState::halt_worthless_to_execute);

        m_st->send_msg(aload_msg);

    }



}

/*
type_size 表示元素类型的大小，单位字节。
nslots 表示占几个ostack的位置，一个位置四个字节。

就算数组元素的类型大小只有一个字节，但读到ostack中之后，仍然是占一个ostack位置，即四个字节。
但数组是紧致存储的。

 */
void
CertainMode::do_array_store(Object* array, int index, int type_size)
{
    MINILOG_IF((m_st->m_id == 0 or m_st->m_id >= 5), order_logger, "arraystore " << index << ", " << type_size);

    Object* target_object = array;

    SpmtThread* target_st = target_object->get_st();
    assert(target_st->m_thread == m_st->m_thread);

    //void* addr = array_elem_addr(array, index, type_size);
    int nslots = size2nslots(type_size); // number of slots for value


    if (target_st == m_st) {
        sp -= nslots;
        store_to_array(sp, array, index, type_size);
        sp -= 2;                    // pop up arrayref and index
        pc += 1;
    }
    else {
        sp -= nslots;

        // 构造确定性的astore_msg发送给目标线程
        AStoreMsg* astore_msg = new AStoreMsg(m_st, target_st,
                                                          array,
                                                          type_size, index, sp);
        sp -= 2;                    // pop up arrayref and index

        m_st->halt(RunningState::halt_worthless_to_execute);

        m_st->send_msg(astore_msg);

    }

}


/*
  转交确定模式，必须自己先放弃确定模式，然后再发出确定消息。如果你先发
  出确定消息，然后才放弃确定模式，在多os线程实现方式下，这之间其他spmt
  线程可能会运行，检测到确定消息从而进入确定模式，从而导致出现两个具有
  确定模式的spmt线程。
 */
void
CertainMode::send_msg(Message* msg)
{
    assert(RopeVM::model >= 2);  // 模型2以上才有消息

    if (RopeVM::model < 3)     // 模型2失去确定控制后停机，模型3、模型4则不。
        m_st->halt(RunningState::halt_model2_requirement);

    MINILOG(certain_msg_logger, "#" << m_st->id()
            << " send cert msg to "
            << "#" << msg->get_target_st()->id()
            << " " << msg);

    MINILOG(control_transfer_logger, "#" << m_st->id()
            << " transfer control to "
            << "#" << msg->get_target_st()->id());

    m_st->switch_to_speculative_mode();

    m_st->start_afresh_spec_execution();

    msg->get_target_st()->set_certain_msg(msg);
}


void
CertainMode::do_spec_barrier()
{
    MINILOG(spec_barrier_logger, "#" << m_st->id()
            << " pass spec barrier");

    pc +=  3; // 越过推测路障
}
