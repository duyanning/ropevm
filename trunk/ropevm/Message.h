#ifndef MESSAGE_H
#define MESSAGE_H

class Object;
class SpmtThread;
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
    Message(Type t);
    virtual bool equal(Message& msg);
    Type get_type();
    Effect* get_effect();
    virtual ~Message();
    virtual void show(std::ostream& os) const = 0;
    virtual void show_detail(std::ostream& os, int id) const = 0;
    Object* get_source_object() { return m_source_object; } // refactor: to remove
    Object* get_target_object() { return m_target_object; } // refactor: to remove
protected:
    Type type;
    Object* m_source_object;    // refactor: to remove
    Object* m_target_object;    // refactor: to remove
	Effect* m_effect;  // 处理该消息所形成的effect
};


class RoundTripMsg : public Message {
public:
    RoundTripMsg(Type t, SpmtThread* source_spmt_thread, Object* target_object)
        : Message(t)
    {
    }
    SpmtThread* get_source_spmt_thread() { return m_source_spmt_thread; }
    Object* get_target_object() { return m_target_object; }
protected:
    SpmtThread* m_source_spmt_thread;
    Object* m_target_object;
};


// class OneWayMsg : public Message {
// };


class InvokeMsg : public RoundTripMsg {
public:
    InvokeMsg(SpmtThread* source_spmt_thread, Object* target_object, MethodBlock* mb, uintptr_t* args, bool is_top = false);
    bool is_top() { return m_is_top; }

    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    MethodBlock* mb;
    std::vector<uintptr_t> parameters;
    bool m_is_top;
};


class ReturnMsg : public Message {
public:

    ReturnMsg(uintptr_t* rv, int len, bool is_top = false);
    bool is_top() { return m_is_top; }

    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    std::vector<uintptr_t> retval;
    bool m_is_top;
};


class GetMsg : public RoundTripMsg {
public:
    GetMsg(SpmtThread* source_spmt_thread, Object* target_object, FieldBlock* fb);
    uintptr_t* get_field_addr();
    int get_field_size();
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    FieldBlock* fb;
};


class GetReturnMsg : public Message {
public:
    GetReturnMsg(uintptr_t* val, int size);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    std::vector<uintptr_t> val;
};


class PutMsg : public RoundTripMsg {
public:
    PutMsg(SpmtThread* source_spmt_thread,
           Object* target_object, FieldBlock* fb,
           uintptr_t* val);
    uintptr_t* get_field_addr();
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    FieldBlock* fb;
    std::vector<uintptr_t> val;
};


class PutReturnMsg : public Message {
public:
    PutReturnMsg();
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
};


class ArrayStoreMsg : public RoundTripMsg {
public:
    ArrayStoreMsg(SpmtThread* source_spmt_thread, Object* array, int index, uintptr_t* slots, int nslots, int type_size);
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


class ArrayStoreReturnMsg : public Message {
public:
    ArrayStoreReturnMsg();
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
};


class ArrayLoadMsg : public RoundTripMsg {
public:
    ArrayLoadMsg(SpmtThread* source_spmt_thread, Object* array, int index, uint8_t* addr, int type_size,
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


class ArrayLoadReturnMsg : public Message {
public:
    ArrayLoadReturnMsg();
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
};




bool g_equal_msg_content(Message* msg1, Message* msg2); // refactor
bool operator==(Message& msg1, Message& msg2); // refactor: remove


std::ostream& operator<<(std::ostream& os, const Message& msg);
void show_msg_detail(std::ostream& os, int id, Message* msg);

bool g_is_async_msg(Message* msg);

//-----------------------------------
bool is_valid_certain_msg(Message* msg);

#endif // MESSAGE_H
