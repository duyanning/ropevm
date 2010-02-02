/* No method preparation is needed on the
   indirect interpreter */
#define PREPARE_MB(mb)

#define ARRAY_TYPE(pc)        READ_U1_OP(pc)
#define SINGLE_INDEX(pc)      READ_U1_OP(pc)
#define DOUBLE_INDEX(pc)      READ_U2_OP(pc)
#define SINGLE_SIGNED(pc)     READ_S1_OP(pc)
#define DOUBLE_SIGNED(pc)     READ_S2_OP(pc)
#define IINC_LVAR_IDX(pc)     SINGLE_INDEX(pc)
#define IINC_DELTA(pc)        SINGLE_SIGNED(pc + 1)
#define INV_QUICK_ARGS(pc)    READ_U1_OP(pc + 1)
#define INV_QUICK_IDX(pc)     READ_U1_OP(pc)
#define INV_INTF_IDX(pc)      DOUBLE_INDEX(pc)
#define INV_INTF_CACHE(pc)    READ_U1_OP(pc + 3)
#define MULTI_ARRAY_DIM(pc)   READ_U1_OP(pc + 2)
#define RESOLVED_CONSTANT(pc) CP_INFO(cp, SINGLE_INDEX(pc))
#define RESOLVED_FIELD(pc)    ((FieldBlock*)CP_INFO(cp, DOUBLE_INDEX(pc)))
#define RESOLVED_METHOD(pc)   ((MethodBlock*)CP_INFO(cp, DOUBLE_INDEX(pc)))
#define RESOLVED_CLASS(pc)    ((Class *)CP_INFO(cp, DOUBLE_INDEX(pc)))

/* Macros for checking for common exceptions */

/* #define THROW_EXCEPTION(excep_name, message)                               \ */
/* {                                                                          \ */
/*     frame->last_pc = pc;                                                   \ */
/*     signalException(excep_name, message);                                  \ */
/*     goto throwException;                                                   \ */
/* } */

#define NULL_POINTER_CHECK(ref)                                            \
    if(!ref) THROW_EXCEPTION(java_lang_NullPointerException, NULL);


#define MAX_INT_DIGITS 11

#define ARRAY_BOUNDS_CHECK(array, idx)                                     \
{                                                                          \
    if((u4)idx >= ARRAY_LEN(array)) {                                     \
        char buff[MAX_INT_DIGITS];                                         \
        snprintf(buff, MAX_INT_DIGITS, "%d", idx);                         \
        THROW_EXCEPTION(java_lang_ArrayIndexOutOfBoundsException, buff);   \
    }                                                                      \
}

#define ZERO_DIVISOR_CHECK(value)                                          \
    if(value == 0)                                                         \
        THROW_EXCEPTION(java_lang_ArithmeticException, "division by zero");

