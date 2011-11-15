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
    os << "ALOAD";
}


void
ArrayLoadMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
