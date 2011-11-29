#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

PutRetMsg::PutRetMsg(SpmtThread* target_st)
:
    Message(MsgType::PUT_RET, target_st)
{
}


bool
PutRetMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    return true;
}


void
PutRetMsg::show(std::ostream& os) const
{
    os << "PUT_RET";
}


void
PutRetMsg::show_detail(std::ostream& os, int id) const
{
}
