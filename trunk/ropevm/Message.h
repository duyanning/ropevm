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
  消息中的数值不紧致。
 */

enum class MsgType {
    INVOKE, RETURN,
    GET, GET_RET,
    PUT, PUT_RET,
    ALOAD, ALOAD_RET,
    ASTORE, ASTORE_RET
};


class Message {
public:
    Message(MsgType type, SpmtThread* target_st);
    virtual ~Message();

    MsgType get_type();
    SpmtThread* get_target_st();
    Effect* get_effect();
    void set_effect(Effect* effect);

    virtual bool equal(Message* msg);
    virtual void show(std::ostream& os) const = 0;
    virtual void show_detail(std::ostream& os, int id) const = 0;

protected:
    MsgType m_type;
    SpmtThread* m_target_st; // 该消息去往哪个spmt线程
	Effect* m_effect;  // 处理该消息所形成的effect
};


class RoundTripMsg : public Message {
public:
    RoundTripMsg(MsgType type,
                 SpmtThread* source_st, SpmtThread* target_st,
                 Object* target_object);
    //virtual bool equal(Message* msg);
    virtual void show(std::ostream& os) const = 0;
    virtual void show_detail(std::ostream& os, int id) const = 0;

    SpmtThread* get_source_st() { return m_source_st; }
    Object* get_target_object() { return m_object; }
protected:
    SpmtThread* m_source_st; // 该消息来自哪个spmt线程（因为要返程嘛）
    Object* m_object;                 // 消息作用在哪个对象身上
};


// class OneWayMsg : public Message {
// };


class InvokeMsg : public RoundTripMsg {
public:
    InvokeMsg(SpmtThread* source_st, SpmtThread* target_st,
              Object* target_object, MethodBlock* mb, uintptr_t* args,
              CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
              bool is_top = false);
    bool is_top() { return m_is_top; }

    //virtual bool equal(Message* msg);
    virtual void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    MethodBlock* mb;
    std::vector<uintptr_t> parameters;

    CodePntr caller_pc;
    Frame* caller_frame;
    uintptr_t* caller_sp;

    bool m_is_top;
};


class GetMsg : public RoundTripMsg {
public:
    GetMsg(SpmtThread* source_st, SpmtThread* target_st,
           Object* target_object, FieldBlock* fb);
    uintptr_t* get_field_addr();
    int get_field_size();
    //virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    FieldBlock* fb;
};


class PutMsg : public RoundTripMsg {
public:
    PutMsg(SpmtThread* source_st, SpmtThread* target_st,
           Object* target_object, FieldBlock* fb,
           uintptr_t* val);
    uintptr_t* get_field_addr();
    //virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    FieldBlock* fb;
    std::vector<uintptr_t> val;
};


class ALoadMsg : public RoundTripMsg {
public:
    ALoadMsg(SpmtThread* source_st, SpmtThread* target_st,
                 Object* array, int type_size, int index);
    //virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    int type_size;
    int index;
};


class AStoreMsg : public RoundTripMsg {
public:
    AStoreMsg(SpmtThread* source_st, SpmtThread* target_st,
                  Object* array, int type_size, int index, uintptr_t* slots);
    //virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    int index;
    int type_size;
    std::vector<uintptr_t> val;
};


//================================================================

class ReturnMsg : public Message {
public:

    ReturnMsg(SpmtThread* target_st,
              uintptr_t* rv, int len,
              CodePntr caller_pc, Frame* caller_frame, uintptr_t* caller_sp,
              bool is_top = false);
    bool is_top() { return m_is_top; }

    virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    std::vector<uintptr_t> retval;

    CodePntr caller_pc;
    Frame* caller_frame;
    uintptr_t* caller_sp;

    bool m_is_top;
};


class GetRetMsg : public Message {
public:
    GetRetMsg(SpmtThread* target_st, uintptr_t* val, int size);
    virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    std::vector<uintptr_t> val;
};


class PutRetMsg : public Message {
public:
    PutRetMsg(SpmtThread* target_st);
    virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
};


class ALoadRetMsg : public Message {
public:
    ALoadRetMsg(SpmtThread* target_st, uintptr_t* val, int size);
    virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    std::vector<uintptr_t> val;
};


class AStoreRetMsg : public Message {
public:
    AStoreRetMsg(SpmtThread* target_st);
    virtual bool equal(Message* msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
};



std::ostream& operator<<(std::ostream& os, const Message* msg);
void show_msg_detail(std::ostream& os, int id, Message* msg);
bool g_is_async_msg(Message* msg);

#endif // MESSAGE_H
