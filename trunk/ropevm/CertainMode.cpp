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


    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();

    assert(target_spmt_thread->get_thread() == g_get_current_thread());

    if (target_spmt_thread == m_spmt_thread) {

        invoke_impl(target_object, new_mb, &jargs[0],
                    m_spmt_thread, 0, dummy, dummy->ostack_base);


    }
    else {
        InvokeMsg* msg = new InvokeMsg(m_spmt_thread, target_spmt_thread,
                                       target_object, new_mb, &jargs[0],
                                       pc, frame, sp,
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
        m_spmt_thread->m_need_spec_msg = false;

        m_spmt_thread->send_msg(msg);
    }


    //g_drive_loop();
    m_spmt_thread->drive_loop();

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

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    if (target_spmt_thread == m_spmt_thread or g_is_pure_code_method(new_mb)) {

        sp -= new_mb->args_count;
        invoke_impl(target_object, new_mb, sp,
                    m_spmt_thread, pc, frame, sp);

    }
    else {

        // pop up arguments
        sp -= new_mb->args_count;

        // construct certain msg
        InvokeMsg* msg = new InvokeMsg(m_spmt_thread, target_spmt_thread,
                                       target_object, new_mb, sp,
                                       pc, frame, sp);

        frame->last_pc = pc;

        // MINILOG0("#" << m_spmt_thread->m_id << " i>>>transfers to #" << target_spmt_thread->id()
        //          // << " " << info(current_object) << " => " << info(target_object)
        //          // << " (" << current_object << "=>" << target_object << ")"
        //          << " because: " << *msg
        //          << " in: "  << info(frame)
        //          // << "("  << frame << ")"
        //          );

        m_spmt_thread->send_msg(msg);

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

    assert(len == 0 || len == 1 || len == 2);

    Frame* current_frame = frame; // some funcitons below will change this->frame, so we save it to destroy


    // 给同步方法解锁
    if (current_frame->mb->is_synchronized()) {
        assert(current_frame->get_object() == current_frame->mb->classobj
               or current_frame->get_object() == (Object*)current_frame->lvars[0]);
        Object* sync_ob = current_frame->get_object();
        objectUnlock(sync_ob);
    }


    SpmtThread* target_spmt_thread = current_frame->caller;
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);


    /*
    if 目标线程是当前线程
        返回值写入上级frame的ostack（if 是从top frame返回，上级frame为dummy frame，并结束drive_loop）
    else
        构造return确定消息（if 是从top frame返回，则带有top标志）
    */


    if (target_spmt_thread == m_spmt_thread) {

        uintptr_t* rv = sp - len;   // rv points to the beginning of retrun value in this frame's operand stack
        sp -= len;

        // 将返回值写入主调方法的ostack
        uintptr_t* caller_sp = current_frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            *caller_sp++ = rv[i];
        }

        pc = current_frame->caller_pc;
        frame = current_frame->prev;
        sp = caller_sp;

        destroy_frame(current_frame);
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

        if (current_frame->is_top_frame()) {

            // MINILOG0("#" << m_spmt_thread->id() << " t<<<transfers to #" << target_core->id()
            //          << " in: "  << info(frame)
            //          << "because top frame return"
            //          );

            m_spmt_thread->signal_quit_step_loop(0);

        }


    }
    else {

        // MINILOG0("#" << m_spmt_thread->id() << " r<<<transfers to #" << target_spmt_thread->id()
        //          // << " " << info(current_object) << " => " << info(target_object)
        //          // << " (" << current_object << "=>" << target_object << ")"
        //          << " because: " << *msg
        //          << " in: "  << info(current_frame)
        //          // << "("  << current_frame << ")"
        //          );


        uintptr_t* rv = sp - len;   // rv points to the beginning of retrun value in this frame's operand stack
        sp -= len;

        ReturnMsg* msg = new ReturnMsg(target_spmt_thread,
                                       rv, len,
                                       current_frame->caller_pc,
                                       current_frame->prev,
                                       current_frame->caller_sp,
                                       current_frame->is_top_frame());
        // state buffer中应无任何东西
        // m_spmt_thread->clear_frame_in_state_buffer(current_frame);
        destroy_frame(current_frame);

        m_spmt_thread->switch_to_speculative_mode();
        m_spmt_thread->send_msg(msg);
        m_spmt_thread->launch_next_spec_msg();
    }

}


void
CertainMode::invoke_impl(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                         SpmtThread* caller, CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp)
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

    frame = create_frame(target_object, new_mb, args,
                         m_spmt_thread, caller_pc, caller_frame, caller_sp);

    // 给同步方法加锁
    if (frame->mb->is_synchronized()) {
        Object *sync_ob = frame->mb->is_static() ?
            frame->mb->classobj : (Object*)frame->lvars[0]; // lvars[0] is 'this' reference
        objectLock(sync_ob);
    }


    // 如果是native方法，在此处立即执行其native code
    if (new_mb->is_native()) {
        // // copy args to ostack
        // if (args)
        //     std::copy(args, args + new_mb->args_count, frame->ostack_base);

        sp = (*(uintptr_t *(*)(Class*, MethodBlock*, uintptr_t*))
              new_mb->native_invoker)(new_mb->classobj, new_mb,
                                      frame->ostack_base);

        if (exception) {
            throw_exception;
        }
        else {
            // native方法已经结束，返回值已经产生。该返回了。
            do_method_return(sp - frame->ostack_base);

        }

        return;

    }

    sp = frame->ostack_base;
    pc = (CodePntr)frame->mb->code;

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
            << " on: #" << frame->get_object()->get_spmt_thread()->id()
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
            << " on: #" << frame->get_object()->get_spmt_thread()->id()
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
CertainMode::do_get_field(Object* target_object, FieldBlock* fb,
                          uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    if (target_spmt_thread == m_spmt_thread) {
        sp -= is_static ? 0 : 1;
        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }
        pc += 3;
    }
    else {
        sp -= is_static ? 0 : 1;
        GetMsg* msg = new GetMsg(m_spmt_thread, target_spmt_thread, target_object, fb);

        m_spmt_thread->sleep();
        m_spmt_thread->send_msg(msg);

    }

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

// size表示占几个ostack位置。
// 就算字段的类型大小只有一个字节，但在存储时，总是有四个或八个字节的位置保留给它。数组则不然，数组是紧致存储的。
void
CertainMode::do_put_field(Object* target_object, FieldBlock* fb,
                          uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    if (target_spmt_thread == m_spmt_thread) {
        sp -= size;

        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }

        sp -= is_static ? 0 : 1;
        pc += 3;
    }
    else {
        sp -= size;

        PutMsg* msg = new PutMsg(m_spmt_thread, target_spmt_thread, target_object, fb, sp);

        sp -= is_static ? 0 : 1;

        m_spmt_thread->sleep();

        m_spmt_thread->send_msg(msg);
    }

}

void
CertainMode::do_array_load(Object* array, int index, int type_size)
{

    Object* target_object = array;

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);


    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value

    if (target_spmt_thread == m_spmt_thread) {
        sp -= 2; // pop up arrayref and index
        load_from_array(sp, array, index, type_size);
        sp += nslots;
        pc += 1;
    }
    else {
        sp -= 2; // pop up arrayref and index
        ArrayLoadMsg* msg = new ArrayLoadMsg(m_spmt_thread, target_spmt_thread,
                                             array, type_size, index);

        m_spmt_thread->sleep();
        m_spmt_thread->send_msg(msg);

    }



}

/*
type_size 表示元素类型的大小，单位字节。
nslots 表示占几个ostack的位置，一个位置四个字节。

就算数组元素的类型大小只有一个字节，但读到ostack中之后，仍然是占一个ostack位置，即四个字节。
但数组是紧致存储的。

 */
void
CertainMode::do_array_store(Object* array, int index, int type_size)
{
    Object* target_object = array;

    SpmtThread* target_spmt_thread = target_object->get_spmt_thread();
    assert(target_spmt_thread->m_thread == m_spmt_thread->m_thread);

    //void* addr = array_elem_addr(array, index, type_size);
    int nslots = size2nslots(type_size); // number of slots for value


    if (target_spmt_thread == m_spmt_thread) {
        sp -= nslots;
        store_to_array(sp, array, index, type_size);
        sp -= 2;                    // pop up arrayref and index
        pc += 1;
    }
    else {

        sp -= nslots;
        ArrayStoreMsg* array_store_msg = new ArrayStoreMsg(m_spmt_thread, target_spmt_thread,
                                                           array,
                                                           type_size, index, sp);
        sp -= 2;                    // pop up arrayref and index

        m_spmt_thread->sleep();
        m_spmt_thread->send_msg(array_store_msg);

    }

}


void
CertainMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(c_invoke_return_logger, is_invoke, m_spmt_thread->id(), "(C)",
                      caller, caller_mb, callee, callee_mb);
}


void
CertainMode::send_msg(Message* msg)
{
    msg->get_target_spmt_thread()->set_certain_msg(msg);
}
