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
    assert(is_valid_certain_msg(msg));

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
SpmtThread::on_enter_certain_mode()
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

    Effect* current_effect = m_spec_msg_queue.current_msg()->get_effect();
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

    m_spec_msg_queue.add(msg);

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

struct Collect_delayed_frame {
public:
    Collect_delayed_frame(set<Frame*>& frames)
        : m_frames(frames)
    {
    }

    void operator()(Snapshot* s)
    {
        Frame* f = s->frame;
        if (f == 0) return;
        assert(f);              // todo: f maybe non null
        assert(f->is_alive());
        for (;;) {
            if (f->is_certain) break;
            MINILOG(free_frames_logger,
                    "#" << g_get_current_core()->id() << " free frame in snapshot " << *f);
            m_frames.insert(f);
            if (f->is_task_frame) break;
            f = f->prev;
        }
    }

private:
    set<Frame*>& m_frames;
    Frame* m_certain_frame;
};

void
SpmtThread::collect_inuse_frames(set<Frame*>& frames)
{
    Mode* mode = original_uncertain_mode();
    if (mode == 0) return;

    assert(not mode->is_certain_mode());

    MINILOG(free_frames_logger,
            "#" << id() << " free no use frames, " << mode->get_name());

    Frame* f = mode->frame;
    if (f) {
        if (f->mb) {
            cout << "inuse " << f->mb->name << endl;
        }
        assert(f->mb);
        //cout << "i am " << *f->mb << endl;

        for (;;) {
            if (f->is_certain) break;
            MINILOG(free_frames_logger,
                    "#" << id() << " free " << (is_rvp_frame(f) ? "rvp" : "spec") << "frame " << *f);
            // if (f->xxx == 999)
            //     cout << "xxx999" << endl;
            frames.insert(f);
            if (f->is_task_frame) break;
            f = f->prev;
        }
    }
}

void
SpmtThread::collect_snapshot_frames(set<Frame*>& frames)
{
    MINILOG(free_frames_logger,
            "#" << id() << " free no use frames in snapshot, len: " << m_snapshots_to_be_committed.size());
    Collect_delayed_frame cdf(frames);
    for_each(m_snapshots_to_be_committed.begin(), m_snapshots_to_be_committed.end(), cdf);
}

void
SpmtThread::free_discarded_frames(bool only_snapshot)
{
    set<Frame*> frames;

    if (not only_snapshot)
        collect_inuse_frames(frames);

    collect_snapshot_frames(frames);

    for_each(frames.begin(), frames.end(), Delete());
}

void
SpmtThread::reload_speculative_tasks()
{
    //return;
    //assert(false);

    std::deque<Message*>::iterator i;
    for (i = m_messages_to_be_verified.begin(); i != m_messages_to_be_verified.end(); ++i) {
        Message* msg = *i;

        if (msg->get_type() == Message::invoke
            // || msg->get_type() == Message::put
            // || msg->get_type() == Message::arraystore
            ) {

            m_speculative_tasks.insert(m_speculative_tasks.begin(), msg);
        }

    }

    m_messages_to_be_verified.clear();
}

void
SpmtThread::discard_uncertain_execution(bool self)
{
    MINILOG0_IF(debug_scaffold::java_main_arrived,
                "#" << id() << " discard uncertain execution");

    for_each(m_messages_to_be_verified.begin(), m_messages_to_be_verified.end(), Delete());
    m_messages_to_be_verified.clear();

    // for_each(m_speculative_tasks.begin(), m_speculative_tasks.end(), Delete());
    // m_speculative_tasks.clear();

    free_discarded_frames(self == true);

    for_each(m_snapshots_to_be_committed.begin(), m_snapshots_to_be_committed.end(), Delete());
    m_snapshots_to_be_committed.clear();

    m_state_buffer.reset();
    m_rvp_buffer.clear();

    m_is_waiting_for_task = false;
}

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


SpmtThread* g_get_current_core()
{
    Thread* this_thread = threadSelf();
    SpmtThread* this_core = this_thread->get_current_core();
    return this_core;
}

void
g_set_current_core(SpmtThread* current_core)
{
    Thread* this_thread = threadSelf();
    this_thread->set_current_core(current_core);
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

bool
SpmtThread::verify(Message* message)
{
    // stat
    m_count_verify_all++;


    assert(is_correspondence_btw_msgs_and_snapshots_ok());

    bool ok = false;
    if (not m_messages_to_be_verified.empty()) {
        Message* spec_msg = m_messages_to_be_verified.front();

        if (*spec_msg == *message) {
            ok = true;
        }

        if (ok) {
            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << id() << " verify " << *message << " OK");

            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << id() << " veri spec: " << *spec_msg << " OK");
        }
        else {
            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << id() << " verify " << *message << " FAILED"
                        << " not " << *spec_msg);

            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << id() << " veri spec: " << *spec_msg << " FAILED");

            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << id() << " details:");
            MINILOGPROC_IF(debug_scaffold::java_main_arrived,
                           verify_detail_logger, show_msg_detail,
                           (os, id(), message));
            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << id() << " ---------");
            MINILOGPROC_IF(debug_scaffold::java_main_arrived,
                           verify_detail_logger, show_msg_detail,
                           (os, id(), spec_msg));

        }

        m_messages_to_be_verified.pop_front();
        delete spec_msg;
    }
    else {
        MINILOG0_IF(debug_scaffold::java_main_arrived,
                    "#" << id() << " verify " << *message << " EMPTY");
        MINILOG0_IF(debug_scaffold::java_main_arrived,
                    "#" << id() << " veri spec: EMPTY");

        // stat
        m_count_verify_empty++;
    }

    return ok;
}

void
SpmtThread::verify_speculation(Message* message, bool self)
{
    bool success = verify(message);

    // stat
    if (success)
        m_count_verify_ok++;
    else
        m_count_verify_fail++;

    if (success) {
        handle_verification_success(message, self);
    }
    else {
        handle_verification_failure(message, self);
    }

    delete message;
}

void
SpmtThread::handle_verification_success(Message* message, bool self)
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
        discard_uncertain_execution(self);
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
SpmtThread::reexecute_failed_message(Message* message)
{
    Message::Type type = message->get_type();

    if (type == Message::invoke) {

        InvokeMsg* msg = static_cast<InvokeMsg*>(message);
        m_certain_mode.invoke_to_my_method(msg->get_target_object(), msg->mb, &msg->parameters[0],
                                        msg->get_source_object(),
                                        msg->caller_pc, msg->caller_frame, msg->caller_sp);

    }
    else if (type == Message::ret) {

        ReturnMsg* msg = static_cast<ReturnMsg*>(message);
        m_certain_mode.return_to_my_method(&msg->retval[0], msg->retval.size(), msg->caller_pc, msg->caller_frame, msg->caller_sp);

    }
    else if (type == Message::get) {

        GetMsg* msg = static_cast<GetMsg*>(message);

        assert(m_certain_mode.pc == msg->caller_pc);
        assert(m_certain_mode.frame == msg->caller_frame);
        assert(m_certain_mode.sp == msg->caller_sp);

        // m_certain_mode.pc = msg->caller_pc;
        // m_certain_mode.frame = msg->caller_frame;
        // m_certain_mode.sp = msg->caller_sp;

        assert(is_pc_ok(m_certain_mode.pc, m_certain_mode.frame->mb));
        assert(is_sp_ok(m_certain_mode.sp, m_certain_mode.frame));

        vector<uintptr_t>::iterator begin = msg->val.begin();
        vector<uintptr_t>::iterator end = msg->val.end();
        for (vector<uintptr_t>::iterator i = begin; i != end; ++i) {
            *m_certain_mode.sp++ = *i;
        }

        m_certain_mode.pc += 3;

    }
    else if (type == Message::put) {

        PutMsg* msg = static_cast<PutMsg*>(message);

        for (int i = 0; i < msg->val.size(); ++i) {
            m_certain_mode.write(msg->addr + i, msg->val[i]);
        }

    }
    else if (type == Message::arrayload) {

        ArrayLoadMsg* msg = static_cast<ArrayLoadMsg*>(message);

        assert(m_certain_mode.pc == msg->caller_pc);
        assert(m_certain_mode.frame == msg->caller_frame);
        assert(m_certain_mode.sp == msg->caller_sp);

        // m_certain_mode.pc = msg->caller_pc;
        // m_certain_mode.frame = msg->caller_frame;
        // m_certain_mode.sp = msg->caller_sp;

        assert(is_pc_ok(m_certain_mode.pc, m_certain_mode.frame->mb));
        assert(is_sp_ok(m_certain_mode.sp, m_certain_mode.frame));

        int type_size = msg->val.size();
        m_certain_mode.load_array_from_no_cache_mem(m_certain_mode.sp, &msg->val[0], type_size);
        m_certain_mode.sp += type_size > 4 ? 2 : 1;
        m_certain_mode.pc += 1;

    }
    else if (type == Message::arraystore) {

        ArrayStoreMsg* msg = static_cast<ArrayStoreMsg*>(message);

        Object* array = msg->get_target_object();
        int index = msg->index;
        int type_size = msg->type_size;

        void* addr = array_elem_addr(array, index, type_size);
        m_certain_mode.store_array_from_no_cache_mem(&msg->slots[0], addr, type_size);

    }
    else {
        assert(false);
    }
}

void
SpmtThread::handle_verification_failure(Message* message, bool self)
{
    reload_speculative_tasks();
    discard_uncertain_execution(self);

    reexecute_failed_message(message);

    if (message->get_type() == Message::put or message->get_type() == Message::arraystore) {

        Object* source_object = message->get_source_object();
        Group* source_group = source_object->get_group();
        SpmtThread* source_core = source_group->get_core();
        source_core->wakeup();
        process_next_spec_msg();

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
SpmtThread::process_spec_msg(Message* spec_msg)
{
    m_is_waiting_for_task = false;

    MINILOG(task_load_logger, "#" << id() << " spec msg loaded: " << *spec_msg);

    Message::Type type = spec_msg->get_type();
    assert(type == Message::invoke
           or type == Message::put
           or type == Message::get
           or type == Message::arraystore
           or type == Message::arrayload);

    if (type == Message::invoke) {

        InvokeMsg* msg = static_cast<InvokeMsg*>(spec_msg);
        MethodBlock* new_mb = msg->mb;

        m_state_buffer.freeze();

        Frame* new_frame = m_spec_mode.create_frame(msg->get_target_object(),
                                                    new_mb,
                                                    msg->caller_frame,
                                                    msg->get_source_object(),
                                                    &msg->parameters[0],
                                                    msg->caller_sp,
                                                    msg->caller_pc);


        m_spec_mode.sp = (uintptr_t*)new_frame->ostack_base;
        m_spec_mode.frame = new_frame;
        m_spec_mode.pc = (CodePntr)new_mb->code;
    }
    else if (type == Message::put) {
        PutMsg* msg = static_cast<PutMsg*>(spec_msg);

        for (int i = 0; i < msg->val.size(); ++i) {
            m_spec_mode.write(msg->addr + i, msg->val[i]);
        }

        snapshot(false);
        process_next_spec_msg();
    }
    else if (type == Message::arraystore) {
        ArrayStoreMsg* msg = static_cast<ArrayStoreMsg*>(spec_msg);

        Object* array = msg->get_target_object();
        int index = msg->index;
        int type_size = msg->type_size;

        void* addr = array_elem_addr(array, index, type_size);
        m_spec_mode.store_array_from_no_cache_mem(&msg->slots[0], addr, type_size);

        // for (int i = 0; i < msg->val.size(); ++i) {
        //     m_spec_mode.write(addr + i, msg->val[i]);
        // }

        snapshot(false);
        process_next_spec_msg();
    }

}


void
SpmtThread::process_next_spec_msg()
{
    MINILOG(task_load_logger, "#" << id() << " try to load a spec msg");

    Message* msg = m_spec_msg_queue.begin_process_next_msg();
    if (msg == nullptr) {
        MINILOG(task_load_logger, "#" << id() << " no spec msg, waiting for spec msg");
        m_spec_mode.pc = 0;
        m_spec_mode.frame = 0;
        m_spec_mode.sp = 0;
        m_need_spec_msg = true;
        sleep();
        return;
    }

    snapshot(false);
    process_spec_msg(msg);
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

    Effect* current_effect = m_spec_msg_queue.current_msg()->get_effect();
    current_effect->snapshot = snapshot;

    if (pin)
        pin_frames();

}


// void
// SpmtThread::snapshot_old(bool shot_frame)
// {
//     if (shot_frame) {
//         Frame* f = this->frame;
//         //assert(f == 0 || m_spmt_thread->is_owner_enclosure(f->get_object()));


//         if (f) {
//             for (;;) {
//                 MINILOG(snapshot_logger,
//                         "#" << m_spmt_thread->id() << " snapshot frame(" << f << ")" << " for " << *f->mb);

//                 if (f->pinned) break;
//                 f->pinned = true;

//                 if (f->calling_object->get_group() != get_group())
//                     break;

//                 //if (f == m_spmt_thread->m_certain_mode.frame) break;
//                 if (f->is_task_frame) break;
//                 if (f->is_top_frame()) break;

//                 f = f->prev;
//             }
//         }
//     }

//     Snapshot* snapshot = new Snapshot;

//     snapshot->version = m_spmt_thread->m_state_buffer.version();
//     //m_spmt_thread->m_states_buffer.freeze();

//     // //{{{ just for debug
//     // MINILOG_IF(m_spmt_thread->id() == 5,
//     //            cache_logger,
//     //            "#" << m_spmt_thread->id() << " ostack base: " << frame->ostack_base);
//     // MINILOGPROC_IF(m_spmt_thread->id() == 5,
//     //                cache_logger, show_cache,
//     //                (os, m_spmt_thread->id(), m_spmt_thread->m_states_buffer, false));
//     // //}}} just for debug
//     //snapshot->user = this->m_user;
//     snapshot->pc = this->pc;
//     snapshot->frame = shot_frame ? this->frame : 0;
//     snapshot->sp = this->sp;

//     //{{{ just for debug
//     assert(m_spmt_thread->has_message_to_be_verified());
//     snapshot->spec_msg = m_spmt_thread->m_messages_to_be_verified.back();
//     MINILOG0("#" << m_spmt_thread->id() << " shot spec: " << *snapshot->spec_msg);
//     //}}} just for debug


//     MINILOG(snapshot_logger,
//             "#" << m_spmt_thread->id() << " snapshot, ver(" << snapshot->version << ")" << " is frozen");
//     if (shot_frame) {
//         MINILOG(snapshot_detail_logger,
//                 "#" << m_spmt_thread->id() << " details:");
//         MINILOGPROC(snapshot_detail_logger, show_snapshot,
//                     (os, m_spmt_thread->id(), snapshot));
//     }

//     if (shot_frame) {
//         assert(snapshot->frame == 0 || is_sp_ok(snapshot->sp, snapshot->frame));
//     }

//     m_spmt_thread->m_snapshots_to_be_committed.push_back(snapshot);
// }
