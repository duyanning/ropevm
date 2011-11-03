#include "std.h"
#include "rope.h"
#include "CertainMode.h"

#include "lock.h"

#include "Message.h"
#include "SpmtThread.h"
#include "RopeVM.h"
#include "interp.h"
#include "Helper.h"
#include "Loggers.h"
#include "DebugScaffold.h"
#include "Snapshot.h"
#include "frame.h"
#include "Group.h"

using namespace std;

CertainMode::CertainMode()
:
    Mode("Certain mode")
{
}

uint32_t
CertainMode::mode_read(uint32_t* addr)
{
    return *addr;
}

void
CertainMode::mode_write(uint32_t* addr, uint32_t value)
{
    *addr = value;
}

void
CertainMode::step()
{
    //{{{ just for debug
    if (m_spmt_thread->m_id == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    // Message* msg = m_spmt_thread->get_certain_msg();
    // if (msg) {
    //     process_certain_message(msg);
    //     return;
    // }

    fetch_and_interpret_an_instruction();
}

// DO NOT forget function 'invoke' in reflect.cpp!!!
void*
CertainMode::do_execute_method(Object* target_object,
                               MethodBlock *new_mb,
                               std::vector<uintptr_t>& jargs)
{
    assert(frame);              // see also: initialiseJavaStack

    // save (pc, frame, sp) of outer drive_loop
    CodePntr old_pc = pc;
    Frame* old_frame = frame;
    uintptr_t* old_sp = sp;


    // dummy frame is used to receive RV of top_frame
    // dummy->prev is current frame
    Frame* dummy = create_dummy_frame(frame);

    void *ret;
    ret = dummy->ostack_base;

    //????frame->last_pc = pc;

    Object* current_object = frame->get_object();

    assert(target_object);


    SpmtThread* target_spmt_thread = target_object->get_group()->get_spmt_thread();

    assert(target_spmt_thread->get_group()->get_thread() == threadSelf());

    if (target_spmt_thread == m_spmt_thread) {

        internal_invoke(target_object, new_mb, &jargs[0],
                        0, dummy, dummy->ostack_base);


    }
    else {
        InvokeMsg* msg = new InvokeMsg(m_spmt_thread, target_object,
                                       new_mb, &jargs[0],
                                       true);


        MINILOG0("#" << m_spmt_thread->id() << " e>>>transfers to #" << target_spmt_thread->id()
                 // << " " << info(current_object) << " => " << info(target_object)
                 // << " (" << current_object << "=>" << target_object << ")"
                 << " because: " << *msg
                 << " in: "  << info(frame)
                 // << "("  << frame << ")"
                 << " offset: " << pc-(CodePntr)frame->mb->code
                 );

        //frame->last_pc = pc;

        // 转入推测模式，但不要执行
        m_spmt_thread->sleep();
        m_spmt_thread->m_is_waiting_for_task = false;

        m_spmt_thread->send_certain_msg(target_spmt_thread, msg);
    }


    g_drive_loop();

    assert(m_spmt_thread->m_mode->is_certain_mode());

    // restore (pc, frame, sp) of outer drive_loop
    pc = old_pc;
    frame = old_frame;
    sp = old_sp;


    return ret;
}

void
CertainMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    assert(target_object);

    //if (intercept_vm_backdoor(target_object, new_mb)) return;

    SpmtThread* target_spmt_thread = target_object->get_group()->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    if (target_spmt_thread == m_spmt_thread) {

        sp -= new_mb->args_count;
        internal_invoke(target_object, new_mb, sp,
                        pc, frame, sp);

    }
    else {

        // pop up arguments
        sp -= new_mb->args_count;

        // construct certain msg
        InvokeMsg* msg = new InvokeMsg(m_spmt_thread, target_object,
                                       new_mb, sp);

        frame->last_pc = pc;

        // MINILOG0("#" << m_spmt_thread->m_id << " i>>>transfers to #" << target_spmt_thread->id()
        //          // << " " << info(current_object) << " => " << info(target_object)
        //          // << " (" << current_object << "=>" << target_object << ")"
        //          << " because: " << *msg
        //          << " in: "  << info(frame)
        //          // << "("  << frame << ")"
        //          );

        target_spmt_thread->send_certain_msg(target_spmt_thread, msg);

        MethodBlock* rvp_method = get_rvp_method(new_mb);

        MINILOG0("#" << m_spmt_thread->m_id
                 << " (S)invokes rvp-method: " << *rvp_method);

        Frame* rvp_frame = m_spmt_thread->m_rvp_mode.create_frame(target_object,
                                                                  rvp_method,
                                                                  sp,
                                                                  0,
                                                                  pc, frame, sp);

        m_spmt_thread->m_rvp_mode.pc = (CodePntr)rvp_method->code;
        m_spmt_thread->m_rvp_mode.frame = rvp_frame;
        m_spmt_thread->m_rvp_mode.sp = rvp_frame->ostack_base;

        m_spmt_thread->switch_to_rvp_mode();

    }

}

void
CertainMode::do_method_return(int len)
{
//     if (is_application_class(frame->mb->classobj)) {
//         MINILOG0("#" << m_spmt_thread->id() << " (C)return from " << *frame->mb);
//     }
    //log_when_invoke_return(false, frame->calling_object, frame->prev->mb, m_user, frame->mb);

    Frame* current_frame = frame; // some funcitons below will change this->frame, so we save it to destroy

    uintptr_t* rv = sp - len;   // rv points to the beginning of retrun value in this frame's operand stack
    sp -= len;

    Object* current_object = current_frame->get_object();
    assert(current_object);
    Object* target_object = current_frame->calling_object;

    Group* current_group = current_object->get_group();
    Group* target_group = target_object ? target_object->get_group() : threadSelf()->get_default_group();

    //assert(target_group->get_thread() == threadSelf());

    SpmtThread* target_spmt_thread = current_frame->caller;

    if (current_frame->mb->is_synchronized()) {
        assert(
               current_frame->get_object() == current_frame->mb->classobj
               or current_frame->get_object() == (Object*)current_frame->lvars[0]
               );
        Object* sync_ob = current_frame->get_object();
        objectUnlock(sync_ob);
    }

    /*
    if 目标线程是当前线程
        if 是从top frame返回，返回值直接写入dummy frame
    else
        if 是从top frame返回，构造return消息
    */

    if (current_frame->is_top_frame()) {

        // MINILOG0("#" << m_spmt_thread->id() << " t<<<transfers to #" << target_core->id()
        //          << " in: "  << info(frame)
        //          << "because top frame return"
        //          );

        // write RV to dummy frame
        uintptr_t* caller_sp = current_frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            *caller_sp++ = rv[i];
        }

        m_spmt_thread->signal_quit_step_loop(current_frame->prev->ostack_base);

    }


    if (target_spmt_thread == m_spmt_thread) {

        internal_return(rv, len, current_frame->caller_pc, current_frame->prev, current_frame->caller_sp);
        return;

    }
    else {


        // MINILOG0("#" << m_spmt_thread->id() << " r<<<transfers to #" << target_spmt_thread->id()
        //          // << " " << info(current_object) << " => " << info(target_object)
        //          // << " (" << current_object << "=>" << target_object << ")"
        //          << " because: " << *msg
        //          << " in: "  << info(current_frame)
        //          // << "("  << current_frame << ")"
        //          );


        ReturnMsg* msg = new ReturnMsg(rv, len, current_frame->is_top_frame());

        m_spmt_thread->switch_to_speculative_mode();
        m_spmt_thread->send_certain_msg(target_spmt_thread, msg);
        m_spmt_thread->launch_next_spec_msg();


        if (current_frame->is_top_frame())
            m_spmt_thread->signal_quit_step_loop(0);



        // m_spmt_thread->clear_frame_in_states_buffer(current_frame);

        // if (not m_spmt_thread->m_messages_to_be_verified.empty()) {
        //     MINILOG0("#" << m_spmt_thread->id() << " resume speculative execution");
        //     m_spmt_thread->restore_original_non_certain_mode();
        // }
        // else {
        //     MINILOG0("#" << m_spmt_thread->id() << " start speculative execution");

        //     m_spmt_thread->sync_speculative_with_certain();
        //     m_spmt_thread->switch_to_speculative_mode();
        //     m_spmt_thread->launch_next_spec_msg();
        // }


    }

    destroy_frame(current_frame);

}

void
CertainMode::internal_invoke(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                             CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp)
{
    MINILOG_IF(debug_scaffold::java_main_arrived,
               invoke_return_logger,
               "III to " << info(new_mb)
               );

    //{{{ just for debug
    if (strcmp("forName", new_mb->name) == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug


    caller_frame->last_pc = pc;

    frame = create_frame(target_object, new_mb, args, m_spmt_thread, caller_pc, caller_frame, caller_sp);

    if (frame->mb->is_synchronized()) {
        Object *sync_ob = frame->mb->is_static() ?
            frame->mb->classobj : (Object*)frame->lvars[0]; // lvars[0] is 'this' reference
        objectLock(sync_ob);
    }

    if (new_mb->is_native()) {
        // copy args to ostack
        if (args)
            std::copy(args, args + new_mb->args_count, frame->ostack_base);

        sp = (*(uintptr_t *(*)(Class*, MethodBlock*, uintptr_t*))
              new_mb->native_invoker)(new_mb->classobj, new_mb,
                                      frame->ostack_base);

        if (exception) {
            throw_exception;
        }
        else {

            do_method_return(sp - frame->ostack_base);

        }

    }
    else {
        sp = frame->ostack_base;
        pc = (CodePntr)frame->mb->code;
    }
}

void
CertainMode::internal_return(uintptr_t* rv, int len, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp)
{
    // MINILOG_IF(debug_scaffold::java_main_arrived,
    //            invoke_return_logger,
    //            "RRR from " << info(current_frame->mb)
    //            );

    assert(len == 0 || len == 1 || len == 2);

    Frame* current_frame = frame;

    // write RV to caller's frame
    for (int i = 0; i < len; ++i) {
        *caller_sp++ = rv[i];
    }

    pc = caller_pc;
    frame = caller_frame;
    sp = caller_sp;

    pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);


    if (current_frame->is_top_frame()) {

        // MINILOG0("#" << m_spmt_thread->id() << " t<<<transfers to #" << target_core->id()
        //          << " in: "  << info(frame)
        //          << "because top frame return"
        //          );

        m_spmt_thread->signal_quit_step_loop(current_frame->prev->ostack_base);

    }

}

void
CertainMode::before_signal_exception(Class *exception_class)
{
    MINILOG(c_exception_logger, "#" << m_spmt_thread->id() << " (C) before signal exception "
            << exception_class->name() << " in: " << info(frame));
    // do nothing
}

void
CertainMode::do_throw_exception()
{
    MINILOG(c_exception_logger, "#" << m_spmt_thread->id() << " (C) throw exception"
            << " in: " << info(frame)
            << " on: #" << frame->get_object()->get_group()->get_core()->id()
            );
    //{{{ just for debug
    if (debug_scaffold::java_main_arrived && m_spmt_thread->id() == 7) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    //ExecEnv *ee = getExecEnv();
    Object *excep = exception;
    exception = NULL;

    CodePntr old_pc = pc;      // for debug
    pc = findCatchBlock(excep->classobj);
    MINILOG(c_exception_logger, "#" << m_spmt_thread->id() << " (C) handler found"
            << " in: " << info(frame)
            << " on: #" << frame->get_object()->get_group()->get_core()->id()
            );

    /* If we didn't find a handler, restore exception and
       return to previous invocation */
    if (pc == NULL) {
        //assert(false);          // when uncaughed exception ocurr, we get here
        exception = excep;
        //{{{ just for debug
        if (m_spmt_thread->id() == 7) {
            int x = 0;
            x++;
        }
        //}}} just for debug
        m_spmt_thread->signal_quit_step_loop(0);
        return;
    }

    // /* If we're handling a stack overflow, reduce the stack
    //    back past the red zone to enable handling of further
    //    overflows */
    // if (ee->overflow) {
    //     ee->overflow = FALSE;
    //     ee->stack_end -= STACK_RED_ZONE_SIZE;
    // }

    /* Setup intepreter to run the found catch block */
    //frame = ee->last_frame;
    sp = frame->ostack_base;
    *sp++ = (uintptr_t)excep;
}


Frame*
CertainMode::create_frame(Object* object, MethodBlock* new_mb, uintptr_t* args,
                          SpmtThread* caller,
                          CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp)
{
    Frame* new_frame = g_create_frame(object, new_mb, args,
                                      caller,
                                      caller_pc, caller_frame, caller_sp);

    return new_frame;
}

void
CertainMode::destroy_frame(Frame* frame)
{
    MINILOG_IF(debug_scaffold::java_main_arrived && is_app_obj(frame->mb->classobj),
               c_destroy_frame_logger, "#" << m_spmt_thread->id()
               << " (C) destroy frame " << *frame);

    //{{{ just for debug
    if (frame->mb
        && strcmp("generatePatient", frame->mb->name) == 0
        ){
        int x = 0;
        x++;
    }
    //}}} just for debug
    //delete frame;
    Mode::destroy_frame(frame);
}

/*
pre and post do_get_field

getfield
 _____             _____
| obj |           | val |
|     |<--sp =>   |     |<--sp
|     |           |     |

getstatic
 _____             _____
|     |<--sp      | val |
|     |      =>   |     |<--sp
|     |           |     |

 */
void
CertainMode::do_get_field(Object* source_object, FieldBlock* fb,
                          uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    //{{{ just for debug
    if (strcmp(frame->mb->name, "main") == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    Object* current_object = frame->get_object();
    Group* current_group = current_object->get_group();
    //Group* source_group = source_object->get_group();

    sp -= is_static ? 0 : 1;

    if (current_group->can_speculate() and m_spmt_thread->has_message_to_be_verified()) {

        GetMsg* msg = new GetMsg(m_spmt_thread, source_object, fb, addr, size, frame, sp, pc);
        m_spmt_thread->verify_speculation(msg);
        return;

    }
    else {

        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }

    }

    pc += 3;

}

/*
pre and post do_put_field

putfield
 _____             _____
| obj |           | obj |<--sp
| val |      =>   | val |
|     |<--sp      |     |

putstatic
 _____             _____
| val |           | val |<--sp
|     |<--sp =>   |     |
|     |           |     |

 */
void
CertainMode::do_put_field(Object* target_object, FieldBlock* fb,
                          uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    // if target object has a core, target core should verify
    // target object may be others or owner of this core

    Object* current_object = frame->get_object();
    //Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    sp -= size;

    if (target_group->can_speculate()) {

        SpmtThread* target_core = target_object->get_group()->get_core();

        PutMsg* msg = new PutMsg(m_spmt_thread, target_object, fb, addr, sp, size, is_static);

        sp -= is_static ? 0 : 1;
        pc += 3;

        m_spmt_thread->sleep();
        m_spmt_thread->send_certain_msg(target_core, msg);

        return;         // avoid sp-=... and pc += ...

    }
    else {

        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }

    }

    sp -= is_static ? 0 : 1;
    pc += 3;
}

void
CertainMode::do_array_load(Object* array, int index, int type_size)
{
    Object* source_object = array;
    Object* current_object = frame->get_object();
    //Group* source_group = source_object->get_group();
    Group* current_group = current_object->get_group();

    sp -= 2; // pop up arrayref and index

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value

    if (current_group->can_speculate() and m_spmt_thread->has_message_to_be_verified()) {

        ArrayLoadMsg* msg =
            new ArrayLoadMsg(m_spmt_thread, array, index, (uint8_t*)addr, type_size, frame, sp, pc);
        m_spmt_thread->verify_speculation(msg);
        return;

    }
    else {

        load_from_array(sp, addr, type_size);
        sp += nslots;

    }

    pc += 1;

}

void
CertainMode::do_array_store(Object* array, int index, int type_size)
{
    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value
    Object* target_object = array;
    Object* current_object = frame->get_object();
    //Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    sp -= nslots;

    if (target_group->can_speculate()) {

        SpmtThread* target_core = target_object->get_group()->get_core();
        assert(target_core);

        ArrayStoreMsg* msg = new ArrayStoreMsg(m_spmt_thread, array, index, sp, nslots, type_size);
        sp -= 2;                    // pop up arrayref and index
        pc += 1;

        m_spmt_thread->sleep();
        m_spmt_thread->send_certain_msg(target_core, msg);

        return;         // avoid sp-=... and pc += ...

    }
    else {

        store_to_array(sp, addr, type_size);

    }

    sp -= 2;                    // pop up arrayref and index
    pc += 1;
}


void
CertainMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(c_invoke_return_logger, is_invoke, m_spmt_thread->id(), "(C)",
                      caller, caller_mb, callee, callee_mb);
}
