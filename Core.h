#ifndef CORE_H
#define CORE_H

#include "jam.h"

#include "CertainMode.h"
#include "SpeculativeMode.h"
#include "RvpMode.h"

#include "Cache.h"
#include "RvpBuffer.h"

class Thread;
class Frame;
class MethodBlock;
class Object;
class Class;

class Message;
class Snapshot;
class Group;

class Core {
    friend class Mode;
    friend class CertainMode;
    friend class SpeculativeMode;
    friend class RvpMode;
    friend class OoSpmtJvm;
public:
    Thread* m_thread;

    Group* m_group;

    bool m_halt;
public:
    ~Core();
    void set_group(Group* group);
    Group* get_group() { return m_group; }
    void set_thread(Thread* thread);
    uintptr_t* run();
    bool is_halt();
    void halt();
    void step();
    void idle();
    void switch_to_certain_mode();
    void switch_to_speculative_mode();
    void switch_to_rvp_mode();
    void change_mode(Mode* new_mode);
    void record_current_non_certain_mode();
    void restore_original_non_certain_mode();
    Mode* original_uncertain_mode() { return m_old_mode; }
    void init();
    void start();
    void send_certain_message(Message* message);
    Message* get_certain_message();
    void add_speculative_task(Message* message);
    void reload_speculative_tasks();
    void enter_certain_mode();
    void leave_certain_mode(Message* msg);
    void leave_rvp_mode(Object* target_object);
    //void snapshot();

    virtual void verify_speculation(Message* message, bool self = true);
    virtual bool verify(Message* message);
    void handle_verification_success(Message* message, bool self);
    void handle_verification_failure(Message* message, bool self);
    void reexecute_failed_message(Message* message);
    void enter_execution();
    void leave_execution();

    void* execute_method();

    //void handle_verification_failure(Message* message);

    int id() { return m_id; }
    void add_message_to_be_verified(Message* message);
    bool has_message_to_be_verified();
    void destroy_frame(Frame* frame);

    //void init_speculative_support();

    //void on_user_change(Object* old_user, Object* new_user);
    //bool change_owner_object(Object* new_owner);
    void mark_frame_certain();
    void discard_uncertain_execution(bool self);
    void free_discarded_frames(bool only_snapshot);
    void collect_inuse_frames(std::set<Frame*>& frames);
    void collect_snapshot_frames(std::set<Frame*>& frames);

    // void notify_certain_put(PutMsg* msg);

    bool has_messages_to_be_verified() { return not m_messages_to_be_verified.empty(); }

    bool quit_step_loop();
    void signal_quit_step_loop(uintptr_t* result);
    uintptr_t* get_result();
    //{{{ just for debug
    void show_msgs_to_be_verified();
    bool is_correspondence_btw_msgs_and_snapshots_ok();
    //}}} just for debug
    bool is_certain_mode() { return m_mode->is_certain_mode(); }

    void before_signal_exception(Class *exception_class);

    void before_alloc_object();
    void after_alloc_object(Object* obj);
    Group* assign_group_for(Object* obj);

    void check_object(Object* obj);

    // void before_alloc_array();
    // void after_alloc_array();

    void scan();

private:
    void sync_speculative_with_certain();
    void sync_certain_with_speculative();
    void sync_certain_with_snapshot(Snapshot* snapshot);

    CertainMode* get_certain_mode() { return &m_certain_mode; }
    SpeculativeMode* get_speculative_mode() { return &m_speculative_mode; }
    RvpMode* get_rvp_mode() { return &m_rvp_mode; }

    void clear_frame_in_cache(Frame* f);
    void clear_frame_in_rvpbuf(Frame* f);
private:
    int m_speculative_depth;
    int m_certain_depth;

    int m_id;

    bool m_quit_step_loop;
    uintptr_t* m_result;

public:
    SpeculativeMode m_speculative_mode;
    CertainMode m_certain_mode;
    RvpMode m_rvp_mode;
    Mode* m_mode;
    Mode* m_old_mode;


    RvpBuffer m_rvpbuf;

    Message* m_certain_msg;

    // speculative execution state
    std::deque<Message*> m_messages_to_be_verified;
    std::deque<Message*> m_speculative_tasks;
    std::deque<Snapshot*> m_snapshots_to_be_committed;
    Cache m_cache;
    bool m_is_waiting_for_task; // speculative core is waiting for task
private:
    Core(int id);

    //--------------------------------------------
    // for debugging
private:
    bool debug_frame_is_not_in_snapshots(Frame* frame);

    // for logging
private:
    void log_when_leave_certain();

    // for stat
public:
    void report_stat(std::ostream& os);
private:
    long long m_count_spec_msgs_sent;
    long long m_count_spec_msgs_used;
    long long m_count_verify_all;
    long long m_count_verify_ok;
    long long m_count_verify_fail;
    long long m_count_verify_empty;
    long long m_count_rvp;
    long long m_count_certain_instr;
    long long m_count_spec_instr;
    long long m_count_rvp_instr;
    long long m_count_step;
    long long m_count_idle;
};

inline
void
Core::before_signal_exception(Class *exception_class)
{
    m_mode->before_signal_exception(exception_class);
}

inline
void
Core::before_alloc_object()
{
    m_mode->before_alloc_object();
}

inline
void
Core:: after_alloc_object(Object* obj)
{
    m_mode->after_alloc_object(obj);
}

inline
Group*
Core::assign_group_for(Object* obj)
{
    return m_mode->assign_group_for(obj);
}

class VerifyFails {
public:
    VerifyFails(Message* message) : m_message(message)  {}

    Message* get_message()  { return m_message; }
private:
    Message* m_message;
};

class NestedStepLoop {
public:
    NestedStepLoop(int core_id, MethodBlock* mb) : m_core_id(core_id), m_mb(mb)  {}
    int get_core_id()  { return m_core_id; }
    MethodBlock* get_mb()  { return m_mb; }

private:
    int m_core_id;
    MethodBlock* m_mb;

};

// break switch-case
class DeepBreak {
};

Core* g_get_current_core();
void g_set_current_core(Core* current_core);

#endif
