#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

PutReturnMsg::PutReturnMsg(SpmtThread* target_spmt_thread)
:
    Message(MsgType::PUT_RET, target_spmt_thread)
{
}


bool
PutReturnMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    return true;
}


void
PutReturnMsg::show(std::ostream& os) const
{
}


void
PutReturnMsg::show_detail(std::ostream& os, int id) const
{
}
