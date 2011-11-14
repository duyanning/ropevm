#ifndef SPMTTHREAD_H
#define SPMTTHREAD_H

#include "rope.h"
#include "CertainMode.h"
#include "SpeculativeMode.h"
#include "RvpMode.h"
#include "StateBuffer.h"
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
    ~SpmtThread();

    int id();

    // spmt线程总是属于某个java线程
    Thread* get_thread();
    void set_thread(Thread* thread);
    Object* get_current_object(); // 当前在执行哪个对象的方法
    GroupingPolicyEnum get_policy(); // spmt线程的分组策略
    void set_leader(Object* leader);
    //Object* get_leader();

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
    void switch_to_previous_mode(); // 切换到进入确定模式之前的推测模式或rvp模式
    bool is_spec_mode();
    bool is_certain_mode();
    bool is_rvp_mode();
    Mode* get_current_mode();

    // 由线程自己调用。无论确定消息还是推测消息，都由send_msg发送，不过
    // 推测消息最终还要被confirm或被revoke。
    void send_msg(Message* msg);
    void confirm_spec_msg(Message* msg);
    void revoke_spec_msg(Message* msg);

    // 由其他线程调用。注意：在多os线程实现方式下，那些由其他spmt线程调
    // 用的方法，会和由自己调用的方法，如不小心同步，就会发生冲突。
    void set_certain_msg(Message* msg);
    void add_spec_msg(Message* msg);
    void add_revoked_spec_msg(Message* msg);

    // 确定消息处理
    Message* get_certain_msg();                // 检测确定消息
    void verify_speculation(Message* message); // 验证推测消息，并进行后续处理
    void on_event_top_invoke(InvokeMsg* msg); // 顶级方法调用事件
    void on_event_top_return(ReturnMsg* msg);

    // 推测消息处理
    bool has_unprocessed_spec_msg();
    void launch_next_spec_msg();
    void launch_spec_msg(Message* msg);
    bool is_waiting_for_spec_msg();
    void remove_revoked_msgs(); // 扔掉被收回的消息
    void remove_revoked_msg(Message* msg);

    // 根据不同的模式对消息进行处理
    void process_msg(Message* msg);

    // 提交与丢弃
    void commit_effect(Effect* effect);
    void discard_effect(Effect* effect);
    void abort_uncertain_execution();

    // 快照
    void snapshot(bool pin);
    void pin_frames();          // 钉住栈帧

    // 栈帧相关
    void destroy_frame(Frame* frame);

    // drive loop相关
    void signal_quit_drive_loop();
    bool check_quit_drive_loop();
    void drive_loop();
    static void S_threadStart(SpmtThread* spmt_thread); // 多os实现下，os线程运行S_threadStart，该函数再调用spmt_thread->drive_loop()

    // 根据模式不同
    void before_signal_exception(Class *exception_class);
    void before_alloc_object();
    void after_alloc_object(Object* obj);



    // 垃圾回收的标记阶段（暂未实现）
    void scan();

    // state buffer或rvp buffer相关
    void clear_frame_in_state_buffer(Frame* f);
    void clear_frame_in_rvp_buffer(Frame* f);

    // 处理异常
    void on_event_exception_throw_to_me(Object* exception); // 发现别人抛来异常时调用
    void do_throw_exception(); // 自己检测到异常时调用
    void process_exception(Object* exception, Frame* excep_frame); // 查找异常处理器及后续
    Object* get_exception_threw_to_me();
    void set_exception_threw_to_me(Object* exception, Frame* excep_frame);
private:
    int m_id;
    Thread* m_thread;           // 所属的java线程
    Object* m_leader;
    bool m_halt;
    bool m_quit_drive_loop;
    SpmtThread* m_quit_causer;

    // 模式相关
    SpeculativeMode m_spec_mode;
    CertainMode m_certain_mode;
    RvpMode m_rvp_mode;
    Mode* m_mode;               // 当前模式
    Mode* m_previous_mode;      // 进入确定模式之前的模式：推测或rvp

    // 确定消息
    Message* m_certain_message;

    // 推测消息
    std::list<Message*> m_spec_msg_queue;
    std::list<Message*>::iterator m_iter_next_spec_msg;
    Message* m_current_spec_msg; // 正在处理的推测消息
    bool m_need_spec_msg;        // 推测执行需要推测消息才能继续。给出了推测模式睡眠的原因。推测模式下睡眠有两种原因：其一，遇到特权指令；其二，无推测消息。
    std::vector<Message*> m_revoked_msgs; // 发送方要求收回这些推测消息

    // 不同模式下读写的去处
    StateBuffer m_state_buffer;
    RvpBuffer m_rvp_buffer;
    std::vector<Frame*> V;

    Object* m_excep_threw_to_me; // 其他线程扔给我的异常
    Frame* m_excep_frame;        // 异常发生的栈帧，跟m_excep_threw_to_me关联

private:
    SpmtThread(int id);

    //--------------------------------------------

    // for stat
public:
    void report_stat(std::ostream& os);

    // for debugging

    // for logging
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


inline
int SpmtThread::id() { return m_id; }

inline
bool SpmtThread::is_spec_mode() { return m_mode == &m_spec_mode; }

inline
bool SpmtThread::is_certain_mode() { return m_mode == &m_certain_mode; }

inline
bool SpmtThread::is_rvp_mode() { return m_mode == &m_rvp_mode; }


SpmtThread* g_get_current_spmt_thread();

#endif
