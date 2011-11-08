#ifndef STATESBUFFER_H
#define STATESBUFFER_H

//typedef uint8_t Word;
typedef uint32_t Word;

class StatesBuffer {
public:
    StatesBuffer();
    ~StatesBuffer();
    Word read(Word* addr);
    void write(Word* addr, Word value);
    void freeze();
    void commit(int ver);
    void discard(int ver);
    void clear(void* begin, void* end);
    void reset();
    int version();
    void scan();                // gc
    // log
    void show(std::ostream& os = std::cout, int id = 0, bool integer = false);
private:
    void clear();
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
    int m_version;
};

inline
int
StatesBuffer::version()
{
    return m_version;
}

void
show_buffer(std::ostream& os, int id, StatesBuffer& buffer, bool integer = false);

#endif // STATESBUFFER_H
