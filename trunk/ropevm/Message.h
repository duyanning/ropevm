#ifndef MESSAGE_H
#define MESSAGE_H

class Object;
class SpmtThread;
class MethodBlock;
class FieldBlock;
class Frame;
class Effect;

/*
 因为纯代码方法的引入，对象跟线程不再保持一致。
 从源对象和目标对象不再能得到正确的源线程和目标线程。所以消息中需要直接记录线程。
*/


/*
  消息的构造函数中读出数值，一律从确定内存中读取。不考虑模式。
 */

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


protected:
    Type type;

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
    InvokeMsg(SpmtThread* source_spmt_thread, Object* target_object, MethodBlock* mb, uintptr_t* args,
              CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
              bool is_top = false);
    bool is_top() { return m_is_top; }

    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    MethodBlock* mb;
    std::vector<uintptr_t> parameters;

    CodePntr caller_pc;
    Frame* caller_frame;
    uintptr_t* caller_sp;

    bool m_is_top;
};


class ReturnMsg : public Message {
public:

    ReturnMsg(uintptr_t* rv, int len,
              CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
              bool is_top = false);
    bool is_top() { return m_is_top; }

    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    std::vector<uintptr_t> retval;

    CodePntr caller_pc;
    Frame* caller_frame;
    uintptr_t* caller_sp;

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
    ArrayStoreMsg(SpmtThread* source_spmt_thread, Object* array, int type_size, int index, uintptr_t* slots);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    int index;
    int type_size;
    std::vector<uintptr_t> val;
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
    ArrayLoadMsg(SpmtThread* source_spmt_thread, Object* array, int type_size, int index);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    int type_size;
    int index;
};


class ArrayLoadReturnMsg : public Message {
public:
    ArrayLoadReturnMsg(uintptr_t* val, int size);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    std::vector<uintptr_t> val;
};


// 消息中数值的紧致性。

bool g_equal_msg_content(Message* msg1, Message* msg2); // refactor
bool operator==(Message& msg1, Message& msg2); // refactor: remove


std::ostream& operator<<(std::ostream& os, const Message& msg);
void show_msg_detail(std::ostream& os, int id, Message* msg);

bool g_is_async_msg(Message* msg);

//-----------------------------------
bool is_valid_certain_msg(Message* msg);

#endif // MESSAGE_H
