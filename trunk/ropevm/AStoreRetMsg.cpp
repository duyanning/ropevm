#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

AStoreRetMsg::AStoreRetMsg(SpmtThread* target_st)
:
    Message(MsgType::ASTORE_RET, target_st)
{
}


bool
AStoreRetMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    return true;
}


void
AStoreRetMsg::show(std::ostream& os) const
{
    os << "ASTORE_RET";
}


void
AStoreRetMsg::show_detail(std::ostream& os, int id) const
{
}
