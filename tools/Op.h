#ifndef OP_H
#define OP_H


struct Op {
    string from_class_name;
    string from_object_addr;
    string to_class_name;
    string to_object_addr;
    intmax_t op_no;
    intmax_t frame_no;          // 操作所在的栈桢
    intmax_t caller_frame_no;   // 操作所在栈桢的上级栈桢
};


struct OpEntry {
    intmax_t op_no;
    //string from_class_name;
    string from_object_addr;
    //string to_class_name;
    string to_object_addr;
};


#endif // OP_H
