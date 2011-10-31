#include "std.h"
#include "rope.h"
#include "UncertainMode.h"
#include "Message.h"
#include "SpmtThread.h"

UncertainMode::UncertainMode(const char* name)
:
    Mode(name)
{
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
        m_spmt_thread->verify_speculation(msg, false);
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
