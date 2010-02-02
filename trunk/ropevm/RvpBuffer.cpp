#include "std.h"
#include "RvpBuffer.h"

using namespace std;

void
RvpBuffer::write(Word* addr, Word value)
{
    m_hashtable[addr] = value;
}

Word
RvpBuffer::read(Word* addr)
{
    Hashtable::iterator i = m_hashtable.find(addr);
    if (i == m_hashtable.end())
        return *addr;

    return i->second;
}

void
RvpBuffer::clear(void* begin, void* end)
{
    //MINILOG0("clear: [" << begin << ", " << end << ")");
    for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end();) {
        Word* addr = i->first;
        if (begin <= addr && addr < end) {
//             History* history = i->second;
//             delete history;
            m_hashtable.erase(i++);
        }
        else {
            ++i;
        }
    }
}
