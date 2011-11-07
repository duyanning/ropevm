#include "std.h"
#include "rope.h"
#include "Mode.h"

#include "lock.h"
#include "SpmtThread.h"
#include "RopeVM.h"
#include "DebugScaffold.h"
#include "Helper.h"
#include "Loggers.h"
#include "jni.h"

using namespace std;

Mode::Mode(const char* name)
:
    m_name(name)
{
    exception = 0;

    frame = 0;
    sp = 0;
    pc = 0;
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
Mode::set_spmt_thread(SpmtThread* spmt_thread)
{
    m_spmt_thread = spmt_thread;
}

void
Mode::before_alloc_object()
{
}


void
Mode::after_alloc_object(Object* new_object)
{
    //if (not RopeVM::do_spec) return;

    SpmtThread* spmt_thread = m_spmt_thread->assign_spmt_thread_for(new_object);
    // assign_spmt_thread_for后要么跟set_spmt_thread，要么跟join_spmt_thread_in_other_threads
    new_object->set_spmt_thread(spmt_thread);
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
    cout << "vmlog: " << "#" << m_spmt_thread->m_id << " " << tag() << " "
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

void
Mode::destroy_frame(Frame* frame)
{
    //assert(m_spmt_thread->frame_is_not_in_snapshots(frame));

    MINILOG(delete_frame_logger, "#" << m_spmt_thread->id() << " delete frame: " << info(frame) << frame);
    delete frame;
    //frame->magic = 2009; // only mark dead, do not delete for debug purpose
}

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
    // if (caller->get_group())
    //     os << "(#" << caller->get_group()->get_core()->id() << ")";
    // else
    //     os << "(#none)";
    os << " ";
    os << *caller_mb;
    os << (is_invoke ? " ===>>> " : " <<<=== ");
    os << callee;
    // if (callee->get_group())
    //     os << "(#" << callee->get_group()->get_core()->id() << ")";
    // else
    //     os << "(#none)";
    os << " ";
    os << *callee_mb;
    os << endl;
}



/*
注意：在以下这些方法的实现中谨记：
元素在数组中是紧致存储的，几个字节就是几个字节；
而在ostack和c内存不足4字节，也要占用4字节。

所以，涉及到数组的读写，是几个字节就几个字节。
而涉及到ostack或c内存的读写，要么是4字节，要么是8字节。
(这点从强制类型转换即可看出)
 */

// 根据模式从数组读数值，根据模式写入ostack
// refactor: 这些函数第一个参数的名字应该改一下，第二个参数换成数组名字和数组索引
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


// 根据模式从ostack读数值，根据模式写入数组
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


// 根据模式从数组读数值，写入c内存
void
Mode::load_from_array_to_c(uintptr_t* sp, void* elem_addr, int type_size)
{
    if (type_size == 1) {
        *sp = read((int8_t*)elem_addr);
    }
    else if (type_size == 2) {
        *sp = read((int16_t*)elem_addr);
    }
    else if (type_size == 4) {
        *sp = read((int32_t*)elem_addr);
    }
    else if (type_size == 8) {
        *(uint64_t*)sp = read((uint64_t*)elem_addr);
    }
    else {
        assert(false);
    }

}


// 从c内存中读数值，根据模式写入数组
void
Mode::store_to_array_from_c(uintptr_t* sp, void* elem_addr, int type_size)
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


void
g_load_from_stable_array_to_c(uintptr_t* sp, void* elem_addr, int type_size)
{
    if (type_size == 1) {
        *sp = *(int8_t*)elem_addr;
    }
    else if (type_size == 2) {
        *sp = *(int16_t*)elem_addr;
    }
    else if (type_size == 4) {
        *sp = *(int32_t*)elem_addr;
    }
    else if (type_size == 8) {
        *(uint64_t*)sp = *(uint64_t*)elem_addr;
    }
    else {
        assert(false);
    }

}


// SpmtThread*
// Mode::assign_spmt_thread_for(Object* obj)
// {
//     if (not RopeVM::do_spec) {
//         return threadSelf()->get_initial_spmt_thread();
//     }

//     MINILOG0_IF(is_app_class(obj->classobj), "app obj of " << obj->classobj->name());
//     if (is_app_class(obj->classobj)) {
//         cout << "app obj for " << obj->classobj->name() << endl;
//         int x = 0;
//         x++;

//     }


//     GroupingPolicyEnum final_policy;
//     SpmtThread* result_spmt_thread = 0;
//     final_policy = get_grouping_policy_self(obj);
//     if (final_policy == GP_CURRENT_GROUP) {

//         Object* current_object = frame->get_object();
//         assert(current_object);
//         result_spmt_thread = current_object->get_spmt_thread();

//     }
//     else if (final_policy == GP_UNSPECIFIED) {

//         Object* current_object = frame ? frame->get_object() : 0;
//         //assert(current_object);
//         if (current_object) {
//             final_policy = get_grouping_policy_others(current_object);

//             if (final_policy == GP_CURRENT_GROUP) {
//                 result_spmt_thread = current_object->get_spmt_thread();
//             }
//             else if (final_policy == GP_UNSPECIFIED) {
//                 SpmtThread* current_spmt_thread = get_spmt_thread();
//                 assert(current_spmt_thread);
//                 // Object* current_spmt_thread_leader = current_spmt_thread->get_leader();
//                 // if (current_spmt_thread_leader) {
//                 //     final_policy = get_grouping_policy_others(current_spmt_thread_leader);
//                 //     if (final_policy == GP_CURRENT_GROUP) {
//                 //         result_spmt_thread = current_spmt_thread;
//                 //     }
//                 // }
//             }
//         }

//     }

//     // default policy
//     if (final_policy == GP_UNSPECIFIED) {
//         if (is_normal_obj(obj)) {
//             final_policy = GP_CURRENT_GROUP;
//             result_spmt_thread = get_spmt_thread();
//         }
//         else if (is_class_obj(obj)) {
//             final_policy = GP_NO_GROUP;
//         }
//         else {
//             assert(false);  // todo
//         }
//     }


//     // grouping according to final policy
//     if (final_policy == GP_NEW_GROUP) {
//         result_spmt_thread = RopeVM::instance()->new_spmt_thread_for(obj, threadSelf());
//         // dubug-tmp
//         //result_group = RopeVM::instance()->new_group_for(0, threadSelf());
//         result_spmt_thread->set_thread(threadSelf());

//         MINILOG_IF(debug_scaffold::java_main_arrived,
//                    (is_certain_mode() ? c_new_main_logger : s_new_main_logger),
//                    "#" << m_spmt_thread->id() << " " << tag() << " new main "
//                    << (is_class_obj(obj) ? static_cast<Class*>(obj)->name() : obj->classobj->name()));

//     }
//     else if (final_policy == GP_CURRENT_GROUP) {

//         // MINILOG_IF(debug_scaffold::java_main_arrived,
//         //            (is_certain_mode() ? c_new_main_logger : s_new_main_logger),
//         //            "#" << m_spmt_thread->id() << " " << tag() << " new sub "
//         //            << (is_class_obj(obj) ? static_cast<Class*>(obj)->name() : obj->classobj->name()));
//     }
//     else if (final_policy == GP_NO_GROUP) {
//         result_spmt_thread = threadSelf()->get_initial_spmt_thread();
//     }
//     else {
//         assert(false);          // MUST get a final policy
//     }


//     assert(result_spmt_thread);
//     return result_spmt_thread;
// }
