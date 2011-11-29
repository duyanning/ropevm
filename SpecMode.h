#ifndef SPECULATIVEMODE_H
#define SPECULATIVEMODE_H

#include "UncertainMode.h"

class SpecMode : public UncertainMode {
    typedef UncertainMode Base;
public:
    SpecMode();
    virtual const char* tag() { return "(S)"; }

    virtual void do_spec_barrier();
    virtual void do_invoke_method(Object* target_object, MethodBlock* new_mb);
    virtual void do_method_return(int len);
    virtual void before_signal_exception(Class *exception_class);
    virtual void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs, DummyFrame* dummy);

    virtual void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);
    virtual void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);

    virtual void do_array_load(Object* array, int index, int type_size);
    virtual void do_array_store(Object* array, int index, int type_size);

    virtual void send_msg(Message* msg);

    // virtual void invoke_impl(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
    //                          SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
    //                          bool is_top);

    virtual Frame* push_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                              SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                              bool is_top);

    virtual void pop_frame(Frame* frame);

    virtual uint32_t mode_read(uint32_t* addr);
    virtual void mode_write(uint32_t* addr, uint32_t value);
    //virtual void step();

private:
};

#endif // SPECULATIVEMODE_H
