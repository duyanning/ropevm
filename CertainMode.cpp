#include "std.h"
#include "jam.h"
#include "CertainMode.h"

#include "lock.h"

#include "Message.h"
#include "Core.h"
#include "OoSpmtJvm.h"
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
    if (m_core->m_id == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    Message* msg = m_core->get_certain_message();
    if (msg) {
        process_certain_message(msg);
        return;
    }

    exec_an_instr();
}

// transfer certain control/mode from src to dst
void transfer_certain_control(Core* src, Core* dst, Message* msg)
{
    // stat
    OoSpmtJvm::instance()->m_count_control_transfer++;


    assert(debug_scaffold::java_main_arrived);
    //{{{ just for debug
    if (src->id() == 6 && dst->id() == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    // MINILOG0_IF(src->get_owner(),
    //             "#" << src->id() << " transfer certain control to #" << dst->id()
    //             << " frame: " << src->m_certain_mode.frame
    //             << " (o:"
    //             << src->get_owner()
    //             << ", u:" << src->get_user()
    //             << "=>o:"
    //             << dst->get_owner()
    //             << ")"
    //             << "(o:"
    //             << type_name(src->get_owner())
    //             << ", u:" << type_name(src->get_user())
    //             << "=>o:"
    //             << type_name(dst->get_owner())
    //             << ")"
    //             << " because: " << *msg)

        // MINILOG0_IF(src->get_owner() && src->m_certain_mode.frame->mb,
        //             "#" << src->id() << " frame:"
        //             << *src->m_certain_mode.frame
        //             );


    dst->send_certain_message(msg);
    src->leave_certain_mode(msg);
}


// DO NOT forget invoke (reflect.cpp)!!!
void*
CertainMode::do_execute_method(Object* target_object,
                               MethodBlock *new_mb,
                               std::vector<uintptr_t>& jargs)
{
    assert(frame);              // see also: initialiseJavaStack

    // save (pc, frame, sp) triples
    CodePntr old_pc = pc;
    Frame* old_frame = frame;
    uintptr_t* old_sp = sp;

    // dummy frame is used to receive RV of top_frame
    // dummy->prev is current frame
    //Frame* dummy = create_frame(0, 0, frame, frame->get_object(), 0, 0, 0);
    Frame* dummy = create_frame(0, 0, frame, 0, 0, 0, 0);
    dummy->_name_ = "dummy frame";

    void *ret;
    ret = dummy->ostack_base;

    //????frame->last_pc = pc;

    Object* current_object = frame->get_object();

    assert(target_object);

    Group* current_group = current_object ? current_object->get_group() : threadSelf()->get_default_group();
    Group* target_group = target_object->get_group();

    assert(target_group->get_thread() == threadSelf());

    if (target_group == current_group) {

        invoke_my_method(target_object, new_mb, &jargs[0],
                         current_object,
                         0, dummy, dummy->ostack_base);

        // run the nested step-loop
        void* ret2;
        ret2 = executeJava();
        //assert(ret == ret2);
        assert(m_core->m_mode->is_certain_mode());

        //assert(frame == dummy->prev);

    }
    else {

        //assert(false);          // caution!

        Core* target_core = target_group->get_core();

        InvokeMsg* msg =
            new InvokeMsg(target_object, new_mb, dummy, current_object, &jargs[0], dummy->ostack_base, 0);

        MINILOG0("#" << m_core->id() << " e>>>transfers to #" << target_core->id()
                 // << " " << info(current_object) << " => " << info(target_object)
                 // << " (" << current_object << "=>" << target_object << ")"
                 << " because: " << *msg
                 << " in: "  << info(frame)
                 // << "("  << frame << ")"
                 << " offset: " << pc-(CodePntr)frame->mb->code
                 );

        //frame->last_pc = pc;

        m_core->halt();
        target_core->send_certain_message(msg);
        //target_core->execute_method();
        executeJava();
        m_core->switch_to_certain_mode();
        m_core->start();

    }

    // restore (pc, frame, pc)
    frame = old_frame;
    sp = old_sp;
    pc = old_pc;

    g_set_current_core(m_core);

    return ret;
}

void
CertainMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    if (intercept_vm_backdoor(target_object, new_mb)) return;

    //log_when_invoke_return(true, m_user, frame->mb, target_object, new_mb);

    MiniLogger logger(cout, false);

    MINILOG_IF(debug_scaffold::java_main_arrived && is_app_obj(new_mb->classobj),
               logger,
               *new_mb);

    assert(target_object);

    Object* current_object = frame->get_object();
    assert(current_object);

    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    assert(target_group->get_thread() == threadSelf());
    if (target_group == current_group) {

        sp -= new_mb->args_count;
        invoke_my_method(target_object, new_mb, sp,
                         frame->get_object(),
                         pc, frame, sp);

    }
    else {

        Core* target_core = target_group->get_core();

        //transfer certain control to target core
        sp -= new_mb->args_count;

        InvokeMsg* msg =
            new InvokeMsg(target_object, new_mb, frame, current_object, sp, sp, pc);

        frame->last_pc = pc;
        frame->snapshoted = true;

        MINILOG0("#" << m_core->id() << " i>>>transfers to #" << target_core->id()
                 // << " " << info(current_object) << " => " << info(target_object)
                 // << " (" << current_object << "=>" << target_object << ")"
                 << " because: " << *msg
                 << " in: "  << info(frame)
                 // << "("  << frame << ")"
                 );

        target_core->send_certain_message(msg);

        if (current_group->can_speculate()) {

            if (not m_core->m_messages_to_be_verified.empty()) {
                MINILOG0("#" << m_core->id() << " resume speculative execution");
                m_core->restore_original_non_certain_mode();
            }
            else {
                MINILOG0("#" << m_core->id() << " start speculative execution");

                m_core->switch_to_speculative_mode();
                if (is_priviledged(new_mb)) {
                    MINILOG(s_logger,
                            "#" << m_core->id() << " (S) " << *new_mb << "is native/sync method");
                    m_core->halt();
                    return;
                }

                MethodBlock* rvp_method = get_rvp_method(new_mb);
                MINILOG0("#" << m_core->id() << " (S)calls rvp-method: " << *rvp_method);
                Frame* rvp_frame = create_frame(target_object, rvp_method, frame, 0, sp, sp, pc);
                rvp_frame->is_certain = false;

                m_core->m_rvp_mode.pc = (CodePntr)rvp_method->code;
                m_core->m_rvp_mode.frame = rvp_frame;
                m_core->m_rvp_mode.sp = rvp_frame->ostack_base;
                m_core->switch_to_rvp_mode();
            }

        }
        else {
            m_core->halt();
        }

    }

}

void
CertainMode::do_method_return(int len)
{
//     if (is_application_class(frame->mb->classobj)) {
//         MINILOG0("#" << m_core->id() << " (C)return from " << *frame->mb);
//     }
    //log_when_invoke_return(false, frame->calling_object, frame->prev->mb, m_user, frame->mb);

    // if (frame->is_top_frame()) { // if return from top frame, we cannot use calling_object as target object
    //     assert(frame->calling_object == 0);
    //     return return_my_method(frame, sp - len, len);
    // }

    Object* current_object = frame->get_object();
    assert(current_object);
    Object* target_object = frame->calling_object;

    Group* current_group = current_object->get_group();
    Group* target_group = target_object ? target_object->get_group() : threadSelf()->get_default_group();

    //assert(target_group->get_thread() == threadSelf());
    if (target_group == current_group) {

        return_my_method(frame, sp - len, len);

    }
    else {

        Core* target_core = target_group->get_core();

        // if (frame->is_top_frame()) { // prev is dummy
        //     assert(false);      // todo

        // }

        //transfer certain control to target core
        sp -= len;

        if (frame->mb->is_synchronized()) {
            // Object *sync_ob = current_frame->mb->is_static() ?
            //     current_frame->mb->classobj : (Object*)current_frame->lvars[0]; // lvars[0] is 'this' reference
            assert(frame->get_object() == frame->mb->classobj or frame->get_object() == (Object*)frame->lvars[0]);
            Object* sync_ob = frame->get_object();
            objectUnlock(sync_ob);
        }

        if (frame->is_top_frame()) {

            MINILOG0("#" << m_core->id() << " t<<<transfers to #" << target_core->id()
                     << " in: "  << info(frame)
                     << "because top frame return"
                     );

            // write RV to dummy frame
            uintptr_t* caller_sp = frame->caller_sp;
            for (int i = 0; i < len; ++i) {
                //*caller_sp++ = current_sp[i];
                *caller_sp++ = sp[i];
            }

            m_core->signal_quit_step_loop(frame->prev->ostack_base);

        }
        else {

            ReturnMsg* msg = new ReturnMsg(current_object, frame->mb, frame->prev,
                                           target_object, sp, len,
                                           frame->caller_sp, frame->caller_pc);
            MINILOG0("#" << m_core->id() << " r<<<transfers to #" << target_core->id()
                     // << " " << info(current_object) << " => " << info(target_object)
                     // << " (" << current_object << "=>" << target_object << ")"
                     << " because: " << *msg
                     << " in: "  << info(frame)
                     // << "("  << frame << ")"
                     );

            target_core->send_certain_message(msg);

        }

        if (current_group->can_speculate()) {
            m_core->clear_frame_in_cache(frame);
            destroy_frame(frame);

            if (not m_core->m_messages_to_be_verified.empty()) {
                MINILOG0("#" << m_core->id() << " resume speculative execution");
                m_core->restore_original_non_certain_mode();
            }
            else {
                MINILOG0("#" << m_core->id() << " start speculative execution");

                m_core->sync_speculative_with_certain();
                m_core->switch_to_speculative_mode();
                m_core->m_speculative_mode.load_next_task();
            }
        }
        else {
            m_core->halt();
            destroy_frame(frame);
        }

    }

}

void
CertainMode::invoke_my_method(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                              Object* calling_object,
                              CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                              Object* calling_owner)
{
    MINILOG_IF(debug_scaffold::java_main_arrived,
               invoke_return_logger,
               "III to " << info(new_mb)
               );
    // if (strcmp(new_mb->name, "replace") == 0) {
    //     cout << "sp-base" << endl;
    // }

    //{{{ just for debug
    if (strcmp("forName", new_mb->name) == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    // MINILOGPROC(c_user_change_logger, show_user_change,
    //             (os, m_core->id(), "(C)",
    //              m_user, target_object, 1, caller_frame->mb, new_mb));
    // change_user(target_object);


    caller_frame->last_pc = pc;

    frame = create_frame(target_object, new_mb, caller_frame, calling_object, args, caller_sp, caller_pc, calling_owner);

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

        //if (debug_scaffold::java_main_arrived and strcmp(new_mb->name, "replace") == 0) {
        //if (debug_scaffold::java_main_arrived) {
        // if (debug_scaffold::java_main_arrived and strcmp(new_mb->name, "arraycopy") == 0) {
        //     cout << "sp-base " << sp - frame->ostack_base << endl;
        //     //cout << "sp-base " << new_mb->name << endl;
        // }

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
CertainMode::return_my_method(Frame* current_frame, uintptr_t* rv, int len)
{
    MINILOG_IF(debug_scaffold::java_main_arrived,
               invoke_return_logger,
               "RRR from " << info(current_frame->mb)
               );

    // MINILOGPROC(c_user_change_logger, show_user_change,
    //             (os, m_core->id(), "(C)",
    //              m_user, current_frame->calling_object, 2,
    //              current_frame->mb, current_frame->prev->mb));

    //change_user(current_frame->calling_object);

    assert(len == 0 || len == 1 || len == 2);

    //current_sp -= len;                  // now, sp points to begin of RV

    // write RV to caller's frame
    uintptr_t* caller_sp = current_frame->caller_sp;
    for (int i = 0; i < len; ++i) {
        //*caller_sp++ = current_sp[i];
        *caller_sp++ = rv[i];
    }

    if (current_frame->mb->is_synchronized()) {
        Object *sync_ob = current_frame->mb->is_static() ?
            current_frame->mb->classobj : (Object*)current_frame->lvars[0]; // lvars[0] is 'this' reference
        objectUnlock(sync_ob);
    }

    if (current_frame->is_top_frame()) {
        // whether native or not
        m_core->signal_quit_step_loop(current_frame->prev->ostack_base);

        destroy_frame(current_frame);
    }
    else {

        // whether native or not

        sp = caller_sp;
        pc = current_frame->caller_pc;
        frame = current_frame->prev;

        destroy_frame(current_frame);

        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    }


}

void
CertainMode::before_signal_exception(Class *exception_class)
{
    MINILOG(c_exception_logger, "#" << m_core->id() << " (C) before signal exception "
            << exception_class->name() << " in: " << info(frame));
    // do nothing
}

void
CertainMode::do_throw_exception()
{
    MINILOG(c_exception_logger, "#" << m_core->id() << " (C) throw exception"
            << " in: " << info(frame)
            << " on: #" << frame->get_object()->get_group()->get_core()->id()
            );
    //{{{ just for debug
    if (debug_scaffold::java_main_arrived && m_core->id() == 7) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    //ExecEnv *ee = getExecEnv();
    Object *excep = exception;
    exception = NULL;

    CodePntr old_pc = pc;      // for debug
    pc = findCatchBlock(excep->classobj);
    MINILOG(c_exception_logger, "#" << m_core->id() << " (C) handler found"
            << " in: " << info(frame)
            << " on: #" << frame->get_object()->get_group()->get_core()->id()
            );

    /* If we didn't find a handler, restore exception and
       return to previous invocation */
    if (pc == NULL) {
        //assert(false);          // when uncaughed exception ocurr, we get here
        exception = excep;
        //{{{ just for debug
        if (m_core->id() == 7) {
            int x = 0;
            x++;
        }
        //}}} just for debug
        m_core->signal_quit_step_loop(0);
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

void
CertainMode::enter_execution()
{
    m_core->m_certain_depth++;
    m_core->m_speculative_depth = m_core->m_certain_depth;
    //    cout << "core " << m_core << " enter certain execution depth: " << m_core->m_certain_depth << endl;
}

void
CertainMode::leave_execution()
{
    //    cout << "core " << m_core <<  " leave certain execution depth: " << m_core->m_certain_depth << endl;
    m_core->m_certain_depth--;
    m_core->m_speculative_depth = m_core->m_certain_depth;
}

void
CertainMode::destroy_frame(Frame* frame)
{
    MINILOG_IF(debug_scaffold::java_main_arrived && is_app_obj(frame->mb->classobj),
               c_destroy_frame_logger, "#" << m_core->id()
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
    Group* source_group = source_object->get_group();

    sp -= is_static ? 0 : 1;

    if (current_group->can_speculate() and m_core->has_message_to_be_verified()) {

        GetMsg* msg = new GetMsg(source_object, current_object, fb, addr, size, frame, sp, pc);
        m_core->verify_speculation(msg);
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
    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    sp -= size;

    if (target_group->can_speculate()) {

        Core* target_core = target_object->get_group()->get_core();

        PutMsg* msg = new PutMsg(current_object, target_object, fb, addr, sp, size, is_static);

        sp -= is_static ? 0 : 1;
        pc += 3;

        m_core->halt();
        target_core->send_certain_message(msg);

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
    Group* source_group = source_object->get_group();
    Group* current_group = current_object->get_group();

    sp -= 2; // pop up arrayref and index

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value

    if (current_group->can_speculate() and m_core->has_message_to_be_verified()) {

        ArrayLoadMsg* msg =
            new ArrayLoadMsg(array, current_object, index, (uint8_t*)addr, type_size, frame, sp, pc);
        m_core->verify_speculation(msg);
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
    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    sp -= nslots;

    if (target_group->can_speculate()) {

        Core* target_core = target_object->get_group()->get_core();
        assert(target_core);

        ArrayStoreMsg* msg = new ArrayStoreMsg(current_object, array, index, sp, nslots, type_size);
        sp -= 2;                    // pop up arrayref and index
        pc += 1;

        m_core->halt();
        target_core->send_certain_message(msg);

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
    log_invoke_return(c_invoke_return_logger, is_invoke, m_core->id(), "(C)",
                      caller, caller_mb, callee, callee_mb);
}

void
CertainMode::process_certain_message(Message* msg)
{
    assert(msg->get_type() == Message::invoke
           || msg->get_type() == Message::ret
           || msg->get_type() == Message::put
           || msg->get_type() == Message::arraystore
           //|| msg->get_type() == Message::ack
           );

    m_core->reexecute_failed_message(msg);
}
