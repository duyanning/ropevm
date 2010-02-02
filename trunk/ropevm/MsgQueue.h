#ifndef MSGQUEUE_H
#define MSGQUEUE_H

class Msg;

class MsgQueue {
 public:
    MsgQueue();
    ~MsgQueue();
    Msg* getMsg();
    void addMsg(Msg* msg);
 private:
    std::queue<Msg*> m_queue;
    pthread_cond_t m_cv;
    pthread_mutex_t m_mutex;
};


#endif // MSGQUEUE_H
