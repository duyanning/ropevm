#include "std.h"
#include "rope.h"
#include "UncertainMode.h"
#include "Message.h"
#include "SpmtThread.h"

#include "RopeVM.h"
#include "interp.h"
#include "MiniLogger.h"
#include "Loggers.h"


UncertainMode::UncertainMode(const char* name)
:
    Mode(name)
{
}


void
UncertainMode::step()
{
    assert(RopeVM::do_spec);

    // 阶段I：检测并处理确定性外部事件（必须处理，推测执行那是你自己的事情，不是必须的，但确定性事件必须响应）
    Message* msg = m_spmt_thread->get_certain_msg();

    if (msg) {

        // 进入确定模式
        m_spmt_thread->enter_certain_mode();

        if (msg->get_type() == Message::invoke) {
            InvokeMsg* invoke_msg = static_cast<InvokeMsg*>(msg);
            if (invoke_msg->is_top()) {
                // discard effect
                m_spmt_thread->m_certain_mode.do_execute_method(
                                                                invoke_msg->get_target_object(),
                                                                invoke_msg->mb,
                                                                invoke_msg->parameters);
                return;
            }
        }

        if (msg->get_type() == Message::ret) {
            ReturnMsg* return_msg = static_cast<ReturnMsg*>(msg);
            if (return_msg->is_top()) {
                m_spmt_thread->signal_quit_step_loop(0);
                return;
            }
        }

        m_spmt_thread->enter_certain_mode();
        m_spmt_thread->verify_speculation(msg);

        return;
    }


    // 阶段II：解释指令

    // 1，移除被收回的消息，更新消息队列
    // ...

    // 2，加载新的待处理消息
    if (m_spmt_thread->is_waiting_for_spec_msg()) {
        // 能到这里，必然不是rvp模式，因为处在rvp模式下就不会等待推测消息
        MINILOG(task_load_logger, "#" << m_spmt_thread->id()
                << " is waiting for task");

        m_spmt_thread->launch_next_spec_msg();

    }

    // 3，解释
    fetch_and_interpret_an_instruction();
}
