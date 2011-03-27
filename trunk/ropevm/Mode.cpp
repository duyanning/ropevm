#include "std.h"
#include "jam.h"
#include "Mode.h"

#include "lock.h"
#include "Core.h"
#include "OoSpmtJvm.h"
#include "DebugScaffold.h"
#include "Helper.h"
#include "Loggers.h"
#include "jni.h"
#include "Group.h"

using namespace std;

Mode::Mode(const char* name)
:
    m_name(name)
{
    exception = 0;

    frame = 0;
    sp = 0;
    pc = 0;

    //m_user = 0;
}

const char*
Mode::get_name()
{
    return m_name;
}

bool
Mode::is_certain_mode()
{
    return *m_name == 'C';
}

bool
Mode::is_speculative_mode()
{
    return *m_name == 'S';
}

bool
Mode::is_rvp_mode()
{
    return *m_name == 'R';
}

void
Mode::set_core(Core* core)
{
    m_core = core;
}

void
Mode::before_alloc_object()
{
}

void
Mode::after_alloc_object(Object* new_object)
{
    //if (not OoSpmtJvm::do_spec) return;

    Group* group = assign_group_for(new_object);
    new_object->set_group(group);
    group->add(new_object);
}

// void vmlog(String msg)
void
Mode::vmlog(MethodBlock* mb)
{
    //assert(mb->is_native());
    //assert(not mb->is_static());
    //assert(*pc == )

    jstring javastr = (jstring*)read(sp-1);
    char* str = String2Utf8((Object*)javastr);
    cout << "vmlog: " << "#" << m_core->m_id << " " << tag() << " "
         << str
         << endl;
    sysFree(str);

    sp--;                       // pop msg
    if (not mb->is_static())
        sp--;                   // pop this
    pc += 3;
}

// void preload(String classname)
void
Mode::preload(MethodBlock* mb)
{
    jstring jclassname = (jstring*)read(sp-1);
    char* classname = String2Utf8((Object*)jclassname);

    // load

    sysFree(classname);

    sp--;                       // pop classname
    if (not mb->is_static())
        sp--;                   // pop this
    pc += 3;
}

bool
Mode::vm_math(MethodBlock* mb)
{
    //cerr << "vm_math: " << mb->name << " " << mb->type << endl;
    if (mb->name == copyUtf8("sqrt")) {
        double x = read((double *) &sp[-sizeof(double) / 4]);
        write((double *) &sp[-sizeof(double) / 4], sqrt(x));
        pc += 3;
        return true;
    }
    // else if (mb->name == copyUtf8("abs")) { // double, float, long, int
    //     double x = read((double *) &sp[-sizeof(double) / 4]);
    //     write((double *) &sp[-sizeof(double) / 4], abs(x));
    //     pc += 3;
    // }
    else {
        return false;
        cerr << "unexpected: " << mb->name << endl;
        assert(false);
    }
}

bool
Mode::intercept_vm_backdoor(Object* objectref, MethodBlock* mb)
{
    if (mb->classobj->name() == copyUtf8("java/lang/Math")) {
        return vm_math(mb);
    }

    if (mb->name == copyUtf8("vmlog")) {
        vmlog(mb);
        return true;
    }
    else if (mb->name == copyUtf8("preload")) {
        preload(mb);
        return true;
    }

    return false;
}

void
Mode::do_invoke_method(Object* objectref, MethodBlock* new_mb)
{
    assert(false);
}

void
Mode::do_method_return(int len)
{
    assert(false);
}

void
Mode::do_get_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(false);
}

void
Mode::do_put_field(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, bool is_static)
{
    assert(false);
}

void
Mode::do_array_load(Object* array, int index, int type_size)
{
    assert(false);
}

void
Mode::do_array_store(Object* array, int index, int type_size)
{
    assert(false);
}

// void
// Mode::do_array_length(Object* array)
// {
//     assert(false);
// }

void
Mode::destroy_frame(Frame* frame)
{
    //{{{ just for debug
    if (frame->c == 23808) {
        int x = 0;
        x++;
    }
    //}}} just for debug

    //assert(m_core->frame_is_not_in_snapshots(frame));
    delete frame;
}

// void
// Mode::handle_verification_failure(Message* message)
// {
//     assert(false);
// }

// bool
// Mode::verify(Message* message)
// {
//     assert(false);
// }

// void
// Mode::verify_speculation(Message* message, bool self)
// {
//     assert(false);
// }

void
show_triple(std::ostream& os, int id, Frame* frame, uintptr_t* sp, CodePntr pc, Object* user,
            bool more)
{
    if (frame) {
        if (frame->is_alive())
            os << "alive" << endl;
        if (frame->is_dead())
            os << "dead" << endl;
        if (frame->is_bad())
            os << "bad" << endl;
    }

    os << "#" << id << " frame = " << frame;
    if (more && frame && frame->mb) {
        os << " " << *frame->mb << "\n";
        os << "#" << id << " lvars = " << frame->lvars << "\n";
        os << "#" << id << " ostack_base = " << frame->ostack_base << "\n";
    }
    os << "\n";



    os << "#" << id << " sp = " << (void*)sp;
    if (more && frame) {
        os << " " << sp - frame->ostack_base;
    }
    os << "\n";

    os << "#" << id << " pc = " << (void*)pc;
    if (more && frame && frame->mb) {
        os << " " << pc - (CodePntr)frame->mb->code;
    }
    os << endl;

    os << "#" << id << " user = " << user;
    os << endl;
}

void log_invoke_return(MiniLogger& logger, bool is_invoke, int id, const char* tag,
                       Object* caller, MethodBlock* caller_mb,
                       Object* callee, MethodBlock* callee_mb)
{
    MINILOGPROC_IF(debug_scaffold::java_main_arrived
                   && caller && callee
                   && caller_mb && callee_mb
                   && (is_app_obj(caller->classobj) || is_app_obj(callee->classobj)),
                   logger, show_invoke_return,
                   (os, is_invoke, id, tag, caller, caller_mb, callee, callee_mb));
}

void
show_invoke_return(std::ostream& os, bool is_invoke, int id, const char* tag,
                   Object* caller, MethodBlock* caller_mb,
                   Object* callee, MethodBlock* callee_mb)
{
    os << "#" << id << " " << tag << " ";
    //os << (is_invoke ? "invoke " : "return ");
    os << caller;
    if (caller->get_group())
        os << "(#" << caller->get_group()->get_core()->id() << ")";
    else
        os << "(#none)";
    os << " ";
    os << *caller_mb;
    os << (is_invoke ? " ===>>> " : " <<<=== ");
    os << callee;
    if (callee->get_group())
        os << "(#" << callee->get_group()->get_core()->id() << ")";
    else
        os << "(#none)";
    os << " ";
    os << *callee_mb;
    os << endl;
}

void
Mode::load_from_array(uintptr_t* sp, void* elem_addr, int type_size)
{
    if (type_size == 1) {
        write(sp, read((int8_t*)elem_addr));
    }
    else if (type_size == 2) {
        write(sp, read((int16_t*)elem_addr));
    }
    else if (type_size == 4) {
        write(sp, read((int32_t*)elem_addr));
    }
    else if (type_size == 8) {
        write((uint64_t*)sp, read((uint64_t*)elem_addr));
    }
    else {
        assert(false);
    }
}

void
Mode::store_to_array(uintptr_t* sp, void* elem_addr, int type_size)
{
    if (type_size == 1) {
        write((int8_t*)elem_addr, (int32_t)read(sp));
    }
    else if (type_size == 2) {
        write((int16_t*)elem_addr, (int32_t)read(sp));
    }
    else if (type_size == 4) {
        write((int32_t*)elem_addr, (int32_t)read(sp));
    }
    else if (type_size == 8) {
        write((uint64_t*)elem_addr, read((uint64_t*)sp));
    }
    else {
        assert(false);
    }
}

void
Mode::load_array_from_no_cache_mem(uintptr_t* sp, void* elem_addr, int type_size)
{
    if (type_size == 1) {
        write(sp, *(int8_t*)elem_addr);
    }
    else if (type_size == 2) {
        write(sp, *(int16_t*)elem_addr);
    }
    else if (type_size == 4) {
        write(sp, *(int32_t*)elem_addr);
    }
    else if (type_size == 8) {
        write((uint64_t*)sp, *(uint64_t*)elem_addr);
    }
    else {
        assert(false);
    }
}

void
Mode::store_array_from_no_cache_mem(uintptr_t* sp, void* elem_addr, int type_size)
{
    if (type_size == 1) {
        write((int8_t*)elem_addr, (int32_t)*sp);
    }
    else if (type_size == 2) {
        write((int16_t*)elem_addr, (int32_t)*sp);
    }
    else if (type_size == 4) {
        write((int32_t*)elem_addr, (int32_t)*sp);
    }
    else if (type_size == 8) {
        write((uint64_t*)elem_addr, *(uint64_t*)sp);
    }
    else {
        assert(false);
    }
}

// void
// Mode::set_user(Object* user)
// {
//     //assert(user != m_user);
//     if (m_user == user)
//         return;

//     m_user = user;
// }

// void
// Mode::change_user(Object* user)
// {
//     //on_user_change(m_user, user);
//     set_user(user);
// }

//{{{ just for debug

    // MINILOGPROC(change_user_logger, show_user_change,
    //             (os, m_core->id(),
    //              old_user, new_user, reason, old_mb, new_mb));

    // MINILOG_IF(debug_scaffold::java_main_arrived
    //            && ((old_user ? is_app_obj(old_user) : true)
    //                || (new_user ? is_app_obj(new_user) : true)
    //                )
    //            //&& (m_coreid == 0 || m_id == 5 || m_id == 6)
    //            ,

// reason:
// 0: execute
// 1: invoke
// 2: return
// should be called before calling change_user
void
show_user_change(std::ostream& os, int id, const char* m,
                 Object* old_user, Object* new_user,
                 int reason,
                 MethodBlock* old_mb, MethodBlock* new_mb)
{
    if (new_user == old_user) return;
    if (not debug_scaffold::java_main_arrived) return;

    bool concern = false;
    if (old_user && is_app_obj(old_user))
        concern = true;
    if (new_user && is_app_obj(new_user))
        concern = true;

    if (not concern) return;

    os << "#" << id << ' ' << m
       << " user change(" << old_user << " => " << new_user << ")"
       << "(" << (old_user ? type_name(old_user) : "0") << " => "
       << (new_user ? type_name(new_user) : "0")  << ")"
       << "("
       << ((old_mb && old_mb->name) ? old_mb->full_name() : "null")
       << "=>"
       << (new_mb ? new_mb->full_name() : "null")
       << ")"
       << (reason == 1 ? (old_mb ? " invoke" : " execute") : " return")
       << '\n';
}

//}}} just for debug

Group*
Mode::get_group()
{
    return m_core->get_group();
}

Group*
Mode::assign_group_for(Object* obj)
{
    if (not OoSpmtJvm::do_spec) {
        return threadSelf()->get_default_group();
    }


    GroupingPolicyEnum final_policy;
    Group* result_group = 0;
    final_policy = get_grouping_policy_self(obj);
    if (final_policy == GP_CURRENT_GROUP) {

        Object* current_object = frame->get_object();
        assert(current_object);
        result_group = current_object->get_group();

    }
    else if (final_policy == GP_UNSPECIFIED) {

        Object* current_object = frame ? frame->get_object() : 0;
        //assert(current_object);
        if (current_object) {
            final_policy = get_grouping_policy_others(current_object);

            if (final_policy == GP_CURRENT_GROUP) {
                result_group = current_object->get_group();
            }
            else if (final_policy == GP_UNSPECIFIED) {
                Group* current_group = get_group();
                assert(current_group);
                Object* current_group_leader = current_group->get_leader();
                if (current_group_leader) {
                    final_policy = get_grouping_policy_others(current_group_leader);
                    if (final_policy == GP_CURRENT_GROUP) {
                        result_group = current_group;
                    }
                }
            }
        }

    }

    // default policy
    if (final_policy == GP_UNSPECIFIED) {
        if (is_normal_obj(obj)) {
            final_policy = GP_CURRENT_GROUP;
            result_group = get_group();
        }
        else if (is_class_obj(obj)) {
            final_policy = GP_NO_GROUP;
        }
        else {
            assert(false);  // todo
        }
    }

    // grouping according to final policy
    if (final_policy == GP_NEW_GROUP) {
        result_group = OoSpmtJvm::instance()->new_group_for(obj, threadSelf());
        threadSelf()->add_core(result_group->get_core());

        MINILOG_IF(debug_scaffold::java_main_arrived,
                   (is_certain_mode() ? c_new_main_logger : s_new_main_logger),
                   "#" << m_core->id() << " " << tag() << " new main "
                   << (is_class_obj(obj) ? static_cast<Class*>(obj)->name() : obj->classobj->name()));

    }
    else if (final_policy == GP_CURRENT_GROUP) {

        // MINILOG_IF(debug_scaffold::java_main_arrived,
        //            (is_certain_mode() ? c_new_main_logger : s_new_main_logger),
        //            "#" << m_core->id() << " " << tag() << " new sub "
        //            << (is_class_obj(obj) ? static_cast<Class*>(obj)->name() : obj->classobj->name()));
    }
    else if (final_policy == GP_NO_GROUP) {
        result_group = threadSelf()->get_default_group();
    }
    else {
        assert(false);          // MUST get a final policy
    }

    assert(result_group);
    return result_group;
}
