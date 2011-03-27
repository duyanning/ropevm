#ifndef UNCERTAINMODE_H
#define UNCERTAINMODE_H

#include "Mode.h"

class Message;

class UncertainMode : public Mode {
public:
    UncertainMode(const char* name);
    void process_certain_message(Message* messge);
};


#endif // UNCERTAINMODE_H
