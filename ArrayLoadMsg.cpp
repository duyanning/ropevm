#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

ArrayLoadMsg::ArrayLoadMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
                           Object* array, int type_size, int index)
:
    RoundTripMsg(MsgType::ALOAD, source_spmt_thread, target_spmt_thread, array)
{
    this->type_size = type_size;
    this->index = index;
}


void
ArrayLoadMsg::show(ostream& os) const
{
    // int type_size = val.size();
    // assert(type_size >= 1 && type_size <= 8);

    // // os << "array load" << type_size << " " << m_source_object->classobj->name() << "(" << index << ") = ";

    // os << hex;
    // for (int i = 0; i < val.size(); ++i) {
    //     os << (int)val[i] << " ";
    // }
    // os << dec;
}


void
ArrayLoadMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
