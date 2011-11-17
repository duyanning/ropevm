#ifndef TRIPLE_H
#define TRIPLE_H


struct Triple {
    Triple(CodePntr pc_1, Frame* frame_1, uintptr_t* sp_1);

    CodePntr pc;
    Frame* frame;
    uintptr_t* sp;
};


std::ostream& operator<<(std::ostream& os, const Triple& triple);


#endif // TRIPLE_H
