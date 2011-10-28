#include "std.h"
#include "rope.h"
#include "SpeculativeMode.h"
#include "SpmtThread.h"

#include "lock.h"

#include "Message.h"
#include "RopeVM.h"
#include "interp.h"
#include "MiniLogger.h"
#include "Loggers.h"
#include "Snapshot.h"
#include "Helper.h"
#include "frame.h"
#include "Group.h"
#include "Effect.h"

using namespace std;


SpeculativeMode::SpeculativeMode()
:
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
    assert(RopeVM::do_spec);

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
        process_next_spec_msg();

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
        m_core->sleep();
        return;
    }

    SpmtThread* me = this_spmt_thread();
    SpmtThread* target_spmt_thread = target_object->get_group()->get_spmt_thread();

    if (target_spmt_thread == me) {

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
        sp -= new_mb->args_count;

        std::vector<uintptr_t> args;
        for (int i = 0; i < new_mb->args_count; ++i) {
            args.push_back(read(&sp[i]));
        }


        // MINILOG(s_logger,
        //         "#" << m_core->id() << " (S) target object has a core #"
        //         << target_core->id());


        // construct speculative message
        InvokeMsg* msg =
            new InvokeMsg(target_object, new_mb, frame, frame->get_object(), &args[0], sp, pc);


        // MINILOG(s_logger,
        //         "#" << m_core->id() << " (S) add invoke task to #"
        //         << target_core->id() << ": " << *msg);


        // send spec msg to target thread
        target_spmt_thread->add_speculative_task(msg);

        // record spec msg sent in current effect
        me->get_current_effect()->add_spec_msg(msg);


        snapshot2();
        pin_frames();


        MethodBlock* rvp_method = get_rvp_method(new_mb);

        // MINILOG(s_logger,
        //          "#" << m_core->id() << " (S)calls p-slice: " << *pslice);

        Frame* new_frame =
            create_frame(target_object, rvp_method, frame, 0, &args[0], sp, pc);

        new_frame->is_certain = false;

        m_core->m_rvp_mode.pc = (CodePntr)rvp_method->code;
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

    Frame* current_frame = frame;

    MINILOG(s_logger,
            // "#" << m_core->id() << " (S) is to return from method: " << *frame->mb
            // << " frame: " << frame);
            "#" << m_core->id() << " (S) is to return from " << *frame);

    if (frame->mb->is_synchronized()) {
        MINILOG(s_logger,
                "#" << m_core->id() << " (S) " << *frame->mb << " is a sync method");
        m_core->sleep();
        return;
    }

    // top frame' caller is native code, so we sleep
    if (frame->is_top_frame()) {
        MINILOG(s_logger,
                "#" << m_core->id() << " (S) " << *frame->mb << " is a top frame");
        m_core->sleep();
        return;
    }


    SpmtThread* me = this_spmt_thread();
    Object* target_object = frame->calling_object;
    SpmtThread* target_spmt_thread = target_object->get_group()->get_spmt_thread();


    if (target_spmt_thread == me) {

        // //{{{ just for debug
        // if (m_core->id() == 0) {
        //     int x = 0;
        //     x++;
        // }
        // //}}} just for debug


        sp -= len;
        uintptr_t* caller_sp = current_frame->caller_sp;
        for (int i = 0; i < len; ++i) {
            write(caller_sp++, read(&sp[i]));
        }


        frame = current_frame->prev;
        sp = caller_sp;
        pc = current_frame->caller_pc;

        destroy_frame(current_frame);
        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

    }
    else {

        // construct speculative message
        ReturnMsg* msg =0;
        //new ReturnMsg();

        // record spec msg sent in current effect
        me->get_current_effect()->add_spec_msg(msg);


        snapshot();

        destroy_frame(current_frame);

        process_next_spec_msg();

    }
}

void
SpeculativeMode::before_signal_exception(Class *exception_class)
{
    MINILOG(s_exception_logger, "#" << m_core->id()
            << " (S) exception detected!!! " << exception_class->name());
    m_core->sleep();
    throw DeepBreak();
}

void
SpeculativeMode::do_throw_exception()
{
    assert(false);
}

void
SpeculativeMode::destroy_frame(Frame* frame)
{
    if (not frame->pinned) {
        MINILOG(s_destroy_frame_logger, "#" << m_core->id()
                << " (S) destroy frame " << frame << " for " << *frame->mb);
        if (m_core->m_id == 5 && strcmp(frame->mb->name, "simulate") == 0) {
            int x = 0;
            x++;
        }
        // m_core->m_states_buffer.clear(frame->lvars, frame->lvars + frame->mb->max_locals);
        // m_core->m_states_buffer.clear(frame->ostack_base, frame->ostack_base + frame->mb->max_stack);
        m_core->clear_frame_in_states_buffer(frame);
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

            SpmtThread* target_core = target_group->get_core();
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
            SpmtThread* target_core = target_group->get_core();

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
    m_core->sleep();

    throw DeepBreak();

    return 0;
}

bool
SpeculativeMode::process_next_spec_msg()
{
    MINILOG(task_load_logger, "#" << m_core->id() << " try to load a task");

    if (m_core->m_speculative_tasks.empty()) { // there are NO tasks
        MINILOG(task_load_logger, "#" << m_core->id() << " no task, waiting for task");
        pc = 0;
        frame = 0;
        sp = 0;
        m_core->m_is_waiting_for_task = true;
        m_core->sleep();
    }
    else {                      // has tasks
        m_core->m_is_waiting_for_task = false;
        Message* task_msg = m_core->m_speculative_tasks.front();
        m_core->m_speculative_tasks.pop_front();

        MINILOG(task_load_logger, "#" << m_core->id() << " task loaded: " << *task_msg);

        Message::Type type = task_msg->get_type();
        assert(
               type == Message::invoke
               or type == Message::put
               or type == Message::arraystore
               );

        if (type == Message::invoke) {

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
                m_core->sleep();
            }
            else {
                m_core->add_message_to_be_verified(msg);
                m_core->m_states_buffer.freeze();

                Frame* new_frame =
                    create_frame(msg->get_target_object(), new_mb, msg->caller_frame, msg->get_source_object(),
                                 &msg->parameters[0], msg->caller_sp, msg->caller_pc);
                new_frame->is_certain = false;
                new_frame->is_task_frame = true;

                sp = (uintptr_t*)new_frame->ostack_base;

                frame = new_frame;
                pc = (CodePntr)new_mb->code;
            }
        }
        else if (type == Message::put) {
            PutMsg* msg = static_cast<PutMsg*>(task_msg);
            m_core->add_message_to_be_verified(msg);

            for (int i = 0; i < msg->val.size(); ++i) {
                write(msg->addr + i, msg->val[i]);
            }

            snapshot(false);
            process_next_spec_msg();
        }
        else if (type == Message::arraystore) {
            ArrayStoreMsg* msg = static_cast<ArrayStoreMsg*>(task_msg);
            m_core->add_message_to_be_verified(msg);

            Object* array = msg->get_target_object();
            int index = msg->index;
            int type_size = msg->type_size;

            void* addr = array_elem_addr(array, index, type_size);
            store_array_from_no_cache_mem(&msg->slots[0], addr, type_size);

            // for (int i = 0; i < msg->val.size(); ++i) {
            //     m_speculative_mode.write(addr + i, msg->val[i]);
            // }

            snapshot(false);
            process_next_spec_msg();
        }
        else {
            assert(false);
        }
    }

    return not m_core->m_is_waiting_for_task;
}


void
SpeculativeMode::pin_frames()
{
    Frame* f = this->frame;

    for (;;) {
        // MINILOG(snapshot_logger,
        //         "#" << m_core->id() << " snapshot frame(" << f << ")" << " for " << *f->mb);

        if (f->pinned)
            break;

        f->pinned = true;

        if (f->is_top_frame())
            break;

        f = f->prev;
        if (f->calling_object->get_group()->get_spmt_thread() != this_spmt_thread())
            break;

    }
}


// refactor
// 不应调用pin_frames，因为不是所有的snapshot都伴随pin_frames
void
SpeculativeMode::snapshot2()
{
    Snapshot* snapshot = new Snapshot;

    SpmtThread* me = this_spmt_thread();

    snapshot->version = me->m_states_buffer.version();
    me->m_states_buffer.freeze();

    snapshot->pc = this->pc;
    snapshot->frame = this->frame;
    snapshot->sp = this->sp;

    get_current_effect()->set_snapshot(snapshot);


    // MINILOG(snapshot_logger,
    //         "#" << m_core->id() << " snapshot, ver(" << snapshot->version << ")" << " is frozen");
    // if (shot_frame) {
    //     MINILOG(snapshot_detail_logger,
    //             "#" << m_core->id() << " details:");
    //     MINILOGPROC(snapshot_detail_logger, show_snapshot,
    //                 (os, m_core->id(), snapshot));
    // }


}


// refactor
// 参数为true的话，将标记栈桢，表明栈桢在推测模式下不能被释放。
// false的话，则不标记栈桢。
// 目前，推测模式下return将标记栈桢；推测模式下执行完put不会标记栈桢。
// 重构1：不应当让本函数来承担标记栈桢的工作。应当另设一函数pin_frames来标记栈桢。
// 推测模式下return的时候，除了快照，还要调用该函数标记栈桢。
// 重构2：在snapshot中调用freeze
void
SpeculativeMode::snapshot(bool shot_frame)
{
    if (shot_frame) {
        Frame* f = this->frame;
        //assert(f == 0 || m_core->is_owner_enclosure(f->get_object()));


        if (f) {
            for (;;) {
                MINILOG(snapshot_logger,
                        "#" << m_core->id() << " snapshot frame(" << f << ")" << " for " << *f->mb);

                if (f->pinned) break;
                f->pinned = true;

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
    //m_core->m_states_buffer.freeze();

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
