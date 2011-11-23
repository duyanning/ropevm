#ifndef STATEBUFFER_H
#define STATEBUFFER_H

//typedef uint8_t Word;
typedef uint32_t Word;

class StateBuffer {
public:
    StateBuffer();
    ~StateBuffer();
    Word read(Word* addr);
    void write(Word* addr, Word value);
    void freeze();
    void commit(int ver);
    void discard(int ver);
    void clear(void* begin, void* end);
    void reset();
    int current_ver();
    void scan();                // gc
    // log
    void show(std::ostream& os = std::cout, int id = 0, bool integer = false);
private:
    int size() { return m_hashtable.size(); }
    void access(Word* addr);    // for debug

    struct Node {
        Node(int ver, Word val)
            : start_ver(ver), value(val)
        {
        }
        int start_ver;
        Word value;
    };

    typedef std::deque<Node> History;

    typedef std::tr1::unordered_map<Word*, History*> Hashtable;

    Hashtable m_hashtable;

    int m_oldest_ver;
    int m_current_ver;
    static const int m_initial_ver = 0;
};

inline
int
StateBuffer::current_ver()
{
    return m_current_ver;
}

void
show_buffer(std::ostream& os, int id, StateBuffer& buffer, bool integer = false);

#endif // STATEBUFFER_H
