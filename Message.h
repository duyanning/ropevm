#ifndef MESSAGE_H
#define MESSAGE_H

class MethodBlock;
class FieldBlock;
class Frame;

class Message {
public:
    enum Type {
        invoke, ret,
        get, put,
        //ack,
        arrayload, arraystore//,
        //arraylength
    };
    Message(Type t, Object* source_object, Object* target_ojbect);
    Type get_type();
    virtual ~Message();
    bool is_equal_to(Message& msg);
    virtual void show(std::ostream& os) const = 0;
    virtual void show_detail(std::ostream& os, int id) const = 0;
    Object* get_source_object() { return m_source_object; }
    Object* get_target_object() { return m_target_object; }
private:
    virtual bool equal(Message& msg) = 0;
public:
    /*     int src_id; // just for debug */
    /*     int dst_id; // just for debug */
    int c; // just for debug
protected:
    Type type;
    Object* m_source_object;
    Object* m_target_object;
};

class Object;

class InvokeMsg : public Message {
public:
    InvokeMsg(Object* object, MethodBlock* mb, Frame* prev, Object* calling_object, uintptr_t* args, uintptr_t* caller_sp, CodePntr caller_pc, Object* calling_owner = 0);
    virtual bool equal(Message& msg);
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

bool operator==(Message& msg1, Message& msg2);
std::ostream& operator<<(std::ostream& os, const Message& msg);
void show_msg_detail(std::ostream& os, int id, Message* msg);

//-----------------------------------
bool is_valid_certain_msg(Message* msg);

#endif // MESSAGE_H
