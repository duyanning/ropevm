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


inline
std::istream&
operator>>(std::istream& strm, Op& op)
{
    string source_class_name;
    string source_object_addr;
    string arrow;
    string target_class_name;
    string target_object_addr;
    intmax_t op_no;
    intmax_t frame_no;
    intmax_t caller_frame_no;

    strm >> source_class_name >> source_object_addr >> arrow >> target_class_name >> target_object_addr >> op_no
         >> frame_no >> caller_frame_no;

    op.from_class_name = source_class_name;
    op.from_object_addr = source_object_addr;
    op.to_class_name = target_class_name;
    op.to_object_addr = target_object_addr;
    op.op_no = op_no;
    op.frame_no = frame_no;
    op.caller_frame_no = caller_frame_no;

    return strm;
}


//=====================================

// .map文件里节点名里编码的信息
struct OpEntry {
    intmax_t op_no;
    //string from_class_name;
    string from_object_addr;
    //string to_class_name;
    string to_object_addr;

    OpEntry() {}
    OpEntry(const Op& op);
};

inline
OpEntry::OpEntry(const Op& op)
{
    op_no = op.op_no;
    from_object_addr = op.from_object_addr;
    to_object_addr = op.to_object_addr;
}


inline
std::ostream&
operator<<(std::ostream& strm, const OpEntry& op)
{
    strm << op.op_no << " " << op.from_object_addr << " -> " << op.to_object_addr;
    
    return strm;
}


inline
std::istream&
operator>>(std::istream& strm, OpEntry& op)
{
    intmax_t op_no;
    strm >> op_no;
    string from_object_addr;
    strm >> from_object_addr;
    string arrow;
    strm >> arrow;
    string to_object_addr;
    strm >> to_object_addr;
    to_object_addr.pop_back(); // "
    //cout << op_no << endl;
    op.op_no = op_no;
    //cout << "aaa: " << op_no << endl;
    op.from_object_addr = from_object_addr;
    op.to_object_addr = to_object_addr;

    return strm;
}

#endif // OP_H
