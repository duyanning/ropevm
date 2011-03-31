#include "std.h"
#include "jam.h"
#include "SpeculativeMode.h"
#include "Core.h"

#include "lock.h"

#include "Message.h"
#include "OoSpmtJvm.h"
#include "interp.h"
#include "MiniLogger.h"
#include "Loggers.h"
#include "Snapshot.h"
#include "Helper.h"
#include "frame.h"
#include "Group.h"

using namespace std;


SpeculativeMode::SpeculativeMode()
:
    //Mode("Speculative mode")
    UncertainMode("Speculative mode")
{
}

uint32_t
SpeculativeMode::mode_read(uint32_t* addr)
{
    return m_core->m_states_buffer.read(addr);
}

void
SpeculativeMode::mode_write(uint32_t* addr, uint32_t value)
{
    m_core->m_states_buffer.write(addr, value);
}

void
SpeculativeMode::step()
{
    assert(OoSpmtJvm::do_spec);

    Message* msg = m_core->get_certain_message();
    if (msg) {
        process_certain_message(msg);
        return;
    }

    if (m_core->m_is_waiting_for_task) {
        MINILOG(task_load_logger, "#" << m_core->id() << " is waiting for task");
        //assert(false);

        // if (not m_core->from_certain_to_spec)
        //     m_core->snapshot();
        load_next_task();

        // if (not m_core->m_is_waiting_for_task) {
        //     exec_an_instr();
        // }
    }

    if (not m_core->is_halt())
        exec_an_instr();
}

void
SpeculativeMode::do_invoke_method(Object* target_object, MethodBlock* new_mb)
{
    if (intercept_vm_backdoor(target_object, new_mb)) return;

    //log_when_invoke_return(true, m_user, frame->mb, target_object, new_mb);

    MINILOG(s_logger,
            "#" << m_core->id() << " (S) is to invoke method: " << *new_mb);

    if (is_priviledged(new_mb)) {
        MINILOG(s_logger,
                "#" << m_core->id() << " (S) " << *new_mb << "is native/sync method");
        m_core->halt();
        return;
    }

    Object* current_object = frame->get_object();

    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    if (target_group == current_group) {

        sp -= new_mb->args_count;

        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }

        Frame* new_frame = create_frame(target_object, new_mb, frame, frame->get_object(), &args[0], sp, pc);
        new_frame->is_certain = false;

        frame->last_pc = pc;

        frame = new_frame;
        sp = (uintptr_t*)frame->ostack_base;
        pc = (CodePntr)frame->mb->code;

    }
    else {

        snapshot();             // yes

        sp -= new_mb->args_count;

        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }

        if (target_group->can_speculate()) {
            // post speculative message
            Core* target_core = target_group->get_core();
            MINILOG(s_logger,
                     "#" << m_core->id() << " (S) target object has a core #"
                     << target_core->id());

            InvokeMsg* msg =
                new InvokeMsg(target_object, new_mb, frame, frame->get_object(), &args[0], sp, pc);

            MINILOG(s_logger,
                     "#" << m_core->id() << " (S) add invoke task to #"
                    << target_core->id() << ": " << *msg);

            target_core->add_speculative_task(msg);
        }
        else {
            MINILOG(s_logger,
                    "#" << m_core->id() << " target object is coreless");
        }

        MethodBlock* pslice = get_rvp_method(new_mb);
        MINILOG(s_logger,
                 "#" << m_core->id() << " (S)calls p-slice: " << *pslice);

        Frame* new_frame =
            create_frame(target_object, pslice, frame, 0, &args[0], sp, pc);

        new_frame->is_certain = false;

        m_core->m_rvp_mode.pc = (CodePntr)pslice->code;
        m_core->m_rvp_mode.frame = new_frame;
        m_core->m_rvp_mode.sp = new_frame->ostack_base;

        m_core->switch_to_rvp_mode();

    }
}

void
SpeculativeMode::do_method_return(int len)
{
    assert(len == 0 || len == 1 || len == 2);
    assert(not is_priviledged(frame->mb));

    MINILOG(s_logger,
            // "#" << m_core->id() << " (S) is to return from method: " << *frame->mb
            // << " frame: " << frame);
            "#" << m_core->id() << " (S) is to return from " << *frame);

    if (frame->mb->is_synchronized()) {
        MINILOG(s_logger,
                "#" << m_core->id() << " (S) " << *frame->mb << " is a sync method");
        m_core->halt();
        return;
    }

    // if (frame->is_top_frame()) {
    //     MINILOG(s_logger,
    //             "#" << m_core->id() << " (S) " << *frame->mb << " is a top frame");
    //     m_core->halt();
    //     return;
    // }

    // if (frame->prev->is_alive()) {
    //     log_when_invoke_return(false, frame->calling_object, frame->prev->mb, m_user, frame->mb);
    // }

    Object* target_object = frame->calling_object;
    Object* current_object = frame->get_object();

    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    if (target_group == current_group) {

        //{{{ just for debug
        if (m_core->id() == 0) {
            int x = 0;
            x++;
        }
        //}}} just for debug
        sp -= len;
        uintptr_t* caller_sp = frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            write(caller_sp++, read(&sp[i]));
        }

        Frame* current_frame = frame;

        // MINILOGPROC(s_user_change_logger, show_user_change,
        //             (os, m_core->id(), "(S)",
        //              m_user, current_frame->calling_object, 2,
        //              current_frame->mb, current_frame->prev->mb));

        frame = current_frame->prev;
        sp = caller_sp;
        pc = current_frame->caller_pc;

        destroy_frame(current_frame);
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

    }
    else {

        snapshot();
        destroy_frame(frame);

        load_next_task();

    }
}

void
SpeculativeMode::before_signal_exception(Class *exception_class)
{
    MINILOG(s_exception_logger, "#" << m_core->id()
            << " (S) exception detected!!! " << exception_class->name());
    m_core->halt();
    throw DeepBreak();
}

void
SpeculativeMode::do_throw_exception()
{
    assert(false);
}

void
SpeculativeMode::enter_execution()
{
    m_core->m_speculative_depth++;
    //    cout << "enter speculative execution depth: " << m_core->m_speculative_depth << endl;
}

void
SpeculativeMode::leave_execution()
{
    //    cout << "leave speculative execution depth: " << m_core->m_speculative_depth << endl;
    m_core->m_speculative_depth--;
}

void
SpeculativeMode::destroy_frame(Frame* frame)
{
    if (not frame->snapshoted) {
        MINILOG(s_destroy_frame_logger, "#" << m_core->id()
                << " (S) destroy frame " << frame << " for " << *frame->mb);
        if (m_core->m_id == 5 && strcmp(frame->mb->name, "simulate") == 0) {
            int x = 0;
            x++;
        }
        // m_core->m_states_buffer.clear(frame->lvars, frame->lvars + frame->mb->max_locals);
        // m_core->m_states_buffer.clear(frame->ostack_base, frame->ostack_base + frame->mb->max_stack);
        m_core->clear_frame_in_cache(frame);
        //delete frame;
        Mode::destroy_frame(frame);
    }
}

void
SpeculativeMode::do_get_field(Object* source_object, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_core->id() << " (S) is to getfield: " << *fb
            //            << " of " << static_cast<void*>(target_object));
            << " of " << source_object);

    Object* current_object = frame->get_object();

    Group* current_group = current_object->get_group();
    Group* source_group = source_object->get_group();

    if (source_group == current_group) {

        sp -= is_static ? 0 : 1;
        for (int i = 0; i < size; ++i) {
            write(sp, read(addr + i));
            sp++;
        }
        pc += 3;

    }
    else {

        snapshot();

        // read all at one time, to avoid data change between two reading.
        vector<uintptr_t> val;
        for (int i = 0; i < size; ++i) {
            val.push_back(addr[i]);
        }

        sp -= is_static ? 0 : 1;

        GetMsg* msg = new GetMsg(source_object, current_object, fb, &val[0], size, frame, sp, pc);
        m_core->add_message_to_be_verified(msg);

        for (int i = 0; i < size; ++i) {
            write(sp, val[i]);
            sp++;
        }
        pc += 3;

    }

}

void
SpeculativeMode::do_put_field(Object* target_object, FieldBlock* fb,
                              uintptr_t* addr, int size, bool is_static)
{
    assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_core->id() << " (S) is to putfield: " << *fb);


    Object* current_object = frame->get_object();

    Group* current_group = current_object->get_group();
    Group* target_group = target_object->get_group();

    if (target_group == current_group) {

        sp -= size;
        for (int i = 0; i < size; ++i) {
            write(addr + i, read(sp + i));
        }
        sp -= is_static ? 0 : 1;
        pc += 3;

    }
    else {

        snapshot();

        if (target_group->can_speculate()) {

            Core* target_core = target_group->get_core();
            sp -= size;

            vector<uintptr_t> val;
            for (int i = 0; i < size; ++i) {
                val.push_back(read(sp + i));
            }

            sp -= is_static ? 0 : 1;
            pc += 3;

            MINILOG(s_logger,
                     "#" << m_core->id() << " (S) target object has a core #"
                     << target_core->id());

            PutMsg* msg = new PutMsg(current_object, target_object, fb, addr, &val[0], size, is_static);

            MINILOG(s_logger,
                     "#" << m_core->id() << " (S) add putfield task to #"
                    << target_core->id() << ": " << *msg);

            target_core->add_speculative_task(msg);
        }
        else {

            MINILOG(s_logger,
                    "#" << m_core->id() << " target object is coreless");
        }

    }
}

void
SpeculativeMode::do_array_load(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_core->id() << " (S) is to load from array");

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value
    assert(array != get_group()->get_leader()); // array never can be leader

    Object* source_object = array;
    Group* source_group = source_object->get_group();
    Object* current_object = frame->get_object();
    Group* current_group = current_object->get_group();

    if (source_group == current_group) {

        sp -= 2; // pop up arrayref and index
        load_from_array(sp, addr, type_size);
        sp += nslots;

        pc += 1;

    }
    else {

        snapshot();

        // read all at one time, to avoid data change between two reading.
        uint8_t val[8]; // longest value is 8 bytes
        std::copy((uint8_t*)addr, (uint8_t*)addr + type_size, val); // get from certain memory

        sp -= 2;

        ArrayLoadMsg* msg = new ArrayLoadMsg(array, current_object, index, &val[0], type_size, frame, sp, pc);
        m_core->add_message_to_be_verified(msg);

        load_array_from_no_cache_mem(sp, &val[0], type_size);
        sp += nslots;

        pc += 1;
    }

}

void
SpeculativeMode::do_array_store(Object* array, int index, int type_size)
{
    //assert(size == 1 || size == 2);

    MINILOG(s_logger,
            "#" << m_core->id() << " (S) is to store to array");

    void* addr = array_elem_addr(array, index, type_size);
    int nslots = type_size > 4 ? 2 : 1; // number of slots for value
    assert(array != get_group()->get_leader()); // array never can be leader

    Object* target_object = array;
    Group* target_group = target_object->get_group();
    Object* current_object = frame->get_object();
    Group* current_group = current_object->get_group();

    if (target_group == current_group) {

        sp -= nslots; // pop up value
        store_to_array(sp, addr, type_size);
        sp -= 2;                    // pop arrayref and index

        pc += 1;

    }
    else {

        snapshot();

        if (target_group->can_speculate()) {      // target_object has a core
            Core* target_core = target_group->get_core();

            sp -= nslots; // pop up value

            uint32_t val[2]; // at most 2 slots
            for (int i = 0; i < nslots; ++i) {
                val[i] = read(sp + i);
            }

            sp -= 2;
            pc += 1;

            MINILOG(s_logger,
                     "#" << m_core->id() << " (S) target object has a core #"
                     << target_core->id());

            ArrayStoreMsg* msg =
                new ArrayStoreMsg(current_object, target_object, index, &val[0], nslots, type_size);

            MINILOG(s_logger,
                     "#" << m_core->id() << " (S) add arraystore task to #"
                    << target_core->id() << ": " << *msg);

            target_core->add_speculative_task(msg);
        }
        else {                  // target_object is a coreless object
            MINILOG(s_logger,
                    "#" << m_core->id() << " target object is coreless");
        }

        // AckMsg* ack = new AckMsg;
        // m_core->add_message_to_be_verified(ack);
    }
}

void*
SpeculativeMode::do_execute_method(Object* target_object,
                                   MethodBlock *mb,
                                   std::vector<uintptr_t>& jargs)
{
    MINILOG(step_loop_in_out_logger, "#" << m_core->id()
            << " (S) throw-> to be execute java method: " << *mb);
    m_core->halt();

    throw NestedStepLoop(m_core->id(), mb);

    return 0;

    assert(false);

    if (is_priviledged(mb)) {
        assert(false);
        m_core->halt();
        return 0;
    }
}

bool
SpeculativeMode::load_next_task()
{
    MINILOG(task_load_logger, "#" << m_core->id() << " try to load a task");

    if (m_core->m_speculative_tasks.empty()) { // there are NO tasks
        MINILOG(task_load_logger, "#" << m_core->id() << " no task, waiting for task");
        frame = 0;
        pc = 0;
        sp = 0;
        //m_user = 0;
        m_core->m_is_waiting_for_task = true;
        m_core->halt();
    }
    else {                      // has tasks
        m_core->m_is_waiting_for_task = false;
        Message* task_msg = m_core->m_speculative_tasks.front();
        m_core->m_speculative_tasks.pop_front();

        MINILOG(task_load_logger, "#" << m_core->id() << " task loaded: " << *task_msg);
        if (task_msg->get_type() == Message::invoke) {

            InvokeMsg* msg = static_cast<InvokeMsg*>(task_msg);

            //{{{ just for debug
            if (m_core->id() == 7 && strcmp("hasMoreElements", msg->mb->name) == 0) {
                int x = 0;
                x++;
            }
            //}}} just for debug
            MethodBlock* new_mb = msg->mb;

            if (is_priviledged(new_mb)) {
                MINILOG(s_logger,
                        "#" << m_core->id() << " (S) " << *new_mb << "is native/sync method");
                m_core->halt();
            }
            else {
                m_core->add_message_to_be_verified(msg);
                //snapshot();

                Frame* new_frame =
                    create_frame(msg->get_target_object(), new_mb, msg->caller_frame, msg->get_source_object(),
                                 &msg->parameters[0], msg->caller_sp, msg->caller_pc);
                new_frame->is_certain = false;
                new_frame->is_task_frame = true;

                // new_frame->xxx = 999;
                // cout << "#" << m_core->id() << " create xxx999 " << *new_frame << endl;

                //change_user(msg->object);
                sp = (uintptr_t*)new_frame->ostack_base;

                frame = new_frame;
                pc = (CodePntr)new_mb->code;
            }
        }
        else if (task_msg->get_type() == Message::put) {
            PutMsg* msg = static_cast<PutMsg*>(task_msg);
            m_core->add_message_to_be_verified(msg);

            //change_user(msg->obj);

            for (int i = 0; i < msg->val.size(); ++i) {
                write(msg->addr + i, msg->val[i]);
            }

            snapshot(false);
            load_next_task();
        }
        else if (task_msg->get_type() == Message::arraystore) {
            ArrayStoreMsg* msg = static_cast<ArrayStoreMsg*>(task_msg);
            m_core->add_message_to_be_verified(msg);

            Object* array = msg->get_target_object();
            int index = msg->index;
            int type_size = msg->type_size;

            //change_user(array);

            void* addr = array_elem_addr(array, index, type_size);
            store_array_from_no_cache_mem(&msg->slots[0], addr, type_size);

            // for (int i = 0; i < msg->val.size(); ++i) {
            //     m_speculative_mode.write(addr + i, msg->val[i]);
            // }

            snapshot(false);
            load_next_task();
        }
        else {
            assert(false);
        }
    }

    return not m_core->m_is_waiting_for_task;
}

void
SpeculativeMode::snapshot(bool shot_frame)
{
    //assert(m_core->is_owner_enclosure(m_user));

    if (shot_frame) {
        Frame* f = this->frame;
        //assert(f == 0 || m_core->is_owner_enclosure(f->get_object()));


        if (f) {
            for (;;) {
                MINILOG(snapshot_logger,
                        "#" << m_core->id() << " snapshot frame(" << f << ")" << " for " << *f->mb);

                if (f->snapshoted) break;
                f->snapshoted = true;

                if (f->calling_object->get_group() != get_group())
                    break;

                //if (f == m_core->m_certain_mode.frame) break;
                if (f->is_task_frame) break;
                if (f->is_top_frame()) break;

                f = f->prev;
            }
        }
    }

    Snapshot* snapshot = new Snapshot;

    snapshot->version = m_core->m_states_buffer.version();
    m_core->m_states_buffer.freeze();
    // //{{{ just for debug
    // MINILOG_IF(m_core->id() == 5,
    //            cache_logger,
    //            "#" << m_core->id() << " ostack base: " << frame->ostack_base);
    // MINILOGPROC_IF(m_core->id() == 5,
    //                cache_logger, show_cache,
    //                (os, m_core->id(), m_core->m_states_buffer, false));
    // //}}} just for debug
    //snapshot->user = this->m_user;
    snapshot->pc = this->pc;
    snapshot->frame = shot_frame ? this->frame : 0;
    snapshot->sp = this->sp;

    //{{{ just for debug
    assert(m_core->has_message_to_be_verified());
    snapshot->spec_msg = m_core->m_messages_to_be_verified.back();
    MINILOG0("#" << m_core->id() << " shot spec: " << *snapshot->spec_msg);
    //}}} just for debug


    MINILOG(snapshot_logger,
            "#" << m_core->id() << " snapshot, ver(" << snapshot->version << ")" << " is frozen");
    if (shot_frame) {
        MINILOG(snapshot_detail_logger,
                "#" << m_core->id() << " details:");
        MINILOGPROC(snapshot_detail_logger, show_snapshot,
                    (os, m_core->id(), snapshot));
    }


    //{{{ just for debug
    // if (snapshot->frame->c == 23808) {
    //     int x = 0;
    //     x++;
    // }
    //}}} just for debug
    if (shot_frame) {
        assert(snapshot->frame == 0 || is_sp_ok(snapshot->sp, snapshot->frame));
    }

    m_core->m_snapshots_to_be_committed.push_back(snapshot);
}

void
SpeculativeMode::log_when_invoke_return(bool is_invoke, Object* caller, MethodBlock* caller_mb,
                      Object* callee, MethodBlock* callee_mb)
{
    log_invoke_return(s_invoke_return_logger, is_invoke, m_core->id(), "(S)",
                      caller, caller_mb, callee, callee_mb);
}
