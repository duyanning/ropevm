#ifndef CERTAINMODE_H
#define CERTAINMODE_H

#include "Mode.h"

class Message;

class CertainMode : public Mode {
    typedef Mode Base;
public:
    CertainMode();
    virtual const char* tag() { return "(C)"; }
    virtual void step();
    virtual void do_invoke_method(Object* target_object, MethodBlock* new_mb);
    virtual void do_method_return(int len);
    virtual void do_throw_exception();
    virtual void before_signal_exception(Class *exception_class);
    virtual void* do_execute_method(Object* target_object, MethodBlock *mb, std::vector<uintptr_t>& jargs);

    virtual void do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);
    virtual void do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static = false);

    virtual void do_array_load(Object* array, int index, int type_size);
    virtual void do_array_store(Object* array, int index, int type_size);
    //virtual void do_array_length(Object* array);

    virtual void destroy_frame(Frame* frame);
    //virtual void on_user_change(Object* old_user, Object* new_user);
    //void on_owner_lose_control();

    void invoke_to_my_method(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                          Object* calling_object,
                          CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp);

    void return_to_my_method(uintptr_t* rv, int len, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp);

    void invoke_my_method(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                          Object* calling_object,
                          CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp);
    void return_my_method(Frame* current_frame, uintptr_t* rv, int len);
    void get_my_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static);
    void put_my_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static);
    void process_certain_message(Message* messge);

    virtual uint32_t mode_read(uint32_t* addr);
    virtual void mode_write(uint32_t* addr, uint32_t value);

    virtual void enter_execution();
    virtual void leave_execution();
private:
    // for logging
private:
    void log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb, Object* callee, MethodBlock* callee_mb);
};

#endif // CERTAINMODE_H
