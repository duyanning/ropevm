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
#include "Group.h"
#include "lock.h"
#include "Effect.h"

using namespace std;

SpmtThread::SpmtThread(int id)
:
    m_id(id)
{
    m_thread = 0;
    m_halt = false;

    m_certain_mode.set_core(this);
    m_spec_mode.set_core(this);
    m_rvp_mode.set_core(this);
    m_mode = &m_certain_mode;
}

SpmtThread::~SpmtThread()
{
}

void
SpmtThread::set_group(Group* group)
{
    m_group = group;
}

void
SpmtThread::set_thread(Thread* thread)
{
    m_thread = thread;
}

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
SpmtThread::change_mode(Mode* new_mode)
{
	m_mode = new_mode;
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
SpmtThread::init()
{
    m_halt = true;
    m_certain_message = 0;
    m_mode = &m_spec_mode;

    m_is_waiting_for_task = true;

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


void
SpmtThread::send_certain_msg(SpmtThread* target_thread, Message* msg)
{
    target_thread->set_certain_msg(msg);
}


void
SpmtThread::set_certain_msg(Message* msg)
{
    m_certain_message = msg;

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
            "#" << id() << " now use cache ver(" << m_state_buffer.version() << ")");
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
SpmtThread::sync_certain_with_speculative()
{
    m_certain_mode.pc = m_spec_mode.pc;
    m_certain_mode.frame = m_spec_mode.frame;
    m_certain_mode.sp = m_spec_mode.sp;
}

void
SpmtThread::sync_certain_with_snapshot(Snapshot* snapshot)
{
    m_certain_mode.pc = snapshot->pc;
    m_certain_mode.frame = snapshot->frame;
    m_certain_mode.sp = snapshot->sp;
}

void
SpmtThread::enter_certain_mode()
{
    assert(not m_mode->is_certain_mode());

    //m_thread->set_certain_core(this);

    record_current_non_certain_mode();
    change_mode(&m_certain_mode);

    //m_certain_catch_up_with_spec = true;

    MINILOG(when_enter_certain_logger,
            "#" << id() << " when enter certain mode");
    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_enter_certain_logger,
            "#" << id() << " cache ver(" << m_state_buffer.version() << ")");

    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");

    MINILOG(when_enter_certain_logger,
            "#" << id() << " SPEC details:");

    if (original_uncertain_mode()->is_speculative_mode()) {
        // MINILOGPROC(when_enter_certain_logger,
        //             show_triple,
        //             (os, id(),
        //              m_spec_mode.frame, m_spec_mode.sp, m_spec_mode.pc,
        //              m_spec_mode.m_user));
    }
    else {

        MINILOG(when_enter_certain_logger,
                "#" << id() << " RVP details:");
        // MINILOGPROC(when_enter_certain_logger,
        //             show_triple,
        //             (os, id(), m_rvp_mode.frame, m_rvp_mode.sp, m_rvp_mode.pc, m_rvp_mode.m_user,
        //              true));
    }

    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");
}


void
SpmtThread::switch_to_certain_mode()
{
    change_mode(&m_certain_mode);
}

void
SpmtThread::switch_to_speculative_mode()
{
    change_mode(&m_spec_mode);
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

    change_mode(&m_rvp_mode);
}


void
SpmtThread::send_spec_msg(SpmtThread* target_thread, Message* msg)
{
    target_thread->add_spec_msg(msg);

    Effect* current_effect = current_spec_msg()->get_effect();
    current_effect->msg_sent = msg;

    // MINILOG(s_logger,
    //         "#" << this->id() << " (S) add invoke task to #"
    //         << target_thread->id() << ": " << *msg);

}


void
SpmtThread::add_spec_msg(Message* msg)
{
    // stat
    // m_count_spec_msgs_sent++;

    m_spec_msg_queue.push_back(msg);


    //{{{ just for debug
    // if (m_id == 6) {
    //     cout << "#6 sleep: " << m_halt << endl;
    //     cout << "#6 waiting: " << m_is_waiting_for_task << endl;
    // }
    //}}} just for debug
    if (m_halt && m_is_waiting_for_task) {
        MINILOG0("#" << id() << " is waken(sleeping, waiting for task)");
        wakeup();
    }
}


void
SpmtThread::add_message_to_be_verified(Message* message)
{
    // stat
    m_count_spec_msgs_used++;


    //{{{ just for debug
    // Snapshot* latest_snapshot;
    // message->snapshot = latest_snapshot;
    // if (m_id == 5 && message->get_type() == Message::ack) {
    //     int x = 0;
    //     x++;
    // }
    //}}} just for debug
    m_messages_to_be_verified.push_back(message);
    MINILOG0("#" << id() << " use spec:  " << *message);
}

bool
SpmtThread::has_message_to_be_verified()
{
    return not m_messages_to_be_verified.empty();
}

struct Delete {
    template <class T>
    void operator()(T* p)
    {
        delete p;
    }
};


void
SpmtThread::destroy_frame(Frame* frame)
{
    m_mode->destroy_frame(frame);
}

//{{{ just for debug
bool
SpmtThread::debug_frame_is_not_in_snapshots(Frame* frame)
{
    bool found = false;

    for (deque<Snapshot*>::iterator i = m_snapshots_to_be_committed.begin();
         i != m_snapshots_to_be_committed.end();
         ++i) {
        Snapshot* s = *i;
        if (s->frame == frame) {
            assert(false);
            found = true;
            break;
        }

    }

    return not found;
}
//}}} just for debug

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

//{{{ just for debug
void
SpmtThread:: show_msgs_to_be_verified()
{
    cout << "msgs to be verified:\n";
    for (deque<Message*>::iterator i = m_messages_to_be_verified.begin();
         i != m_messages_to_be_verified.end(); ++i) {
        cout << **i << endl;
    }
}

bool
SpmtThread::is_correspondence_btw_msgs_and_snapshots_ok()
{
    if (not m_messages_to_be_verified.empty()) {
        Message* spec_msg = m_messages_to_be_verified.front();

        if (not m_snapshots_to_be_committed.empty()) {
            Snapshot* snapshot = m_snapshots_to_be_committed.front();
            if (spec_msg != snapshot->spec_msg) {
                cerr << "#" << id() << " spec msg is NOT corresponding to snapshot\n";
                cerr << "#" << id() << " spec msg: " << *spec_msg << "\n";
                cerr << "#" << id() << " snapshot msg: " << *snapshot->spec_msg << "\n";
                assert(false);
                return false;
            }
        }
    }
    return true;
}
//}}} just for debug


SpmtThread* g_get_current_spmt_thread()
{
    Thread* this_thread = threadSelf();
    SpmtThread* this_core = this_thread->get_current_spmt_thread();
    return this_core;
}

void
g_set_current_spmt_thread(SpmtThread* st)
{
    Thread* this_thread = threadSelf();
    this_thread->set_current_spmt_thread(st);
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
SpmtThread::mark_frame_certain()
{
    Frame* f = m_certain_mode.frame;
    assert(f->is_alive());
    for (;;) {
        MINILOG(mark_frame_certain_logger,
                "#" << id() << " mark frame certain " << *f);
        if (f->is_certain) break;
        f->is_certain = true;
        if (f->is_top_frame()) break;
        f = f->prev;
    }
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
SpmtThread::commit(Effect* effect)
{
    // 提交快照
    // 确认消息
}


void
SpmtThread::revoke_spec_msg(SpmtThread* target_thread, Message* msg)
{
    target_thread->remove_spec_msg(msg);
}


void
SpmtThread::remove_spec_msg(Message* msg)
{
}

void
SpmtThread::discard_all_effect()
{
    MINILOG0_IF(debug_scaffold::java_main_arrived,
                "#" << id() << " discard uncertain execution");


    // for each 队列中每个消息
    //     收回effect中发出的消息
    //     处理effect中记录的栈帧
    //     释放effect

    // 重置state buffer与rvp buffer

    for (auto i = m_spec_msg_queue.begin(); i != m_next_spec_msg; ++i) {
        Message* msg = *i;
        Effect* effect = msg->get_effect();

        SpmtThread* target_thread = msg->get_target_object()->get_group()->get_spmt_thread();
        revoke_spec_msg(target_thread, msg);

        for (auto frame : effect->C) {
            delete frame;
        }

        delete effect;
    }


    m_state_buffer.reset();
    m_rvp_buffer.clear();

    m_is_waiting_for_task = false;

}


void
SpmtThread::verify_speculation(Message* certain_msg)
{

    // 如果无待验证消息
    //     处理确定消息
    //     结束
    if (m_spec_msg_queue.begin() == m_next_spec_msg) {
        process_certain_msg(certain_msg);
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
        commit(effect);
        delete effect;
        m_spec_msg_queue.pop_front();
    }
    else {
        //discard_all_effect();

        if (g_is_async_msg(certain_msg)) {
        }
        process_certain_msg(certain_msg);
    }



    // bool success = verify(message);

    // // stat
    // if (success)
    //     m_count_verify_ok++;
    // else
    //     m_count_verify_fail++;

    // if (success) {
    //     handle_verification_success(message);
    // }
    // else {
    //     handle_verification_failure(message);
    // }

    // delete message;
}

void
SpmtThread::handle_verification_success(Message* message)
{
    //bool should_reset_spec_exec = false;

    if (m_snapshots_to_be_committed.empty()) {
        sync_certain_with_speculative();

        if (m_certain_mode.frame) {
            //assert(m_core->is_owner_or_subsidiary(frame->get_object()));
            assert(is_sp_ok(m_certain_mode.sp, m_certain_mode.frame));
            assert(is_pc_ok(m_certain_mode.pc, m_certain_mode.frame->mb));
        }

        MINILOG(commit_logger,
                "#" << id()
                << " commit to latest, ver(" << m_state_buffer.version() << ")");
        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << id() << " ostack base: " << m_certain_mode.frame->ostack_base);
        MINILOGPROC(cache_logger, show_buffer,
                    (os, id(), m_state_buffer, false));
        //}}} just for debug
        m_state_buffer.commit(m_state_buffer.version());
        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << id() << " ostack base: " << m_certain_mode.frame->ostack_base);
        MINILOGPROC(cache_logger, show_buffer,
                    (os, id(), m_state_buffer, false));
        //}}} just for debug

        // because has commited to latest version, cache should restart from ver 0
        //should_reset_spec_exec = true;

        //{{{ just for debug
        // if (m_id == 6 && message->get_type() == Message::ack) {
        //     int x = 0;
        //     x++;
        // }
        //}}} just for debug
        MINILOG0_IF(debug_scaffold::java_main_arrived,
                    "#" << id() << " comt spec: latest");
    }
    else {
        Snapshot* snapshot = m_snapshots_to_be_committed.front();
        m_snapshots_to_be_committed.pop_front();
        sync_certain_with_snapshot(snapshot);

        if (message->get_type() == Message::ret && m_certain_mode.frame) {
            //assert(m_core->is_owner_enclosure(frame->get_object()));
            assert(get_group() == m_certain_mode.frame->get_object()->get_group());
            assert(is_sp_ok(m_certain_mode.sp, m_certain_mode.frame));
            assert(is_pc_ok(m_certain_mode.pc, m_certain_mode.frame->mb));
        }

        MINILOG(commit_logger,
                "#" << id() << " commit to ver(" << snapshot->version << ")");

        // MINILOG0_IF(debug_scaffold::java_main_arrived,
        //             "#" << m_core->id() << " comt spec: " << *snapshot->spec_msg);

        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << id() << " ostack base: " << m_certain_mode.frame->ostack_base);
        MINILOGPROC(cache_logger, show_buffer,
                    (os, id(), m_state_buffer, false));
        //}}} just for debug
        m_state_buffer.commit(snapshot->version);
        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << id() << " ostack base: " << m_certain_mode.frame->ostack_base);
        MINILOGPROC(cache_logger, show_buffer,
                    (os, id(), m_state_buffer, false));
        //}}} just for debug

        // if (snapshot->version == m_core->m_state_buffer.prev_version()) {
        //     //{{{ just for debug
        //     int x = 0;
        //     x++;
        //     //}}} just for debug

        //     //should_reset_spec_exec = true;
        // }

        delete snapshot;
    }

    if (message->get_type() != Message::put
        and message->get_type() != Message::arraystore) {
        mark_frame_certain();
    }


    //if (should_reset_spec_exec) {
    if (not has_message_to_be_verified()) {
        discard_uncertain_execution(true);
    }

    MINILOG(commit_detail_logger,
            "#" << id() << " CERT details:");
    // MINILOGPROC(commit_detail_logger, show_triple,
    //             (os, m_core->id(),
    //              frame, sp, pc, m_user,
    //              true));

    if (message->get_type() == Message::put or message->get_type() == Message::arraystore) {
        Object* source_object = message->get_source_object();
        Group* source_group = source_object->get_group();
        SpmtThread* source_core = source_group->get_core();
        source_core->wakeup();
    }

}

void
SpmtThread::process_certain_msg(Message* msg)
{
    Message::Type type = msg->get_type();

    if (type == Message::invoke) {

        InvokeMsg* msg = static_cast<InvokeMsg*>(msg);

        m_certain_mode.internal_invoke(msg->get_target_object(),
                                       msg->mb,
                                       &msg->parameters[0],
                                       0, 0, 0);

    }
    else if (type == Message::put) {

        PutMsg* put_msg = static_cast<PutMsg*>(msg);

        // 按照put消息的指示，往某字段写入一个数值
        for (int i = 0; i < put_msg->val.size(); ++i) {
            put_msg->addr[i] = put_msg->val[i];
        }

        PutReturnMsg* put_return_msg = new PutReturnMsg();

        send_certain_msg(put_msg->get_source_spmt_thread(), put_return_msg);


    }
    else if (type == Message::get) {

        GetMsg* get_msg = static_cast<GetMsg*>(msg);

        GetReturnMsg* get_return_msg = new GetReturnMsg(get_msg->addr,
                                                        get_msg->size);

        send_certain_msg(get_msg->get_source_spmt_thread(), get_return_msg);

    }
    else if (type == Message::arraystore) {

        ArrayStoreMsg* arraystore_msg = static_cast<ArrayStoreMsg*>(msg);

        Object* array = arraystore_msg->get_target_object();
        int index = arraystore_msg->index;
        int type_size = arraystore_msg->type_size;

        void* addr = array_elem_addr(array, index, type_size);
        m_certain_mode.store_array_from_no_cache_mem(&arraystore_msg->slots[0], addr, type_size);

        ArrayStoreReturn* arraystore_return_msg = new ArrayStoreReturn();
        send_certain_msg(arraystore_msg->get_source_spmt_thread(),
                         arraystore_return_msg);

    }
    else if (type == Message::arrayload) {

        ArrayLoadMsg* arrayload_msg = static_cast<ArrayLoadMsg*>(msg);

        int type_size = arrayload_msg->val.size();
        m_certain_mode.load_array_from_no_cache_mem(m_certain_mode.sp, &arrayload_msg->val[0], type_size);
        m_certain_mode.sp += type_size > 4 ? 2 : 1;


        ArrayLoadReturn* arrayload_return_msg = new ArrayLoadReturn();
        send_certain_msg(arrayload_msg->get_source_spmt_thread(),
                         arrayload_return_msg);


    }
    else if (type == Message::ret) {

        ReturnMsg* msg = static_cast<ReturnMsg*>(msg);

        // 将返回值写入ostack
        for (auto i : msg->retval) {
            *m_spec_mode.sp++ = i;
        }

        m_certain_mode.pc += (*m_certain_mode.pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

    }
    else if (type == Message::put_return) {

        PutReturnMsg* put_return_msg = static_cast<PutReturnMsg*>(msg);
        // 什么也不需要做

        m_certain_mode.pc += 3;

    }
    else if (type == Message::get_return) {

        GetReturnMsg* get_return_msg = static_cast<GetReturnMsg*>(msg);

        for (auto i : get_return_msg->val) {
            *m_certain_mode.sp++ = i;
        }

        m_certain_mode.pc += 3;

    }
    else if (type == Message::arraystore_return) {
        // 没有什么需要写入ostack

        m_spec_mode.pc += 1;
    }
    else if (type == Message::arrayload_return) {
        ArrayLoadMsg* arrayload_msg = static_cast<ArrayLoadMsg*>(msg);

        // 将读到的值写入ostack
        // for (auto i : msg->val) {
        //     m_spec_mode.write(m_spec_mode.sp++, i);
        // }

        m_spec_mode.pc += 3;
    }
    else {
        assert(false);
    }

}

void
SpmtThread::handle_verification_failure(Message* message)
{
    reload_speculative_tasks();
    discard_uncertain_execution(true);

    reexecute_failed_message(message);

    if (message->get_type() == Message::put or message->get_type() == Message::arraystore) {

        Object* source_object = message->get_source_object();
        Group* source_group = source_object->get_group();
        SpmtThread* source_core = source_group->get_core();
        source_core->wakeup();
        launch_next_spec_msg();

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

Group*
SpmtThread::assign_group_for(Object* obj)
{
    return m_mode->assign_group_for(obj);
}

Mode*
SpmtThread::get_current_mode()
{
    return m_mode;
}


void
SpmtThread::process_spec_msg(Message* msg)
{
    m_is_waiting_for_task = false;

    MINILOG(task_load_logger, "#" << id() << " spec msg loaded: " << *msg);

    Message::Type type = msg->get_type();

    if (type == Message::invoke) {

        InvokeMsg* invoke_msg = static_cast<InvokeMsg*>(msg);

        Frame* new_frame = m_spec_mode.create_frame(invoke_msg->get_target_object(),
                                                    invoke_msg->mb,
                                                    &invoke_msg->parameters[0],
                                                    invoke_msg->get_source_spmt_thread(),
                                                    0,
                                                    0,
                                                    0);


        m_spec_mode.sp = (uintptr_t*)new_frame->ostack_base;
        m_spec_mode.frame = new_frame;
        m_spec_mode.pc = (CodePntr)new_frame->mb->code;
    }
    else if (type == Message::put) {
        PutMsg* put_msg = static_cast<PutMsg*>(msg);

        for (int i = 0; i < put_msg->val.size(); ++i) {
            m_spec_mode.write(put_msg->addr + i, put_msg->val[i]);
        }

        PutReturnMsg* put_return_msg = new PutReturnMsg();
        // 将put_return消息记录在effect中

        launch_next_spec_msg();
    }
    else if (type == Message::get) {
        GetMsg* get_msg = static_cast<GetMsg*>(msg);

        GetReturnMsg* get_return_msg = new GetReturnMsg(get_msg->addr,
                                                        get_msg->size);
        // 将get_return消息记录在effect中
        launch_next_spec_msg();
    }
    else if (type == Message::arraystore) {
        ArrayStoreMsg* arraystore_msg = static_cast<ArrayStoreMsg*>(msg);

        Object* array = arraystore_msg->get_target_object();
        int index = arraystore_msg->index;
        int type_size = arraystore_msg->type_size;

        void* addr = array_elem_addr(array, index, type_size);
        m_spec_mode.store_array_from_no_cache_mem(&arraystore_msg->slots[0], addr, type_size);

        // for (int i = 0; i < msg->val.size(); ++i) {
        //     m_spec_mode.write(addr + i, msg->val[i]);
        // }
        ArrayStoreReturn* arraystore_return_msg = new ArrayStoreReturn();
        // 将消息记入effect
        launch_next_spec_msg();
    }
    else if (type == Message::arrayload) {
        ArrayLoadReturn* arrayload_return_msg = new ArrayLoadReturn();
        // 将消息记入effect
        launch_next_spec_msg();
    }
    else if (type == Message::ret) {
        ReturnMsg* return_msg = static_cast<ReturnMsg*>(msg);

        // 将返回值写入ostack
        for (auto i : return_msg->retval) {
            m_spec_mode.write(m_spec_mode.sp++, i);
        }

        m_spec_mode.pc += (*m_spec_mode.pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    }
    else if (type == Message::put_return) {
        PutReturnMsg* put_return_msg = static_cast<PutReturnMsg*>(msg);
        // 什么也不需要做

        m_spec_mode.pc += 3;
    }
    else if (type == Message::get_return) {

        GetReturnMsg* get_return_msg = static_cast<GetReturnMsg*>(msg);

        for (auto i : get_return_msg->val) {
            m_spec_mode.write(m_spec_mode.sp++, i);
        }

        m_spec_mode.pc += 3;
    }
    else if (type == Message::arraystore_return) {
        // 没有什么需要写入ostack

        m_spec_mode.pc += 1;
    }
    else if (type == Message::arrayload_return) {
        ArrayLoadMsg* arrayload_msg = static_cast<ArrayLoadMsg*>(msg);

        // 将读到的值写入ostack
        // for (auto i : msg->val) {
        //     m_spec_mode.write(m_spec_mode.sp++, i);
        // }

        m_spec_mode.pc += 3;
    }
    else {
        assert(false);
    }

}


bool
SpmtThread::has_unprocessed_spec_msg()
{
    if (m_next_spec_msg != m_spec_msg_queue.end())
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
    ++m_next_spec_msg;

    // assert(type == Message::invoke
    //        or type == Message::put
    //        or type == Message::get
    //        or type == Message::arraystore
    //        or type == Message::arrayload);

    process_spec_msg(current_spec_msg());
}


void
SpmtThread::launch_spec_msg(Message* msg)
{
    // 在处理新的推测之前对处理上一消息形成的状态进行快照
    snapshot(false);

    // 将msg插在待处理消息之前，作为当前消息
    // ...

    process_spec_msg(current_spec_msg());
}


void
SpmtThread::pin_frames()
{
    Frame* f = m_spec_mode.frame;

    for (;;) {
        // MINILOG(snapshot_logger,
        //         "#" << m_spmt_thread->id() << " snapshot frame(" << f << ")" << " for " << *f->mb);

        if (f->pinned)
            break;

        f->pinned = true;

        if (f->is_top_frame())
            break;

        f = f->prev;
        if (f->calling_object->get_group()->get_spmt_thread() != this)
            break;

    }
}


void
SpmtThread::snapshot(bool pin)
{
    Snapshot* snapshot = new Snapshot;

    snapshot->version = m_state_buffer.version();
    m_state_buffer.freeze();

    snapshot->pc = m_spec_mode.pc;
    snapshot->frame = m_spec_mode.frame;
    snapshot->sp = m_spec_mode.sp;

    Effect* current_effect = current_spec_msg()->get_effect();
    current_effect->snapshot = snapshot;

    if (pin)
        pin_frames();

}


void
SpmtThread::on_event_top_invoke(InvokeMsg* msg)
{
    // assert 必为确定模式
    // discard effect

    // 按正常处理invoke消息那样处理
    // internal_invoke
    m_certain_mode.internal_invoke(msg->get_target_object(), msg->mb, &msg->parameters[0],
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
