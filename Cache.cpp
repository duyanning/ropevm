#include "std.h"
#include "Cache.h"

#define USE_NEW

using namespace std;

#ifdef USE_NEW
void
Cache::write(Word* addr, Word value)
{
    access(addr);

    Hashtable::iterator i = m_hashtable.find(addr);
    if (i == m_hashtable.end()) {
        History* history = new History;
        history->push_back(Line(m_version, value));
        m_hashtable.insert(Hashtable::value_type(addr, history));
        return;
    }

    History* history = i->second;
    assert(history);
    assert(not history->empty());
    Line& back = history->back();
    if (back.version == m_version) {
        back.value = value;
        return;
    }

    assert(back.version < m_version);
    history->push_back(Line(m_version, value));
}

Word
Cache::read(Word* addr)
{
    access(addr);

    Hashtable::iterator i = m_hashtable.find(addr);
    if (i == m_hashtable.end()) {
        return *addr;
    }

    History* history = i->second;
    assert(history);
    assert(not history->empty());
    return history->back().value;
}

class IsVerLessEqual {
public:
    IsVerLessEqual(int ver)
        : m_ver(ver)
    {
    }

    template <class T>
    bool operator()(T& line)
    {
        if (line.version <= m_ver)
            return true;
        return false;
    }
private:
    int m_ver;
};

void
Cache::commit(int ver)
{
    access(0);
    assert(ver <= m_version);

    if (ver == m_version) {
        for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
            Word* addr = i->first;
            History* history = i->second;

            assert(history);
            assert(not history->empty());

            *addr = history->back().value; // write back
            delete history;
        }
        m_hashtable.clear();
        return;
    }

    for (Hashtable::iterator n = m_hashtable.begin(); n != m_hashtable.end(); ) {
        Hashtable::iterator i = n++;

        Word* addr = i->first;
        History* history = i->second;

        History::reverse_iterator here =
            find_if(history->rbegin(), history->rend(), IsVerLessEqual(ver));

        if (here != history->rend()) {
            *addr = here->value; // write back
            here->version = ver;

            History::iterator last = (here + 1).base();

            // History::iterator next = last + 1;
            // if (next != history->end()) {
            //     if (last->version == next->version)
            //         last = next;
            // }
            // else if (last->version == m_version) {
            //     last = history->end();
            // }

            history->erase(history->begin(), last);
        }

        if (history->empty()) {
            delete history;
            m_hashtable.erase(i);
        }

    }
}
#endif

void
Cache::snapshot()
{
    m_version++;
}

void
Cache::show(std::ostream& os, int id, bool integer)
{
    os << "#" << id << " {{{{----------------\n";
    os << "#" << id << " latest version: " << version()
       << "\t" << "size: " << m_hashtable.size()
       << "\t" << "this: " << this << endl;
    vector<Word*> addrs;
    for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
        Word* address = i->first;
        addrs.push_back(address);
    }
    std::sort(addrs.begin(), addrs.end());
    for (vector<Word*>::iterator i = addrs.begin(); i != addrs.end(); ++i) {
        Word* address = *i;
        os << "#" << id << " address: " << (void*)address << '\n';
        History* history = m_hashtable[address];
        os << "#" << id;
        for (History::iterator j = history->begin(); j != history->end(); ++j) {
            if (integer)
                os << '\t' << (int)j->value << '(' << j->version << ')';
            else
                os << '\t' << hex << j->value << dec << '(' << j->version << ')';
        }
        os << endl;
    }
    os << "#" << id << " ----------------}}}}\n";

}

void
Cache::clear()
{
    for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
        History* history = i->second;
        assert(history);
        assert(not history->empty());
        delete history;
    }
    m_hashtable.clear();
    m_version = 0;
}

void
Cache::clear(void* begin, void* end)
{
    for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end();) {
        Word* addr = i->first;
        if (begin <= addr && addr < end) {
            History* history = i->second;
            assert(history);
            assert(not history->empty());
            delete history;
            m_hashtable.erase(i++);
        }
        else {
            ++i;
        }
    }
}


Cache::Cache()
:
    m_version(0)
{
}

Cache::~Cache()
{
    for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
        History* history = i->second;
        delete history;
    }
}

void
Cache::reset()
{
    clear();
    m_version = 0;
}

void
Cache::scan()
{
}

void
Cache::access(Word* addr)
{
    int x = 0;
    x++;
}

void
show_cache(ostream& os, int id, Cache& cache, bool integer)
{
    cache.show(os, id, integer);
}

//--------------------------------------------
// old
#ifndef USE_NEW
// void
// Cache::write(Word* addr, Word value)
// {
//     access(addr);

//     History* history = m_hashtable[addr];
//     if (history == 0) {
//         history = new History;
//         m_hashtable[addr] = history;
//         //history->push_back(Line(m_version, value));
//         if (m_version == 0) {
//             history->push_back(Line(0, *addr));
//         }
//         else if (m_version > 0) {
//             history->push_back(Line(m_version - 1, *addr));
//         }
//         else {
//             assert(false);
//         }
//     }


//     Line& back = history->back();
//     if (back.version < m_version) {
//         back.version = m_version - 1;
//         history->push_back(Line(m_version, value));
//     }
//     else if (back.version == m_version) {
//         if (back.value == value) {
//         }
//         else {
//             back.value = value;
//         }
//     }
//     else {
//         assert(false);
//     }
// }

// Word
// Cache::read(Word* addr)
// {
//     access(addr);

//     History* history = m_hashtable[addr];
//     if (history == 0) {
//         history = new History;
//         m_hashtable[addr] = history;
//         //history->push_back(Line(m_version, *addr));
//         if (m_version == 0) {
//             history->push_back(Line(0, *addr));
//         }
//         else if (m_version > 0) {
//             history->push_back(Line(m_version - 1, *addr));
//         }
//         else {
//             assert(false);
//         }
//     }

//     Line& back = history->back();
//     if (back.version < m_version) {
//         //back.version = m_version;
//         back.version = m_version - 1;
//         // should add new value with ver == m_version? see also commit
//     }
//     else if (back.version == m_version) {
//     }
//     else {
//         assert(false);
//     }

//     return history->back().value;
// }

// void
// Cache::commit(int ver)
// {
//     assert(ver <= m_version);

//     for (Hashtable::iterator n = m_hashtable.begin(); n != m_hashtable.end(); ) {
//         Hashtable::iterator i = n++;

//         Word* address = i->first;
//         History* history = i->second;
//         while (not history->empty() && history->front().version < ver) {
//             history->pop_front();
//         }

//         if (not history->empty()) {
//             *address = history->front().value;
//             if (history->front().version == ver)
//                 history->pop_front();
//         }

//         if (history->empty()) {
//             delete history;
//             m_hashtable.erase(i);
//         }

//     }
// }
#endif
