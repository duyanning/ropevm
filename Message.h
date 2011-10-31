#ifndef MESSAGE_H
#define MESSAGE_H

class MethodBlock;
class FieldBlock;
class Frame;
class Effect;

// enum class MsgType {
//     INVOKE, RETURN,
//     GET, GET_RETURN,
//     PUT, PUT_RETURN,
//     LOAD, LOAD_RETURN,
//     STORE, STORE_RETURN
// };


class Message {
public:
    enum Type {
        invoke, ret,
        get, get_return,
        put, put_return,
        arrayload, arrayload_return,
        arraystore, arraystore_return
    };
    Message(Type t, Object* source_object, Object* target_ojbect);
    Type get_type();
    Effect* get_effect();
    virtual ~Message();
    virtual void show(std::ostream& os) const = 0;
    virtual void show_detail(std::ostream& os, int id) const = 0;
    Object* get_source_object() { return m_source_object; }
    Object* get_target_object() { return m_target_object; }
protected:
    Type type;
    Object* m_source_object;
    Object* m_target_object;
	Effect* m_effect;  // 处理该消息所形成的effect
};

class Object;

class InvokeMsg : public Message {
public:
    InvokeMsg(Object* object, MethodBlock* mb, Frame* prev, Object* calling_object, uintptr_t* args, uintptr_t* caller_sp, CodePntr caller_pc, Object* calling_owner = 0);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
    MethodBlock* mb;
    std::vector<uintptr_t> parameters;
    Frame* caller_frame;
    uintptr_t* caller_sp;
    CodePntr caller_pc;
    Object* calling_owner;
};

class ReturnMsg : public Message {
public:
    ReturnMsg(Object* object, MethodBlock* mb, Frame* caller_frame, Object* calling_object, uintptr_t* rv, int len, uintptr_t* caller_sp, CodePntr caller_pc);
    // virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
    std::vector<uintptr_t> retval;
    Frame* caller_frame;
    uintptr_t* caller_sp;
    CodePntr caller_pc;

    // for debug
    MethodBlock* mb;
};

class GetMsg : public Message {
public:
    GetMsg(Object* source_object, Object* target_object, FieldBlock* fb, uintptr_t* addr, int size, Frame* caller_frame, uintptr_t* caller_sp, CodePntr caller_pc);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    FieldBlock* fb;
    std::vector<uintptr_t> val;
    //int instr_len;
    Frame* caller_frame;
    uintptr_t* caller_sp;
    CodePntr caller_pc;
};

class PutMsg : public Message {
public:
    PutMsg(Object* source_object, Object* target_object, FieldBlock* fb, uintptr_t* addr, uintptr_t* val, int len, bool is_static);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    FieldBlock* fb;
    uintptr_t* addr;
    std::vector<uintptr_t> val;
    /*     Frame* caller_frame; */
    /*     uintptr_t* caller_sp; */
    /*     CodePntr caller_pc; */
    bool is_static;
};

// class AckMsg : public Message {
// public:
//     AckMsg();
//     virtual bool equal(Message& msg);
//     void show(std::ostream& os) const;
//     virtual void show_detail(std::ostream& os, int id) const;
// };


class ArrayLoadMsg : public Message {
public:
    ArrayLoadMsg(Object* array, Object* target_object, int index, uint8_t* addr, int type_size,
                 Frame* caller_frame, uintptr_t* caller_sp, CodePntr caller_pc);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    int index;
    std::vector<uint8_t> val;
    //int instr_len;
    Frame* caller_frame;
    uintptr_t* caller_sp;
    CodePntr caller_pc;
};

class ArrayStoreMsg : public Message {
public:
    ArrayStoreMsg(Object* source_object, Object* array, int index, uintptr_t* slots, int nslots, int type_size);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    int index;
    std::vector<uintptr_t> slots;
    int type_size;
    /*     Frame* caller_frame; */
    /*     uintptr_t* caller_sp; */
    /*     CodePntr caller_pc; */
};

bool are_the_same_in_content(Message* msg1, Message* msg2); // refactor
bool operator==(Message& msg1, Message& msg2); // refactor: remove
std::ostream& operator<<(std::ostream& os, const Message& msg);
void show_msg_detail(std::ostream& os, int id, Message* msg);

//-----------------------------------
bool is_valid_certain_msg(Message* msg);

#endif // MESSAGE_H
