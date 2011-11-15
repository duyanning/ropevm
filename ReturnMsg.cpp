#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

ReturnMsg::ReturnMsg(SpmtThread* target_spmt_thread,
                     uintptr_t* rv, int len,
                     CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                     bool is_top)
:
    Message(MsgType::RETURN, target_spmt_thread)
{
    for (int i = 0; i < len; ++i) {
        retval.push_back(rv[i]);
    }

    this->caller_pc = caller_pc;
    this->caller_frame = caller_frame;
    this->caller_sp = caller_sp;

    m_is_top = is_top;
}

bool
ReturnMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    ReturnMsg* m = static_cast<ReturnMsg*>(msg);
    if (retval != m->retval)
        return false;

    return true;
}

void
ReturnMsg::show(ostream& os) const
{
    int len = retval.size();
    assert(len == 0 || len == 1 || len == 2);
    if (len == 0) {
        os << "RETURN";
    }
    else if (len == 1) {
        os << "RETURN" << hex <<  *((int*)&retval[0]) << dec;
    }
    else if (len == 2) {
        os << "RETURN" << hex << *((long*)&retval[0]) << dec;
    }

    //os << " from " << mb->classobj->name() << '.' << mb->name << ':' << mb->type;
}


void
ReturnMsg::show_detail(std::ostream& os, int id) const
{
    // os << "#" << id << " caller frame:\t" << (void*)caller_frame << "\n";
    // os << "#" << id << " caller pc:\t" << (void*)caller_pc << "\n";
    // os << "#" << id << " caller sp:\t" << (void*)caller_sp << "\n";
}
