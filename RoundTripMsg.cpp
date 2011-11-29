#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

RoundTripMsg::RoundTripMsg(MsgType type,
                           SpmtThread* source_st,
                           SpmtThread* target_st,
                           Object* target_object)
    :
    Message(type, target_st),
    m_source_st(source_st),
    m_object(target_object)
{
}
