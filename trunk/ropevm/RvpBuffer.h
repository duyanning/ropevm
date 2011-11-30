#ifndef RVPBUFFER_H
#define RVPBUFFER_H

typedef uint32_t Word;

class RvpBuffer {
 public:
    void write(Word* addr, Word value);
    Word read(Word* addr);
    void clear() { m_hashtable.clear(); }
    void clear(void* begin, void* end);
 private:
    typedef std::unordered_map<Word*, Word> Hashtable;
    Hashtable m_hashtable;
};


#endif // RVPBUFFER_H
