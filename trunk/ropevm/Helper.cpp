#include "std.h"
#include "jam.h"
#include "Helper.h"

using namespace std;

bool begin_with(const string& s, const string prefix)
{
    if (s.find(prefix) != string::npos)
        return true;
    return false;
}

bool is_std_class_name(const string& name)
{
    if (begin_with(name, "java/"))
        return true;
    if (begin_with(name, "gnu/"))
        return true;
    return false;
}
// -----------------------------------------

bool is_class_class(Class* c)
{
    return IS_CLASS_CLASS(c->classblock());
}

bool is_array_class(Class* c)
{
    return IS_ARRAY(c->classblock());
}

bool is_array_object(Object* o)
{
    return is_array_class(o->classobj);
}
// -----------------------------------------
bool is_class_obj(Object* o)
{
    return o->classobj == java_lang_Class;
    //return IS_CLASS(o);
}

bool is_normal_obj(Object* o)
{
    return not is_class_obj(o);
}
// -----------------------------------------

bool is_std_obj(Object* o)
{
    assert(o);

    if (is_normal_obj(o)) {
        return is_std_class(o->classobj);
    }
    else {
        return is_std_class((Class*)o);
    }
}

bool is_app_obj(Object* o)
{
    if (is_normal_obj(o)) {
        return is_app_class(o->classobj);
    }
    else {
        return is_app_class((Class*)o);
    }
}
// -----------------------------------------
bool is_std_class(Class* c)
{
    return is_std_class_name(c->name());
}

bool is_app_class(Class* c)
{
    if (is_std_class(c))
        return false;
    if (is_array_class(c))
        return false;
    return true;
}
// -----------------------------------------

std::string type_name(Object* o)
{
    if (is_normal_obj(o)) {
        return o->classobj->name();
    }
    if  (is_class_obj(o)) {
        return string("c") + ((Class*)o)->name();
    }
    return "";
}


namespace {
const char* opcode_names[] = {
    "nop",
    "aconst_null",
    "iconst_m1",
    "iconst_0",
    "iconst_1",
    "iconst_2",
    "iconst_3",
    "iconst_4",
    "iconst_5",
    "lconst_0",
    "lconst_1",
    "fconst_0",
    "fconst_1",
    "fconst_2",
    "dconst_0",
    "dconst_1",
    "bipush",
    "sipush",
    "ldc",
    "ldc_w",
    "ldc2_w",
    "iload",
    "lload",
    "fload",
    "dload",
    "aload",
    "iload_0",
    "iload_1",
    "iload_2",
    "iload_3",
    "lload_0",
    "lload_1",
    "lload_2",
    "lload_3",
    "fload_0",
    "fload_1",
    "fload_2",
    "fload_3",
    "dload_0",
    "dload_1",
    "dload_2",
    "dload_3",
    "aload_0",
    "aload_1",
    "aload_2",
    "aload_3",
    "iaload",
    "laload",
    "faload",
    "daload",
    "aaload",
    "baload",
    "caload",
    "saload",
    "istore",
    "lstore",
    "fstore",
    "dstore",
    "astore",
    "istore_0",
    "istore_1",
    "istore_2",
    "istore_3",
    "lstore_0",
    "lstore_1",
    "lstore_2",
    "lstore_3",
    "fstore_0",
    "fstore_1",
    "fstore_2",
    "fstore_3",
    "dstore_0",
    "dstore_1",
    "dstore_2",
    "dstore_3",
    "astore_0",
    "astore_1",
    "astore_2",
    "astore_3",
    "iastore",
    "lastore",
    "fastore",
    "dastore",
    "aastore",
    "bastore",
    "castore",
    "sastore",
    "pop",
    "pop2",
    "dup",
    "dup_x1",
    "dup_x2",
    "dup2",
    "dup2_x1",
    "dup2_x2",
    "swap",
    "iadd",
    "ladd",
    "fadd",
    "dadd",
    "isub",
    "lsub",
    "fsub",
    "dsub",
    "imul",
    "lmul",
    "fmul",
    "dmul",
    "idiv",
    "ldiv",
    "fdiv",
    "ddiv",
    "irem",
    "lrem",
    "frem",
    "drem",
    "ineg",
    "lneg",
    "fneg",
    "dneg",
    "ishl",
    "lshl",
    "ishr",
    "lshr",
    "iushr",
    "lushr",
    "iand",
    "land",
    "ior",
    "lor",
    "ixor",
    "lxor",
    "iinc",
    "i2l",
    "i2f",
    "i2d",
    "l2i",
    "l2f",
    "l2d",
    "f2i",
    "f2l",
    "f2d",
    "d2i",
    "d2l",
    "d2f",
    "i2b",
    "i2c",
    "i2s",
    "lcmp",
    "fcmpl",
    "fcmpg",
    "dcmpl",
    "dcmpg",
    "ifeq",
    "ifne",
    "iflt",
    "ifge",
    "ifgt",
    "ifle",
    "if_icmpeq",
    "if_icmpne",
    "if_icmplt",
    "if_icmpge",
    "if_icmpgt",
    "if_icmple",
    "if_acmpeq",
    "if_acmpne",
    "goto",
    "jsr",
    "ret",
    "tableswitch",
    "lookupswitch",
    "ireturn",
    "lreturn",
    "freturn",
    "dreturn",
    "areturn",
    "return",
    "getstatic",
    "putstatic",
    "getfield",
    "putfield",
    "invokevirtual",
    "invokespecial",
    "invokestatic",
    "invokeinterface",
    "new",
    "newarray",
    "anewarray",
    "arraylength",
    "athrow",
    "checkcast",
    "instanceof",
    "monitorenter",
    "monitorexit",
    "wide",
    "multianewarray",
    "ifnull",
    "ifnonnull",
    "goto_w",
    "jsr_w",
    0,
    "ldc_quick",
    "ldc_w_quick",
    0,
    "getfield_quick",
    "putfield_quick",
    "getfield2_quick",
    "putfield2_quick",
    "getstatic_quick",
    "putstatic_quick",
    "getstatic2_quick",
    "putstatic2_quick",
    "invokevirtual_quick",
    "invokenonvirtual_quick",
    "invokesuper_quick",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "invokevirtual_quick_w",
    "getfield_quick_w",
    "putfield_quick_w",
    "getfield_this",
    "lock",
    "aload_this",
    "invokestatic_quick",
    "new_quick",
    0,
    "anewarray_quick",
    0,
    0,
    "checkcast_quick",
    "instanceof_quick",
    0,
    0,
    0,
    "multianewarray_quick",
    "invokeinterface_quick",
    "abstract_method_error",
    "inline_rewriter",
};

}

const char* opcode_name(int op)
{
    //MINILOG0(op);
//     if (op >= sizeof opcode_names / sizeof opcode_names[0]) {
//         cerr << "opcode is: " << op << endl;
//     }
    assert(op < sizeof opcode_names / sizeof opcode_names[0]);
    return opcode_names[op];
}

//--------------------------------------------------------------------------------------
std::string
MethodBlock::full_name()
{
    string class_name(classobj->name());
    return class_name + "." + name;
}
//--------------------------------------------------------------------------------------

bool is_sp_ok(uintptr_t* sp, Frame* frame)
{
    return sp >= frame->ostack_base && sp <= frame->ostack_base + frame->mb->max_stack;
}

bool is_pc_ok(CodePntr pc, MethodBlock* mb)
{
    return pc >= mb->code && pc <= (CodePntr)mb->code + mb->code_size;
}

//--------------------------------
struct Str_bool {
    const char* strval;
    bool boolval;
};

Str_bool strbool[] = {
    {"1", true},
    {"0", false},
    {"on", true},
    {"off", false},
    {"true", true},
    {"false", false},
    {"t", true},
    {"f", false}
};

bool
get_bool(const char* val, const char* defval)
{
    if (val == 0)
        val = defval;

    for (size_t i = 0; i < sizeof strbool / sizeof strbool[0]; ++i) {
        if (strcmp(val, strbool[i].strval) == 0)
            return strbool[i].boolval;
    }
    assert(false);
}

bool is_priviledged(MethodBlock* mb)
{
    // if (mb->classobj->name() == copyUtf8("java/lang/VMMath")
    //     && mb->name == copyUtf8("sqrt"))
    //     return false;

    if (mb->is_native()) return true;
    if (mb->is_synchronized()) return true;

    return false;
}

GroupingPolicyEnum
get_grouping_policy_self(Object* obj)
{
    assert(obj);

    int policy = 0;
    if (is_class_obj(obj)) {
        policy = static_cast<Class*>(obj)->classblock()->class_grouping_policy_self;
    }
    else {
        Class* classobj = obj->classobj;
        policy = classobj->classblock()->grouping_policy_self;
    }
    // else {
    //     assert(false);
    // }

    assert(policy >= 0 and policy <= 3);
    return static_cast<GroupingPolicyEnum>(policy);
}

GroupingPolicyEnum
get_grouping_policy_others(Object* obj)
{
    assert(obj);

    int policy = 0;
    if (is_class_obj(obj)) {
        policy = (GroupingPolicyEnum)static_cast<Class*>(obj)->classblock()->class_grouping_policy_others;
    }
    else {
        Class* classobj = obj->classobj;
        policy = classobj->classblock()->grouping_policy_others;
    }
    // else {
    //     assert(false);
    // }
    assert(policy >= 0 and policy <= 3);
    return static_cast<GroupingPolicyEnum>(policy);
}

string
info(MethodBlock* mb)
{
    ostringstream os;

    if (mb) {
        os << (is_priviledged(mb) ? "p" : "");
        os << (mb->is_static() ? "c" : "");
        os << mb->classobj->name() << '.' << mb->name << ':' << mb->type;
    }
    else {
        os << "mb=0";
    }
    return os.str();
}

string
info(Frame* frame)
{
    ostringstream os;
    if (frame->is_dummy()) {
        os << "dummy";
    }
    else {
        if (frame->mb) {
            os << (is_priviledged(frame->mb) ? "p" : "");
            os << (frame->mb->is_static() ? "c" : "");
            os << frame->mb->classobj->name() << '.' << frame->mb->name << ':' << frame->mb->type;
        }
        else {
            os << "mb=0";
        }
    }
    return os.str();
}

string
info(Object* o)
{
    if (is_normal_obj(o)) {
        return o->classobj->name();
    }
    if  (is_class_obj(o)) {
        return string("c") + ((Class*)o)->name();
    }
    return "";
}
