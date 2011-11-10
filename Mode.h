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

    virtual void before_alloc_object();
    virtual void after_alloc_object(Object* obj);
    //SpmtThread* assign_spmt_thread_for(Object* obj);

    virtual void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs) = 0;
    virtual void do_invoke_method(Object* objectref, MethodBlock* new_mb) = 0;
    virtual void do_method_return(int len) = 0;
    virtual void do_throw_exception() = 0;
    virtual void before_signal_exception(Class *exception_class) = 0;

    virtual void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false) = 0;
    virtual void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false) = 0;

    virtual void do_array_load(Object* array, int index, int type_size) = 0;
    virtual void do_array_store(Object* array, int index, int type_size) = 0;

    virtual void process_msg(Message* msg);
    virtual void send_msg(Message* msg);

    // 参数用无模式方式读取
    virtual void invoke_impl(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                             SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp) =0;


    void load_from_array(uintptr_t* sp, Object* array, int index, int type_size);
    void store_to_array(uintptr_t* sp, Object* array, int index, int type_size);
    void load_from_array_to_c(uintptr_t* sp, Object* array, int index, int type_size);
    void store_to_array_from_c(uintptr_t* sp, Object* array, int index, int type_size);


    virtual Frame* create_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                                SpmtThread* caller,
                                CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp) = 0;

    virtual void destroy_frame(Frame* frame) = 0;


    void set_spmt_thread(SpmtThread* spmt_thread);

    virtual uint32_t mode_read(uint32_t* addr) = 0;
    virtual void mode_write(uint32_t* addr, uint32_t value) = 0;

    template <class T> T read(T* addr);
    template <class T, class U> void write(T* addr, U value);

    bool is_certain_mode();
    bool is_speculative_mode();
    bool is_rvp_mode();

public:
    //-------------------
    CodePntr pc;
    Frame* frame;
    uintptr_t* sp;
    //------------------
    Object* exception;          // state?

protected:
    bool intercept_vm_backdoor(Object* objectref, MethodBlock* mb);
    void vmlog(MethodBlock* mb);
    void preload(MethodBlock* mb);
    bool vm_math(MethodBlock* mb);
    SpmtThread* m_spmt_thread;
private:
    const char* m_name;

    // for logging
private:
    virtual void log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb, Object* callee, MethodBlock* callee_mb) = 0;
};

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
            *dst++ = mode_read(src++);
        }
        return v;
    }
    else {
        assert(sizeof (T) == 1 || sizeof (T) == 2);
        void* base = aligned_base(addr, sizeof (uint32_t));
        uint32_t v = mode_read((uint32_t*)base);
        v >>= addr_diff(addr, base) * 8;
        assert((T)v == *addr);  // only in certain mode
        return (T)v;
    }
}

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
            mode_write(dst++, *src++);
        }
    }
    else {
        assert(sizeof (T) == 1 || sizeof (T) == 2);
        uint32_t* base = (uint32_t*)aligned_base(addr, sizeof (uint32_t));
        uint32_t v = mode_read(base);
        //std::cerr << std::hex << v << " ";
        //std::cerr << addr_diff(addr, base) << " " << value << " ";
        void* p = addr_add(&v, addr_diff(addr, base));
        *(T*)p = value;
        //std::cerr << v << "\n" << std::dec;
        mode_write(base, v);
    }
}



void
g_load_from_stable_array_to_c(uintptr_t* sp, Object* array, int index, int type_size);



#define invoke_method                                       \
    return do_invoke_method((Object*)read(arg1), new_mb);



#define method_return                           \
    return do_method_return(len);




#define throw_exception                         \
    return do_throw_exception();


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
