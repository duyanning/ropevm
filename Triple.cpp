#include "std.h"
#include "rope.h"
#include "Triple.h"

using namespace std;

Triple::Triple(CodePntr pc_1, Frame* frame_1, uintptr_t* sp_1)
:
    pc(pc_1),
    frame(frame_1),
    sp(sp_1)
{
}


ostream&
operator<<(ostream& os, const Triple& triple)
{
    if (triple.frame) {
        os << " at " << triple.pc - (CodePntr)triple.frame->mb->code
           << " of " << triple.frame;
    }
    else {
        os << " at none";
    }

    return os;
}
