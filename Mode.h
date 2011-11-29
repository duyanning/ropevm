#ifndef MODE_H
#define MODE_H

#include "align.h"

class Message;
class Effect;

class Mode {
public:
    Mode(const char* name);
    const char* get_name();
    virtual const char* tag() = 0;
    virtual void step() = 0;
    void fetch_and_interpret_an_instruction();

    virtual void do_spec_barrier() = 0;

    virtual void before_alloc_object();
    virtual void after_alloc_object(Object* obj);

    virtual void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs, DummyFrame* dummy) = 0;
    virtual void do_invoke_method(Object* objectref, MethodBlock* new_mb) = 0;
    virtual void do_method_return(int len) = 0;
    virtual void before_signal_exception(Class *exception_class) = 0;

    virtual void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false) = 0;
    virtual void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false) = 0;

    virtual void do_array_load(Object* array, int index, int type_size) = 0;
    virtual void do_array_store(Object* array, int index, int type_size) = 0;

    virtual void process_msg(Message* msg);
    virtual void send_msg(Message* msg);

    // 参数用无模式方式读取
    void invoke_impl(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                     SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                     bool is_top);


    void load_from_array(uintptr_t* sp, Object* array, int index, int type_size);
    void store_to_array(uintptr_t* sp, Object* array, int index, int type_size);
    void load_from_array_to_c(uintptr_t* sp, Object* array, int index, int type_size);
    void store_to_array_from_c(uintptr_t* sp, Object* array, int index, int type_size);


    virtual Frame* push_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                              SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                              bool is_top) = 0;

    virtual void pop_frame(Frame* frame) = 0;


    void set_st(SpmtThread* st);

    virtual uint32_t mode_read(uint32_t* addr) = 0;
    virtual void mode_write(uint32_t* addr, uint32_t value) = 0;

    template <class T> T read(T* addr);
    template <class T, class U> void write(T* addr, U value);


public:
    //-------------------
    CodePntr pc;
    Frame* frame;
    uintptr_t* sp;
    //------------------


protected:
    bool intercept_vm_backdoor(Object* objectref, MethodBlock* mb);
    void vmlog(MethodBlock* mb);
    void preload(MethodBlock* mb);
    bool vm_math(MethodBlock* mb);
    SpmtThread* m_st;
private:
    const char* m_name;
};

// 因为mode_read和mode_write都是以4字节为单位读写。所以对于读取1或2字节
// 的数值，需要把包含这1或2个字节的4字节整个都读出来，然后再从其中找到
// 我们需要的那1或2个字节。对于写1或2个字节，需要先把包含这1或2个字节的
// 4字节数据整个读出来，然后只改动其中你要写的那1或2个字节，其余部分保
// 持不变，然后再把这改动后的4个字节写回去。aligned_base用来计算你要读
// 写的1或2个字节以4字节对齐的起始地址。addr_diff计算两个地址相差几个字
// 节。addr_add将地址加上几个字节的偏移。


// 读出1字节，无论如何这1字节都包含在作为整体读出的4个字节之中。然而，
// 读出2字节，可能你读出的4字节只包含了这2字节中的1字节，你还要再读一个
// 4字节，从中取出另一个1字节，然后将这两个1字节拼接起来。

// to factor: T的大小对模板进行特化，以避免运行时判断类型大小。
template <class T>
T
Mode::read(T* addr)
{
    if (sizeof (T) >= sizeof (uint32_t)) {
        assert(sizeof (T) == 4 || sizeof (T) == 8);
        T v;
        uint32_t* dst = reinterpret_cast<uint32_t*>(&v);
        uint32_t* src = reinterpret_cast<uint32_t*>(addr);
        for (size_t i = 0; i < sizeof (T) / sizeof (uint32_t); i++) {
            //*dst++ = *src++;
            *dst++ = mode_read(src++);
        }
        return v;
    }
    else if (sizeof (T) == 1) {
        void* base = aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *(uint32_t*)base;
        uint32_t v = mode_read((uint32_t*)base);
        v >>= addr_diff(addr, base) * 8;
        return (T)v;
    }
    else if (sizeof (T) == 2) {
        void* base = aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *(uint32_t*)base;
        uint32_t v = mode_read((uint32_t*)base);
        v >>= addr_diff(addr, base) * 8;

        ptrdiff_t offset = addr_diff(addr, base);
        if (offset == 3) {
            // 读出下一个4字节
            //uint32_t v2 = *((uint32_t*)base + 1);
            uint32_t v2 = mode_read((uint32_t*)base+1);
            v2 <<= 8;
            v2 &= 0xffff;

            v |= v2;

        }

        return (T)v;
    }
    else {
        assert(false);
    }
}

// 对于写2字节，先读出4字节，如果要写的2字节完全包含在这4字节中，就改写这4字节中的相应部分，然后将4字节写回。如果要写的2字节中的1字节在下一个4字节中，那么再读出下一个4字节，改写其中的1字节，写回。
template <class T, class U>
void
Mode::write(T* addr, U value)
{
    //assert(sizeof (T) <= sizeof (U));

    if (sizeof (T) >= sizeof (uint32_t)) {
        assert(sizeof (T) == 4 || sizeof (T) == 8);
        T v = value;
        uint32_t* dst = reinterpret_cast<uint32_t*>(addr);
        uint32_t* src = reinterpret_cast<uint32_t*>(&v);
        for (size_t i = 0; i < sizeof (T) / sizeof (uint32_t); i++) {
            //*dst++ = *src++;
            mode_write(dst++, *src++);
        }
    }
    else if (sizeof (T) == 1) {
        uint32_t* base = (uint32_t*)aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *base;
        uint32_t v = mode_read(base);

        void* p = addr_add(&v, addr_diff(addr, base));
        *(T*)p = value;         // 改动4字节中的2字节（或其中1的字节）
        //*base = v;              // 写回
        mode_write(base, v);

    }
    else if (sizeof (T) == 2) {
        uint16_t val = value;

        uint32_t* base = (uint32_t*)aligned_base(addr, sizeof (uint32_t));
        //uint32_t v = *base;     // 读出4字节
        uint32_t v = mode_read(base);

        void* p = addr_add(&v, addr_diff(addr, base)); // 要写的2字节在4字节中的位置
        *(T*)p = val;           // 改动4字节中的2字节（或其中1的字节）
        //*base = v;              // 写回
        mode_write(base, v);

        ptrdiff_t offset = addr_diff(addr, base);
        if (offset == 3) {
            //uint32_t v2 = *(base + 1); // 读出另一个4字节
            uint32_t v2 = mode_read(base + 1);
            v2 &= 0xffffff00;
            val >>= 8;
            v2 |= val;
            //*(base+1) = v2;     // 写回
            mode_write(base+1, v2);
        }
    }
    else {
        assert(false);
    }
}



void
g_load_from_stable_array_to_c(uintptr_t* sp, Object* array, int index, int type_size);




#define throw_exception                         \
    return m_st->do_throw_exception();


void show_triple(std::ostream& os, int id,
                 Frame* frame, uintptr_t* sp, CodePntr pc, Object* user,
                 bool more = false);

void log_invoke_return(MiniLogger& logger, bool is_invoke, int id, const char* tag,
                       Object* caller, MethodBlock* caller_mb,
                       Object* callee, MethodBlock* callee_mb);
void
show_invoke_return(std::ostream& os, bool is_invoke, int id, const char* tag,
                   Object* caller, MethodBlock* caller_mb,
                   Object* callee, MethodBlock* callee_mb);


#endif // MODE_H
