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

    virtual void do_spec_barrier();
    virtual void before_alloc_object();
    virtual void after_alloc_object(Object* obj);

    virtual void do_invoke_method(Object* target_object, MethodBlock* new_mb);
    virtual void do_method_return(int len);
    virtual void before_signal_exception(Class *exception_class);
    virtual void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs, DummyFrame* dummy);

    virtual void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);
    virtual void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);

    virtual void do_array_load(Object* array, int index, int type_size);
    virtual void do_array_store(Object* array, int index, int type_size);

    virtual void invoke_impl(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                             SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                             bool is_top);

    virtual Frame* create_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                                SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                                bool is_top);

    virtual void destroy_frame(Frame* frame);

    //virtual void step();
};


#endif // RVPMODE_H
