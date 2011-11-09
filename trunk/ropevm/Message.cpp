#include "std.h"
#include "rope.h"
#include "Message.h"

using namespace std;


int msg_count = 0;              // just for debug

Message::Message(Type t, SpmtThread* target_spmt_thread)
   :
    type(t),
    m_target_spmt_thread(target_spmt_thread)

{
}

Message::~Message()
{
}

Message::Type
Message::get_type()
{
    return type;
}

// bool
// Message::is_equal_to(Message& msg)
// {
//     if (type != msg.type)
//         return false;
//     // if (m_source_object != msg.m_source_object)
//     //     return false;
//     // if (m_target_object != msg.m_target_object)
//     //     return false;
//     return equal(msg);
// }

//------------------------------------------------

InvokeMsg::InvokeMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
                     Object* target_object, MethodBlock* mb, uintptr_t* args,
                     CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                     bool is_top)
:
    RoundTripMsg(Message::invoke, source_spmt_thread, target_spmt_thread, target_object)
{
    this->mb = mb;

    for (int i = 0; i < mb->args_count; ++i) {
        parameters.push_back(args[i]);
    }

    this->caller_sp = caller_sp;
    this->caller_frame = caller_frame;
    this->caller_pc = caller_pc;

    m_is_top = is_top;
}

bool
InvokeMsg::equal(Message& msg)
{
    InvokeMsg& m = static_cast<InvokeMsg&>(msg);
    if (mb != m.mb)
        return false;
    if (parameters != m.parameters)
        return false;

    // if (caller_frame != m.caller_frame)
    //     return false;
    // if (caller_pc != m.caller_pc)
    //     return false;
    // if (caller_sp != m.caller_sp)
    //     return false;


    return true;
}

void
InvokeMsg::show(ostream& os) const
{
    //cout << "call message\n";
    os << "invoke to " << mb->classobj->name() << '.' << mb->name << ':' << mb->type;
}

void
InvokeMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}

//------------------------------------------------
ReturnMsg::ReturnMsg(SpmtThread* target_spmt_thread,
                     uintptr_t* rv, int len,
                     CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
                     bool is_top)
:
    Message(Message::ret, target_spmt_thread)
{
    //this->mb = mb;

    for (int i = 0; i < len; ++i) {
        retval.push_back(rv[i]);
    }

    this->caller_pc = caller_pc;
    this->caller_frame = caller_frame;
    this->caller_sp = caller_sp;

    m_is_top = is_top;
}

bool
ReturnMsg::equal(Message& msg)
{
    ReturnMsg& m = static_cast<ReturnMsg&>(msg);
    if (retval != m.retval)
        return false;

    // eqmore
    // if (caller_frame != m.caller_frame)
    //     return false;
    // if (caller_pc != m.caller_pc)
    //     return false;
    // if (caller_sp != m.caller_sp)
    //     return false;

    return true;
}

void
ReturnMsg::show(ostream& os) const
{
    int len = retval.size();
    assert(len == 0 || len == 1 || len == 2);
    if (len == 0) {
        os << "retvoid";
    }
    else if (len == 1) {
        os << "ret32 " << hex <<  *((int*)&retval[0]) << dec;
    }
    else if (len == 2) {
        os << "ret64 " << hex << *((long*)&retval[0]) << dec;
    }

    //os << " from " << mb->classobj->name() << '.' << mb->name << ':' << mb->type;
}

void
ReturnMsg::show_detail(std::ostream& os, int id) const
{
    // os << "#" << id << " caller frame:\t" << (void*)caller_frame << "\n";
    // os << "#" << id << " caller pc:\t" << (void*)caller_pc << "\n";
    // os << "#" << id << " caller sp:\t" << (void*)caller_sp << "\n";
}

//-----------------------------------------------

GetMsg::GetMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
               Object* target_object, FieldBlock* fb)
:
    RoundTripMsg(Message::get, source_spmt_thread, target_spmt_thread, target_object)
{
    // for (int i = 0; i < size; ++i) {
    //     this->val.push_back(addr[i]);
    // }

    // this->caller_frame = caller_frame;
    // this->caller_sp = caller_sp;
    // this->caller_pc = caller_pc;

    //this->instr_len = instr_len;
    this->fb = fb;
    // this->addr = addr;
    // this->size = size;
}

uintptr_t*
GetMsg::get_field_addr()
{
    return fb->field_addr(get_target_object());
}

int
GetMsg::get_field_size()
{
    return fb->field_size();
}

bool
GetMsg::equal(Message& msg)
{
    GetMsg& m = static_cast<GetMsg&>(msg);
    // if (obj != m.obj)
    //     return false;
    // if (val != m.val)
    //     return false;
    if (fb != m.fb)
        return false;
    // eqmore
    // if (caller_frame != m.caller_frame)
    //     return false;
    // if (caller_pc != m.caller_pc)
    //     return false;
    // if (caller_sp != m.caller_sp)
    //     return false;
    return true;
}

void
GetMsg::show(ostream& os) const
{
    // int len = val.size();
    // assert(len == 1 || len == 2);

    // if (len == 1) {
    //     os << "get32 " << *fb << hex << *((int*)&val[0]) << dec;
    // }
    // else if (len == 2) {
    //     os << "get64 " << *fb << hex << *((long*)&val[0]) << dec;
    // }
}

void
GetMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
//-----------------------------------------------

PutMsg::PutMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
               Object* target_object, FieldBlock* fb,
               uintptr_t* val)
:
    RoundTripMsg(Message::put, source_spmt_thread, target_spmt_thread, target_object)
{

    for (int i = 0; i < fb->field_size(); ++i) {
        this->val.push_back(val[i]);
    }
    this->fb = fb;

    //this->addr = addr;

//     this->caller_frame = caller_frame;
//     this->caller_sp = caller_sp;
//     this->caller_pc = caller_pc;
    //this->is_static = is_static;


}


uintptr_t*
PutMsg::get_field_addr()
{
    return fb->field_addr(get_target_object());
}


bool
PutMsg::equal(Message& msg)
{
    PutMsg& m = static_cast<PutMsg&>(msg);
    if (val != m.val)
        return false;
    if (fb != m.fb)
        return false;
//     if (caller_frame != m.caller_frame)
//         return false;
//     if (caller_pc != m.caller_pc)
//         return false;
//     if (caller_sp != m.caller_sp)
//         return false;
    return true;
}

void
PutMsg::show(ostream& os) const
{
    int len = val.size();
    assert(len == 1 || len == 2);

    if (len == 1) {
        os << "put32 " << *fb << hex << *((int*)&val[0]) << dec;
    }
    else if (len == 2) {
        os << "put64 " << *fb << hex << *((long*)&val[0]) << dec;
    }
}

void
PutMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}

//-----------------------------------------------

ArrayLoadMsg::ArrayLoadMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
                           Object* array, int type_size, int index)
:
    RoundTripMsg(Message::arrayload, source_spmt_thread, target_spmt_thread, array)
{
    this->type_size = type_size;
    this->index = index;
}

bool
ArrayLoadMsg::equal(Message& msg)
{
    ArrayLoadMsg& m = static_cast<ArrayLoadMsg&>(msg);

    if (index != m.index)
        return false;
    // if (caller_frame != m.caller_frame)
    //     return false;
    // if (caller_pc != m.caller_pc)
    //     return false;
    // if (caller_sp != m.caller_sp)
    //     return false;
    return true;
}

void
ArrayLoadMsg::show(ostream& os) const
{
    // int type_size = val.size();
    // assert(type_size >= 1 && type_size <= 8);

    // // os << "array load" << type_size << " " << m_source_object->classobj->name() << "(" << index << ") = ";

    // os << hex;
    // for (int i = 0; i < val.size(); ++i) {
    //     os << (int)val[i] << " ";
    // }
    // os << dec;
}

void
ArrayLoadMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}
//-----------------------------------------------

ArrayStoreMsg::ArrayStoreMsg(SpmtThread* source_spmt_thread, SpmtThread* target_spmt_thread,
                             Object* array, int type_size, int index, uintptr_t* slots)

:
    RoundTripMsg(Message::arraystore, source_spmt_thread, target_spmt_thread, array)
{

    this->type_size = type_size;
    this->index = index;
    int nslots = size2nslots(type_size);
    for (int i = 0; i < nslots; ++i) {
        this->val.push_back(slots[i]);
    }

}

bool
ArrayStoreMsg::equal(Message& msg)
{
    ArrayStoreMsg& m = static_cast<ArrayStoreMsg&>(msg);
    if (val != m.val)
        return false;
    if (index != m.index)
        return false;
//     if (caller_frame != m.caller_frame)
//         return false;
//     if (caller_pc != m.caller_pc)
//         return false;
//     if (caller_sp != m.caller_sp)
//         return false;
    return true;
}

void
ArrayStoreMsg::show(ostream& os) const
{
    int nslots = val.size();
    assert(nslots == 1 || nslots == 2);

    os << "array store" << type_size << " " << m_object->classobj->name() << "(" << index << ") = ";

    if (nslots == 1) {
        int i = *(int*)&val[0];
        os << hex << i << dec;
    }
    else {                      //  nslots == 2
        double d = *(double*)&val[0];
        os << hex << d << dec;
    }
}

void
ArrayStoreMsg::show_detail(std::ostream& os, int id) const
{
    //os << "#" << id << "\n";
}

//-----------------------------------------------

bool operator==(Message& msg1, Message& msg2)
{
    return msg1.equal(msg2);
}

ostream& operator<<(ostream& os, const Message& msg)
{
    msg.show(os);
    os << " (" << (void*)&msg << ")";
    return os;
}

void
show_msg_detail(ostream& os, int id, Message* msg)
{
    msg->show_detail(os, id);
}


bool
g_is_async_msg(Message* msg)
{
    Message::Type type = msg->get_type();
    if (type == Message::invoke
        or type == Message::put
        or type == Message::get
        or type == Message::arraystore
        or type == Message::arrayload)
        return true;

    return false;
}
