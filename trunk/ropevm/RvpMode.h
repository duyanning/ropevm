#ifndef RVPMODE_H
#define RVPMODE_H

#include "UncertainMode.h"

class RvpMode : public UncertainMode {
    typedef UncertainMode Base;
public:
    RvpMode();
    virtual const char* tag() { return "(R)"; }


    void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs, DummyFrame* dummy) override;
    void before_signal_exception(Class *exception_class) override;
    void before_alloc_object() override;
    void after_alloc_object(Object* obj) override;
    Frame* push_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                      SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                      bool is_top) override;
    void pop_frame(Frame* frame) override;

    //virtual void step();
private:
    void do_invoke_method(Object* target_object, MethodBlock* new_mb) override;
    void do_method_return(int len) override;
    void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false) override;
    void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false) override;
    void do_array_load(Object* array, int index, int type_size) override;
    void do_array_store(Object* array, int index, int type_size) override;

    uint32_t mode_read(uint32_t* addr) override;
    void mode_write(uint32_t* addr, uint32_t value) override;

    void do_spec_barrier() override;
};


#endif // RVPMODE_H
