#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;

RoundTripMsg::RoundTripMsg(MsgType type,
                           SpmtThread* source_spmt_thread,
                           SpmtThread* target_spmt_thread,
                           Object* target_object)
    :
    Message(type, target_spmt_thread),
    m_source_spmt_thread(source_spmt_thread),
    m_object(target_object)
{
}
