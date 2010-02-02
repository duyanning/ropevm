#ifndef CACHE_H
#define CACHE_H

//typedef uint8_t Word;
typedef uint32_t Word;

class Cache {
public:
    Cache();
    ~Cache();
    Word read(Word* addr);
    void write(Word* addr, Word value);
    void snapshot();
    void commit(int ver);
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

    struct Line {
        Line(int ver, Word val)
            : version(ver), value(val)
        {
        }
        int version;
        Word value;
    };

    typedef std::deque<Line> History;

    typedef std::tr1::unordered_map<Word*, History*> Hashtable;

    Hashtable m_hashtable;
    int m_version;
};

inline
int
Cache::version()
{
    return m_version;
}

void
show_cache(std::ostream& os, int id, Cache& cache, bool integer = false);

#endif // CACHE_H
