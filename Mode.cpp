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

#include "Message.h"
#include "Statistic.h"

using namespace std;

Mode::Mode(const char* name)
:
    pc(0),
    frame(0),
    sp(0),
    m_name(name)
{
}

const char*
Mode::get_name()
{
    return m_name;
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
    SpmtThread* spmt_thread = m_spmt_thread->get_thread()->assign_spmt_thread_for(new_object);
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
Mode::send_msg(Message* msg)
{
    assert(false);
}


// 在确定模式下处理确定消息，在推测模式下处理推测消息。
void
Mode::process_msg(Message* msg)
{
    if (debug_scaffold::java_main_arrived &&
        m_spmt_thread->is_certain_mode()) {
        m_spmt_thread->m_count_certain_instr++;
        Statistic::instance()->probe_instr_exec(0);
    }

    MsgType type = msg->get_type();

    if (type == MsgType::INVOKE) {
        InvokeMsg* invoke_msg = static_cast<InvokeMsg*>(msg);

        assert(not invoke_msg->is_top()); // 顶级方法不在这里处理

        // 为invoke_msg指定的方法及参数创建栈帧并设置
        invoke_impl(invoke_msg->get_target_object(),
                    invoke_msg->mb, &invoke_msg->parameters[0],
                    invoke_msg->get_source_spmt_thread(),
                    invoke_msg->caller_pc,
                    invoke_msg->caller_frame,
                    invoke_msg->caller_sp, false);
    }
    else if (type == MsgType::PUT) {
        PutMsg* put_msg = static_cast<PutMsg*>(msg);

        // 向put_msg指定的字段写入一个值
        uintptr_t* field_addr = put_msg->get_field_addr();
        for (int i = 0; i < put_msg->val.size(); ++i) {
            write(field_addr + i, put_msg->val[i]);
        }

        // 构造put_ret_msg作为回复
        PutReturnMsg* put_ret_msg = new PutReturnMsg(put_msg->get_source_spmt_thread());

        send_msg(put_ret_msg);

        // 需要加载下一条待处理消息
        m_spmt_thread->m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;
    }
    else if (type == MsgType::GET) {
        GetMsg* get_msg = static_cast<GetMsg*>(msg);

        // 从get_msg指定的字段读出一个值
        uintptr_t* field_addr = get_msg->get_field_addr();
        int field_size = get_msg->get_field_size();
        std::vector<uintptr_t> value;
        for (int i = 0; i < field_size; ++i) {
            value.push_back(read(field_addr + i));
        }

        // 构造get_ret_msg作为回复
        GetReturnMsg* get_ret_msg = new GetReturnMsg(get_msg->get_source_spmt_thread(),
                                                     &value[0], value.size());

        send_msg(get_ret_msg);

        // 需要加载下一条待处理消息
        m_spmt_thread->m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;
    }
    else if (type == MsgType::ASTORE) {
        ArrayStoreMsg* astore_msg = static_cast<ArrayStoreMsg*>(msg);

        // 向astore_msg指定的数组的指定位置处写入一个值
        Object* array = astore_msg->get_target_object();
        int index = astore_msg->index;
        int type_size = astore_msg->type_size;

        store_to_array_from_c(&astore_msg->val[0],
                              array, index, type_size);

        // 构造astore_ret_msg作为回复
        ArrayStoreReturnMsg* astore_ret_msg = new ArrayStoreReturnMsg(astore_msg->get_source_spmt_thread());

        send_msg(astore_ret_msg);

        // 需要加载下一条待处理消息
        m_spmt_thread->m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;
    }
    else if (type == MsgType::ALOAD) {
        ArrayLoadMsg* aload_msg = static_cast<ArrayLoadMsg*>(msg);

        // 从aload_msg指定的数组的指定位置处读出一个值
        Object* array = aload_msg->get_target_object();
        int index = aload_msg->index;
        int type_size = aload_msg->type_size;

        std::vector<uintptr_t> value(2); // at most 2 slots
        load_from_array_to_c(&value[0],
                             array, index, type_size);

        // 构造aload_ret_msg作为回复
        ArrayLoadReturnMsg* aload_ret_msg = new ArrayLoadReturnMsg(aload_msg->get_source_spmt_thread(),
                                                                   &value[0], size2nslots(type_size));

        send_msg(aload_ret_msg);

        // 需要加载下一条待处理消息
        m_spmt_thread->m_spec_running_state = RunningState::ongoing_but_need_launch_new_msg;
    }
    else if (type == MsgType::RETURN) {
        ReturnMsg* return_msg = static_cast<ReturnMsg*>(msg);

        // 方法重入会导致这些东西改变，所以需要按消息中的重新设置。
        pc = return_msg->caller_pc;
        frame = return_msg->caller_frame;
        sp = return_msg->caller_sp;

        // 将return_msg携带的返回值写入ostack
        for (auto i : return_msg->retval) {
            write(sp++, i);
        }

        pc += (*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    }
    else if (type == MsgType::PUT_RET) {
        //PutReturnMsg* put_ret_msg = static_cast<PutReturnMsg*>(msg);

        pc += 3;
    }
    else if (type == MsgType::GET_RET) {
        GetReturnMsg* get_ret_msg = static_cast<GetReturnMsg*>(msg);

        // 将get_ret_msg携带的值写入ostack
        for (auto i : get_ret_msg->val) {
            write(sp++, i);
        }

        pc += 3;
    }
    else if (type == MsgType::ASTORE_RET) {
        //ArrayStoreReturnMsg* astore_ret_msg = static_cast<ArrayStoreReturnMsg*>(msg);

        pc += 1;
    }
    else if (type == MsgType::ALOAD_RET) {
        ArrayLoadReturnMsg* aload_ret_msg = static_cast<ArrayLoadReturnMsg*>(msg);

        // 将aload_ret_msg携带的值写入ostack
        for (auto i : aload_ret_msg->val) {
            write(sp++, i);
        }

        pc += 1;
    }
    else {
        assert(false);
    }

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
void
Mode::load_from_array(uintptr_t* sp, Object* array, int index, int type_size)
{
    void* elem_addr = array_elem_addr(array, index, type_size);

    if (type_size == 1) {
        write(sp, read((uint8_t*)elem_addr));
    }
    else if (type_size == 2) {
        write(sp, read((uint16_t*)elem_addr));
    }
    else if (type_size == 4) {
        write(sp, read((uint32_t*)elem_addr));
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
Mode::store_to_array(uintptr_t* sp, Object* array, int index, int type_size)
{
    void* elem_addr = array_elem_addr(array, index, type_size);

    if (type_size == 1) {
        write((uint8_t*)elem_addr, (uint32_t)read(sp));
    }
    else if (type_size == 2) {
        write((uint16_t*)elem_addr, (uint32_t)read(sp));
    }
    else if (type_size == 4) {
        write((uint32_t*)elem_addr, (uint32_t)read(sp));
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
Mode::load_from_array_to_c(uintptr_t* sp, Object* array, int index, int type_size)
{
    void* elem_addr = array_elem_addr(array, index, type_size);

    if (type_size == 1) {
        *sp = read((uint8_t*)elem_addr);
    }
    else if (type_size == 2) {
        *sp = read((uint16_t*)elem_addr);
    }
    else if (type_size == 4) {
        *sp = read((uint32_t*)elem_addr);
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
Mode::store_to_array_from_c(uintptr_t* sp, Object* array, int index, int type_size)
{
    void* elem_addr = array_elem_addr(array, index, type_size);

    if (type_size == 1) {
        write((uint8_t*)elem_addr, (uint32_t)*sp);
    }
    else if (type_size == 2) {
        write((uint16_t*)elem_addr, (uint32_t)*sp);
    }
    else if (type_size == 4) {
        write((uint32_t*)elem_addr, (uint32_t)*sp);
    }
    else if (type_size == 8) {
        write((uint64_t*)elem_addr, *(uint64_t*)sp);
    }
    else {
        assert(false);
    }
}


void
g_load_from_stable_array_to_c(uintptr_t* sp, Object* array, int index, int type_size)
{
    void* elem_addr = array_elem_addr(array, index, type_size);

    if (type_size == 1) {
        *sp = *(uint8_t*)elem_addr;
    }
    else if (type_size == 2) {
        *sp = *(uint16_t*)elem_addr;
    }
    else if (type_size == 4) {
        *sp = *(uint32_t*)elem_addr;
    }
    else if (type_size == 8) {
        *(uint64_t*)sp = *(uint64_t*)elem_addr;
    }
    else {
        assert(false);
    }

}
