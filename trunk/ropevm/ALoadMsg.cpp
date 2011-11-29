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
