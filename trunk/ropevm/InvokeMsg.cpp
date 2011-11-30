#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

InvokeMsg::InvokeMsg(SpmtThread* source_st, SpmtThread* target_st,
                     Object* target_object, MethodBlock* mb, uintptr_t* args,
                     CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                     bool is_top)
:
    RoundTripMsg(MsgType::INVOKE, source_st, target_st, target_object)
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


bool
InvokeMsg::is_irrevocable()
{
    return mb->is_rope_irrevocable();
}


bool
InvokeMsg::equal(Message* msg)
{
    if (m_type != msg->get_type())
        return false;

    InvokeMsg* m = static_cast<InvokeMsg*>(msg);
// + 目标对象（即方法所属的对象）
// + 方法的名字
// + 消息的参数值
// + 源线程（即发起方法的线程。）
// + 源线程的(PC, FP, SP)

    if (m_target_st != m->m_target_st)
        return false;
    if (mb != m->mb)
        return false;
    if (parameters != m->parameters)
        return false;
    if (m_source_st != m->m_source_st)
        return false;
    if (caller_pc != m->caller_pc)
        return false;
    if (caller_frame != m->caller_frame)
        return false;
    if (caller_sp != m->caller_sp)
        return false;

    return true;
}


void
InvokeMsg::show(ostream& os) const
{
    os << "INVOKE" << (m_is_top ? "(top) " : " ") << mb->classobj->name() << '.' << mb->name << ':' << mb->type;
}

void
InvokeMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
