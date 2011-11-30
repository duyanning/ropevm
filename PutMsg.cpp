#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

PutMsg::PutMsg(SpmtThread* source_st, SpmtThread* target_st,
               Object* target_object, FieldBlock* fb,
               uintptr_t* val)
:
    RoundTripMsg(MsgType::PUT, source_st, target_st, target_object)
{

    for (int i = 0; i < fb->field_size(); ++i) {
        this->val.push_back(val[i]);
    }
    this->fb = fb;
}


uintptr_t*
PutMsg::get_field_addr()
{
    return fb->field_addr(get_target_object());
}


bool
PutMsg::is_irrevocable()
{
    return false;
}


void
PutMsg::show(ostream& os) const
{
    os << "PUT";
    // int len = val.size();
    // assert(len == 1 || len == 2);

    // if (len == 1) {
    //     os << "PUT" << *fb << hex << *((int*)&val[0]) << dec;
    // }
    // else if (len == 2) {
    //     os << "PUT" << *fb << hex << *((long*)&val[0]) << dec;
    // }
}

void
PutMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}


bool
PutMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    PutMsg* m = static_cast<PutMsg*>(msg);
// - 目标对象
// - 字段的名字
// - 写入的数值
// + 源线程（即发起方法的线程。）



    if (m_target_st != m->m_target_st)
        return false;
    if (fb != m->fb)
        return false;
    if (val != m->val)
        return false;
    if (m_source_st != m->m_source_st)
        return false;

    return true;
}
