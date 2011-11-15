#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

ArrayStoreMsg::ArrayStoreMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
                             Object* array, int type_size, int index, uintptr_t* slots)

:
    RoundTripMsg(MsgType::ASTORE, source_spmt_thread, target_spmt_thread, array)
{

    this->type_size = type_size;
    this->index = index;
    int nslots = size2nslots(type_size);
    for (int i = 0; i < nslots; ++i) {
        this->val.push_back(slots[i]);
    }

}


void
ArrayStoreMsg::show(ostream& os) const
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
ArrayStoreMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
