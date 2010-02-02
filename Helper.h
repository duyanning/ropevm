#ifndef HELPER_H
#define HELPER_H

class Class;
class Object;

bool is_class_class(Class* c);
bool is_class_obj(Object* o);
bool is_normal_obj(Object* o);
bool is_std_obj(Object* o);
bool is_app_obj(Object* o);
bool is_std_class(Class* c);
bool is_app_class(Class* c);
bool is_array_class(Class* c);
bool is_array_object(Object* o);

std::string type_name(Object* o);

const char* opcode_name(int op);

bool is_sp_ok(uintptr_t* sp, Frame* frame);
bool is_pc_ok(CodePntr pc, MethodBlock* mb);

bool get_bool(const char* val, const char* defval = "1");

class MethodBlock;
bool is_priviledged(MethodBlock* mb);

#endif // HELPER_H
