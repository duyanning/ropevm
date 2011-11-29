#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

ALoadRetMsg::ALoadRetMsg(SpmtThread* target_st, uintptr_t* val, int size)
:
    Message(MsgType::ALOAD_RET, target_st)
{
    for (int i = 0; i < size; ++i) {
        this->val.push_back(val[i]);
    }
}


bool
ALoadRetMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    ALoadRetMsg* m = static_cast<ALoadRetMsg*>(msg);
    if (val != m->val)
        return false;

    return true;
}


void
ALoadRetMsg::show(std::ostream& os) const
{
    os << "ALOAD_RET";
}


void
ALoadRetMsg::show_detail(std::ostream& os, int id) const
{
}
