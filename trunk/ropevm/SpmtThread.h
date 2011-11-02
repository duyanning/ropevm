#ifndef SPMTTHREAD_H
#define SPMTTHREAD_H

#include "rope.h"

#include "CertainMode.h"
#include "SpeculativeMode.h"
#include "RvpMode.h"

#include "SpecMsgQueue.h"
#include "StatesBuffer.h"
#include "RvpBuffer.h"

class Thread;
class Frame;
class MethodBlock;
class Object;
class Class;

class Message;
class Snapshot;
class Group;

// struct ControlTuple {
//     ControlTuple(CodePntr pc1, Frame *frame1, uintptr_t *sp1)
//         : pc(pc1), frame(frame1), sp(sp1)
//     {
//     }
//     CodePntr pc;
//     Frame* frame;
//     uintptr_t* sp;
// };

class SpmtThread {
    friend class Mode;
    friend class CertainMode;
    friend class UncertainMode;
    friend class SpeculativeMode;
    friend class RvpMode;
    friend class RopeVM;
public:
    ~SpmtThread();
    void set_group(Group* group);
    Group* get_group() { return m_group; }
    void set_thread(Thread* thread);
    uintptr_t* run();
    bool is_halt();
    void sleep();
    void step();
    void idle();
    void switch_to_certain_mode();
    void switch_to_speculative_mode();
    void switch_to_rvp_mode();
    Mode* get_current_mode();
    void change_mode(Mode* new_mode);
    void record_current_non_certain_mode();
    void restore_original_non_certain_mode();
    Mode* original_uncertain_mode() { return m_old_mode; }
    void init();
    void wakeup();



    void send_certain_msg(SpmtThread* target_thread, Message* msg);
    void send_spec_msg(SpmtThread* target_thread, Message* msg);
    void revoke_spec_msg(SpmtThread* target_thread, Message* msg);

    void set_certain_msg(Message* msg);
    void add_spec_msg(Message* msg);
    void remove_spec_msg(Message* msg);

    Message* get_certain_msg();


    Message* current_spec_msg();
    void commit(Effect* effect);
    void discard_all_effect();

    void reload_speculative_tasks(); // refactor: to remove
    void on_enter_certain_mode();
    void leave_certain_mode(Message* msg);


    void process_certain_msg(Message* msg);

    void process_next_spec_msg();
    void process_spec_msg(Message* msg);
    bool is_waiting_for_spec_msg();


    void snapshot(bool pin);
    void pin_frames();

    void verify_speculation(Message* message);
    void handle_verification_success(Message* message);
    void handle_verification_failure(Message* message);
    void reexecute_failed_message(Message* message);

    int id() { return m_id; }
    void add_message_to_be_verified(Message* message); // refactor: remove
    bool has_message_to_be_verified();                 // refactor: remove


    void destroy_frame(Frame* frame);

    void mark_frame_certain();  // refactor: to remove
    void discard_uncertain_execution(bool self); // refactor: to remove
    void free_discarded_frames(bool only_snapshot); // refactor: to remove
    void collect_inuse_frames(std::set<Frame*>& frames); // refactor: to remove
    void collect_snapshot_frames(std::set<Frame*>& frames); // refactor: to remove


    bool check_quit_step_loop();
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

    void scan();

    // for stat
    void report_stat(std::ostream& os);
private:
    bool has_unprocessed_spec_msg();
    void begin_process_next_msg();

    void sync_speculative_with_certain();
    void sync_certain_with_speculative();
    void sync_certain_with_snapshot(Snapshot* snapshot);

    CertainMode* get_certain_mode() { return &m_certain_mode; }
    SpeculativeMode* get_spec_mode() { return &m_spec_mode; }
    RvpMode* get_rvp_mode() { return &m_rvp_mode; }

    void clear_frame_in_states_buffer(Frame* f);
    void clear_frame_in_rvp_buffer(Frame* f);
private:
    Thread* m_thread;
    Group* m_group;

    bool m_halt;

    int m_id;

    bool m_quit_step_loop;
    uintptr_t* m_result;

private:
    SpeculativeMode m_spec_mode;
    CertainMode m_certain_mode;
    RvpMode m_rvp_mode;
    Mode* m_mode;
    Mode* m_old_mode;


    Message* m_certain_message;
    std::list<Message*> m_spec_msg_queue;
    std::list<Message*>::iterator m_next_spec_msg;

    StatesBuffer m_state_buffer;
    RvpBuffer m_rvp_buffer;
    bool m_need_spec_msg;


    //std::stack<ControlTuple> m_certain_control_tuple_stack;


    std::deque<Message*> m_messages_to_be_verified; // refactor: remove
    std::deque<Message*> m_speculative_tasks; // refactor: remove
    std::deque<Snapshot*> m_snapshots_to_be_committed; // refactor: remove
    bool m_is_waiting_for_task; // refactor: to remove

private:
    SpmtThread(int id);

    //--------------------------------------------
    // for debugging
private:
    bool debug_frame_is_not_in_snapshots(Frame* frame);

    // for logging
private:
    void log_when_leave_certain();

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

// break to loop-switch
class DeepBreak {
};

SpmtThread* g_get_current_spmt_thread();
void g_set_current_spmt_thread(SpmtThread* st);

#endif
