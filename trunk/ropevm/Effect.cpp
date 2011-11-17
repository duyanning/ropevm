#include "std.h"
#include "rope.h"
#include "Effect.h"


Effect::Effect()
    :
    msg_sent(nullptr),
    snapshot(nullptr)
{
}


void
Effect:: add_to_C(Frame* frame)
{
    frame->iter_in_C_of_effect = C.insert(C.end(), frame);
    frame->is_iter_in_current_effect = true;
}


void
Effect::remove_from_C(Frame* frame)
{
    // 如果iter_in_C_of_effect所在的effect已不是当前effect，那就不能删
    // 除。其所在effect已经被提交。（其所在effect绝不可能被丢弃，因为那
    // 样的话，栈桢在丢弃的时候就已被销毁了，根本等不到返回时销毁）
    if (not frame->is_iter_in_current_effect)
        return;

    C.erase(frame->iter_in_C_of_effect);
}


void
Effect::add_to_R(Frame* frame)
{
    R.push_back(frame);
}
