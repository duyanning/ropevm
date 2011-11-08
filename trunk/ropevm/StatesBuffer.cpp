#include "std.h"
#include "StatesBuffer.h"

#define USE_NEW

using namespace std;

#ifdef USE_NEW
void
StatesBuffer::write(Word* addr, Word value)
{
    access(addr);

    // 先看缓存中是否有该地址
    auto i = m_hashtable.find(addr);

    // if 没有，为其产生版本历史
    // 并记录当前版本下的该数值
    if (i == m_hashtable.end()) {
        History* history = new History;
        history->push_back(Node(m_version, value));
        m_hashtable.insert(Hashtable::value_type(addr, history));
        return;
    }

    // if 有，看最后的版本是否当前版本，若是，则改变其数值
    History* history = i->second;
    assert(history);
    assert(not history->empty());
    Node& back = history->back();
    if (back.start_ver == m_version) {
        back.value = value;
        return;
    }

    // 在历史中添加新的版本
    assert(back.start_ver < m_version);
    history->push_back(Node(m_version, value));
}


// 读出最新版本下的数据
Word
StatesBuffer::read(Word* addr)
{
    access(addr);

    // if 该地址不在buffer中，就从稳定内存取数。
    Hashtable::iterator i = m_hashtable.find(addr);
    if (i == m_hashtable.end()) {
        return *addr;
    }

    // if 该地址在buffer，返回最新版本的数值
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
    bool operator()(T& node)
    {
        if (node.start_ver <= m_ver)
            return true;
        return false;
    }
private:
    int m_ver;
};


// 提交指定版本到稳定内存
/*
  buffer中尚无如此高的版本。此为错误。
  buffer中有此版本。常见情况。
  buffer中已无如此低的版本。
 */
void
StatesBuffer::commit(int ver)
{
    access(0);
    assert(ver <= m_version);   // buffer中尚无如此高的版本。此为错误。

    if (ver == m_version) {
        for (auto i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
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

    for (auto n = m_hashtable.begin(); n != m_hashtable.end(); ) {
        Hashtable::iterator i = n++;

        Word* addr = i->first;
        History* history = i->second;

        History::reverse_iterator here =
            find_if(history->rbegin(), history->rend(), IsVerLessEqual(ver));

        if (here != history->rend()) {
            *addr = here->value; // write back
            here->start_ver = ver;

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
StatesBuffer::freeze()
{
    m_version++;
}

void
StatesBuffer::show(std::ostream& os, int id, bool integer)
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
                os << '\t' << (int)j->value << '(' << j->start_ver << ')';
            else
                os << '\t' << hex << j->value << dec << '(' << j->start_ver << ')';
        }
        os << endl;
    }
    os << "#" << id << " ----------------}}}}\n";

}

void
StatesBuffer::clear()
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
StatesBuffer::clear(void* begin, void* end)
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


StatesBuffer::StatesBuffer()
:
    m_version(0)
{
}

StatesBuffer::~StatesBuffer()
{
    for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
        History* history = i->second;
        delete history;
    }
}

void
StatesBuffer::reset()
{
    clear();
    m_version = 0;
}

void
StatesBuffer::scan()
{
}

void
StatesBuffer::access(Word* addr)
{
    int x = 0;
    x++;
}

void
show_buffer(ostream& os, int id, StatesBuffer& buffer, bool integer)
{
    buffer.show(os, id, integer);
}

//--------------------------------------------
// old
#ifndef USE_NEW
// void
// StatesBuffer::write(Word* addr, Word value)
// {
//     access(addr);

//     History* history = m_hashtable[addr];
//     if (history == 0) {
//         history = new History;
//         m_hashtable[addr] = history;
//         //history->push_back(Node(m_version, value));
//         if (m_version == 0) {
//             history->push_back(Node(0, *addr));
//         }
//         else if (m_version > 0) {
//             history->push_back(Node(m_version - 1, *addr));
//         }
//         else {
//             assert(false);
//         }
//     }


//     Node& back = history->back();
//     if (back.version < m_version) {
//         back.version = m_version - 1;
//         history->push_back(Node(m_version, value));
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
// StatesBuffer::read(Word* addr)
// {
//     access(addr);

//     History* history = m_hashtable[addr];
//     if (history == 0) {
//         history = new History;
//         m_hashtable[addr] = history;
//         //history->push_back(Node(m_version, *addr));
//         if (m_version == 0) {
//             history->push_back(Node(0, *addr));
//         }
//         else if (m_version > 0) {
//             history->push_back(Node(m_version - 1, *addr));
//         }
//         else {
//             assert(false);
//         }
//     }

//     Node& back = history->back();
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
// StatesBuffer::commit(int ver)
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
