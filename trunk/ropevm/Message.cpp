#include "std.h"
#include "rope.h"
#include "Message.h"
#include "Loggers.h"

using namespace std;


int msg_count = 0;              // just for debug

Message::Message(MsgType type, SpmtThread* target_spmt_thread)
   :
    m_type(type),
    m_target_spmt_thread(target_spmt_thread),
    m_effect(nullptr)
{
}

Message::~Message()
{
}


MsgType
Message::get_type()
{
    return m_type;
}


SpmtThread*
Message::get_target_spmt_thread()
{
    return m_target_spmt_thread;
}


Effect*
Message::get_effect()
{
    return m_effect;
}


void
Message::set_effect(Effect* effect)
{
    m_effect = effect;
}


bool
Message::equal(Message* msg)
{
    assert(false);              // 异步消息比较指针，所以其equal没有实现。
    return false;
}


ostream&
operator<<(ostream& os, const Message* msg)
{
    msg->show(os);
    if (p) {
        os << " (" << (void*)msg << ")";
    }
    return os;
}

void
show_msg_detail(ostream& os, int id, Message* msg)
{
    msg->show_detail(os, id);
}


bool
g_is_async_msg(Message* msg)
{
    MsgType type = msg->get_type();
    if (type == MsgType::INVOKE
        or type == MsgType::PUT
        or type == MsgType::GET
        or type == MsgType::ASTORE
        or type == MsgType::ALOAD)
        return true;

    return false;
}
