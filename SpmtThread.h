#ifndef SPMTTHREAD_H
#define SPMTTHREAD_H

#include "rope.h"
#include "CertainMode.h"
#include "SpecMode.h"
#include "RvpMode.h"
#include "StateBuffer.h"
#include "RvpBuffer.h"


class Thread;
class Frame;
class MethodBlock;
class Object;
class Class;
class Message;
class RoundTripMsg;
class InvokeMsg;
class ReturnMsg;
class Snapshot;


// 注意！！！所以这些状态都是指非确定模式的状态。
// 非确定模式下才会停机，确定模式是不会停机的，所以确定模式无所谓状态。
enum class RunningState {
    ongoing,                         // 推测执行进行中
    ongoing_but_need_launch_new_msg, // 推测执行进行中，但是需要加载新的异步消息
    halt_no_asyn_msg, // 无异步消息，无法继续推测执行。（和ongoing_but_need_launch_new_msg并不重复，详见SpmtThread::add_spec_msg）
    halt_no_syn_msg, // 无同步消息（即无返回值方法），无法继续推测执行
    halt_cannot_return_from_top_method,    // 推测模式不能从顶级方法返回（因为顶级方法的调用方是native代码）
    halt_cannot_signal_exception,   // 非确定模式不能抛出异常
    halt_cannot_alloc_object,       // rvp模式不能产生新对象
    halt_cannot_exec_native_method, // 非确定模式不能执行native方法
    halt_cannot_exec_sync_method, // 非确定模式不能执行synchronized方法
    halt_spec_barrier,            // 遇到推测执行路障
    halt_cannot_exec_method,      // 非确定模式不能execute_method
    halt_worthless_to_execute,    // 时间太短，不值得推测执行
    halt_model2_requirement       // 模型2
};



class SpmtThread {
    friend class Mode;
    friend class CertainMode;
    friend class UncertainMode;
    friend class SpecMode;
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

    // 唤醒与停机。为什么叫停机而不叫睡眠，停机是指停止解释java指令，睡
    // 眠是指线程睡眠，因为停机而睡眠，不可混淆。
    bool is_halt();
    void halt(RunningState reason); // 推测模式下停机（即停止解释java指令），停机必须给出原因。
    //void halt();
    void wakeup();

    // 只要没停机就step
    void step();
    void idle();                // 仅为统计而存在

    // 模式相关
    void switch_to_mode(Mode* mode);
    void switch_to_certain_mode();
    void switch_to_speculative_mode();
    void switch_to_rvp_mode();
    void switch_to_previous_mode(); // 切换到进入确定模式之前的推测模式或rvp模式
    bool is_spec_mode();
    bool is_certain_mode();
    bool is_rvp_mode();
    Mode* get_current_mode();

    // 由线程自己调用。无论确定消息还是推测消息，都由send_msg发送，不过
    // 推测消息最终还要被affirm或被revoke。
    void send_msg(Message* msg);
    void affirm_spec_msg(Message* msg);
    void revoke_spec_msg(RoundTripMsg* msg);

    void start_afresh_spec_execution(); // 开始新的推测执行
    void resume_suspended_spec_execution(); // 恢复被确定执行挂起的推测执行或rvp执行

    // 由其他线程调用。注意：在多os线程实现方式下，那些由其他spmt线程调
    // 用的方法，会和由自己调用的方法，如不小心同步，就会发生冲突。
    void set_certain_msg(Message* msg);
    void add_spec_msg(Message* msg);
    void add_revoked_spec_msg(RoundTripMsg* msg);

    // 确定消息处理
    Message* get_certain_msg();                // 检测确定消息
    void verify_speculation(Message* message); // 验证推测消息，并进行后续处理
    void on_event_top_invoke(InvokeMsg* msg); // 顶级方法调用事件
    void on_event_top_return(ReturnMsg* msg);

    // 推测消息处理
    bool has_unprocessed_spec_msg();
    void launch_next_spec_msg();        // 加载异步推测消息
    void launch_spec_msg(Message* msg); // 加载同步推测消息
    bool is_waiting_for_spec_msg();
    void discard_all_revoked_msgs(); // 丢弃被收回的消息
    void discard_revoked_msg(RoundTripMsg* revoked_msg);

    // 根据不同的模式对消息进行处理
    void process_msg(Message* msg);

    // 提交与丢弃
    void commit_effect(Effect* effect);
    void discard_effect(Effect* effect);
    void abort_uncertain_execution();

    // 快照
    void take_snapshot(bool pin);
    void pin_frames();          // 钉住栈帧

    // 栈帧相关
    void pop_frame(Frame* frame); // 因return而销毁栈桢（应改名为pop_frame）
    void unwind_frame(Frame* frame);  // 因unwind而销毁栈桢

    // drive loop相关
    void signal_quit_drive_loop();
    bool check_quit_drive_loop();
    void drive_loop();
    static void S_threadStart(SpmtThread* st); // 多os实现下，os线程运行S_threadStart，该函数再调用st->drive_loop()

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
    void set_exception_threw_to_me(Object* exception, Frame* excep_frame, bool is_top);
private:
    int m_id;
    Thread* m_thread = nullptr;           // 所属的java线程
    Object* m_leader = nullptr;
    bool m_halt = true;
    bool m_quit_drive_loop = false;
    SpmtThread* m_quit_causer = nullptr;

    // 模式相关
    SpecMode m_spec_mode;
    CertainMode m_certain_mode;
    RvpMode m_rvp_mode;
    Mode* m_mode;               // 当前模式
    Mode* m_previous_mode;      // 进入确定模式之前的模式：推测或rvp

    // 确定消息
    Message* m_certain_message = nullptr;

    // 推测消息
    std::list<Message*> m_spec_msg_queue;
    std::list<Message*>::iterator m_iter_next_spec_msg;
    Message* m_current_spec_msg = nullptr; // 正在处理的推测消息。只要有正在进行的推测执行，此变量就不为nullptr。
    RunningState m_spec_running_state = RunningState::halt_no_asyn_msg; // 推测执行的状态
    std::vector<RoundTripMsg*> m_revoked_msgs; // 发送方要求收回这些推测消息

    // 不同模式下读写的去处
    StateBuffer m_state_buffer;
    RvpBuffer m_rvp_buffer;
    std::vector<Frame*> V;

    Object* m_excep_threw_to_me = nullptr; // 其他线程扔给我的异常
    Frame* m_excep_frame = nullptr;        // 异常发生的栈帧，跟m_excep_threw_to_me关联
    bool m_is_unwind_top;        // 被unwind的栈桢是否top frame，被unwind的栈桢的其上级为m_excep_frame

private:
    SpmtThread(int id);

    //--------------------------------------------

public:
    STAT_DECL\
    (
     void report_stat(std::ostream& os);
     ) // STAT_DECL

private:
    STAT_DECL                                   \
    (
     long long m_count_verify_ok; // 验证成功次数
     long long m_count_verify_fail; // 验证失败次数（不含无待验证消息的情况）
     long long m_count_verify_empty; // 验证时无待验证消息的次数

     long long m_count_cert_cycle; // 确定模式周期数
     long long m_count_spec_cycle; // 推测模式周期数
     long long m_count_rvp_cycle;  // rvp模式周期数

     long long m_count_busy_cycle; // 繁忙的周期数
     long long m_count_idle_cycle; // 空闲的周期数

     long long m_count_revoked; // 消息被召回的次数
     ); // STAT_DECL
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
