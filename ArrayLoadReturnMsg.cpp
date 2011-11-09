#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

ArrayLoadReturnMsg::ArrayLoadReturnMsg(SpmtThread* target_spmt_thread, uintptr_t* val, int size)
:
    Message(MsgType::ARRAYLOAD_RET, target_spmt_thread)
{
}


bool
ArrayLoadReturnMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    ArrayLoadReturnMsg* m = static_cast<ArrayLoadReturnMsg*>(msg);
    if (val != m->val)
        return false;

    return true;
}


void
ArrayLoadReturnMsg::show(std::ostream& os) const
{
}


void
ArrayLoadReturnMsg::show_detail(std::ostream& os, int id) const
{
}