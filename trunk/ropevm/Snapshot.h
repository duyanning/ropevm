#ifndef SNAPSHOT_H
#define SNAPSHOT_H

class Object;
class Frame;
typedef unsigned char* CodePntr;
class Message;

class Snapshot {
public:
    Snapshot();
    ~Snapshot();
    int version;

    Object* user;
    Frame* frame;
    CodePntr pc;
    uintptr_t* sp;

    int c;                      // just for debug

    // when spec_msg is verified OK, can commit to this snapshot
    Message* spec_msg;          // just for debug
};

void
show_snapshot(std::ostream& os, int id , Snapshot* snapshot);

#endif // SNAPSHOT_H
