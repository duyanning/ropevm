#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

ALoadMsg::ALoadMsg(SpmtThread* source_st, SpmtThread* target_st,
                           Object* array, int type_size, int index)
:
    RoundTripMsg(MsgType::ALOAD, source_st, target_st, array)
{
    this->type_size = type_size;
    this->index = index;
}


bool
ALoadMsg::is_irrevocable()
{
    return false;
}


void
ALoadMsg::show(ostream& os) const
{
    os << "ALOAD";
}


void
ALoadMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}


bool
ALoadMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    ALoadMsg* m = static_cast<ALoadMsg*>(msg);
// - 目标对象(即数组对象)
// - 元素的大小
// - 元素的索引
// + 源线程（即发起方法的线程。）


    if (m_target_st != m->m_target_st)
        return false;
    if (type_size != m->type_size)
        return false;
    if (index != m->index)
        return false;
    if (m_source_st != m->m_source_st)
        return false;

    return true;
}
