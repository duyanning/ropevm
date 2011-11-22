#include "std.h"
#include "StateBuffer.h"


using namespace std;


void
StateBuffer::write(Word* addr, Word value)
{
    access(addr);

    // 先看缓存中是否有该地址
    auto i = m_hashtable.find(addr);

    // if 没有，为其产生版本历史
    // 并记录当前版本下的该数值
    if (i == m_hashtable.end()) {
        History* history = new History;
        history->push_back(Node(m_latest_ver, value));
        m_hashtable.insert(Hashtable::value_type(addr, history));
        return;
    }

    // if 有，看最后的版本是否当前版本，若是，则改变其数值
    History* history = i->second;
    assert(history);
    assert(not history->empty());
    Node& back = history->back();
    if (back.start_ver == m_latest_ver) {
        back.value = value;
        return;
    }

    // 在历史中添加新的版本
    assert(back.start_ver < m_latest_ver);
    history->push_back(Node(m_latest_ver, value));
}


// 读出最新版本下的数据
Word
StateBuffer::read(Word* addr)
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

// class IsVerLessEqual {
// public:
//     IsVerLessEqual(int ver)
//         : m_ver(ver)
//     {
//     }

//     template <class T>
//     bool operator()(T& node)
//     {
//         if (node.start_ver <= m_ver)
//             return true;
//         return false;
//     }
// private:
//     int m_ver;
// };


// 提交指定版本到稳定内存
/*
  buffer中尚无如此高的版本。此为错误。
  buffer中有此版本。常见情况。
  buffer中已无如此低的版本。
 */
void
StateBuffer::commit(int ver)
{
    access(0);
    assert(ver <= m_latest_ver);   // buffer中尚无如此高的版本。此为错误。

    m_oldest_ver = ver;

    // 若i等于buf的当前版本，提交所有历史的最后一个节点。
    // 然后扔掉所有历史。
    if (ver == m_latest_ver) {
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

    // 否则如下处理
    // 对各个地址：
    for (auto n = m_hashtable.begin(); n != m_hashtable.end(); ) {
        Hashtable::iterator i = n++;

        Word* addr = i->first;
        History* history = i->second;

        // 在历史中找到版本i+1所在节点（因为版本i提交后，版本i及之前版本要从buf从清理出去）
        // （怎么找到某版本x所在节点？ 答：从后向前，倒着找到第一个起始版本小于或等于x的节点）
        History::reverse_iterator reverse_iter_iplus1 =
            find_if(history->rbegin(), history->rend(),
                    [ver](Node& node){ return node.start_ver <= ver+1; });

        // 本节点之前的节点全部扔掉。
        if (reverse_iter_iplus1 != history->rend()) {

            // 若版本i在版本i+1的上一个节点（本节点起始版本必为i+1）。提交上一个节点。（可能没有上一个节点）
            // 若版本i与版本i+1位于同一节点，将该节点起始版本改为i+1。提交本节点。
            if (reverse_iter_iplus1->start_ver == ver + 1) {
                auto reverse_iter_i = reverse_iter_iplus1 + 1;
                if (reverse_iter_i != history->rend()) {
                    *addr = reverse_iter_i->value;
                }
            }
            else {
                *addr = reverse_iter_iplus1->value;
                reverse_iter_iplus1->start_ver = ver + 1;
            }


            History::iterator last = (reverse_iter_iplus1 + 1).base();


            history->erase(history->begin(), last);
        }

        assert(not history->empty());
        // if (history->empty()) {
        //     delete history;
        //     m_hashtable.erase(i);
        // }

    }
}

void
StateBuffer::discard(int ver)
{
    assert(ver >= m_oldest_ver); // m_oldest_ver在提交后增加，即消息得到确认后增加，该消息不可能再被丢弃。
    //assert(ver <= m_latest_ver); 这个assert有误，因为有可能先丢弃的消息对应buffer中的低版本（从而拉低m_latest_ver至低版本），后丢弃的消息对应buffer中高版本。

    m_latest_ver = ver;

    // 对各个地址：
    for (auto n = m_hashtable.begin(); n != m_hashtable.end(); ) {
        Hashtable::iterator i = n++;

        //Word* addr = i->first;
        History* history = i->second;

        // 在历史中找到版本i-1所在节点（因为版本i提交后，版本i及之前版本要从buf从清理出去）
        // （怎么找到某版本x所在节点？ 答：从后向前，倒着找到第一个起始版本小于或等于x的节点）
        History::reverse_iterator reverse_iter_iminus1 =
            find_if(history->rbegin(), history->rend(), [ver](Node& node){ return node.start_ver <= ver-1; });

        // 将其后节点全部扔掉。
        auto first = reverse_iter_iminus1.base();
        history->erase(first, history->end());

        // 若历史为空，删除历史。删除哈希表项。
        if (history->empty()) {
            delete history;
            m_hashtable.erase(i);
        }

    }

    // 将buf的当前版本改为i-1。
    m_latest_ver = ver-1;
    if (m_latest_ver < m_oldest_ver) {
        m_latest_ver = m_oldest_ver;
    }

}


void
StateBuffer::freeze()
{
    m_latest_ver++;
}

void
StateBuffer::show(std::ostream& os, int id, bool integer)
{
    os << "#" << id << " /----------\\\n";
    os << "#" << id << " latest version: " << latest_ver()
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
    os << "#" << id << " \\----------/\n";

}


void
StateBuffer::clear(void* begin, void* end)
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


StateBuffer::StateBuffer()
:
    m_hashtable(),
    m_oldest_ver(m_initial_ver),
    m_latest_ver(m_initial_ver)

{
}

StateBuffer::~StateBuffer()
{
    for (Hashtable::iterator i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
        History* history = i->second;
        delete history;
    }
}

void
StateBuffer::reset()
{
    for (auto i = m_hashtable.begin(); i != m_hashtable.end(); ++i) {
        History* history = i->second;
        assert(history);
        assert(not history->empty());
        delete history;
    }
    m_hashtable.clear();

    m_latest_ver = m_initial_ver;
    m_oldest_ver = m_initial_ver;
}

void
StateBuffer::scan()
{
}

void
StateBuffer::access(Word* addr)
{
    int x = 0;
    x++;
}

void
show_buffer(ostream& os, int id, StateBuffer& buffer, bool integer)
{
    buffer.show(os, id, integer);
}

