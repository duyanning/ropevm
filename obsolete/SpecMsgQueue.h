#ifndef SPECMSGQUEUE_H
#define SPECMSGQUEUE_H

class Message;

class SpecMsgQueue {
public:
    void add(Message* msg);
    Message* begin_process_next_msg();
    void begin_process_msg(Message* msg);
    void verify(Message* msg);
    void revoke(Message* msg);
    Message* current_msg();
};


#endif // SPECMSGQUEUE_H
