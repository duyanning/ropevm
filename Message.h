#ifndef MESSAGE_H
#define MESSAGE_H

class MethodBlock;
class FieldBlock;
class Frame;

class Message {
public:
    enum Type {
        call, ret,
        get, put, ack,
        arrayload, arraystore//,
        //arraylength
    };
    Message(Type t);
    // Message(Type t, Object* object);
    Type get_type();
    virtual ~Message();
    bool is_equal_to(Message& msg);
    virtual void show(std::ostream& os) const = 0;
    virtual void show_detail(std::ostream& os, int id) const = 0;
    // Object* get_object() { return m_object; }
    //void set_object(Object* object) { m_object = object; }
private:
    virtual bool equal(Message& msg) = 0;
    Type type;
protected:
    Object* m_object;
public:
    /*     int src_id; // just for debug */
    /*     int dst_id; // just for debug */
    int c; // just for debug
};

class Object;

class InvokeMsg : public Message {
public:
    InvokeMsg(Object* object, MethodBlock* mb, Frame* prev, Object* calling_object, uintptr_t* args, uintptr_t* caller_sp, CodePntr caller_pc, Object* calling_owner = 0);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
    Object* object;
    MethodBlock* mb;
    std::vector<uintptr_t> parameters;
    Frame* caller_frame;
    Object* calling_object;
    uintptr_t* caller_sp;
    CodePntr caller_pc;
    Object* calling_owner;
};

class ReturnMsg : public Message {
public:
    ReturnMsg(Object* object, MethodBlock* mb, Frame* caller_frame, Object* calling_object, uintptr_t* rv, int len, uintptr_t* caller_sp, CodePntr caller_pc);
    // ReturnMsg(MethodBlock* mb, Frame* caller_frame, uintptr_t* rv, int len, uintptr_t* caller_sp, CodePntr caller_pc, Frame* frame);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
    // private:
    std::vector<uintptr_t> retval;
    Frame* caller_frame;
    uintptr_t* caller_sp;
    CodePntr caller_pc;

    //Frame* frame;

    // for debug
    MethodBlock* mb;
    Object* object;
    Object* calling_object;
};

class GetMsg : public Message {
public:
    GetMsg(Object* obj, FieldBlock* fb, uintptr_t* addr, int size, Frame* caller_frame, uintptr_t* caller_sp, CodePntr caller_pc);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    Object* obj;
    FieldBlock* fb;
    std::vector<uintptr_t> val;
    //int instr_len;
    Frame* caller_frame;
    uintptr_t* caller_sp;
    CodePntr caller_pc;
};

class PutMsg : public Message {
public:
    //PutMsg(FieldBlock* fb, uintptr_t* addr, uintptr_t* val, int len, Frame* caller_frame, uintptr_t* caller_sp, CodePntr caller_pc);
    PutMsg(Object* obj, FieldBlock* fb, uintptr_t* addr, uintptr_t* val, int len, bool is_static, Core* sender);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    Object* obj;
    FieldBlock* fb;
    uintptr_t* addr;
    std::vector<uintptr_t> val;
    /*     Frame* caller_frame; */
    /*     uintptr_t* caller_sp; */
    /*     CodePntr caller_pc; */
    bool is_static;
    Core* sender;
};

class AckMsg : public Message {
public:
    AckMsg();
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;
};


class ArrayLoadMsg : public Message {
public:
    ArrayLoadMsg(Object* array, int index, uint8_t* addr, int type_size,
                 Frame* caller_frame, uintptr_t* caller_sp, CodePntr caller_pc);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    Object* array;
    int index;
    std::vector<uint8_t> val;
    //int instr_len;
    Frame* caller_frame;
    uintptr_t* caller_sp;
    CodePntr caller_pc;
};

class ArrayStoreMsg : public Message {
public:
    ArrayStoreMsg(Object* array, int index, uintptr_t* slots, int nslots, int type_size,
                  Core* sender);
    virtual bool equal(Message& msg);
    void show(std::ostream& os) const;
    virtual void show_detail(std::ostream& os, int id) const;

    Object* array;
    int index;
    std::vector<uintptr_t> slots;
    int type_size;
    /*     Frame* caller_frame; */
    /*     uintptr_t* caller_sp; */
    /*     CodePntr caller_pc; */
    Core* sender;
};

bool operator==(Message& msg1, Message& msg2);
std::ostream& operator<<(std::ostream& os, const Message& msg);
void show_msg_detail(std::ostream& os, int id, Message* msg);

//-----------------------------------
bool is_valid_certain_msg(Message* msg);

#endif // MESSAGE_H
