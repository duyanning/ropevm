#include "std.h"
#include "jam.h"
#include "Core.h"
#include "thread.h"
#include "interp-indirect.h"
#include "interp.h"
#include "excep.h"
#include "Message.h"
#include "OoSpmtJvm.h"
#include "Snapshot.h"
#include "Loggers.h"
#include "DebugScaffold.h"
#include "frame.h"
#include "Helper.h"

using namespace std;

Core::Core(int id)
:
    m_id(id)
{
    //cout << "#" << id << " newed" << endl;
    m_thread = 0;
    m_halt = false;

    m_certain_mode.set_core(this);
    m_speculative_mode.set_core(this);
    m_rvp_mode.set_core(this);
    m_mode = &m_certain_mode;
}

Core::~Core()
{
    //    cout << "cache size: " << m_cache.size() << endl;
}

void
Core::set_owner(Object* object)
{
    assert(object != m_owner);
    object->set_core(this);
    object->set_is_owner(true);
    m_owner = object;
}

void
Core::add_subsidiary(Object* object)
{
    object->set_core(this);
    object->set_is_owner(false);
}

void
Core::set_thread(Thread* thread)
{
    m_thread = thread;
}

bool
Core::is_halt()
{
    return m_halt;
}

void
Core::step()
{
    try {
        m_mode->step();
    }
    catch (VerifyFails& e) {
        assert(false);
        //std::cout << "catch VerifyFails" << std::endl;
        if (m_speculative_depth == m_certain_depth) {
            handle_verification_failure(e.get_message());
        }
        else {
            m_speculative_depth--;
            assert(false);
            throw;
        }
    }
    catch (NestedStepLoop& e) {
        MINILOG(step_loop_in_out_logger, "#" << m_id
                << " catch-> to be execute java method: " << *e.get_mb());
        //assert(false);
    }
    catch (DeepBreak& e) {
    }
}

void
Core::change_mode(Mode* new_mode)
{
	m_mode = new_mode;
}

void
Core::record_current_non_certain_mode()
{
    assert(not m_mode->is_certain_mode());

	m_old_mode = m_mode;
    //MINILOG0("#" << id() << " record " << m_mode->get_name());
}

void
Core::restore_original_non_certain_mode()
{
    assert(m_mode->is_certain_mode());

	m_mode = m_old_mode;
    MINILOG0("#" << id() << " restore to " << m_mode->get_name());
}

bool
Core::verify(Message* message)
{
    return m_mode->verify(message);
}

void
Core::verify_and_commit(Message* message, bool self)
{
    m_mode->verify_and_commit(message, self);
}

void
Core::init()
{
    m_halt = true;
    m_certain_msg = 0;
    m_mode = &m_speculative_mode;
    //m_user = 0;
    m_owner = 0;

    m_certain_depth = 0;
    m_speculative_depth = 0;

    m_is_waiting_for_task = true;
    //from_certain_to_spec = true;

//     m_owner_pc = 0;
//     m_owner_frame = 0;
//     m_owner_sp = 0;

    m_quit_step_loop = false;
    m_result = 0;
}

void
Core::start()
{
    if (m_halt) {
        m_halt = false;
        MINILOG0("#" << id() << " (state)start");
    }
}

void
Core::send_certain_message(Message* message)
{
    assert(is_valid_certain_msg(message));

    m_certain_msg = message;
    start();
}

void
Core::halt()
{
    //{{{ just for debug
    if (id() == 6) {
        int x = 0;
        x++;
    }
    //}}} just for debug
    MINILOG0("#" << id() << " (state)halt");
    m_halt = true;
}

Message*
Core::get_certain_message()
{
    Message* msg = 0;
    if (m_certain_msg) {
        assert(is_valid_certain_msg(m_certain_msg));
        msg = m_certain_msg;
        m_certain_msg = 0;
    }

    return msg;
}

void
Core::log_when_leave_certain()
{
    MINILOG(when_leave_certain_logger,
            "#" << id() << " when leave certain mode");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " now use cache ver(" << m_cache.version() << ")");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " SPEC details:");
    MINILOGPROC(when_leave_certain_logger,
                show_triple,
                (os, id(),
                 m_speculative_mode.frame, m_speculative_mode.sp, m_speculative_mode.pc,
                 m_speculative_mode.m_user,
                 true));

    MINILOG(when_leave_certain_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_leave_certain_logger,
            "#" << id() << " RVP details:");
    MINILOGPROC(when_leave_certain_logger,
                show_triple,
                (os, id(), m_rvp_mode.frame, m_rvp_mode.sp, m_rvp_mode.pc, m_rvp_mode.m_user, true));
    MINILOG(when_leave_certain_logger,
            "#" << id() << " ---------------------------");
}

void
Core::sync_speculative_with_certain()
{
    m_speculative_mode.pc = m_certain_mode.pc;
    m_speculative_mode.frame = m_certain_mode.frame;
    m_speculative_mode.sp = m_certain_mode.sp;
    m_speculative_mode.m_user = m_certain_mode.m_user;
}

void
Core::sync_certain_with_speculative()
{
    m_certain_mode.m_user = m_speculative_mode.m_user;
    m_certain_mode.pc = m_speculative_mode.pc;
    m_certain_mode.frame = m_speculative_mode.frame;
    m_certain_mode.sp = m_speculative_mode.sp;
}

void
Core::sync_certain_with_snapshot(Snapshot* snapshot)
{
    m_certain_mode.m_user = snapshot->user;
    m_certain_mode.pc = snapshot->pc;
    m_certain_mode.frame = snapshot->frame;
    m_certain_mode.sp = snapshot->sp;
}

void
Core::leave_certain_mode(Message* message)
{
    MINILOG0("#" << id() << " leaves certain mode");


    if (not m_messages_to_be_verified.empty()) {
        MINILOG0("#" << id() << " resume speculative execution");
        restore_original_non_certain_mode();
        log_when_leave_certain();
    }
    else {
        MINILOG0("#" << id() << " start speculative execution");

        sync_speculative_with_certain();
        change_mode(&m_speculative_mode);

        if (message->get_type() == Message::call) {
            InvokeMsg* msg = static_cast<InvokeMsg*>(message);

            MethodBlock* pslice = get_rvp_method(msg->mb);
            MINILOG0("#" << id() << " (S)calls p-slice: " << *pslice);
            Frame* new_frame = create_frame(msg->object, pslice, msg->caller_frame, 0,
                                            &msg->parameters[0], msg->caller_sp, msg->caller_pc);
            new_frame->is_certain = false;
            m_rvp_mode.pc = (CodePntr)pslice->code;
            m_rvp_mode.frame = new_frame;
            m_rvp_mode.sp = new_frame->ostack_base;

            enter_rvp_mode();
        }
        else if (message->get_type() == Message::ret) {
            m_speculative_mode.load_next_task();
        }
        else if (message->get_type() == Message::put) {
            AckMsg* ack = new AckMsg;
            add_message_to_be_verified(ack);
            halt();
        }
        else if (message->get_type() == Message::arraystore) {
            AckMsg* ack = new AckMsg;
            add_message_to_be_verified(ack);
            halt();
        }
        else if (message->get_type() == Message::ack) {
            m_speculative_mode.load_next_task();
        }
        else {
            assert(false);
        }

    }
}

void
Core::enter_certain_mode()
{
    //assert(m_user == m_owner);
    assert(not m_mode->is_certain_mode());

    m_thread->set_certain_core(this);

    record_current_non_certain_mode();
    change_mode(&m_certain_mode);

    //m_certain_catch_up_with_spec = true;

    MINILOG(when_enter_certain_logger,
            "#" << id() << " when enter certain mode");
    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_enter_certain_logger,
            "#" << id() << " cache ver(" << m_cache.version() << ")");

    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");

    MINILOG(when_enter_certain_logger,
            "#" << id() << " SPEC details:");

    if (original_uncertain_mode()->is_speculative_mode()) {
        MINILOGPROC(when_enter_certain_logger,
                    show_triple,
                    (os, id(),
                     m_speculative_mode.frame, m_speculative_mode.sp, m_speculative_mode.pc,
                     m_speculative_mode.m_user));
    }
    else {

        MINILOG(when_enter_certain_logger,
                "#" << id() << " RVP details:");
        MINILOGPROC(when_enter_certain_logger,
                    show_triple,
                    (os, id(), m_rvp_mode.frame, m_rvp_mode.sp, m_rvp_mode.pc, m_rvp_mode.m_user,
                     true));
    }

    MINILOG(when_enter_certain_logger,
            "#" << id() << " ---------------------------");
}

void
Core::enter_rvp_mode()
{
    MINILOG0("#" << id() << " enter RVP mode");

//     m_rvp_mode.pc = m_speculative_mode.pc;
//     m_rvp_mode.frame = m_speculative_mode.frame;
//     m_rvp_mode.ostack = m_speculative_mode.ostack;

    change_mode(&m_rvp_mode);
}

void
Core::leave_rvp_mode(Object* target_object)
{
    MINILOG0("#" << id() << " leave RVP mode");

    //m_speculative_mode.m_user = m_rvp_mode.m_user;
    m_speculative_mode.m_user = target_object;
    m_speculative_mode.pc = m_rvp_mode.pc;
    m_speculative_mode.frame = m_rvp_mode.frame;
    m_speculative_mode.sp = m_rvp_mode.sp;
    //m_speculative_mode.pc += (*m_speculative_mode.pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    change_mode(&m_speculative_mode);

    MINILOG(when_leave_rvp_logger,
            "#" << id() << " when leave rvp mode");
    MINILOG(when_leave_rvp_logger,
            "#" << id() << " ---------------------------");
    MINILOG(when_leave_rvp_logger,
            "#" << id() << " now use cache ver(" << m_cache.version() << ")");
    MINILOG(when_leave_rvp_logger,
            "#" << id() << " SPEC details:");
    MINILOGPROC(when_leave_rvp_logger,
                show_triple,
                (os, id(),
                 m_speculative_mode.frame, m_speculative_mode.sp, m_speculative_mode.pc,
                 m_speculative_mode.m_user,
                 true));

    MINILOG(when_leave_rvp_logger,
            "#" << id() << " ---------------------------");

    m_rvpbuf.clear();
}

void
Core::add_speculative_task(Message* message)
{
    //assert(false);
    m_speculative_tasks.push_back(message);

    //{{{ just for debug
    // if (m_id == 6) {
    //     cout << "#6 halt: " << m_halt << endl;
    //     cout << "#6 waiting: " << m_is_waiting_for_task << endl;
    // }
    //}}} just for debug
    if (m_halt && m_is_waiting_for_task) {
        MINILOG0("#" << id() << " is waken(sleeping, waiting for task)");
        start();
    }
}

void
Core::enter_execution()
{
//     MINILOG_IF(true, step_loop_in_out_logger,
//                "#" << id() << " jump in of a step-loop");

    m_mode->enter_execution();
}

void
Core::leave_execution()
{
    m_mode->leave_execution();
}

void
Core::handle_verification_failure(Message* message)
{
    m_mode->handle_verification_failure(message);
}

void
Core::add_message_to_be_verified(Message* message)
{
    //{{{ just for debug
    // Snapshot* latest_snapshot;
    // message->snapshot = latest_snapshot;
    if (m_id == 5 && message->get_type() == Message::ack) {
        int x = 0;
        x++;
    }
    //}}} just for debug
    m_messages_to_be_verified.push_back(message);
    MINILOG0("#" << id() << " use spec:  " << *message);
}

bool
Core::has_message_to_be_verified()
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
                    "#" << g_current_core()->id() << " free frame in snapshot " << *f);
            m_frames.insert(f);
            if (f->is_task_frame) break;
            f = f->prev;
        }
    }

private:
    set<Frame*>& m_frames;
    Frame* m_certain_frame;
};

// void
// Core::free_discarded_frames(bool only_snapshot)
// {
//     Mode* mode = original_uncertain_mode();
//     if (mode == 0) return;

//     assert(not mode->is_certain_mode());

//     MINILOG(free_frames_logger,
//             "#" << id() << " free no use frames, " << mode->get_name());

//     set<Frame*> frames;

//     Frame* f = mode->frame;
//     if (f) {
//         assert(f->mb);

//         for (;;) {
//             if (f->is_certain) break;
//             MINILOG(free_frames_logger,
//                     "#" << id() << " free " << (is_rvp_frame(f) ? "rvp" : "spec") << "frame " << *f);
//             // if (f->xxx == 999)
//             //     cout << "xxx999" << endl;
//             frames.insert(f);
//             if (f->is_task_frame) break;
//             f = f->prev;
//         }
//     }

//     MINILOG(free_frames_logger,
//             "#" << id() << " free no use frames in snapshot, len: " << m_snapshots_to_be_committed.size());
//     Collect_delayed_frame cdf(frames);
//     for_each(m_snapshots_to_be_committed.begin(), m_snapshots_to_be_committed.end(), cdf);

//     for_each(frames.begin(), frames.end(), Delete());
// }

void
Core::collect_inuse_frames(set<Frame*>& frames)
{
    Mode* mode = original_uncertain_mode();
    if (mode == 0) return;

    assert(not mode->is_certain_mode());

    MINILOG(free_frames_logger,
            "#" << id() << " free no use frames, " << mode->get_name());

    Frame* f = mode->frame;
    if (f) {
        assert(f->mb);

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
Core::collect_snapshot_frames(set<Frame*>& frames)
{
    MINILOG(free_frames_logger,
            "#" << id() << " free no use frames in snapshot, len: " << m_snapshots_to_be_committed.size());
    Collect_delayed_frame cdf(frames);
    for_each(m_snapshots_to_be_committed.begin(), m_snapshots_to_be_committed.end(), cdf);
}

void
Core::free_discarded_frames(bool only_snapshot)
{
    set<Frame*> frames;

    if (not only_snapshot)
        collect_inuse_frames(frames);

    collect_snapshot_frames(frames);

    for_each(frames.begin(), frames.end(), Delete());
}

void
Core::discard_uncertain_execution(bool self)
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

    m_cache.reset();
    m_rvpbuf.clear();

    m_is_waiting_for_task = false;
}

void
Core::destroy_frame(Frame* frame)
{
    m_mode->destroy_frame(frame);
}

//{{{ just for debug
bool
Core::debug_frame_is_not_in_snapshots(Frame* frame)
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
Core::quit_step_loop()
{
    bool quit = m_quit_step_loop;
    if (quit) {
        m_quit_step_loop = false;
    }
    return quit;
}

void
Core::signal_quit_step_loop(uintptr_t* result)
{
    assert(m_mode->is_certain_mode());

    if (result == 0) {
        MINILOG(c_exception_logger, "#" << id() << " (C) signal quit step loop");
        //{{{ just for debug
        if (m_id == 7) {
            int x = 0;
            x++;
        }
        //}}} just for debug
    }

    m_quit_step_loop = true;
    m_result = result;
}

uintptr_t*
Core::get_result()
{
    return m_result;
}

//{{{ just for debug
void
Core:: show_msgs_to_be_verified()
{
    cout << "msgs to be verified:\n";
    for (deque<Message*>::iterator i = m_messages_to_be_verified.begin();
         i != m_messages_to_be_verified.end(); ++i) {
        cout << **i << endl;
    }
}

bool
Core::is_correspondence_btw_msgs_and_snapshots_ok()
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


Core* g_current_core()
{
    Thread* this_thread = threadSelf();
    Core* this_core = this_thread->current_core();
    return this_core;
}

void
Core::scan()
{
    // scan snapshots
    // scan cache
    // scan rvp buffer
}

bool
Core::is_subsidiary(Object* obj)
{
    return obj->get_core() == this && not obj->is_owner();
}

bool
Core::is_owner_or_subsidiary(Object* obj)
{
    if (obj == 0) return false;
    return obj->get_core() == this;
}

void
Core::clear_frame_in_cache(Frame* f)
{
    m_cache.clear(f->lvars, f->lvars + f->mb->max_locals);
    m_cache.clear(f->ostack_base, f->ostack_base + f->mb->max_stack);
}

void
Core::clear_frame_in_rvpbuf(Frame* f)
{
    m_rvpbuf.clear(f->lvars, f->lvars + f->mb->max_locals);
    m_rvpbuf.clear(f->ostack_base, f->ostack_base + f->mb->max_stack);
}

void
Core::mark_frame_certain()
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
