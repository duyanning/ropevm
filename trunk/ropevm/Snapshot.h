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
    CodePntr pc;
    Frame* frame;
    uintptr_t* sp;
};

std::ostream& operator<<(std::ostream& os, const Snapshot* snapshot);

#endif // SNAPSHOT_H
