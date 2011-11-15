#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

InvokeMsg::InvokeMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
                     Object* target_object, MethodBlock* mb, uintptr_t* args,
                     CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                     bool is_top)
:
    RoundTripMsg(MsgType::INVOKE, source_spmt_thread, target_spmt_thread, target_object)
{
    this->mb = mb;

    for (int i = 0; i < mb->args_count; ++i) {
        parameters.push_back(args[i]);
    }

    this->caller_sp = caller_sp;
    this->caller_frame = caller_frame;
    this->caller_pc = caller_pc;

    m_is_top = is_top;
}


void
InvokeMsg::show(ostream& os) const
{
    os << "INVOKE " << mb->classobj->name() << '.' << mb->name << ':' << mb->type;
}

void
InvokeMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
