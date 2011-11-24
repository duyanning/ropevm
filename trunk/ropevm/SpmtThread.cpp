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
#include "Break.h"
#include "Triple.h"

using namespace std;

SpmtThread::SpmtThread(int id)
:
    m_id(id),
    m_thread(nullptr),
    m_leader(nullptr),
    m_halt(true),
    m_quit_drive_loop(false),
    m_quit_causer(nullptr),
    m_certain_message(nullptr),
    m_current_spec_msg(nullptr),
    m_spec_running_state(RunningState::halt_no_asyn_msg),
    m_excep_threw_to_me(nullptr),
    m_excep_frame(nullptr)
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

    m_mode->step();
}

void
SpmtThread::idle()
{
    // stat
    m_count_idle++;
}


ostream&
operator<<(ostream& os, RunningState reason)
{
    if (reason == RunningState::ongoing)
        os << "ongoing";
    else if (reason == RunningState::ongoing_but_need_launch_new_msg)
        os << "ongoing_but_need_launch_new_msg";
    else if (reason == RunningState::halt_no_asyn_msg)
        os << "halt_no_asyn_msg";
    else if (reason == RunningState::halt_no_syn_msg)
        os << "halt_no_syn_msg";
    else if (reason == RunningState::halt_cannot_signal_exception)
        os << "halt_cannot_signal_exception";
    else if (reason == RunningState::halt_cannot_alloc_object)
        os << "halt_cannot_alloc_object";
    else if (reason == RunningState::halt_cannot_exec_priviledged_method)
        os << "halt_cannot_exec_priviledged_method";
    else if (reason == RunningState::halt_cannot_exec_method)
        os << "halt_cannot_exec_method";
    else if (reason == RunningState::halt_worthless_to_execute)
        os << "halt_worthless_to_execute";
    else
        os << "unknow state";
    return os;
}

void
SpmtThread::wakeup()
{
    MINILOG(wakeup_halt_logger, "#" << id() << " awake. "
            << "(before: " << m_spec_running_state << ")");
    if (m_halt) {
        m_halt = false;
    }
}


void
SpmtThread::halt(RunningState reason)
{
    m_halt = true;
    m_spec_running_state = reason;
    MINILOG(wakeup_halt_logger, "#" << id() << " halt. "
            << "(now: " << m_spec_running_state << ")");
}


// void
// SpmtThread::halt()
// {
//     m_halt = true;
//     MINILOG(wakeup_halt_logger, "#" << id() << " halt " << m_spec_running_state);
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
        msg = m_certain_message;
        m_certain_message = 0;
    }

    return msg;
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

    MINILOG_IF(is_spec_mode(), certain_msg_logger, "#" << m_id
               << " resume SPEC mode"
               << Triple(m_spec_mode.pc, m_spec_mode.frame, m_spec_mode.sp)
               << " " << m_spec_running_state);

    MINILOG_IF(is_rvp_mode(), certain_msg_logger, "#" << m_id
               << " resume RVP mode"
               << Triple(m_rvp_mode.pc, m_rvp_mode.frame, m_rvp_mode.sp)
               << " " << m_spec_running_state);

}


void
SpmtThread::add_spec_msg(Message* msg)
{
    // stat
    // m_count_spec_msgs_sent++;

    /*
      如果队列中有待处理消息，添加到末尾就行。
      如果队列中无待处理消息，添加到末尾，然后让待处理指针指向该消息。
     */
    if (m_iter_next_spec_msg != m_spec_msg_queue.end()) {
        m_spec_msg_queue.push_back(msg);
    }
    else {
        m_iter_next_spec_msg = m_spec_msg_queue.insert(m_spec_msg_queue.end(), msg);
    }


    // 如果本线程是因为推测执行缺乏异步消息而停机，则唤醒。
    if (m_halt and m_spec_running_state == RunningState::halt_no_asyn_msg) {
        m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;
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
    // effect被提交，将被销毁，在该effect中创建的那些栈桢还记录有指向该
    // effect中C的迭代器。把这些迭代器标记为失效。
    for (Frame* f : effect->C) {
        f->is_iter_in_current_effect = false;
    }

    // 为effect->C与effect->R可能有交集，所以得在销毁R中栈桢之前设置该值。

    // 释放栈帧R（推测模式下返回时因为被钉住而没能释放）
    for (Frame* f : effect->R) {

        clear_frame_in_state_buffer(f);
        g_destroy_frame(f);
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
        int commit_ver = snapshot->version;
        m_state_buffer.commit(commit_ver);

        m_certain_mode.pc = snapshot->pc;
        m_certain_mode.frame = snapshot->frame;
        m_certain_mode.sp = snapshot->sp;

        MINILOG(snapshot_commit_logger, "#" << m_id << " commit "<< snapshot);

        delete snapshot;
        snapshot = nullptr;

    }
    else {
        int commit_ver = m_state_buffer.current_ver();
        m_state_buffer.commit(commit_ver);

        m_certain_mode.pc = m_spec_mode.pc;
        m_certain_mode.frame = m_spec_mode.frame;
        m_certain_mode.sp = m_spec_mode.sp;

        MINILOG(snapshot_commit_logger, "#" << m_id
                << " commit ("<< commit_ver << ") latest"
                << Triple(m_spec_mode.pc, m_spec_mode.frame, m_spec_mode.sp));

        // 此时已无推测状态。
        m_current_spec_msg = nullptr;
    }


    // 确认消息
    if (effect->msg_sent) {
        affirm_spec_msg(effect->msg_sent);
    }

    delete effect;
}


void
SpmtThread::resume_suspended_spec_execution()
{
    switch_to_previous_mode();  // 转入之前的模式：推测模式或rvp模式

    //assert(m_current_spec_msg); 这个断言有误，因为恢复之前的推测模式，而之前的推测模式可能没有异步消息正在处理。

    // 原因见SpeculativeMode::do_execute_method。
    if (m_spec_running_state == RunningState::halt_cannot_exec_method) {
        m_spec_running_state = RunningState::ongoing;
    }

    if (m_spec_running_state != RunningState::ongoing and
        m_spec_running_state != RunningState::ongoing_but_need_launch_new_msg) { // 如果之前处于停机状态，那就恢复停机状态。
        halt(m_spec_running_state);
    }
}


void
SpmtThread::start_afresh_spec_execution()
{
    m_spec_running_state = RunningState::ongoing;
}


void
SpmtThread::affirm_spec_msg(Message* msg)
{
    assert(is_certain_mode());

    resume_suspended_spec_execution();

    MINILOG(certain_msg_logger, "#" << m_id
            << " affirm spec msg to "
            <<"#" << msg->get_target_spmt_thread()->id()
            << " " << msg);

    MINILOG(control_transfer_logger, "#" << m_id
            << " transfer control to "
            << "#" << msg->get_target_spmt_thread()->id());

    msg->get_target_spmt_thread()->set_certain_msg(msg);
}

void
SpmtThread::revoke_spec_msg(RoundTripMsg* msg)
{
    MINILOG(spec_msg_logger, "#" << m_id
            << " revoke spec msg to "
            << "#" << msg->get_target_spmt_thread()->id()
            << " " << msg);

    msg->get_target_spmt_thread()->add_revoked_spec_msg(msg);
}


void
SpmtThread::add_revoked_spec_msg(RoundTripMsg* msg)
{
    m_revoked_msgs.push_back(msg);
}


void
SpmtThread::discard_all_revoked_msgs()
{
    if (m_revoked_msgs.empty())
        return;

    for (RoundTripMsg* msg : m_revoked_msgs) {
        discard_revoked_msg(msg);
    }

    m_revoked_msgs.clear();
}


// 丢弃被其他线程收回的消息
void
SpmtThread::discard_revoked_msg(RoundTripMsg* revoked_msg)
{
    MINILOG(spec_msg_logger, "#" << m_id
            << " discard spec msg from "
            << "#" << revoked_msg->get_source_spmt_thread()->id()
            << " " << revoked_msg);


    /*
      如果在队列中找不到该消息
          销毁该消息
          结束
    */
    auto i_revoked_msg = find(m_spec_msg_queue.begin(),
                                 m_spec_msg_queue.end(),
                                 revoked_msg);
    if (i_revoked_msg == m_spec_msg_queue.end()) {
        delete revoked_msg;
        return;
    }

    /*
      如果该消息在待处理部分
          从队列中移除并销毁
      否则（即在待验证部分）
          从后向前遍历受影响的其他消息（即区间[iter_to_revoke + 1, iter_next)内的消息）
              丢弃其effect
              如果是异步消息
                  变为待处理消息
              否则
                  从队列中移除并销毁
          移除并销毁被收回的消息
     */

    if (revoked_msg->get_effect() == nullptr) { // 被收回的消息在待处理部分

        // 如果被收回的消息恰巧就是下一个待处理消息，还得调整一下m_iter_next_spec_msg
        if (i_revoked_msg == m_iter_next_spec_msg) {
            ++m_iter_next_spec_msg;
        }

        delete revoked_msg;
        m_spec_msg_queue.erase(i_revoked_msg);

    }
    else {                      // 被收回的消息在待验证部分
        auto ri_msg = reverse_iterator<decltype(m_iter_next_spec_msg)>(m_iter_next_spec_msg);
        auto ri_revoked_msg = reverse_iterator<decltype(i_revoked_msg)>(i_revoked_msg);

        while (ri_msg != ri_revoked_msg) {
            Message* msg = *ri_msg++;

            discard_effect(msg->get_effect());
            msg->set_effect(0);

            if (g_is_async_msg(msg)) {
                // 异步消息变为待处理

                //++ri_msg;
                m_iter_next_spec_msg = ri_msg.base();
            }
            else {
                // 同步消息，移除并销毁

                //++ri_msg;

                // 注意，l.erase(ri.base())之后，下次迭代再*ri，
                // valgrind会说*ri读了l.erase(ri.base())释放的内存。所
                // 以我们要在l.erase(ri.base())之后，重新产生ri。
                auto i = ri_msg.base();
                ++i;
                m_spec_msg_queue.erase(ri_msg.base());
                ri_msg = reverse_iterator<decltype(i)>(i);

                delete msg;
            }
        }

        assert(i_revoked_msg == m_iter_next_spec_msg); // 经过上面的循环，此处必相等。

        // 移除并销毁被收回的消息
        ++m_iter_next_spec_msg;
        m_spec_msg_queue.erase(i_revoked_msg);
        delete revoked_msg;


        // 当前正在处理的消息必然下马。
        // 如果之前还有未被丢弃的待验证消息，将其作为m_current_spec_msg。
        // 否则，表明无当前推测消息。
        if (m_spec_msg_queue.begin() != m_iter_next_spec_msg) {
            auto iter_current_spec_msg = m_iter_next_spec_msg;
            --iter_current_spec_msg;
            m_current_spec_msg = *iter_current_spec_msg;
        }
        else {
            m_current_spec_msg = nullptr;
        }


        // 丢弃之前，可能处于推测模式或rvp模式。因为rvp执行必是推测执行
        // 的最前沿。所以但凡丢弃了待验证部分的消息，rvp执行必然被终止。
        // 所以无论如何，转入推测模式。
        switch_to_speculative_mode();


        // 表明推测执行需要加载推测消息（因为收回消息把人家正在处理的消息下马了）
        m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;

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
    assert(effect);

    // 释放C中的栈帧（不用从状态缓存中清除栈帧中的内容，等会一把丢弃）
    for (Frame* f : effect->C) {

        g_destroy_frame(f);
    }

    // 丢弃快照中记录的版本以及之后所有版本（若无快照，丢弃最新版本）
    if (effect->snapshot) {
        // 此时snapshot中的frame可以已经被销毁，所以不要访问frame所指对象。
        MINILOG(snapshot_discard_logger, "#" << m_id <<
                " discard (" << effect->snapshot->version << ")"
                << "(snapshot: " << (void*)effect->snapshot << ")");

        m_state_buffer.discard(effect->snapshot->version);

        delete effect->snapshot;
        effect->snapshot = nullptr;

    }
    else {
        MINILOG(snapshot_discard_logger, "#" << m_id
                << " discard ("<< m_state_buffer.current_ver() << ") latest");

        m_state_buffer.discard(m_state_buffer.current_ver());
    }

    // 收回消息（如果是异步消息，则收回）
    if (effect->msg_sent) {
        if (g_is_async_msg(effect->msg_sent)) {
            revoke_spec_msg(static_cast<RoundTripMsg*>(effect->msg_sent));
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
            msg->set_effect(0);
        }

        if (not g_is_async_msg(msg)) {
            delete msg;
            m_spec_msg_queue.erase(i);
        }

    }

    // 所有消息变为待处理
    m_iter_next_spec_msg = m_spec_msg_queue.begin();
    m_current_spec_msg = nullptr;

    // 清空state buffer与rvp buffer
    m_state_buffer.reset();
    m_rvp_buffer.clear();

    m_spec_running_state = RunningState::halt_no_asyn_msg;

}


void
SpmtThread::verify_speculation(Message* certain_msg)
{
    assert(is_certain_mode());

    // if 无待验证消息
    //     处理确定消息
    //     结束
    if (m_spec_msg_queue.begin() == m_iter_next_spec_msg) {

        MINILOG(verify_logger, "#" << m_id << " VERIFY empty." << " cert: " << certain_msg);

        if (g_is_async_msg(certain_msg)) {
            assert(m_iter_next_spec_msg == m_spec_msg_queue.begin());

            auto iter_certain_msg = find(m_spec_msg_queue.begin(),
                                         m_spec_msg_queue.end(),
                                         certain_msg);

            // 如果确定消息在队列中，将其移除。
            if (iter_certain_msg != m_spec_msg_queue.end()) {
                m_spec_msg_queue.erase(iter_certain_msg);

                // 万一被移除的消息恰好就是队列中的第一条消息，我们还得调整m_iter_next_spec_msg
                m_iter_next_spec_msg = m_spec_msg_queue.begin();
            }
        }

        process_msg(certain_msg);
        delete certain_msg;
        certain_msg = nullptr;

        return;
    }


    // 比较第一个待验证消息与确定消息（异步消息比指针，同步消息比内容）
    Message* spec_msg = *m_spec_msg_queue.begin();
    bool success = false;
    if (g_is_async_msg(certain_msg)) {
        success = (spec_msg == certain_msg);
    }
    else {
        success = certain_msg->equal(spec_msg);
    }



    // if 二者相同
    //     提交第一个消息关联的effect
    //     从队列中移除该消息
    // else
    //     丢弃所有effect
    if (success) {
        MINILOG(verify_logger, "#" << m_id <<
                " VERIFY ok." << " cert: " << certain_msg << " spec: " << spec_msg);

        m_spec_msg_queue.pop_front();

        Effect* effect = spec_msg->get_effect();
        assert(effect);

        commit_effect(effect);


        // 比较指针就相等的消息，确定消息和推测消息是同一个消息。只销毁一次。
        if (g_is_async_msg(certain_msg)) {
            assert(certain_msg == spec_msg);

            delete certain_msg;
            certain_msg = nullptr;
            spec_msg = nullptr;
        }
        else {
            delete certain_msg;
            certain_msg = nullptr;

            delete spec_msg;
            spec_msg = nullptr;
        }

    }
    else {
        MINILOG(verify_logger, "#" << m_id <<
                " VERIFY error." << " cert: " << certain_msg << " spec: " << spec_msg);

        abort_uncertain_execution();

        // 验证失败时，如果确定消息是个异步消息，那么它有可能在队列中。从队列中移除（但不销毁）。
        if (g_is_async_msg(certain_msg)) {
            // 因为上面的abort_uncertain_execution，此时队列中没有待验证消息。
            assert(m_iter_next_spec_msg == m_spec_msg_queue.begin());


            auto iter_certain_msg = find(m_spec_msg_queue.begin(),
                                         m_spec_msg_queue.end(),
                                         certain_msg);

            // 如果确定消息在队列中，将其移除。
            if (iter_certain_msg != m_spec_msg_queue.end()) {
                m_spec_msg_queue.erase(iter_certain_msg);

                // 万一被移除的消息恰好就是队列中的第一条消息，我们还得调整m_iter_next_spec_msg
                m_iter_next_spec_msg = m_spec_msg_queue.begin();
            }

        }

        process_msg(certain_msg);

        delete certain_msg;
        certain_msg = nullptr;

        // 推测消息一直放在队列中，它们的接受方从不因验证失败而销毁他们，
        // 只有在他们被发送方收回时才会被销毁。

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
    assert(RopeVM::model >= 2);

    if (RopeVM::model < 3)     // 模型3以上才支持推测执行
        return;

    assert(is_spec_mode());

    // 在加载新的消息之前，先丢弃被撤销的消息，免的加载了已经被撤销的消息。
    discard_all_revoked_msgs();


    // 不一定有待处理的消息
    MINILOG(task_load_logger, "#" << id() << " try to launch a spec msg");

    if (not has_unprocessed_spec_msg()) {
        MINILOG(task_load_logger, "#" << id() << " no spec msg, waiting for spec msg");

        m_spec_mode.pc = 0;
        m_spec_mode.frame = 0;
        m_spec_mode.sp = 0;

        halt(RunningState::halt_no_asyn_msg);
        //m_current_spec_msg = nullptr; 不能把m_current_spec_msg设为nullptr，因为使用下一个消息前快照还需要用上一个消息。

        return;
    }

    MINILOG(task_load_logger, "#" << id()
            << " launch spec msg " << *m_iter_next_spec_msg);

    // 在处理新的推测之前对处理上一消息形成的状态进行快照
    take_snapshot(false);

    // 使下一个待处理消息成为当前消息
    m_current_spec_msg = *m_iter_next_spec_msg++;

    assert(g_is_async_msg(m_current_spec_msg));

    m_current_spec_msg->set_effect(new Effect); // 处理前给消息配上effect
    m_spec_running_state = RunningState::ongoing;

    process_msg(m_current_spec_msg);
}


void
SpmtThread::launch_spec_msg(Message* msg)
{
    assert(is_spec_mode());

    MINILOG(task_load_logger, "#" << id()
            << " launch spec msg " << msg);

    // 在处理新的推测之前对处理上一消息形成的状态进行快照
    take_snapshot(true);

    m_current_spec_msg = msg;

    assert(not g_is_async_msg(m_current_spec_msg));


    // 将msg插在待处理消息之前，作为当前消息
    m_spec_msg_queue.insert(m_iter_next_spec_msg, msg);

    m_current_spec_msg->set_effect(new Effect); // 处理前给消息配上effect
    m_spec_running_state = RunningState::ongoing;

    process_msg(m_current_spec_msg);
    // if 刚处理的是get,put等，就要加载下一条。
}


void
SpmtThread::pin_frames()
{
    Frame* f = m_spec_mode.frame;

    for (;;) {
        MINILOG(snapshot_take_pin_logger, "#" << m_id
                << " pin " << f);

        if (f->pinned)
            break;

        f->pinned = true;

        // execute_method开始新的执行，top frame就是新老执行的边界。新老执行的推测执行彼此无关。
        if (f->is_top_frame())
            break;

        // 只管钉住本线程创建
        if (f->caller != this)
            break;

        f = f->prev;
    }
}


void
SpmtThread::take_snapshot(bool pin)
{
    // 若是从确定模式进入推测模式，使用推测性return_msg之前快照，之前并
    // 没有什么推测状态需要快照。
    if (m_current_spec_msg == nullptr) {
        MINILOG(snapshot_take_logger, "#" << m_id << " snapshot NOT needed, no prev spec state");
        return;
    }

    Effect* current_effect = m_current_spec_msg->get_effect();
    assert(current_effect);

    // 处理消息n之前，要在消息n-1的effect中添加快照。如果消息n后来被丢
    // 弃，然后在消息n-1之后处理消息n+1，那么消息n-1的effect中已经有快
    // 照了，就不用再照了。
    if (current_effect->snapshot != nullptr) {
        MINILOG(snapshot_take_logger, "#" << m_id << " snapshot already exists");
        return;
    }


    Snapshot* snapshot = new Snapshot;

    snapshot->version = m_state_buffer.current_ver();
    assert(snapshot->version >= 0);
    m_state_buffer.freeze();

    // 如果不钉住栈帧，快照中就只有个版本号。其他部分都为0。
    if (pin) {

        snapshot->pc = m_spec_mode.pc;
        snapshot->frame = m_spec_mode.frame;
        snapshot->sp = m_spec_mode.sp;

        pin_frames();
    }

    MINILOG(snapshot_take_logger, "#" << m_id << " freeze " << snapshot);

    current_effect->snapshot = snapshot;
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
                               msg->caller_sp,
                               true);

    delete msg;
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

    delete return_msg;

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

        Frame* old_current_frame = current_frame;
        current_frame = current_frame->prev;
        m_certain_mode.destroy_frame(old_current_frame);
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
