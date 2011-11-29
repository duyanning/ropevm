#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

GetRetMsg::GetRetMsg(SpmtThread* target_st, uintptr_t* val, int size)
:
    Message(MsgType::GET_RET, target_st)
{
    for (int i = 0; i < size; ++i) {
        this->val.push_back(val[i]);
    }

}


bool
GetRetMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    GetRetMsg* m = static_cast<GetRetMsg*>(msg);
    if (val != m->val)
        return false;

    return true;
}


void
GetRetMsg::show(std::ostream& os) const
{
    os << "GET_RET";
}


void
GetRetMsg::show_detail(std::ostream& os, int id) const
{
}
