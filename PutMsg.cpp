#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

PutMsg::PutMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
               Object* target_object, FieldBlock* fb,
               uintptr_t* val)
:
    RoundTripMsg(MsgType::PUT, source_spmt_thread, target_spmt_thread, target_object)
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


void
PutMsg::show(ostream& os) const
{
    int len = val.size();
    assert(len == 1 || len == 2);

    if (len == 1) {
        os << "PUT" << *fb << hex << *((int*)&val[0]) << dec;
    }
    else if (len == 2) {
        os << "PUT" << *fb << hex << *((long*)&val[0]) << dec;
    }
}

void
PutMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
