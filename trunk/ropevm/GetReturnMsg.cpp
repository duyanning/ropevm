#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

GetReturnMsg::GetReturnMsg(SpmtThread* target_spmt_thread, uintptr_t* val, int size)
:
    Message(MsgType::GET_RET, target_spmt_thread)
{
    for (int i = 0; i < size; ++i) {
        this->val.push_back(val[i]);
    }

}


bool
GetReturnMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    GetReturnMsg* m = static_cast<GetReturnMsg*>(msg);
    if (val != m->val)
        return false;

    return true;
}


void
GetReturnMsg::show(std::ostream& os) const
{
    os << "GET_RET";
}


void
GetReturnMsg::show_detail(std::ostream& os, int id) const
{
}
