#ifndef SPECMSGQUEUE_H
#define SPECMSGQUEUE_H

class Message;

class SpecMsgQueue {
public:
    void add(Message* msg);
    void process_next_msg();
    void process(Message* msg);
    void verify(Message* msg);
    void revoke(Message* msg);
    Message* current_msg();
};


#endif // SPECMSGQUEUE_H
