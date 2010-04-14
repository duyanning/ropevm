#include "std.h"
#include "jam.h"
#include "UncertainMode.h"
#include "Message.h"
#include "Core.h"

UncertainMode::UncertainMode(const char* name)
:
    Mode(name)
{
}

void
UncertainMode::process_certain_message(Message* msg)
{
    if (msg->get_type() == Message::call
        || msg->get_type() == Message::ret
        || msg->get_type() == Message::put
        || msg->get_type() == Message::arraystore
        || msg->get_type() == Message::ack
        ) {

        m_core->enter_certain_mode();
        m_core->verify_speculation(msg, false);
        return;
    }
    // else if (msg->get_type() == Message::ack) {
    //     m_core->enter_certain_mode();
    //     m_core->start();
    //     return;
    // }
    else {
        std::cerr << *msg << std::endl;
        assert(false);
    }
}
