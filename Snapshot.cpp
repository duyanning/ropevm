#include "std.h"
#include "rope.h"
#include "Snapshot.h"
#include "Mode.h"
#include "Message.h"
#include "Triple.h"

using namespace std;


Snapshot::Snapshot()
:
    version(-1),
    pc(0),
    frame(0),
    sp(0)
{
}

Snapshot::~Snapshot()
{
}


void
show_snapshot(std::ostream& os, const Snapshot* snapshot)
{
    os << "(" << snapshot->version << ")"
       << Triple(snapshot->pc, snapshot->frame, snapshot->sp);
}


ostream& operator<<(ostream& os, const Snapshot* snapshot)
{
    show_snapshot(os, snapshot);
    return os;
}
