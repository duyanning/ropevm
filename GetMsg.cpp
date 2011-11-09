#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

GetMsg::GetMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
               Object* target_object, FieldBlock* fb)
:
    RoundTripMsg(MsgType::GET, source_spmt_thread, target_spmt_thread, target_object)
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


void
GetMsg::show(ostream& os) const
{
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
