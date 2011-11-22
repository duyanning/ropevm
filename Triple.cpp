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
        os << " without frame";
    }

    return os;
}

// void
// show_triple(std::ostream& os, int id, Frame* frame, uintptr_t* sp, CodePntr pc, Object* user,
//             bool more)
// {
//     if (frame) {
//         // if (frame->is_alive())
//         //     os << "alive" << endl;
//         // if (frame->is_dead())
//         //     os << "dead" << endl;
//         // if (frame->is_bad())
//         //     os << "bad" << endl;
//     }

//     os << "#" << id << " frame = " << frame;
//     if (more && frame && frame->mb) {
//         os << " " << frame->mb << "\n";
//         os << "#" << id << " lvars = " << frame->lvars << "\n";
//         os << "#" << id << " ostack_base = " << frame->ostack_base << "\n";
//     }
//     os << "\n";



//     os << "#" << id << " sp = " << (void*)sp;
//     if (more && frame) {
//         os << " " << sp - frame->ostack_base;
//     }
//     os << "\n";

//     os << "#" << id << " pc = " << (void*)pc;
//     if (more && frame && frame->mb) {
//         os << " " << pc - (CodePntr)frame->mb->code;
//     }
//     os << endl;

//     os << "#" << id << " user = " << user;
//     os << endl;
// }
