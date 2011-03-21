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

void*
CertainMode::do_execute_method(Object* target_object,
                               MethodBlock *new_mb,
                               std::vector<uintptr_t>& jargs)
{
    // save (pc, frame, sp) triples
    CodePntr old_pc = pc;
    Frame* old_frame = frame;
    uintptr_t* old_sp = sp;


    // dummy frame is used to receive RV of top_frame
    // dummy->prev is current frame
    Frame* dummy = create_frame(0, 0, frame, frame->get_object(), 0, 0, 0);
    dummy->_name_ = "dummy frame";

    void *ret;
    ret = dummy->ostack_base;


    //Object* old_user = m_user;


    Object* current_object = frame->get_object();

    Group* current_group = current_object ? current_object->get_group() : 0;
    Group* target_group = target_object->get_group();

    if (target_group == get_group() or target_group == 0) { // target object is in my group or no group

        invoke_my_method(target_object, new_mb, &jargs[0],
                         frame->get_object(),
                         0, dummy, dummy->ostack_base);

    }
    else {                      // target object is in other groups

        assert(false);          // todo

        Core* target_core = target_group->get_core();

        InvokeMsg* msg =
            new InvokeMsg(target_object, new_mb, dummy, frame->get_object(), &jargs[0], dummy->ostack_base, 0);
        m_core->halt();

        frame->last_pc = pc;
        transfer_certain_control(m_core, target_core, msg);

        // if (not current_group) { // current object is in no group
        //     m_core->halt();
        // }
    }

    // run the nested step-loop
    void* ret2;
    ret2 = executeJava();
    //assert(ret == ret2);
    assert(m_core->m_mode->is_certain_mode());

    m_core->start();


    //assert(m_core->m_owner == old_owner);
    //assert(m_user == old_user);


    // restore (pc, frame, pc)
    frame = old_frame;
    sp = old_sp;
    pc = old_pc;

    assert(frame == dummy->prev);

    //destroy_frame(dummy);

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

    Object* current_object = frame->get_object();

    Group* current_group = current_object ? current_object->get_group() : 0;
    Group* target_group = target_object->get_group();


    if (target_group == get_group() or target_group == 0) { // target object is in my group or no group
        sp -= new_mb->args_count;
        invoke_my_method(target_object, new_mb, sp,
                         frame->get_object(),
                         pc, frame, sp);
    }
    else {                      // target object is in other groups

        Core* target_core = target_group->get_core();

        //transfer certain control to target core
        sp -= new_mb->args_count;

        InvokeMsg* msg =
            new InvokeMsg(target_object, new_mb, frame, frame->get_object(), sp, sp, pc);

        frame->last_pc = pc;
        frame->snapshoted = true;
        transfer_certain_control(m_core, target_core, msg);

        if (not current_group) { // current object is in no group
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

    Object* current_object = frame->get_object();
    Object* target_object = frame->calling_object;

    Group* current_group = current_object ? current_object->get_group() : 0;
    Group* target_group = target_object ? target_object->get_group() : 0;

    if (target_group == get_group() or target_group == 0) { // target object is in my group or no group
        return_my_method(frame, sp - len, len);
    }
    else {                      // target object is in other groups

        Core* target_core = target_group->get_core();

        if (frame->is_top_frame()) { // prev is dummy
            assert(false);      // todo

        }

        //transfer certain control to target core
        sp -= len;

        ReturnMsg* msg = new ReturnMsg(frame->object, frame->mb, frame->prev,
                                       frame->calling_object, sp, len,
                                       frame->caller_sp, frame->caller_pc);
        m_core->clear_frame_in_cache(frame);
        destroy_frame(frame);
        transfer_certain_control(m_core, target_core, msg);

        if (not current_group) { // current object is in no group
            m_core->halt();
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

    Object* current_object = frame->get_object();
    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    sp -= is_static ? 0 : 1;

    if (current_group == get_group() and (target_group == 0 or target_group != get_group())
        and m_core->has_message_to_be_verified()) {

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

    if (target_group == get_group() or target_group == 0) { // target object is in my group or no group

        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }

    }
    else {                      // target object is in other groups


        Core* target_core = target_object->get_group()->get_core();
        assert(target_core);

        PutMsg* msg = new PutMsg(target_object, fb, addr, sp, size, is_static, m_core);

        sp -= is_static ? 0 : 1;
        pc += 3;
        transfer_certain_control(m_core, target_core, msg);

        if (not current_group) { // current object is in no group
            m_core->halt();
        }

        return;         // avoid sp-=... and pc += ...
    }

    sp -= is_static ? 0 : 1;
    pc += 3;
}

void
CertainMode::do_array_load(Object* array, int index, int type_size)
{
    Object* target_object = array;
    Object* current_object = frame->get_object();
    Group* target_group = target_object->get_group();
    Group* current_group = current_object->get_group();

    sp -= 2; // pop up arrayref and index

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value

    if (current_group == get_group() and (target_group == 0 or target_group != get_group())
        and m_core->has_message_to_be_verified()) {

        ArrayLoadMsg* msg =
            new ArrayLoadMsg(array, index, (uint8_t*)addr, type_size, frame, sp, pc);
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

    if (target_group == get_group() or target_group == 0) { // target object is in my group or no group

        store_to_array(sp, addr, type_size);

    }
    else {                      // target object is in other groups


        Core* target_core = target_object->get_group()->get_core();
        assert(target_core);

        ArrayStoreMsg* msg = new ArrayStoreMsg(array, index, sp, nslots, type_size,
                                               m_core);
        sp -= 2;                    // pop up arrayref and index
        pc += 1;
        transfer_certain_control(m_core, target_core, msg);

        if (not current_group) { // current object is in no group
            m_core->halt();
        }

        return;         // avoid sp-=... and pc += ...
    }

    sp -= 2;                    // pop up arrayref and index
    pc += 1;
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

        m_core->m_messages_to_be_verified.pop_front();
        delete spec_msg;
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
        handle_verification_failure(message, self);
    }

    delete message;
}

void
CertainMode::handle_verification_success(Message* message, bool self)
{
    //bool should_reset_spec_exec = false;

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
        //should_reset_spec_exec = true;

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
            //assert(m_core->is_owner_enclosure(frame->get_object()));
            assert(get_group() == frame->get_object()->get_group());
            assert(is_sp_ok(sp, frame));
            assert(is_pc_ok(pc, frame->mb));
        }

        MINILOG(commit_logger,
                "#" << m_core->id() << " commit to ver(" << snapshot->version << ")");

        // MINILOG0_IF(debug_scaffold::java_main_arrived,
        //             "#" << m_core->id() << " comt spec: " << *snapshot->spec_msg);

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


    //if (should_reset_spec_exec) {
    if (not m_core->has_message_to_be_verified()) {
        m_core->discard_uncertain_execution(self);
    }

    MINILOG(commit_detail_logger,
            "#" << m_core->id() << " CERT details:");
    // MINILOGPROC(commit_detail_logger, show_triple,
    //             (os, m_core->id(),
    //              frame, sp, pc, m_user,
    //              true));

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
}

void
CertainMode::handle_verification_failure(Message* message, bool self)
{
    m_core->reload_speculative_tasks();
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
}

void
CertainMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(c_invoke_return_logger, is_invoke, m_core->id(), "(C)",
                      caller, caller_mb, callee, callee_mb);
}
