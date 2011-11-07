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
class InvokeMsg;
class ReturnMsg;
class Snapshot;


class SpmtThread {
    friend class Mode;
    friend class CertainMode;
    friend class UncertainMode;
    friend class SpeculativeMode;
    friend class RvpMode;
    friend class RopeVM;
public:

    //Object* get_leader();

    ~SpmtThread();

    // spmt线程总是属于某个java线程
    Thread* get_thread();
    void set_thread(Thread* thread);

    // 唤醒与睡眠
    bool is_halt();
    void sleep();
    void wakeup();

    // 只要醒着就step
    void step();
    void idle();                // 仅为统计而存在


    // 模式相关
    void switch_to_certain_mode();
    void switch_to_speculative_mode();
    void switch_to_rvp_mode();
    Mode* get_current_mode();
    void change_mode(Mode* new_mode);
    void record_current_non_certain_mode();
    void restore_original_non_certain_mode();
    Mode* original_uncertain_mode() { return m_old_mode; }
    void init();



    // 向其他线程发送消息
    void send_certain_msg(SpmtThread* target_thread, Message* msg);
    void send_spec_msg(SpmtThread* target_thread, Message* msg);
    void revoke_spec_msg(SpmtThread* target_thread, Message* msg);

    // 由其他线程给自己发送消息
    void set_certain_msg(Message* msg);
    void add_spec_msg(Message* msg);
    void remove_spec_msg(Message* msg);

    void discard_revoked_msgs();

    // 检测并处理确定消息
    Message* get_certain_msg();
    void process_certain_msg(Message* msg);

    // 检测并处理推测消息
    // msg queue related
    bool has_unprocessed_spec_msg();
    void launch_next_spec_msg();
    void launch_spec_msg(Message* msg);
    //Message* current_spec_msg();
    void process_spec_msg(Message* msg);
    bool is_waiting_for_spec_msg();

    // 提交与丢弃
    void commit_effect(Effect* effect);
    void discard_all_effect();


    void enter_certain_mode();
    void leave_certain_mode(Message* msg); // refactor: to remove

    // 快照
    void snapshot(bool pin);
    void pin_frames();

    // 顶级方法调用事件
    void on_event_top_invoke(InvokeMsg* msg);
    void on_event_top_return(ReturnMsg* msg);

    void verify_speculation(Message* message); // 验证推测消息，并进行后续处理
    //void handle_verification_success(Message* message);
    //void handle_verification_failure(Message* message);
    //void reexecute_failed_message(Message* message);

    int id() { return m_id; }


    void destroy_frame(Frame* frame);

    void mark_frame_certain();  // refactor: to remove



    bool check_quit_step_loop();
    void signal_quit_step_loop(uintptr_t* result); // refactor: 不需要这个参数
    uintptr_t* get_result();

    bool is_certain_mode() { return m_mode->is_certain_mode(); }

    void before_signal_exception(Class *exception_class);

    void before_alloc_object();
    void after_alloc_object(Object* obj);
    SpmtThread* assign_spmt_thread_for(Object* obj);

    Object* get_current_object();
    GroupingPolicyEnum get_leader_policy();
    void set_leader(Object* leader);


    void scan();                // GC scan

    // for stat
    void report_stat(std::ostream& os);


    // 多os实现下，os线程运行S_threadStart，该函数再调用spmt_thread->drive_loop()
    static void S_threadStart(SpmtThread* spmt_thread);
    uintptr_t* drive_loop();

private:

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


    // 确定消息
    Message* m_certain_message;

    // 推测消息
    std::list<Message*> m_spec_msg_queue;
    std::list<Message*>::iterator m_iter_next_spec_msg;
    Message* m_current_spec_msg; // 正在处理的推测消息
    bool m_need_spec_msg;        // 推测执行需要推测消息才能继续
    std::vector<Message*> m_revoked_msgs; // 发送方要求收回这些推测消息


    // 不同模式下读写的去处
    StatesBuffer m_state_buffer;
    RvpBuffer m_rvp_buffer;



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

#endif
