#ifndef UNCERTAINMODE_H
#define UNCERTAINMODE_H

#include "Mode.h"

class Message;

class UncertainMode : public Mode {
public:
    UncertainMode(const char* name);
    virtual void step();
};


#endif // UNCERTAINMODE_H
