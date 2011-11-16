#include "std.h"
#include "rope.h"

using namespace std;

ostream&
operator<<(ostream& os, const MethodBlock* mb)
{
    os << (mb->is_static() ? "c" : "");
    os << mb->classobj->name() << '.' << mb->name;
    //<< ':' << mb->type;
    return os;
}


ostream&
operator<<(ostream& os, const FieldBlock& fb)
{
    os << (fb.is_static() ? "c" : "");
    os << fb.classobj->name() << '.' << fb.name;
        //<< ':' << fb.type;
    return os;
}


ostream&
operator<<(ostream& os, const Frame* f)
{
    os << f->mb << " (" << (void*)f << ")";

       // << " obj: " << f.object
       // << " prev: " << f.prev
       // << " prev_obj: " << f.prev->object
       // << " caller: " << f.caller
       // << " ["
       // << f.lvars << ", " << f.lvars + f.mb->max_locals
       // << ")"
       // << " ["
       // << f.ostack_base << ", " << f.ostack_base + f.mb->max_stack << ") "
       // << ")";

    return os;
}
