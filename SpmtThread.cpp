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
    m_id(id),
    m_thread(0),
    m_leader(0),
    m_halt(true),
    m_quit_drive_loop(false),
    m_quit_causer(0),
    m_certain_message(0),
    m_need_spec_msg(true),
    m_excep_threw_to_me(0),
    m_excep_frame(0)
{

    m_certain_mode.set_spmt_thread(this);
    m_spec_mode.set_spmt_thread(this);
    m_rvp_mode.set_spmt_thread(this);

    m_mode = &m_spec_mode;

    m_iter_next_spec_msg = m_spec_msg_queue.begin();

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


Thread*
SpmtThread::get_thread()
{
    return m_thread;
}


void
SpmtThread::set_thread(Thread* thread)
{
    assert(m_thread == 0);

    m_thread = thread;
    thread->m_spmt_threads.push_back(this);
}


void
SpmtThread::set_leader(Object* leader)
{
    m_leader = leader;
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
        cout << "DeepBreak" << endl;
    }
}

void
SpmtThread::idle()
{
    // stat
    m_count_idle++;
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
        // MINILOG0("#" << id() << " (state)start");
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
    // MINILOG0("#" << id() << " (state)sleep");
    m_halt = true;
}


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
SpmtThread::switch_to_certain_mode()
{
    // MINILOG0("#" << id() << " CERT mode");
    m_mode = &m_certain_mode;
}

void
SpmtThread::switch_to_speculative_mode()
{
    // MINILOG0("#" << id() << " SPEC mode");
    m_mode = &m_spec_mode;
}

void
SpmtThread::switch_to_rvp_mode()
{
    // stat
    m_count_rvp++;

    // MINILOG0("#" << id() << " RVP mode");

    m_mode = &m_rvp_mode;
}


void
SpmtThread::switch_to_previous_mode()
{
    assert(is_certain_mode());
    m_mode = m_previous_mode;
}

void
SpmtThread::add_spec_msg(Message* msg)
{
    // stat
    // m_count_spec_msgs_sent++;

    m_spec_msg_queue.push_back(msg);

    // 如果本线程是因为推测执行缺乏任务而睡眠，则唤醒。
    if (m_halt and is_spec_mode() and m_need_spec_msg) {
        // MINILOG0("#" << id() << " is waken(sleeping, waiting for task)");
        wakeup();
    }
}


void
SpmtThread::destroy_frame(Frame* frame)
{
    m_mode->destroy_frame(frame);
}


bool
SpmtThread::check_quit_drive_loop()
{
    bool quit = m_quit_drive_loop;
    if (quit) {
        m_quit_drive_loop = false;
    }
    return quit;
}

void
SpmtThread::signal_quit_drive_loop()
{
    assert(is_certain_mode());

    m_quit_drive_loop = true;
}


// 在多os线程实现方式下，应该是调用os_api_current_os_thread()获得当前的
// os线程，然后再从thread local strage中获得SpmtThread*
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
SpmtThread::clear_frame_in_state_buffer(Frame* f)
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

    delete effect;
}


void
SpmtThread::confirm_spec_msg(Message* msg)
{
    assert(is_certain_mode());

    switch_to_previous_mode();  // 转入之前的模式：推测模式或rvp模式
    msg->get_target_spmt_thread()->set_certain_msg(msg);
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
        delete msg;
    }

    m_revoked_msgs.clear();
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
          从后向前遍历区间[iter_to_revoke + 1, iter_next)内的消息
              丢弃与消息关联的effect
              if 消息是异步消息
                  让iter_next指向该位置
              else
                  释放该消息
                  移除该消息
          从队列中删除被收回的消息
     */

    Message* revoked_msg = *iter_revoked_msg;
    if (revoked_msg->get_effect() == nullptr) { // 被收回的消息在待处理部分
        if (iter_revoked_msg == m_iter_next_spec_msg) {
            ++m_iter_next_spec_msg;
        }
        m_spec_msg_queue.erase(iter_revoked_msg);
    }
    else {                      // 被收回的消息在待验证部分
        auto reverse_iter_msg = reverse_iterator<decltype(m_iter_next_spec_msg)>(m_iter_next_spec_msg);
        auto reverse_iter_revoked_msg = reverse_iterator<decltype(iter_revoked_msg)>(iter_revoked_msg);

        while (reverse_iter_msg != reverse_iter_revoked_msg) {
            Message* msg = *reverse_iter_msg;
            discard_effect(msg->get_effect());
            if (g_is_async_msg(msg)) {
                // 从待验证部分拿掉异步消息时不销毁，而是进入待处理部分。
                m_iter_next_spec_msg = reverse_iter_msg.base();
                --m_iter_next_spec_msg;
            }
            else {
                // 从待验证部分拿掉同步消息时销毁消息
                delete msg;
                ++reverse_iter_msg;
                m_spec_msg_queue.erase(reverse_iter_msg.base());
            }
        }

        // 拿掉被收回的消息
        assert(iter_revoked_msg == m_iter_next_spec_msg); // 经过上面的循环，此处必相等。
        ++m_iter_next_spec_msg;
        m_spec_msg_queue.erase(iter_revoked_msg);

        m_need_spec_msg = false; // 表明推测执行需要加载推测消息（因为收回消息把人家正在处理的消息下马了）
        switch_to_speculative_mode();
        wakeup();

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

    // 收回消息（如果是异步消息，则收回）
    if (effect->msg_sent) {
        if (g_is_async_msg(effect->msg_sent)) {
            revoke_spec_msg(effect->msg_sent);
        }
        else {
            delete effect->msg_sent;
        }
    }

    delete effect;

    // 释放RVP栈帧
    for (Frame* f : V) {
        delete f;
    }
    V.clear();
}


void
SpmtThread::abort_uncertain_execution()
{
    // MINILOG0_IF(debug_scaffold::java_main_arrived,
    //             "#" << id() << " discard uncertain execution");

    /*
      for each 队列中每个消息
          如果有关联的effect，丢弃effect。
          如果是同步消息，拿掉。
    */
    for (auto n = m_spec_msg_queue.begin(); n != m_iter_next_spec_msg; ) {
        auto i = n++;

        Message* msg = *i;
        Effect* effect = msg->get_effect();

        if (effect) {
            discard_effect(effect);
        }

        if (not g_is_async_msg(msg)) {
            delete msg;
            m_spec_msg_queue.erase(i);
        }

    }

    // 所有消息变为待处理
    m_iter_next_spec_msg = m_spec_msg_queue.begin();

    // 清空state buffer与rvp buffer
    m_state_buffer.reset();
    m_rvp_buffer.clear();

}


void
SpmtThread::verify_speculation(Message* certain_msg)
{
    assert(is_certain_mode());

    // if 无待验证消息
    //     处理确定消息
    //     结束
    if (m_spec_msg_queue.begin() == m_iter_next_spec_msg) {
        process_msg(certain_msg);
        delete certain_msg;
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
        // 验证成功时，确定消息得销毁
        delete certain_msg;

        Effect* effect = spec_msg->get_effect();
        assert(effect);
        commit_effect(effect);

        m_spec_msg_queue.pop_front();

        // 验证成功时，推测消息无论是异步还是同步，都要销毁。
        delete spec_msg;
    }
    else {
        abort_uncertain_execution();

        // 验证失败时，确定消息如果是同步的，就立即销毁，如果是异步的，还得从队列中拿掉，此时再删除。
        if (g_is_async_msg(certain_msg)) {
            remove_revoked_msg(certain_msg);
        }

        process_msg(certain_msg);

        if (not g_is_async_msg(certain_msg)) {
            delete certain_msg;
        }

        // 验证失败时，推测消息只有同步的才销毁，异步的仍在队列中。
        if (not g_is_async_msg(spec_msg)) {
            delete spec_msg;
        }

    }

}


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


Mode*
SpmtThread::get_current_mode()
{
    return m_mode;
}


// 如果是确定模式下send_msg，发出去的就是确定消息。如果是推测模式下
// send_msg，发出去的就是推测消息。
void
SpmtThread::send_msg(Message* msg)
{
    m_mode->send_msg(msg);
}


/*
  为了在模式已经改变的情况下，按照原来的模式发出消息，所以提供了这个函
  数。
 */
// void
// SpmtThread::send_msg(Message* msg, Mode* mode)
// {
//     mode->send_msg(msg);
// }


void
SpmtThread::process_msg(Message* msg)
{
    m_mode->process_msg(msg);
}


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
    m_current_spec_msg->set_effect(new Effect); // 处理前给消息配上effect
    m_need_spec_msg = false;
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
    m_current_spec_msg->set_effect(new Effect); // 处理前给消息配上effect
    m_need_spec_msg = false;
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
    assert(is_certain_mode());

    abort_uncertain_execution();

    // 按正常处理invoke消息那样处理
    m_certain_mode.invoke_impl(msg->get_target_object(),
                               msg->mb, &msg->parameters[0],
                               msg->get_source_spmt_thread(),
                               msg->caller_pc,
                               msg->caller_frame,
                               msg->caller_sp);


}


void
SpmtThread::on_event_top_return(ReturnMsg* return_msg)
{
    // 方法重入会导致这些东西改变，所以需要按消息中的重新设置。
    m_certain_mode.pc = return_msg->caller_pc;
    m_certain_mode.frame = return_msg->caller_frame;
    m_certain_mode.sp = return_msg->caller_sp;

    // 把消息中的返回值写入接收返回值的dummy frame的ostack
    for (auto i : return_msg->retval) {
        *m_certain_mode.sp++ = i;
    }

    assert(m_certain_mode.pc == 0);
    assert(m_certain_mode.frame->is_dummy());

    signal_quit_drive_loop();
}


void
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


            bool quit = st->check_quit_drive_loop();
            if (quit) {
                m_quit_causer = st;

                m_thread->m_current_spmt_thread = current_spmt_thread_of_outer_loop;
                return;
            }

        }

        assert(non_idle_total > 0);
    }

    assert(false);
}


Object*
SpmtThread::get_current_object()
{
    Frame* current_frame = m_mode->frame;
    return current_frame->get_object();;
}


GroupingPolicyEnum
SpmtThread::get_policy()
{
    /*
      初始线程无leader。所谓leader，即因其诞生而导致spmt线程被创建的对象
      如果有leader，返回leader的policy
      否则，说不知道。
     */
    if (m_leader) {
        return get_foreign_policy(m_leader);
    }

    return GP_UNSPECIFIED;
}


Object*
SpmtThread::get_exception_threw_to_me()
{
    Object* excep = 0;
    if (m_excep_threw_to_me) {
        excep = m_excep_threw_to_me;
        m_excep_threw_to_me = 0;
    }

    return excep;
}


void
SpmtThread::set_exception_threw_to_me(Object* exception, Frame* excep_frame)
{
    m_excep_threw_to_me = exception;
    m_excep_frame = excep_frame;

    wakeup();
}


void
SpmtThread::on_event_exception_throw_to_me(Object* exception)
{
    // 废弃推测状态
    abort_uncertain_execution();
    process_exception(exception, m_excep_frame);
}


void
SpmtThread::do_throw_exception()
{
    Object* excep = m_thread->exception;
    m_thread->exception = NULL;

    Frame* current_frame = m_certain_mode.frame;
    process_exception(excep, current_frame);
}



// excep.cpp
extern CodePntr findCatchBlockInMethod(MethodBlock *mb, Class *exception, CodePntr pc_pntr);

void
SpmtThread::process_exception(Object* excep, Frame* excep_frame)
{
    assert(excep_frame->owner == this);

    // MINILOG(c_exception_logger, "#" << m_id << " (C) throw exception"
    //         << " in: " << info(frame)
    //         << " on: #" << frame->get_object()->get_spmt_thread()->id()
    //         );


    /*
      在当前栈帧中查找异常处理器
      如果找到，结束。
      如果没找到
          在去上级栈帧找之前，先看上级栈帧是不是属于其他线程，或者是dummy frame
          如果上级栈帧属于其他线程，就通知该线程有异常。
          如果是dummy，结束。
          如果是自己的栈帧，又不是 dummy frame，则开始在其中查找。
     */

    Frame* current_frame = excep_frame;
    CodePntr handler_pc = findCatchBlockInMethod(current_frame->mb,
                                                 excep->classobj,
                                                 current_frame->last_pc);
    while (handler_pc == NULL) {
        if (current_frame->is_top_frame()) {
            break;
        }

        if (current_frame->prev->owner != this) {
            // 通知current_frame->prev->owner有异常。
            switch_to_speculative_mode();
            current_frame->prev->owner->set_exception_threw_to_me(excep,
                                                                  current_frame->prev);
            return;
        }

        if (current_frame->mb->is_synchronized()) { // 解开栈帧之前要给同步方法解锁
            Object *sync_ob = current_frame->get_object();
            objectUnlock(sync_ob);
        }

        current_frame = current_frame->prev;
        // destroy_frame();
        handler_pc = findCatchBlockInMethod(current_frame->mb,
                                            excep->classobj,
                                            current_frame->last_pc);

    }
    // MINILOG(c_exception_logger, "#" << threadSelf()->get_current_spmt_thread()->id() << " finding handler"
    //         << " in: " << info(frame)
    //         << " on: #" << frame->get_object()->get_spmt_thread()->id()
    //         );

    // MINILOG(c_exception_logger, "#" << m_spmt_thread->id() << " (C) handler found"
    //         << " in: " << info(current_frame)
    //         << " on: #" << frame->get_object()->get_spmt_thread()->id()
    //         );



    /*
      流程至此，要么是找到异常处理器，要么是找罢top frame也没找到。如果
      是后者，只能向发出top invoke的人表明异常仍然存在。同时top frame也
      该结束了（因为异常）。
     */
    if (handler_pc == NULL) {
        m_thread->exception = excep;

        signal_quit_drive_loop();
        return;
    }

    // 如果找到异常处理器，设置好(pc, frame, sp)。
    m_certain_mode.pc = handler_pc;
    m_certain_mode.frame = current_frame;
    m_certain_mode.sp = current_frame->ostack_base;
    *m_certain_mode.sp++ = (uintptr_t)excep;

}
