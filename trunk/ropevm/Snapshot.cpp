#include "std.h"
#include "rope.h"
#include "Snapshot.h"
#include "Mode.h"
#include "Message.h"

int snapshot_count = 0; // just for debug

Snapshot::Snapshot()
{
    snapshot_count++;
    //{{{ just for debug
    c = snapshot_count;
    if (c == 34) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    //{{{ just for debug
    spec_msg = 0;
    //}}} just for debug

}

Snapshot::~Snapshot()
{
    user = 0;
    frame = 0;
    sp = 0;
    pc = 0;
    spec_msg = 0;
}


void
show_snapshot(std::ostream& os, int id , Snapshot* snapshot)
{
    show_triple(os, id , snapshot->frame, snapshot->sp, snapshot->pc, snapshot->user, true);
    os << "#" << id << " spec msg = " << *snapshot->spec_msg;
    os << "\n";
}
