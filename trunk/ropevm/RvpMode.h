#ifndef RVPMODE_H
#define RVPMODE_H


#include "UncertainMode.h"

class RvpMode : public UncertainMode {
    typedef UncertainMode Base;
public:
    RvpMode();
    virtual const char* tag() { return "(R)"; }

    virtual uint32_t mode_read(uint32_t* addr);
    virtual void mode_write(uint32_t* addr, uint32_t value);

    virtual void before_alloc_object();
    virtual void after_alloc_object(Object* obj);

    virtual void do_invoke_method(Object* target_object, MethodBlock* new_mb);
    virtual void do_method_return(int len);
    virtual void do_throw_exception();
    virtual void before_signal_exception(Class *exception_class);
    virtual void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs);

    virtual void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);
    virtual void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);

    virtual void do_array_load(Object* array, int index, int type_size);
    virtual void do_array_store(Object* array, int index, int type_size);

    virtual void destroy_frame(Frame* frame);

    virtual void step();
    virtual void enter_execution();
    virtual void leave_execution();
    // for logging
private:
    void log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb, Object* callee, MethodBlock* callee_mb);
};


#endif // RVPMODE_H
