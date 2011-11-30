#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

AStoreMsg::AStoreMsg(SpmtThread* source_st, SpmtThread* target_st,
                             Object* array, int type_size, int index, uintptr_t* slots)

:
    RoundTripMsg(MsgType::ASTORE, source_st, target_st, array)
{

    this->type_size = type_size;
    this->index = index;
    int nslots = size2nslots(type_size);
    for (int i = 0; i < nslots; ++i) {
        this->val.push_back(slots[i]);
    }

}


bool
AStoreMsg::is_irrevocable()
{
    return false;
}

void
AStoreMsg::show(ostream& os) const
{
    os << "ASTORE";

    // int nslots = val.size();
    // assert(nslots == 1 || nslots == 2);

    // os << "array store" << type_size << " " << m_object->classobj->name() << "(" << index << ") = ";

    // if (nslots == 1) {
    //     int i = *(int*)&val[0];
    //     os << hex << i << dec;
    // }
    // else {                      //  nslots == 2
    //     double d = *(double*)&val[0];
    //     os << hex << d << dec;
    // }
}

void
AStoreMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}


bool
AStoreMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    AStoreMsg* m = static_cast<AStoreMsg*>(msg);
// - 目标对象(即数组对象)
// - 元素的大小
// - 元素的索引
// - 写入的数值
// + 源线程（即发起方法的线程。）



    if (m_target_st != m->m_target_st)
        return false;
    if (type_size != m->type_size)
        return false;
    if (index != m->index)
        return false;
    if (val != m->val)
        return false;
    if (m_source_st != m->m_source_st)
        return false;

    return true;
}
