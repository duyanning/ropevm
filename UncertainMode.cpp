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

    Message* msg = m_spmt_thread->get_certain_msg();
    if (msg) {

        // 进入确定模式

        if (msg->get_type() == Message::invoke and msg->is_top()) {
            // discard effect
            m_spmt_thread->do_execute_method();
            return;
        }

        if (msg->get_type() == Message::ret and msg->is_top()) {
            m_spmt_thread->signal_quit_step_loop(0);
            return;
        }

        process_certain_message(msg);
        return;
    }

    if (m_spmt_thread->m_is_waiting_for_task) {
        MINILOG(task_load_logger, "#" << m_spmt_thread->id() << " is waiting for task");
        //assert(false);

        // if (not m_spmt_thread->from_certain_to_spec)
        //     m_spmt_thread->snapshot();
        m_spmt_thread->process_next_spec_msg();

        // if (not m_spmt_thread->m_is_waiting_for_task) {
        //     exec_an_instr();
        // }
        return;
    }

    exec_an_instr();
}


void
UncertainMode::process_certain_message(Message* msg)
{
    if (msg->get_type() == Message::invoke
        || msg->get_type() == Message::ret
        || msg->get_type() == Message::put
        || msg->get_type() == Message::arraystore
        //|| msg->get_type() == Message::ack
        ) {

        m_spmt_thread->on_enter_certain_mode();
        m_spmt_thread->verify_speculation(msg);
        return;
    }
    // else if (msg->get_type() == Message::ack) {
    //     m_spmt_thread->enter_certain_mode();
    //     m_spmt_thread->start();
    //     return;
    // }
    else {
        std::cerr << *msg << std::endl;
        assert(false);
    }
}
