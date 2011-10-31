#ifndef SPECULATIVEMODE_H
#define SPECULATIVEMODE_H

#include "UncertainMode.h"

class SpeculativeMode : public UncertainMode {
    typedef UncertainMode Base;
public:
    SpeculativeMode();
    virtual const char* tag() { return "(S)"; }

    virtual void do_invoke_method(Object* target_object, MethodBlock* new_mb);
    virtual void do_method_return(int len);
    virtual void do_throw_exception();
    virtual void before_signal_exception(Class *exception_class);
    virtual void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs);

    virtual void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);
    virtual void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);

    virtual void do_array_load(Object* array, int index, int type_size);
    virtual void do_array_store(Object* array, int index, int type_size);

    virtual Frame* create_frame(Object* object, MethodBlock* new_mb, Frame* caller_prev, Object* calling_object, uintptr_t* args, uintptr_t* caller_sp, CodePntr caller_pc);
    virtual void destroy_frame(Frame* frame);

    void get_my_field(Object* obj, uintptr_t* addr, int size);
    void put_my_field(Object* obj, uintptr_t* addr, int size, bool is_static);

    virtual uint32_t mode_read(uint32_t* addr);
    virtual void mode_write(uint32_t* addr, uint32_t value);
    virtual void step();

    bool process_next_spec_msg();
    void snapshot(bool shot_frame = true); // refactor: remove
    void snapshot2();
    void pin_frames();
private:
    // for logging
private:
    void log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb, Object* callee, MethodBlock* callee_mb);
};

#endif // SPECULATIVEMODE_H
