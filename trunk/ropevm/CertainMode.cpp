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

using namespace std;

CertainMode::CertainMode()
:
    Mode("Certain mode")
{
}

uint32_t
CertainMode::mode_read(uint32_t* addr)
// uint8_t
// CertainMode::mode_read(uint8_t* addr)
{
    return *addr;
}

void
CertainMode::mode_write(uint32_t* addr, uint32_t value)
// CertainMode::mode_write(uint8_t* addr, uint8_t value)
{
    *addr = value;
}

void
CertainMode::step()
{
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

    MINILOG0_IF(src->get_owner(),
                "#" << src->id() << " transfer certain control to #" << dst->id()
                << " frame: " << src->m_certain_mode.frame
                << " (o:"
                << src->get_owner()
                << ", u:" << src->get_user()
                << "=>o:"
                << dst->get_owner()
                << ")"
                << "(o:"
                << type_name(src->get_owner())
                << ", u:" << type_name(src->get_user())
                << "=>o:"
                << type_name(dst->get_owner())
                << ")"
                << " because: " << *msg)

        // MINILOG0_IF(src->get_owner() && src->m_certain_mode.frame->mb,
        //             "#" << src->id() << " frame:"
        //             << *src->m_certain_mode.frame
        //             );


    dst->send_certain_message(msg);
    src->leave_certain_mode(msg);
}

void*
CertainMode::do_execute_method(Object* target_object,
                               MethodBlock *new_mb,
                               std::vector<uintptr_t>& jargs)
{
    // save (pc, frame, sp) triples
    Object* old_user = m_user;
    CodePntr old_pc = pc;
    Frame* old_frame = frame;
    uintptr_t* old_sp = sp;


    // dummy frame is used to receive RV of top_frame
    // dummy->prev is current frame
    Frame* dummy = create_frame(0, 0, frame, m_user, 0, 0, 0);
    dummy->_name_ = "dummy frame";

    void *ret;
    ret = dummy->ostack_base;


    Object* old_owner = m_core->m_owner;
    //Object* old_user = m_user;


    if (is_user_enclosure(target_object)) { // target_object is myself
        invoke_my_method(target_object, new_mb, &jargs[0],
                         m_user,
                         0, dummy, dummy->ostack_base);
    }
    else {                      // target_object is others
        if (m_core->is_owner_or_subsidiary(target_object)) {
            invoke_my_method(target_object, new_mb, &jargs[0],
                             m_user,
                             0, dummy, dummy->ostack_base);
        }
        else {

            Core* target_core =
                OoSpmtJvm::instance()->core_for_object(target_object);
            if (target_core) {      // target_object has a core
                assert(false);
                InvokeMsg* msg =
                    new InvokeMsg(target_object, new_mb, dummy, m_user, &jargs[0], dummy->ostack_base, 0, m_core->get_owner());
                m_core->halt();

                frame->last_pc = pc;
                transfer_certain_control(m_core, target_core, msg);
            }
            else {                  // target_object is coreless object
                invoke_my_method(target_object, new_mb, &jargs[0],
                                 m_user,
                                 0, dummy, dummy->ostack_base);
            }
        }

    }


    // run the nested step-loop
    void* ret2;
    ret2 = executeJava();
    //assert(ret == ret2);
    assert(m_core->m_mode->is_certain_mode());

    m_core->start();


    assert(m_core->m_owner == old_owner);
    assert(m_user == old_user);


    // restore (pc, frame, pc)
    frame = old_frame;
    sp = old_sp;
    pc = old_pc;
    m_user = old_user;

    assert(frame == dummy->prev);

    //destroy_frame(dummy);

    return ret;
}

void
CertainMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    if (intercept_vm_backdoor(target_object, new_mb)) return;

    log_when_invoke_return(true, m_user, frame->mb, target_object, new_mb);

    MiniLogger logger(cout, false);

    MINILOG_IF(debug_scaffold::java_main_arrived && is_app_obj(new_mb->classobj),
               logger,
               *new_mb);

    if (is_user_enclosure(target_object)) { // target_object is myself
        sp -= new_mb->args_count;
        invoke_my_method(target_object, new_mb, sp,
                         m_user,
                         pc, frame, sp);
    }
    else {                      // target_object is others
        if (m_core->is_owner_or_subsidiary(target_object)) {
            sp -= new_mb->args_count;
            invoke_my_method(target_object, new_mb, sp,
                             m_user,
                             pc, frame, sp);
        }
        else {
            Core* target_core =
                OoSpmtJvm::instance()->core_for_object(target_object);

            if (target_core) {      // target_object has a core
                //transfer certain control to target core
                sp -= new_mb->args_count;

                InvokeMsg* msg =
                    new InvokeMsg(target_object, new_mb, frame, m_user, sp, sp, pc, m_core->get_owner());

                frame->last_pc = pc;
                frame->snapshoted = true;
                transfer_certain_control(m_core, target_core, msg);
            }
            else { // target_object is a coreless object
                sp -= new_mb->args_count;
                invoke_my_method(target_object, new_mb, sp,
                                 m_user,
                                 pc, frame, sp);
            }
        }
    }

}

void
CertainMode::do_method_return(int len)
{
//     if (is_application_class(frame->mb->classobj)) {
//         MINILOG0("#" << m_core->id() << " (C)return from " << *frame->mb);
//     }
    log_when_invoke_return(false, frame->calling_object, frame->prev->mb, m_user, frame->mb);

    Object* target_object = frame->calling_object;

    if (is_user_enclosure(target_object)) { // target_object is myself
        return_my_method(frame, sp - len, len);
    }
    else {                      // target_object is others
        if (m_core->is_owner_or_subsidiary(target_object)) {
            if (m_core->has_message_to_be_verified()) {
                // ReturnMsg* msg = new ReturnMsg(frame->mb, frame->prev, sp - len, len,
                //                                frame->caller_sp, frame->caller_pc, frame);
                ReturnMsg* msg = new ReturnMsg(frame->object, frame->mb, frame->prev,
                                               frame->calling_object, sp - len, len,
                                               frame->caller_sp, frame->caller_pc);
                destroy_frame(frame);
                m_core->verify_speculation(msg);
            }
            else {
                return_my_method(frame, sp - len, len);
            }

        }
        else {
            //{{{ just for debug
            if (m_core->id() == 8) {
                int x = 0;
                x++;
            }
            //}}} just for debug
            Core* target_core =
                OoSpmtJvm::instance()->core_for_object(target_object);

            if (target_core == 0) {
                Object* calling_owner = frame->calling_owner;
                Core* c =
                    OoSpmtJvm::instance()->core_for_object(calling_owner);
                if (c) {
                    int x = 0;
                    x++;
                    target_core = c;
                }
            }

            if (target_core) {      // target_object has a core
                assert(frame->prev->mb); // dummy

                //transfer certain control to target core
                sp -= len;

                // ReturnMsg* msg = new ReturnMsg(frame->mb, frame->prev, sp, len,
                //                                frame->caller_sp, frame->caller_pc, frame);
                ReturnMsg* msg = new ReturnMsg(frame->object, frame->mb, frame->prev,
                                               frame->calling_object, sp, len,
                                               frame->caller_sp, frame->caller_pc);
                m_core->clear_frame_in_cache(frame);
                destroy_frame(frame);
                transfer_certain_control(m_core, target_core, msg);
            }
            else {                  // target_object is coreless object
                return_my_method(frame, sp - len, len);
            }
        }
    }
}


void
CertainMode::invoke_my_method(Object* target_object, MethodBlock* new_mb, uintptr_t* args,
                              Object* calling_object,
                              CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                              Object* calling_owner)
{
    //{{{ just for debug
    if (strcmp("forName", new_mb->name) == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    MINILOGPROC(c_user_change_logger, show_user_change,
                (os, m_core->id(), "(C)",
                 m_user, target_object, 1, caller_frame->mb, new_mb));
    change_user(target_object);


    caller_frame->last_pc = pc;

    frame = create_frame(target_object, new_mb, caller_frame, calling_object, args, caller_sp, caller_pc, calling_owner);

    if (frame->mb->is_synchronized()) {
        Object *sync_ob = frame->mb->is_static() ?
            frame->mb->classobj : (Object*)frame->lvars[0]; // lvars[0] is 'this' reference
        objectLock(sync_ob);
    }

    if (new_mb->is_native()) {
        // copy args to ostack
        if (args) std::copy(args, args + new_mb->args_count, frame->ostack_base);

        sp = (*(uintptr_t *(*)(Class*, MethodBlock*, uintptr_t*))
              new_mb->native_invoker)(new_mb->classobj, new_mb,
                                      frame->ostack_base);

        if (exception) {
            throw_exception;
        }
        else {
            return_my_method(frame, frame->ostack_base, sp - frame->ostack_base);
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
    MINILOGPROC(c_user_change_logger, show_user_change,
                (os, m_core->id(), "(C)",
                 m_user, current_frame->calling_object, 2,
                 current_frame->mb, current_frame->prev->mb));

    change_user(current_frame->calling_object);

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
    MINILOG(c_exception_logger, "#" << m_core->id()
            << " (C) exception detected!!! " << exception_class->name());
    // do nothing
}

void
CertainMode::do_throw_exception()
{
    MINILOG(c_exception_logger, "#" << m_core->id() << " (C) throw exception");
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

    /* If we didn't find a handler, restore exception and
       return to previous invocation */
    if (pc == NULL) {
        assert(false);          // when uncaughed exception ocurr, we get here
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
CertainMode::do_get_field(Object* target_object, FieldBlock* fb,
                          uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    //{{{ just for debug
    if (strcmp(frame->mb->name, "main") == 0) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    sp -= is_static ? 0 : 1;

    if (is_user_enclosure(target_object)) { // target_object is myself
        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }
    }
    else {                      // target_object is others
        if (m_core->is_owner_or_subsidiary(m_user)) { // i am owner
            if (m_core->has_message_to_be_verified()) {
                GetMsg* msg = new GetMsg(target_object, fb, addr, size, frame, sp, pc);
                m_core->verify_speculation(msg);
                return;
            }
            else {
                for (int i = 0; i < size; ++i) {
                    write(sp, read(addr + i));
                    sp++;
                }
            }
        }
        else {                  // i am guest
            for (int i = 0; i < size; ++i) {
                write(sp, read(addr + i));
                sp++;
            }
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

    sp -= size;

    if (is_user_enclosure(target_object)) { // target_object is myself
        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }
    }
    else {                      // target_object is others
        if (m_core->is_owner_or_subsidiary(target_object)) {
            // guest put owner.field or subsidiary.field
            PutMsg* msg = new PutMsg(target_object, fb, addr, sp, size, is_static, 0);
            m_core->verify_speculation(msg);
        }
        else {
            Core* target_core =
                OoSpmtJvm::instance()->core_for_object(target_object);

            if (target_core) {      // target_object has a core
                PutMsg* msg = new PutMsg(target_object, fb, addr, sp, size, is_static, m_core);

                sp -= is_static ? 0 : 1;
                pc += 3;
                transfer_certain_control(m_core, target_core, msg);

                return;         // avoid sp-=... and pc += ...
            }
            else { // target_object is a coreless object
                for (int i = 0; i < size; ++i) {
                    write(addr + i, read(sp + i));
                }

                // AckMsg* ack = new AckMsg;
                // m_core->verify_speculation(ack);
            }
        }
    }

    sp -= is_static ? 0 : 1;
    pc += 3;
}

void
CertainMode::do_array_load(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    sp -= 2; // pop up arrayref and index

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value
    Object* target_object = array;

    if (is_user_enclosure(target_object)) { // target_object is myself
        load_from_array(sp, addr, type_size);
        sp += nslots;
    }
    else {                      // target_object is others
        if (m_core->is_owner_or_subsidiary(m_user)) { // i am owner

            if (m_core->has_message_to_be_verified()) {
                ArrayLoadMsg* msg =
                    new ArrayLoadMsg(array, index, (uint8_t*)addr, type_size, frame, sp, pc);
                m_core->verify_speculation(msg);
                return;
            }
            else {
                load_from_array(sp, addr, type_size);
                sp += nslots;
            }
        }
        else {                  // i am guest
            load_from_array(sp, addr, type_size);
            sp += nslots;
        }
    }

    pc += 1;
}

void
CertainMode::do_array_store(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    // if target object has a core, target core should verify
    // target object may be others or owner of this core

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value
    Object* target_object = array;

    sp -= nslots;

    if (is_user_enclosure(target_object)) { // target_object is myself
        store_to_array(sp, addr, type_size);
    }
    else {                      // target_object is others
        if (m_core->is_owner_or_subsidiary(target_object)) {
            // guest store owner[i] or subsidiary[i]
            ArrayStoreMsg* msg = new ArrayStoreMsg(array, index, sp, nslots, type_size, 0);
            m_core->verify_speculation(msg);
        }
        else {

            Core* target_core =
                OoSpmtJvm::instance()->core_for_object(target_object);

            if (target_core) {      // target_object has a core
                ArrayStoreMsg* msg = new ArrayStoreMsg(array, index, sp, nslots, type_size,
                                                       m_core);
                sp -= 2;                    // pop up arrayref and index
                pc += 1;
                transfer_certain_control(m_core, target_core, msg);

                return;         // avoid sp-=... and pc += ...
            }
            else { // target_object is a coreless object
                store_to_array(sp, addr, type_size);
            }
        }
    }

    sp -= 2;                    // pop up arrayref and index
    pc += 1;
}

void
CertainMode::before_alloc_object()
{
}

void
CertainMode::after_alloc_object(Object* obj)
{
    if (not OoSpmtJvm::do_spec) return;

    if (is_speculative_object(obj)) {
        MINILOG_IF(debug_scaffold::java_main_arrived,
                   c_new_main_logger,
                   "#" << m_core->id() << " (C) new main " << obj->classobj->name());

        Core* core = OoSpmtJvm::instance()->alloc_core_for_object(obj);
        if (core) {
            threadSelf()->add_core(core);
        }
    }
    else {
        if (m_core->has_owner() && m_core->is_owner_or_subsidiary(m_user)) {
            MINILOG_IF(debug_scaffold::java_main_arrived,
                       c_new_sub_logger,
                       "#" << m_core->id() << " (C) new sub " << obj->classobj->name());

            m_core->add_subsidiary(obj);
        }
    }
}

bool
CertainMode::verify(Message* message)
{
    // stat
    m_core->m_count_verify_all++;


    assert(m_core->is_correspondence_btw_msgs_and_snapshots_ok());

    bool ok = false;
    if (not m_core->m_messages_to_be_verified.empty()) {
        Message* spec_msg = m_core->m_messages_to_be_verified.front();
        //m_core->m_messages_to_be_verified.pop_front();


        if (*spec_msg == *message) {
            ok = true;
        }

        if (ok) {
            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << m_core->id() << " verify " << *message << " OK");

            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << m_core->id() << " veri spec: " << *spec_msg << " OK");
        }
        else {
            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << m_core->id() << " verify " << *message << " FAILED"
                        << " not " << *spec_msg);

            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << m_core->id() << " veri spec: " << *spec_msg << " FAILED");

            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << m_core->id() << " details:");
            MINILOGPROC_IF(debug_scaffold::java_main_arrived,
                           verify_detail_logger, show_msg_detail,
                           (os, m_core->id(), message));
            MINILOG0_IF(debug_scaffold::java_main_arrived,
                        "#" << m_core->id() << " ---------");
            MINILOGPROC_IF(debug_scaffold::java_main_arrived,
                           verify_detail_logger, show_msg_detail,
                           (os, m_core->id(), spec_msg));

        }

        //delete spec_msg;
    }
    else {
        MINILOG0_IF(debug_scaffold::java_main_arrived,
                    "#" << m_core->id() << " verify " << *message << " EMPTY");
        MINILOG0_IF(debug_scaffold::java_main_arrived,
                    "#" << m_core->id() << " veri spec: EMPTY");

        // stat
        m_core->m_count_verify_empty++;
    }

    return ok;
}

void
CertainMode::verify_speculation(Message* message, bool self)
{
    //assert(m_user == m_owner);
    bool success = verify(message);

    // stat
    if (success)
        m_core->m_count_verify_ok++;
    else
        m_core->m_count_verify_fail++;

    if (success) {
        handle_verification_success(message, self);
    }
    else {
        //m_core->discard_uncertain_execution(self);
        handle_verification_failure(message, self);
    }
}

void
CertainMode::handle_verification_success(Message* message, bool self)
{
    bool should_reset_spec_exec = false;

    if (m_core->m_snapshots_to_be_committed.empty()) {
        m_core->sync_certain_with_speculative();

        if (frame) {
            //assert(m_core->is_owner_or_subsidiary(frame->get_object()));
            assert(is_sp_ok(sp, frame));
            assert(is_pc_ok(pc, frame->mb));
        }

        MINILOG(commit_logger,
                "#" << m_core->id()
                << " commit to latest, ver(" << m_core->m_cache.version() << ")");
        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << m_core->id() << " ostack base: " << frame->ostack_base);
        MINILOGPROC(cache_logger, show_cache,
                    (os, m_core->id(), m_core->m_cache, false));
        //}}} just for debug
        m_core->m_cache.commit(m_core->m_cache.version());
        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << m_core->id() << " ostack base: " << frame->ostack_base);
        MINILOGPROC(cache_logger, show_cache,
                    (os, m_core->id(), m_core->m_cache, false));
        //}}} just for debug
        // because has commited to latest version, cache should restart from ver 0
        should_reset_spec_exec = true;
        //{{{ just for debug
        if (m_core->m_id == 6 && message->get_type() == Message::ack) {
            int x = 0;
            x++;
        }
        //}}} just for debug
        MINILOG0_IF(debug_scaffold::java_main_arrived,
                    "#" << m_core->id() << " comt spec: latest");
    }
    else {
        Snapshot* snapshot = m_core->m_snapshots_to_be_committed.front();
        m_core->m_snapshots_to_be_committed.pop_front();
        m_core->sync_certain_with_snapshot(snapshot);

        if (message->get_type() == Message::ret && frame) {
            assert(m_core->is_owner_enclosure(frame->get_object()));
            assert(is_sp_ok(sp, frame));
            assert(is_pc_ok(pc, frame->mb));
        }

        MINILOG(commit_logger,
                "#" << m_core->id() << " commit to ver(" << snapshot->version << ")");

        MINILOG0_IF(debug_scaffold::java_main_arrived,
                    "#" << m_core->id() << " comt spec: " << *snapshot->spec_msg);

        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << m_core->id() << " ostack base: " << frame->ostack_base);
        MINILOGPROC(cache_logger, show_cache,
                    (os, m_core->id(), m_core->m_cache, false));
        //}}} just for debug
        m_core->m_cache.commit(snapshot->version);
        //{{{ just for debug
        MINILOG(cache_logger,
                "#" << m_core->id() << " ostack base: " << frame->ostack_base);
        MINILOGPROC(cache_logger, show_cache,
                    (os, m_core->id(), m_core->m_cache, false));
        //}}} just for debug

        // if (snapshot->version == m_core->m_cache.prev_version()) {
        //     //{{{ just for debug
        //     int x = 0;
        //     x++;
        //     //}}} just for debug

        //     //should_reset_spec_exec = true;
        // }

        delete snapshot;
    }

    if (message->get_type() != Message::put
        and message->get_type() != Message::arraystore) {
        m_core->mark_frame_certain();
    }

    assert(not m_core->m_messages_to_be_verified.empty());
    Message* spec_msg = m_core->m_messages_to_be_verified.front();
    m_core->m_messages_to_be_verified.pop_front();
    delete spec_msg;

    //if (should_reset_spec_exec) {
    if (not m_core->has_message_to_be_verified()) {
        m_core->discard_uncertain_execution(self);
    }

    MINILOG(commit_detail_logger,
            "#" << m_core->id() << " CERT details:");
    MINILOGPROC(commit_detail_logger, show_triple,
                (os, m_core->id(),
                 frame, sp, pc, m_user,
                 true));

    if (message->get_type() == Message::put) {
        PutMsg* msg = static_cast<PutMsg*>(message);

        AckMsg* ack = new AckMsg;
        transfer_certain_control(m_core, msg->sender, ack);
    }

    if (message->get_type() == Message::arraystore) {
        ArrayStoreMsg* msg = static_cast<ArrayStoreMsg*>(message);

        AckMsg* ack = new AckMsg;
        transfer_certain_control(m_core, msg->sender, ack);
    }

    delete message;
}

void
CertainMode::handle_verification_failure(Message* message, bool self)
{
    m_core->discard_uncertain_execution(self);

    if (message->get_type() == Message::call) {
        InvokeMsg* msg = static_cast<InvokeMsg*>(message);
        invoke_my_method(msg->object, msg->mb, &msg->parameters[0],
                         msg->calling_object,
                         msg->caller_pc, msg->caller_frame, msg->caller_sp, msg->calling_owner);
        //cout << "#" << m_core->id() << " handle, create " << *frame << endl;
    }
    else if (message->get_type() == Message::ret) {
        //{{{ just for debug
        if (m_core->id() == 0) {
            int x = 0;
            x++;
        }
        //}}} just for debug
        ReturnMsg* msg = static_cast<ReturnMsg*>(message);
        //return_my_method(msg->frame, &msg->retval[0], msg->retval.size());
        change_user(msg->calling_object);
        uintptr_t* caller_sp = msg->caller_sp;
        for (int i = 0; i < msg->retval.size(); ++i) {
            *caller_sp++ = msg->retval[i];
        }
        if (msg->mb->is_synchronized()) {
            Object *sync_ob = msg->mb->is_static() ?
                msg->mb->classobj : (Object*)msg->object; // lvars[0] is 'this' reference
            objectUnlock(sync_ob);
        }

        assert(msg->caller_frame->mb);
        // whether native or not

        sp = caller_sp;
        pc = msg->caller_pc;
        frame = msg->caller_frame;
        pc += (*msg->caller_pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    }
    else if (message->get_type() == Message::get) {
        //change_user()
        GetMsg* msg = static_cast<GetMsg*>(message);

        // to trap some exceptions
        assert(pc == msg->caller_pc);
        assert(frame == msg->caller_frame);
        //assert(m_certain_mode.sp == msg->caller_sp);

        //m_user = msg->obj;
        pc = msg->caller_pc;
        frame = msg->caller_frame;
        sp = msg->caller_sp;

        assert(is_pc_ok(pc, frame->mb));
        assert(is_sp_ok(sp, frame));

        vector<uintptr_t>::iterator begin = msg->val.begin();
        vector<uintptr_t>::iterator end = msg->val.end();
        for (vector<uintptr_t>::iterator i = begin; i != end; ++i) {
            *sp++ = *i;
        }

        pc += 3;
    }
    else if (message->get_type() == Message::put) {
        PutMsg* msg = static_cast<PutMsg*>(message);

        for (int i = 0; i < msg->val.size(); ++i) {
            write(msg->addr + i, msg->val[i]);
        }

        if (msg->sender) {
            m_core->halt();
            AckMsg* ack = new AckMsg;
            transfer_certain_control(m_core, msg->sender, ack);
        }
    }
    else if (message->get_type() == Message::arrayload) {
        ArrayLoadMsg* msg = static_cast<ArrayLoadMsg*>(message);

        // to trap some exceptions
        assert(pc == msg->caller_pc);
        assert(frame == msg->caller_frame);
        //assert(m_certain_mode.sp == msg->caller_sp);

        //m_user = msg->array;
        pc = msg->caller_pc;
        frame = msg->caller_frame;
        sp = msg->caller_sp;

        assert(is_pc_ok(pc, frame->mb));
        assert(is_sp_ok(sp, frame));

        int type_size = msg->val.size();
        load_array_from_no_cache_mem(sp, &msg->val[0], type_size);
        // vector<uintptr_t>::iterator begin = msg->val.begin();
        // vector<uintptr_t>::iterator end = msg->val.end();
        // for (vector<uintptr_t>::iterator i = begin; i != end; ++i) {
        //     *sp++ = *i;
        // }
        sp += type_size > 4 ? 2 : 1;

        pc += 1;
    }
    else if (message->get_type() == Message::arraystore) {
        ArrayStoreMsg* msg = static_cast<ArrayStoreMsg*>(message);

        Object* array = msg->array;
        int index = msg->index;
        int type_size = msg->type_size;

        void* addr = array_elem_addr(array, index, type_size);
        store_array_from_no_cache_mem(&msg->slots[0], addr, type_size);

        // for (int i = 0; i < msg->val.size(); ++i) {
        //     write(addr + i, msg->val[i]);
        // }

        if (msg->sender) {
            m_core->halt();
            AckMsg* ack = new AckMsg;
            transfer_certain_control(m_core, msg->sender, ack);
        }
    }
    else {
        assert(false);
    }

    delete message;
}

void
CertainMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(c_invoke_return_logger, is_invoke, m_core->id(), "(C)",
                      caller, caller_mb, callee, callee_mb);
}
