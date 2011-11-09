#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

ArrayStoreReturnMsg::ArrayStoreReturnMsg(SpmtThread* target_spmt_thread)
:
    Message(MsgType::ARRAYSTORE_RET, target_spmt_thread)
{
}


bool
ArrayStoreReturnMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    return true;
}


void
ArrayStoreReturnMsg::show(std::ostream& os) const
{
}


void
ArrayStoreReturnMsg::show_detail(std::ostream& os, int id) const
{
}
