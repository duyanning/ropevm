#include "std.h"
#include "rope.h"
#include "SpmtThread.h"
#include "thread.h"
#include "interp-indirect.h"
#include "interp.h"
#include "excep.h"
#include "Message.h"
#include "RopeVM.h"
#include "Snapshot.h"
#include "Loggers.h"
#include "DebugScaffold.h"
#include "frame.h"
#include "Helper.h"
#include "lock.h"
#include "Effect.h"

using namespace std;

SpmtThread::SpmtThread(int id)
:
    m_id(id)
{
    m_thread = 0;
    m_halt = true;

    m_certain_mode.set_spmt_thread(this);
    m_spec_mode.set_spmt_thread(this);
    m_rvp_mode.set_spmt_thread(this);

    m_certain_message = 0;
    m_mode = &m_spec_mode;

    m_need_spec_msg = true;

    m_quit_step_loop = false;
    m_result = 0;

    // stat
    m_count_spec_msgs_sent = 0;
    m_count_spec_msgs_used = 0;
    m_count_verify_all = 0;
    m_count_verify_ok = 0;
    m_count_verify_fail = 0;
    m_count_verify_empty = 0;
    m_count_rvp = 0;
    m_count_certain_instr = 0;
    m_count_spec_instr = 0;
    m_count_rvp_instr = 0;
    m_count_step = 0;
    m_count_idle = 0;

}

SpmtThread::~SpmtThread()
{
}

// void
// SpmtThread::set_group(Group* group)
// {
//     m_group = group;
// }

void
SpmtThread::set_thread(Thread* thread)
{
    assert(m_thread == 0);

    m_thread = thread;
    thread->m_spmt_threads.push_back(this);
}


// void
// SpmtThread::add_object(Object* obj)
// {
//     obj->set_spmt_thread(this);
// }

bool
SpmtThread::is_halt()
{
    return m_halt;
}

void
SpmtThread::step()
{
    // stat
    m_count_step++;

    try {
        m_mode->step();
    }
    catch (DeepBreak& e) {
        //assert(false);          // caution needed
    }
}

void
SpmtThread::idle()
{
    // stat
    m_count_idle++;
}


void
SpmtThread::record_current_non_certain_mode()
{
    assert(not m_mode->is_certain_mode());

	m_old_mode = m_mode;
    //MINILOG0("#" << id() << " record " << m_mode->get_name());
}

void
SpmtThread::restore_original_non_certain_mode()
{
    assert(m_mode->is_certain_mode());

	m_mode = m_old_mode;
    MINILOG0("#" << id() << " restore to " << m_mode->get_name());
}


void
SpmtThread::wakeup()
{
    if (m_id == 5) {
        int x = 0;
        x++;
    }
    if (m_halt) {
        m_halt = false;
        MINILOG0("#" << id() << " (state)start");
    }
}


void
SpmtThread::sleep()
{
    //{{{ just for debug
    if (id() == 6) {
        int x = 0;
        x++;
    }
    //}}} just for debug
    MINILOG0("#" << id() << " (state)sleep");
    m_halt = true;
}


// void
// SpmtThread::send_certain_msg(Message* msg)
// {
//     msg->get_target_spmt_thread()->set_msg(msg);
// }


void
SpmtThread::set_certain_msg(Message* msg)
{
    m_certain_message = msg;

    // 确定控制到来，不管当前处于什么状态，必须起来处理。
    wakeup();
}

Message*
SpmtThread::get_certain_msg()
{
    Message* msg = 0;
    if (m_certain_message) {
        assert(is_valid_certain_msg(m_certain_message));
        msg = m_certain_message;
        m_certain_message = 0;
    }

    return msg;
}

void
SpmtThread::log_when_leave_certain()
{
    MINILOG(when_leave_certain_logger,
            "#" << id() << " when leave certain mode");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " now use cache ver(" << m_state_buffer.latest_ver() << ")");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " SPEC details:");
    // MINILOGPROC(when_leave_certain_logger,
    //             show_triple,
    //             (os, id(),
    //              m_spec_mode.frame, m_spec_mode.sp, m_spec_mode.pc,
    //              m_spec_mode.m_user,
    //              true));

    MINILOG(when_leave_certain_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " RVP details:");
    // MINILOGPROC(when_leave_certain_logger,
    //             show_triple,
    //             (os, id(), m_rvp_mode.frame, m_rvp_mode.sp, m_rvp_mode.pc, m_rvp_mode.m_user, true));
    MINILOG(when_leave_certain_logger,
            "#" << id() << " ---------------------------");
}

void
SpmtThread::sync_speculative_with_certain()
{
    m_spec_mode.pc = m_certain_mode.pc;
    m_spec_mode.frame = m_certain_mode.frame;
    m_spec_mode.sp = m_certain_mode.sp;
}


void
SpmtThread::enter_certain_mode()
{
    assert(not m_mode->is_certain_mode());

    //m_thread->set_certain_core(this);

    record_current_non_certain_mode();
    switch_to_certain_mode();


    MINILOG(when_enter_certain_logger,
            "#" << id() << " when enter certain mode");
    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_enter_certain_logger,
            "#" << id() << " cache ver(" << m_state_buffer.latest_ver() << ")");

    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");

    MINILOG(when_enter_certain_logger,
            "#" << id() << " SPEC details:");


}


void
SpmtThread::switch_to_certain_mode()
{
    m_mode = &m_certain_mode;
}

void
SpmtThread::switch_to_speculative_mode()
{
    m_mode = &m_spec_mode;
}

void
SpmtThread::switch_to_rvp_mode()
{
    // stat
    m_count_rvp++;

    MINILOG0("#" << id() << " enter RVP mode");

//     m_rvp_mode.pc = m_spec_mode.pc;
//     m_rvp_mode.frame = m_spec_mode.frame;
//     m_rvp_mode.ostack = m_spec_mode.ostack;

    m_mode = &m_rvp_mode;
}


// void
// SpmtThread::send_spec_msg(Message* msg)
// {
//     Effect* current_effect = m_current_spec_msg->get_effect();
//     current_effect->msg_sent = msg;

//     if (g_is_async_msg(msg)) {
//         msg->get_target_spmt_thread()->add_spec_msg(msg);
//     }


//     // MINILOG(s_logger,
//     //         "#" << this->id() << " (S) add invoke task to #"
//     //         << target_thread->id() << ": " << *msg);

// }


void
SpmtThread::add_spec_msg(Message* msg)
{
    // stat
    // m_count_spec_msgs_sent++;

    m_spec_msg_queue.push_back(msg);

    // 如果本线程是因为推测执行缺乏任务而睡眠，则唤醒。
    if (m_halt and is_spec_mode() and m_need_spec_msg) {
        MINILOG0("#" << id() << " is waken(sleeping, waiting for task)");
        wakeup();
    }
}


void
SpmtThread::destroy_frame(Frame* frame)
{
    m_mode->destroy_frame(frame);
}


bool
SpmtThread::check_quit_step_loop()
{
    bool quit = m_quit_step_loop;
    if (quit) {
        m_quit_step_loop = false;
    }
    return quit;
}

void
SpmtThread::signal_quit_step_loop(uintptr_t* result)
{
    assert(m_mode->is_certain_mode());

    m_quit_step_loop = true;
    m_result = result;
}

uintptr_t*
SpmtThread::get_result()
{
    return m_result;
}


// 将来spmt线程对应os线程，这个函数的实现将会很简单。
// 目前这种写法是为了兼容多os线程实现
SpmtThread* g_get_current_spmt_thread()
{
    Thread* thread = threadSelf();
    SpmtThread* current_spmt_thread = thread->get_current_spmt_thread();
    return current_spmt_thread;
}


void
SpmtThread::scan()
{
    // scan snapshots
    // scan states buffer
    // scan rvp buffer
}

void
SpmtThread::clear_frame_in_states_buffer(Frame* f)
{
    m_state_buffer.clear(f->lvars, f->lvars + f->mb->max_locals);
    m_state_buffer.clear(f->ostack_base, f->ostack_base + f->mb->max_stack);
}

void
SpmtThread::clear_frame_in_rvp_buffer(Frame* f)
{
    m_rvp_buffer.clear(f->lvars, f->lvars + f->mb->max_locals);
    m_rvp_buffer.clear(f->ostack_base, f->ostack_base + f->mb->max_stack);
}


void
SpmtThread::report_stat(ostream& os)
{
    os << '#' << m_id << '\t' << "spec msg sent" << '\t' << m_count_spec_msgs_sent << '\n';
    os << '#' << m_id << '\t' << "spec msg used" << '\t' << m_count_spec_msgs_used << '\n';
    os << '#' << m_id << '\t' << "verify all" << '\t' << m_count_verify_all << '\n';
    os << '#' << m_id << '\t' << "verify ok" << '\t' << m_count_verify_ok << '\n';
    os << '#' << m_id << '\t' << "verify fail" << '\t' << m_count_verify_fail << '\n';
    os << '#' << m_id << '\t' << "verify empty" << '\t' << m_count_verify_empty << '\n';
    os << '#' << m_id << '\t' << "rvp count" << '\t' << m_count_rvp << '\n';
    os << '#' << m_id << '\t' << "certain instr count" << '\t' << m_count_certain_instr << '\n';
    os << '#' << m_id << '\t' << "spec instr count" << '\t' << m_count_spec_instr << '\n';
    os << '#' << m_id << '\t' << "rvp instr count" << '\t' << m_count_rvp_instr << '\n';
    os << '#' << m_id << '\t' << "step count" << '\t' << m_count_step << '\n';
    os << '#' << m_id << '\t' << "idle count" << '\t' << m_count_idle << '\n';
}


void
SpmtThread::commit_effect(Effect* effect)
{
    // 释放栈帧R（推测模式下返回时因为被钉住而没能释放）
    for (Frame* f : effect->R) {
        f->pinned = false;
        m_spec_mode.destroy_frame(f);
    }


    // 提交快照
    /*
      if 有快照
          提交快照
      else
          提交最新的推测状态
     */
    if (effect->snapshot) {
        Snapshot* snapshot = effect->snapshot;

        m_state_buffer.commit(snapshot->version);

        m_certain_mode.pc = snapshot->pc;
        m_certain_mode.frame = snapshot->frame;
        m_certain_mode.sp = snapshot->sp;
    }
    else {
        m_state_buffer.commit(m_state_buffer.latest_ver());

        m_certain_mode.pc = m_spec_mode.pc;
        m_certain_mode.frame = m_spec_mode.frame;
        m_certain_mode.sp = m_spec_mode.sp;
    }


    // 确认消息
    if (effect->msg_sent) {
        confirm_spec_msg(effect->msg_sent);
    }
}


void
SpmtThread::confirm_spec_msg(Message* msg)
{
    assert(is_certain_mode());
    send_msg(msg);
}

void
SpmtThread::revoke_spec_msg(Message* msg)
{
    msg->get_target_spmt_thread()->add_revoked_spec_msg(msg);
}


void
SpmtThread::add_revoked_spec_msg(Message* msg)
{
    m_revoked_msgs.push_back(msg);
}


void
SpmtThread::remove_revoked_msgs()
{
    if (m_revoked_msgs.empty())
        return;

    for (Message* msg : m_revoked_msgs) {
        remove_revoked_msg(msg);
    }
}


void
SpmtThread::remove_revoked_msg(Message* msg)
{
    /*
      if 队列中找不到该消息
          返回
    */
    auto iter_revoked_msg = find(m_spec_msg_queue.begin(),
                                 m_spec_msg_queue.end(),
                                 msg);
    if (iter_revoked_msg == m_spec_msg_queue.end())
        return;

    /*
      if 消息并无关联的effect（说明还未被处理）
          if iter_to_revoke与iter_next相同
              iter_next指向下一个
          从队列中删除iter_to_revoke所指
      else
          从后向前遍历区间[iter_to_revoke, iter_next)内的消息
              丢弃与消息关联的effect
              if 消息是异步消息
                  让iter_next指向该位置
              else
                  释放该消息
                  移除该消息
     */

    Message* revoked_msg = *iter_revoked_msg;
    if (revoked_msg->get_effect() == nullptr) {
        if (iter_revoked_msg == m_iter_next_spec_msg) {
            ++m_iter_next_spec_msg;
        }
        m_spec_msg_queue.erase(iter_revoked_msg);
    }
    else {
        auto reverse_iter_msg = reverse_iterator<decltype(m_iter_next_spec_msg)>(m_iter_next_spec_msg);
        auto reverse_iter_revoked_msg = reverse_iterator<decltype(iter_revoked_msg)>(iter_revoked_msg);

        while (reverse_iter_msg != reverse_iter_revoked_msg) {
            Message* msg = *reverse_iter_msg;
            discard_effect(msg->get_effect());
            if (g_is_async_msg(msg)) {
                m_iter_next_spec_msg = reverse_iter_msg.base();
                --m_iter_next_spec_msg;
            }
            else {
                delete msg;
                ++reverse_iter_msg;
                m_spec_msg_queue.erase(reverse_iter_msg.base());
            }
        }

    }
}


/*
  只要丢弃effect，无论是全部丢弃，还是从某个之后丢弃。所有的rvp栈帧就都要释放。
  只有最新的推测执行才会处于rvp模式，才会生成rvp栈帧。
  而无论丢弃哪个effect，最新的推测执行难免会被丢弃。
 */
void
SpmtThread::discard_effect(Effect* effect)
{
    // 释放C中的栈帧（不用从状态缓存中清除栈帧中的内容，等会一把丢弃）
    for (Frame* f : effect->C) {
        delete f;
    }

    // 丢弃快照中记录的版本以及之后所有版本（若无快照，丢弃最新版本）
    if (effect->snapshot) {
        m_state_buffer.discard(effect->snapshot->version);
    }
    else {
        m_state_buffer.discard(m_state_buffer.latest_ver());
    }

    // 收回消息（如果是往返消息，则收回）
    if (effect->msg_sent and g_is_async_msg(effect->msg_sent)) {
        revoke_spec_msg(effect->msg_sent);
    }

    // 释放RVP栈帧
    for (Frame* f : V) {
        delete f;
    }
}


void
SpmtThread::abort_uncertain_execution()
{
    MINILOG0_IF(debug_scaffold::java_main_arrived,
                "#" << id() << " discard uncertain execution");


    // for each 队列中每个消息
    //     收回effect中发出的消息
    //     处理effect中记录的栈帧
    //     释放effect

    // 清空state buffer与rvp buffer

    for (auto i = m_spec_msg_queue.begin(); i != m_iter_next_spec_msg; ++i) {
        Message* msg = *i;
        Effect* effect = msg->get_effect();

        revoke_spec_msg(effect->msg_sent);

        for (auto frame : effect->C) {
            delete frame;
        }

        delete effect;
    }


    m_state_buffer.reset();
    m_rvp_buffer.clear();

    m_need_spec_msg = false;

}


void
SpmtThread::verify_speculation(Message* certain_msg)
{

    // if 无待验证消息
    //     处理确定消息
    //     结束
    if (m_spec_msg_queue.begin() == m_iter_next_spec_msg) {
        process_msg(certain_msg);
        return;
    }


    // 比较第一个待验证消息与确定消息（异步消息比指针，同步消息比内容）
    Message* spec_msg = *m_spec_msg_queue.begin();
    bool success = false;
    if (g_is_async_msg(certain_msg)) {
        success = (spec_msg == certain_msg);
    }
    else {
        success = g_equal_msg_content(spec_msg, certain_msg);
    }



    // if 二者相同
    //     提交第一个消息关联的effect
    //     释放该effect
    //     从队列中删除该消息
    // else
    //     丢弃所有effect
    //     if 确定消息是异步消息
    //         收回该消息
    //     处理确定消息
    if (success) {
        Effect* effect = spec_msg->get_effect();
        commit_effect(effect);
        delete effect;
        m_spec_msg_queue.pop_front();
    }
    else {
        abort_uncertain_execution();

        if (g_is_async_msg(certain_msg)) {
            remove_revoked_msg(certain_msg);
        }
        process_msg(certain_msg);
    }

}


// 该方法跟process_spec_msg有很多重复，可以考虑合并为一个方法process_msg放在Mode中
// void
// SpmtThread::process_certain_msg(Message* msg)
// {
//     Message::Type type = msg->get_type();

//     if (type == Message::invoke) {

//         InvokeMsg* msg = static_cast<InvokeMsg*>(msg);

//         m_certain_mode.invoke_impl(msg->get_target_object(),
//                                        msg->mb, &msg->parameters[0],
//                                        msg->caller_pc,
//                                        msg->caller_frame,
//                                        msg->caller_sp);

//     }
//     else if (type == Message::put) {

//         PutMsg* put_msg = static_cast<PutMsg*>(msg);

//         // 向put消息指定的字段写入一个指定的数值
//         uintptr_t* field_addr = put_msg->get_field_addr();
//         for (int i = 0; i < put_msg->val.size(); ++i) {
//             m_certain_mode.write(field_addr + i, put_msg->val[i]);
//         }

//         PutReturnMsg* put_return_msg = new PutReturnMsg(put_msg->get_source_spmt_thread());

//         send_certain_msg(put_return_msg);


//     }
//     else if (type == Message::get) {

//         GetMsg* get_msg = static_cast<GetMsg*>(msg);

//         // 从get消息的指定的字段中读取一个值，并根据该值构造get_return消息
//         uintptr_t* field_addr = get_msg->get_field_addr();
//         int field_size = get_msg->get_field_size();
//         std::vector<uintptr_t> value;
//         for (int i = 0; i < field_size; ++i) {
//             value.push_back(m_certain_mode.read(field_addr + i));
//         }


//         GetReturnMsg* get_return_msg = new GetReturnMsg(get_msg->get_source_spmt_thread(),
//                                                         &value[0],
//                                                         value.size());

//         send_certain_msg(get_return_msg);

//     }
//     else if (type == Message::arraystore) {

//         ArrayStoreMsg* arraystore_msg = static_cast<ArrayStoreMsg*>(msg);

//         // 向arraystore消息指定的数组的指定索引处写入一个指定的数值

//         Object* array = arraystore_msg->get_target_object();
//         int index = arraystore_msg->index;
//         int type_size = arraystore_msg->type_size;
//         void* addr = array_elem_addr(array, index, type_size);

//         m_certain_mode.store_to_array_from_c(&arraystore_msg->val[0],
//                                              array, index, type_size);

//         ArrayStoreReturnMsg* arraystore_return_msg = new ArrayStoreReturnMsg(arraystore_msg->get_source_spmt_thread());
//         send_certain_msg(arraystore_return_msg);

//     }
//     else if (type == Message::arrayload) {

//         ArrayLoadMsg* arrayload_msg = static_cast<ArrayLoadMsg*>(msg);

//         // 从arrayload消息指定的数组的指定索引处读入一个数值

//         Object* array = arrayload_msg->get_target_object();
//         int index = arrayload_msg->index;
//         int type_size = arrayload_msg->type_size;
//         void* addr = array_elem_addr(array, index, type_size);

//         std::vector<uintptr_t> value(2); // at most 2 slots
//         m_certain_mode.load_from_array_to_c(&value[0],
//                                             array, index, type_size);

//         ArrayLoadReturnMsg* arrayload_return_msg = new ArrayLoadReturnMsg(arrayload_msg->get_source_spmt_thread(), &value[0], value.size());
//         send_certain_msg(arrayload_return_msg);


//     }
//     else if (type == Message::ret) {

//         ReturnMsg* msg = static_cast<ReturnMsg*>(msg);

//         m_certain_mode.pc = msg->caller_pc;
//         m_certain_mode.frame = msg->caller_frame;
//         m_certain_mode.sp = msg->caller_sp;

//         // 将返回值写入ostack
//         for (auto i : msg->retval) {
//             *m_certain_mode.sp++ = i;
//         }

//         m_certain_mode.pc += (*m_certain_mode.pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

//     }
//     else if (type == Message::put_return) {

//         PutReturnMsg* put_return_msg = static_cast<PutReturnMsg*>(msg);
//         // 什么也不需要做

//         m_certain_mode.pc += 3;

//     }
//     else if (type == Message::get_return) {

//         GetReturnMsg* get_return_msg = static_cast<GetReturnMsg*>(msg);

//         for (auto i : get_return_msg->val) {
//             *m_certain_mode.sp++ = i;
//         }

//         m_certain_mode.pc += 3;

//     }
//     else if (type == Message::arraystore_return) {
//         // 没有什么需要写入ostack

//         m_spec_mode.pc += 1;
//     }
//     else if (type == Message::arrayload_return) {
//         ArrayLoadMsg* arrayload_msg = static_cast<ArrayLoadMsg*>(msg);

//         // 将读到的值写入ostack
//         // for (auto i : msg->val) {
//         //     m_spec_mode.write(m_spec_mode.sp++, i);
//         // }
//         // load_from_array_to_c(m_spec_mode.sp,

//         //                       );

//         m_spec_mode.pc += 3;
//     }
//     else {
//         assert(false);
//     }

// }


void
SpmtThread::before_signal_exception(Class *exception_class)
{
    m_mode->before_signal_exception(exception_class);
}

void
SpmtThread::before_alloc_object()
{
    m_mode->before_alloc_object();
}

void
SpmtThread::after_alloc_object(Object* obj)
{
    m_mode->after_alloc_object(obj);
}


GroupingPolicyEnum
default_policy(Object* obj)
{
    if (is_normal_obj(obj))
        return GP_CURRENT_GROUP;

    if (is_class_obj(obj))
        return GP_NO_GROUP;

    assert(false);  // todo
}


GroupingPolicyEnum
query_grouping_policy_for_object(Object* object)
{
    // 先问新对象自己
    GroupingPolicyEnum policy = get_policy(object);
    if (policy != GP_UNSPECIFIED)
        return policy;

    // 再问当前对象
    SpmtThread* current_spmt_thread = g_get_current_spmt_thread();
    Object* current_object = current_spmt_thread->get_current_object();
    policy = get_foreign_policy(current_object);
    if (policy != GP_UNSPECIFIED)
        return policy;

    // 再问当前线程
    policy = current_spmt_thread->get_leader_policy();
    if (policy != GP_UNSPECIFIED)
        return policy;

    // 最后采用缺省策略
    return ::default_policy(object);

}


SpmtThread*
SpmtThread::assign_spmt_thread_for(Object* object)
{
    GroupingPolicyEnum policy = ::query_grouping_policy_for_object(object);

    SpmtThread* spmt_thread = 0;

    if (policy == GP_NEW_GROUP) {
        spmt_thread = RopeVM::instance()->create_spmt_thread();
        spmt_thread->set_thread(this->get_thread());
        spmt_thread->set_leader(object);
    }
    else if (policy == GP_CURRENT_GROUP)
        spmt_thread = this;
    else if (policy == GP_NO_GROUP)
        spmt_thread = g_get_current_thread()->get_initial_spmt_thread();

    assert(spmt_thread);

    return spmt_thread;
}

Mode*
SpmtThread::get_current_mode()
{
    return m_mode;
}


void
SpmtThread::process_msg(Message* msg)
{
    m_mode->process_msg(msg);
}

// void
// SpmtThread::process_spec_msg(Message* msg)
// {
//     m_need_spec_msg = false;

//     MINILOG(task_load_logger, "#" << id() << " spec msg loaded: " << *msg);

//     Message::Type type = msg->get_type();

//     if (type == Message::invoke) {

//         InvokeMsg* invoke_msg = static_cast<InvokeMsg*>(msg);

//         Frame* new_frame = m_spec_mode.create_frame(invoke_msg->get_target_object(),
//                                                     invoke_msg->mb,
//                                                     &invoke_msg->parameters[0],
//                                                     invoke_msg->get_source_spmt_thread(),
//                                                     0,
//                                                     0,
//                                                     0);


//         m_spec_mode.sp = (uintptr_t*)new_frame->ostack_base;
//         m_spec_mode.frame = new_frame;
//         m_spec_mode.pc = (CodePntr)new_frame->mb->code;
//     }
//     else if (type == Message::put) {
//         PutMsg* put_msg = static_cast<PutMsg*>(msg);

//         uintptr_t* field_addr = put_msg->get_field_addr();

//         for (int i = 0; i < put_msg->val.size(); ++i) {
//             m_spec_mode.write(field_addr + i, put_msg->val[i]);
//         }

//         PutReturnMsg* put_return_msg = new PutReturnMsg(put_msg->get_source_spmt_thread());
//         // 将put_return消息记录在effect中
//         send_spec_msg(put_return_msg);

//         launch_next_spec_msg();
//     }
//     else if (type == Message::get) {
//         GetMsg* get_msg = static_cast<GetMsg*>(msg);

//         // 从get消息的指定的字段中读取一个值，并根据该值构造get_return消息
//         uintptr_t* field_addr = get_msg->get_field_addr();
//         int field_size = get_msg->get_field_size();
//         std::vector<uintptr_t> value;
//         for (int i = 0; i < field_size; ++i) {
//             value.push_back(m_spec_mode.read(field_addr + i));
//         }


//         GetReturnMsg* get_return_msg = new GetReturnMsg(get_msg->get_source_spmt_thread(),
//                                                         &value[0], value.size());

//         // 将get_return消息记录在effect中
//         send_spec_msg(get_return_msg);

//         launch_next_spec_msg();
//     }
//     else if (type == Message::arraystore) {
//         ArrayStoreMsg* arraystore_msg = static_cast<ArrayStoreMsg*>(msg);

//         Object* array = arraystore_msg->get_target_object();
//         int index = arraystore_msg->index;
//         int type_size = arraystore_msg->type_size;
//         void* addr = array_elem_addr(array, index, type_size);

//         m_spec_mode.store_to_array_from_c(&arraystore_msg->val[0],
//                                           array, index, type_size);

//         ArrayStoreReturnMsg* arraystore_return_msg = new ArrayStoreReturnMsg(arraystore_msg->get_source_spmt_thread());
//         // 将消息记入effect
//         send_spec_msg(arraystore_return_msg);

//         launch_next_spec_msg();
//     }
//     else if (type == Message::arrayload) {

//         ArrayLoadMsg* arrayload_msg = static_cast<ArrayLoadMsg*>(msg);

//         // 从arrayload消息指定的数组的指定索引处读入一个数值

//         Object* array = arrayload_msg->get_target_object();
//         int index = arrayload_msg->index;
//         int type_size = arrayload_msg->type_size;
//         void* addr = array_elem_addr(array, index, type_size);

//         std::vector<uintptr_t> value(2); // at most 2 slots
//         m_certain_mode.load_from_array_to_c(&value[0],
//                                             array, index, type_size);

//         ArrayLoadReturnMsg* arrayload_return_msg = new ArrayLoadReturnMsg(arrayload_msg->get_source_spmt_thread(),
//                                                                           &value[0], value.size());

//         // 将消息记入effect
//         send_spec_msg(arrayload_return_msg);

//         launch_next_spec_msg();
//     }
//     else if (type == Message::ret) {
//         ReturnMsg* return_msg = static_cast<ReturnMsg*>(msg);

//         // 将返回值写入ostack
//         for (auto i : return_msg->retval) {
//             m_spec_mode.write(m_spec_mode.sp++, i);
//         }

//         m_spec_mode.pc += (*m_spec_mode.pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
//     }
//     else if (type == Message::put_return) {
//         PutReturnMsg* put_return_msg = static_cast<PutReturnMsg*>(msg);
//         // 什么也不需要做

//         m_spec_mode.pc += 3;
//     }
//     else if (type == Message::get_return) {

//         GetReturnMsg* get_return_msg = static_cast<GetReturnMsg*>(msg);

//         for (auto i : get_return_msg->val) {
//             m_spec_mode.write(m_spec_mode.sp++, i);
//         }

//         m_spec_mode.pc += 3;
//     }
//     else if (type == Message::arraystore_return) {
//         // 没有什么需要写入ostack

//         m_spec_mode.pc += 1;
//     }
//     else if (type == Message::arrayload_return) {
//         ArrayLoadReturnMsg* arrayloadreturn_msg = static_cast<ArrayLoadReturnMsg*>(msg);

//         // 将读到的值写入ostack
//         for (auto i : arrayloadreturn_msg->val) {
//             m_spec_mode.write(m_spec_mode.sp++, i);
//         }


//         m_spec_mode.pc += 3;
//     }
//     else {
//         assert(false);
//     }

// }


bool
SpmtThread::has_unprocessed_spec_msg()
{
    if (m_iter_next_spec_msg != m_spec_msg_queue.end())
        return true;

    return false;
}


void
SpmtThread::launch_next_spec_msg()
{
    // 不一定有待处理的消息
    MINILOG(task_load_logger, "#" << id() << " try to load a spec msg");

    if (not has_unprocessed_spec_msg()) {
        MINILOG(task_load_logger, "#" << id() << " no spec msg, waiting for spec msg");

        m_spec_mode.pc = 0;
        m_spec_mode.frame = 0;
        m_spec_mode.sp = 0;
        m_need_spec_msg = true;

        sleep();
        return;
    }

    // 在处理新的推测之前对处理上一消息形成的状态进行快照
    snapshot(false);

    // 使下一个待处理消息成为当前消息
    ++m_iter_next_spec_msg;

    // assert(type == Message::invoke
    //        or type == Message::put
    //        or type == Message::get
    //        or type == Message::arraystore
    //        or type == Message::arrayload);
    m_current_spec_msg = *m_iter_next_spec_msg;
    process_msg(m_current_spec_msg);
}


void
SpmtThread::launch_spec_msg(Message* msg)
{
    // 在处理新的推测之前对处理上一消息形成的状态进行快照
    snapshot(false);

    // 将msg插在待处理消息之前，作为当前消息
    m_spec_msg_queue.insert(m_iter_next_spec_msg, msg);

    m_current_spec_msg = msg;
    process_msg(m_current_spec_msg);
    // if 刚处理的是get,put等，就要加载下一条。
}


void
SpmtThread::pin_frames()
{
    Frame* f = m_spec_mode.frame;

    for (;;) {
        MINILOG(snapshot_logger, "#" << m_id
                << " pin frame(" << f << ")" << " for " << *f->mb);

        if (f->pinned)
            break;

        f->pinned = true;

        // execute_method开始新的执行，top frame就是新老执行的边界。新老执行的推测执行彼此无关。
        if (f->is_top_frame())
            break;

        f = f->prev;

        // 只管钉住本线程创建
        if (f->caller != this)
            break;

    }
}


void
SpmtThread::snapshot(bool pin)
{
    Snapshot* snapshot = new Snapshot;

    snapshot->version = m_state_buffer.latest_ver();
    m_state_buffer.freeze();

    snapshot->pc = m_spec_mode.pc;
    snapshot->frame = m_spec_mode.frame;
    snapshot->sp = m_spec_mode.sp;

    Effect* current_effect = m_current_spec_msg->get_effect();
    current_effect->snapshot = snapshot;

    if (pin)
        pin_frames();

}


void
SpmtThread::on_event_top_invoke(InvokeMsg* msg)
{
    // assert 必为确定模式
    abort_uncertain_execution();

    // 按正常处理invoke消息那样处理
    m_certain_mode.invoke_impl(msg->get_target_object(),
                               msg->mb, &msg->parameters[0],
                               msg->get_source_spmt_thread(),
                               0, 0, 0);

}


void
SpmtThread::on_event_top_return(ReturnMsg* msg)
{
    // 把消息中的返回值写入接收返回值的dummy frame的ostack
    for (auto i : msg->retval) {
        *m_certain_mode.sp++ = i;
    }

    // 不需要调整pc，因为dummy frame没有对应的方法

    signal_quit_step_loop(0);
}


uintptr_t*
SpmtThread::drive_loop()
{
    using namespace std;

    SpmtThread* current_spmt_thread_of_outer_loop = m_thread->m_current_spmt_thread;

    for (;;) {

        // 执行会往m_spmt_threads中添加新spmt线程，会导致迭代器失效。
        // 所以先复制一份，在复制品中遍历
        vector<SpmtThread*> spmt_threads(m_thread->m_spmt_threads);

        int non_idle_total = 0;
        for (auto st : spmt_threads) {
            m_thread->m_current_spmt_thread = st;

            if (st->is_halt()) {
                st->idle(); // only serve for statistics
                continue;
            }

            non_idle_total++;


            st->step();


            bool quit = st->check_quit_step_loop();
            if (quit) {
                uintptr_t* result = st->get_result();

                m_thread->m_current_spmt_thread = current_spmt_thread_of_outer_loop;
                return result;
            }

        }

        assert(non_idle_total > 0);
    }

    assert(false);
    return 0;
}
