#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

GetMsg::GetMsg(SpmtThread* source_st, SpmtThread* target_st,
               Object* target_object, FieldBlock* fb)
:
    RoundTripMsg(MsgType::GET, source_st, target_st, target_object)
{
    this->fb = fb;
}

uintptr_t*
GetMsg::get_field_addr()
{
    return fb->field_addr(get_target_object());
}

int
GetMsg::get_field_size()
{
    return fb->field_size();
}


bool
GetMsg::is_irrevocable()
{
    return false;
}


bool
GetMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    GetMsg* m = static_cast<GetMsg*>(msg);
// - 目标对象
// - 字段的名字
// + 源线程（即发起方法的线程。）


    if (m_target_st != m->m_target_st)
        return false;
    if (fb != m->fb)
        return false;
    if (m_source_st != m->m_source_st)
        return false;

    return true;
}


void
GetMsg::show(ostream& os) const
{
    os << "GET";

    // int len = val.size();
    // assert(len == 1 || len == 2);

    // if (len == 1) {
    //     os << "get32 " << *fb << hex << *((int*)&val[0]) << dec;
    // }
    // else if (len == 2) {
    //     os << "get64 " << *fb << hex << *((long*)&val[0]) << dec;
    // }
}

void
GetMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
