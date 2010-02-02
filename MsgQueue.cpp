#include "std.h"
#include "MsgQueue.h"

MsgQueue::MsgQueue()
{
    pthread_mutex_init(&m_mutex, 0);
    pthread_cond_init(&m_cv, 0);
}

MsgQueue::~MsgQueue()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cv);
}

Msg*
MsgQueue::getMsg()
{
    pthread_mutex_lock(&m_mutex);

    Msg* msg = m_queue.front();
    m_queue.pop();

    pthread_mutex_unlock(&m_mutex);

    return msg;
}

void
MsgQueue::addMsg(Msg* msg)
{
    pthread_mutex_lock(&m_mutex);

    m_queue.push(msg);
    pthread_cond_signal(&m_cv);

    pthread_mutex_unlock(&m_mutex);
}
