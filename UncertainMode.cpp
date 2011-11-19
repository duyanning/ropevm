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

    // 阶段I：检测并处理确定性外部事件（在失去确定控制后是否推测执行那是你自己的事情，不是必须的，但确定性事件是必须响应的。）
    Object* excep = m_spmt_thread->get_exception_threw_to_me();
    if (excep) {
        m_spmt_thread->switch_to_certain_mode();
        m_spmt_thread->on_event_exception_throw_to_me(excep);
        return;
    }


    Message* msg = m_spmt_thread->get_certain_msg();
    if (msg) {

        // 进入确定模式
        m_spmt_thread->m_previous_mode = this;
        m_spmt_thread->switch_to_certain_mode();

        if (msg->get_type() == MsgType::INVOKE) {
            InvokeMsg* invoke_msg = static_cast<InvokeMsg*>(msg);
            if (invoke_msg->is_top()) {
                m_spmt_thread->on_event_top_invoke(invoke_msg);
                return;
            }
        }

        if (msg->get_type() == MsgType::RETURN) {
            ReturnMsg* return_msg = static_cast<ReturnMsg*>(msg);
            if (return_msg->is_top()) {
                m_spmt_thread->on_event_top_return(return_msg);
                return;
            }
        }


        m_spmt_thread->verify_speculation(msg);
        return;
    }


    // 阶段II：解释指令

    // 1，移除被收回的消息，更新消息队列(这步一定要再推测执行之前，免得人家都收回了，你还瞎忙)
    m_spmt_thread->discard_all_revoked_msgs();


    // 2，加载新的待处理消息
    if (m_spmt_thread->is_spec_mode() and m_spmt_thread->m_spec_running_state == RunningState::halt_no_asyn_msg) {

        MINILOG(task_load_logger, "#" << m_spmt_thread->id()
                << " is waiting for task");

        m_spmt_thread->launch_next_spec_msg();

        return;                 // 有可能没有异步消息，这样的话就不能执行下边的fetch_and_interpret_an_instruction

    }

    // 3，取出并解释一条java指令
    fetch_and_interpret_an_instruction();
}
