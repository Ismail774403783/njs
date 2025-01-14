
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) NGINX, Inc.
 */

#ifndef _NJS_VALUE_H_INCLUDED_
#define _NJS_VALUE_H_INCLUDED_


/*
 * The order of the enum is used in njs_vmcode_typeof()
 * and njs_object_prototype_to_string().
 */

typedef enum {
    NJS_NULL                  = 0x00,
    NJS_UNDEFINED             = 0x01,

    /* The order of the above type is used in njs_is_null_or_undefined(). */

    NJS_BOOLEAN               = 0x02,
    /*
     * The order of the above type is used in
     * njs_is_null_or_undefined_or_boolean().
     */
    NJS_NUMBER                = 0x03,
    /*
     * The order of the above type is used in njs_is_numeric().
     * Booleans, null and void values can be used in mathematical operations:
     *   a numeric value of the true value is one,
     *   a numeric value of the null and false values is zero,
     *   a numeric value of the void value is NaN.
     */
    NJS_SYMBOL                = 0x04,

    NJS_STRING                = 0x05,

    /* The order of the above type is used in njs_is_primitive(). */

    NJS_DATA                  = 0x06,

    /* The type is external code. */
    NJS_EXTERNAL              = 0x07,

    /*
     * The invalid value type is used:
     *   for uninitialized array members,
     *   to detect non-declared explicitly or implicitly variables,
     *   for native property getters.
     */
    NJS_INVALID               = 0x08,

    /*
     * The object types are >= NJS_OBJECT, this is used in njs_is_object().
     * NJS_OBJECT_BOOLEAN, NJS_OBJECT_NUMBER, and NJS_OBJECT_STRING must be
     * in the same order as NJS_BOOLEAN, NJS_NUMBER, and NJS_STRING.  It is
     * used in njs_primitive_prototype_index().  The order of object types
     * is used in vm->prototypes and vm->constructors arrays.
     */
    NJS_OBJECT                = 0x10,
    NJS_ARRAY                 = 0x11,
    NJS_OBJECT_BOOLEAN        = 0x12,
    NJS_OBJECT_NUMBER         = 0x13,
    NJS_OBJECT_SYMBOL         = 0x14,
    NJS_OBJECT_STRING         = 0x15,
    NJS_FUNCTION              = 0x16,
    NJS_REGEXP                = 0x17,
    NJS_DATE                  = 0x18,
    NJS_OBJECT_VALUE          = 0x19,
    NJS_ARRAY_BUFFER          = 0x1A,
    NJS_VALUE_TYPE_MAX
} njs_value_type_t;


typedef struct njs_object_prop_s      njs_object_prop_t;
typedef struct njs_string_s           njs_string_t;
typedef struct njs_object_s           njs_object_t;
typedef struct njs_object_value_s     njs_object_value_t;
typedef struct njs_function_lambda_s  njs_function_lambda_t;
typedef struct njs_regexp_pattern_s   njs_regexp_pattern_t;
typedef struct njs_array_s            njs_array_t;
typedef struct njs_array_buffer_s     njs_array_buffer_t;
typedef struct njs_regexp_s           njs_regexp_t;
typedef struct njs_date_s             njs_date_t;
typedef struct njs_property_next_s    njs_property_next_t;
typedef struct njs_object_init_s      njs_object_init_t;


/*
 * njs_prop_handler_t operates as a property getter and/or setter.
 * The handler receives NULL setval if it is invoked in GET context and
 * non-null otherwise.
 *
 * njs_prop_handler_t is expected to return:
 *   NJS_OK - handler executed successfully;
 *   NJS_ERROR - some error, vm->retval contains appropriate exception;
 *   NJS_DECLINED - handler was applied to inappropriate object, vm->retval
 *   contains undefined value.
 */
typedef njs_int_t (*njs_prop_handler_t) (njs_vm_t *vm, njs_object_prop_t *prop,
    njs_value_t *value, njs_value_t *setval, njs_value_t *retval);
typedef njs_int_t (*njs_function_native_t) (njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t retval);

#if (!NJS_HAVE_GCC_ATTRIBUTE_ALIGNED)
#error "aligned attribute is required"
#endif

union njs_value_s {
    /*
     * The njs_value_t size is 16 bytes and must be aligned to 16 bytes
     * to provide 4 bits to encode scope in njs_index_t.  This space is
     * used to store short strings.  The maximum size of a short string
     * is 14 (NJS_STRING_SHORT).  If the short_string.size field is 15
     * (NJS_STRING_LONG) then the size is in the long_string.size field
     * and the long_string.data field points to a long string.
     *
     * The number of the string types is limited to 2 types to minimize
     * overhead of processing string fields.  It is also possible to add
     * strings with size from 14 to 254 which size and length are stored in
     * the string_size and string_length byte wide fields.  This will lessen
     * the maximum size of short string to 13.
     */
    struct {
        njs_value_type_t              type:8;  /* 6 bits */
        /*
         * The truth field is set during value assignment and then can be
         * quickly tested by logical and conditional operations regardless
         * of value type.  The truth field coincides with short_string.size
         * and short_string.length so when string size and length are zero
         * the string's value is false.
         */
        uint8_t                       truth;

        uint16_t                      magic16;
        uint32_t                      magic32;

        union {
            double                    number;
            njs_object_t              *object;
            njs_array_t               *array;
            njs_array_buffer_t        *array_buffer;
            njs_object_value_t        *object_value;
            njs_function_t            *function;
            njs_function_lambda_t     *lambda;
            njs_regexp_t              *regexp;
            njs_date_t                *date;
            njs_prop_handler_t        prop_handler;
            njs_value_t               *value;
            njs_property_next_t       *next;
            void                      *data;
        } u;
    } data;

    struct {
        njs_value_type_t              type:8;  /* 6 bits */

#define NJS_STRING_SHORT              14
#define NJS_STRING_LONG               15

        uint8_t                       size:4;
        uint8_t                       length:4;

        u_char                        start[NJS_STRING_SHORT];
    } short_string;

    struct {
        njs_value_type_t              type:8;  /* 6 bits */
        uint8_t                       truth;

        /* 0xff if data is external string. */
        uint8_t                       external;
        uint8_t                       _spare;

        uint32_t                      size;
        njs_string_t                  *data;
    } long_string;

    struct {
        njs_value_type_t              type:8;  /* 6 bits */
        uint8_t                       truth;

        uint16_t                      _spare;

        uint32_t                      index;
        const njs_extern_t            *proto;
    } external;

    njs_value_type_t                  type:8;  /* 6 bits */
};


struct njs_object_s {
    /* A private hash of njs_object_prop_t. */
    njs_lvlhsh_t                      hash;

    /* A shared hash of njs_object_prop_t. */
    njs_lvlhsh_t                      shared_hash;

    /* An object __proto__. */
    njs_object_t                      *__proto__;

    /* The type is used in constructor prototypes. */
    njs_value_type_t                  type:8;
    uint8_t                           shared;     /* 1 bit */

    uint8_t                           extensible:1;
    uint8_t                           error_data:1;
};


struct njs_object_value_s {
    njs_object_t                      object;
    /* The value can be unaligned since it never used in nJSVM operations. */
    njs_value_t                       value;
};


struct njs_array_s {
    njs_object_t                      object;
    uint32_t                          size;
    uint32_t                          length;
    njs_value_t                       *start;
    njs_value_t                       *data;
};


struct njs_array_buffer_s {
    njs_object_t                      object;
    size_t                            size;
    union {
        uint8_t                       *u8;
        uint16_t                      *u16;
        uint32_t                      *u32;
        uint64_t                      *u64;
        int8_t                        *i8;
        int16_t                       *i16;
        int32_t                       *i32;
        int64_t                       *i64;
        float                         *f32;
        double                        *f64;

        void                          *data;
    } u;
};


typedef struct {
    union {
        uint32_t                      count;
        njs_value_t                   values;
    } u;

    njs_value_t                       values[1];
} njs_closure_t;


struct njs_function_s {
    njs_object_t                      object;

    uint8_t                           args_offset;

    uint8_t                           args_count:5;

    /*
     * If "closure" is true njs_closure_t[] is available right after the
     * njs_function_t and njs_function_closures() may be used to access it.
     */

#define njs_function_closures(function)                                      \
    ((njs_closure_t **) ((u_char *) function + sizeof(njs_function_t)))

    uint8_t                           closure:1;
    uint8_t                           native:1;
    uint8_t                           ctor:1;

    uint8_t                           magic;

    union {
        njs_function_lambda_t         *lambda;
        njs_function_native_t         native;
        njs_function_t                *bound_target;
    } u;

    njs_value_t                       *bound;
};


struct njs_regexp_s {
    njs_object_t                      object;
    njs_value_t                       last_index;
    njs_regexp_pattern_t              *pattern;
    /*
     * This string value can be unaligned since
     * it never used in nJSVM operations.
     */
    njs_value_t                       string;
};


struct njs_date_s {
    njs_object_t                      object;
    double                            time;
};


typedef union {
    njs_object_t                      object;
    njs_object_value_t                object_value;
    njs_array_t                       array;
    njs_function_t                    function;
    njs_regexp_t                      regexp;
    njs_date_t                        date;
} njs_object_prototype_t;


typedef struct {
    njs_function_t            constructor;
    const njs_object_init_t   *constructor_props;
    const njs_object_init_t   *prototype_props;
    njs_object_prototype_t    prototype_value;
} njs_object_type_init_t;


typedef enum {
    NJS_ENUM_KEYS,
    NJS_ENUM_VALUES,
    NJS_ENUM_BOTH,
} njs_object_enum_t;


typedef enum {
    NJS_ENUM_STRING = 1,
    NJS_ENUM_SYMBOL = 2,
} njs_object_enum_type_t;


typedef enum {
    NJS_PROPERTY = 0,
    NJS_PROPERTY_REF,
    NJS_PROPERTY_HANDLER,
    NJS_WHITEOUT,
} njs_object_prop_type_t;


/*
 * Attributes are generally used as Boolean values.
 * The UNSET value is can be seen:
 * for newly created property descriptors in njs_define_property(),
 * for writable attribute of accessor descriptors (desc->writable
 * cannot be used as a boolean value).
 */
typedef enum {
    NJS_ATTRIBUTE_FALSE = 0,
    NJS_ATTRIBUTE_TRUE = 1,
    NJS_ATTRIBUTE_UNSET,
} njs_object_attribute_t;


struct njs_object_prop_s {
    /* Must be aligned to njs_value_t. */
    njs_value_t                 value;
    njs_value_t                 name;
    njs_value_t                 getter;
    njs_value_t                 setter;

    /* TODO: get rid of types */
    njs_object_prop_type_t      type:8;          /* 3 bits */

    njs_object_attribute_t      writable:8;      /* 2 bits */
    njs_object_attribute_t      enumerable:8;    /* 2 bits */
    njs_object_attribute_t      configurable:8;  /* 2 bits */
};


typedef struct {
    njs_lvlhsh_query_t          lhq;

    /* scratch is used to get the value of an NJS_PROPERTY_HANDLER property. */
    njs_object_prop_t           scratch;

    /* These three fields are used for NJS_EXTERNAL setters. */
    uintptr_t                   ext_data;
    const njs_extern_t          *ext_proto;
    uint32_t                    ext_index;

    njs_value_t                 key;
    njs_object_t                *prototype;
    njs_object_prop_t           *own_whiteout;
    uint8_t                     query;
    uint8_t                     shared;
    uint8_t                     own;
} njs_property_query_t;


#define njs_value(_type, _truth, _number) {                                   \
    .data = {                                                                 \
        .type = _type,                                                        \
        .truth = _truth,                                                      \
        .u.number = _number,                                                  \
    }                                                                         \
}


#define njs_wellknown_symbol(key) {                                           \
    .data = {                                                                 \
        .type = NJS_SYMBOL,                                                   \
        .truth = 1,                                                           \
        .magic32 = key,                                                       \
        .u = { .value = NULL }                                                \
    }                                                                         \
}


#define njs_string(s) {                                                       \
    .short_string = {                                                         \
        .type = NJS_STRING,                                                   \
        .size = njs_length(s),                                                \
        .length = njs_length(s),                                              \
        .start = s,                                                           \
    }                                                                         \
}


/* NJS_STRING_LONG is set for both big and little endian platforms. */

#define njs_long_string(s) {                                                  \
    .long_string = {                                                          \
        .type = NJS_STRING,                                                   \
        .truth = (NJS_STRING_LONG << 4) | NJS_STRING_LONG,                    \
        .size = njs_length(s),                                                \
        .data = & (njs_string_t) {                                            \
            .start = (u_char *) s,                                            \
            .length = njs_length(s),                                          \
        }                                                                     \
    }                                                                         \
}


#define _njs_function(_function, _args_count, _ctor, _magic) {                \
    .native = 1,                                                              \
    .magic = _magic,                                                          \
    .args_count = _args_count,                                                \
    .ctor = _ctor,                                                            \
    .args_offset = 1,                                                         \
    .u.native = _function,                                                    \
    .object = { .type = NJS_FUNCTION,                                         \
                .shared = 1,                                                  \
                .extensible = 1 },                                            \
}


#define _njs_native_function(_func, _args, _ctor, _magic) {                   \
    .data = {                                                                 \
        .type = NJS_FUNCTION,                                                 \
        .truth = 1,                                                           \
        .u.function = & (njs_function_t) _njs_function(_func, _args,          \
                                                       _ctor, _magic)         \
    }                                                                         \
}


#define njs_native_function(_function, _args_count)                           \
    _njs_native_function(_function, _args_count, 0, 0)


#define njs_native_function2(_function, _args_count, _magic)                  \
    _njs_native_function(_function, _args_count, 0, _magic)


#define njs_native_ctor(_function, _args_count, _magic)                       \
    _njs_function(_function, _args_count, 1, _magic)


#define njs_prop_handler(_handler) {                                          \
    .data = {                                                                 \
        .type = NJS_INVALID,                                                  \
        .truth = 1,                                                           \
        .u = { .prop_handler = _handler }                                     \
    }                                                                         \
}


#define njs_prop_handler2(_handler, _magic16, _magic32) {                     \
    .data = {                                                                 \
        .type = NJS_INVALID,                                                  \
        .truth = 1,                                                           \
        .magic16 = _magic16,                                                  \
        .magic32 = _magic32,                                                  \
        .u = { .prop_handler = _handler }                                     \
    }                                                                         \
}


#define njs_is_null(value)                                                    \
    ((value)->type == NJS_NULL)


#define njs_is_undefined(value)                                               \
    ((value)->type == NJS_UNDEFINED)


#define njs_is_defined(value)                                                 \
    ((value)->type != NJS_UNDEFINED)


#define njs_is_null_or_undefined(value)                                       \
    ((value)->type <= NJS_UNDEFINED)


#define njs_is_boolean(value)                                                 \
    ((value)->type == NJS_BOOLEAN)


#define njs_is_null_or_undefined_or_boolean(value)                            \
    ((value)->type <= NJS_BOOLEAN)


#define njs_is_true(value)                                                    \
    ((value)->data.truth != 0)


#define njs_is_number(value)                                                  \
    ((value)->type == NJS_NUMBER)


/* Testing for NaN first generates a better code at least on i386/amd64. */

#define njs_is_number_true(num)                                               \
    (!isnan(num) && num != 0)


#define njs_is_numeric(value)                                                 \
    ((value)->type <= NJS_NUMBER)


#define njs_is_symbol(value)                                                  \
    ((value)->type == NJS_SYMBOL)


#define njs_is_string(value)                                                  \
    ((value)->type == NJS_STRING)


#define njs_is_key(value)                                                     \
    (njs_is_string(value) || njs_is_symbol(value))


/*
 * The truth field coincides with short_string.size and short_string.length
 * so when string size and length are zero the string's value is false and
 * otherwise is true.
 */
#define njs_string_truth(value, size)


#define njs_string_get(value, str)                                            \
    do {                                                                      \
        if ((value)->short_string.size != NJS_STRING_LONG) {                  \
            (str)->length = (value)->short_string.size;                       \
            (str)->start = (u_char *) (value)->short_string.start;            \
                                                                              \
        } else {                                                              \
            (str)->length = (value)->long_string.size;                        \
            (str)->start = (u_char *) (value)->long_string.data->start;       \
        }                                                                     \
    } while (0)


#define njs_string_short_start(value)                                         \
    (value)->short_string.start


#define njs_string_short_set(value, _size, _length)                           \
    do {                                                                      \
        (value)->type = NJS_STRING;                                           \
        njs_string_truth(value, _size);                                       \
        (value)->short_string.size = _size;                                   \
        (value)->short_string.length = _length;                               \
    } while (0)


#define njs_string_length_set(value, _length)                                 \
    do {                                                                      \
        if ((value)->short_string.size != NJS_STRING_LONG) {                  \
            (value)->short_string.length = length;                            \
                                                                              \
        } else {                                                              \
            (value)->long_string.data->length = length;                       \
        }                                                                     \
    } while (0)

#define njs_is_primitive(value)                                               \
    ((value)->type <= NJS_STRING)


#define njs_is_data(value)                                                    \
    ((value)->type == NJS_DATA)


#define njs_is_object(value)                                                  \
    ((value)->type >= NJS_OBJECT)


#define njs_has_prototype(vm, value, proto)                                   \
    (((njs_object_prototype_t *)                                              \
        njs_object(value)->__proto__ - (vm)->prototypes) == proto)


#define njs_is_object_value(value)                                            \
    ((value)->type == NJS_OBJECT_VALUE)


#define njs_is_object_string(value)                                           \
    ((value)->type == NJS_OBJECT_STRING)


#define njs_object_value_type(type)                                           \
    (type + NJS_OBJECT)


#define njs_is_array(value)                                                   \
    ((value)->type == NJS_ARRAY)


#define njs_is_array_buffer(value)                                            \
    ((value)->type == NJS_ARRAY_BUFFER)


#define njs_is_function(value)                                                \
    ((value)->type == NJS_FUNCTION)


#define njs_is_function_or_undefined(value)                                   \
    ((value)->type == NJS_FUNCTION || (value)->type == NJS_UNDEFINED)


#define njs_is_regexp(value)                                                  \
    ((value)->type == NJS_REGEXP)


#define njs_is_date(value)                                                    \
    ((value)->type == NJS_DATE)


#define njs_is_error(value)                                                   \
    ((value)->type == NJS_OBJECT && njs_object(value)->error_data)


#define njs_is_external(value)                                                \
    ((value)->type == NJS_EXTERNAL)


#define njs_is_valid(value)                                                   \
    ((value)->type != NJS_INVALID)


#define njs_bool(value)                                                       \
    ((value)->data.truth)


#define njs_number(value)                                                     \
    ((value)->data.u.number)


#define njs_data(value)                                                       \
    ((value)->data.u.data)


#define njs_function(value)                                                   \
    ((value)->data.u.function)


#define njs_function_lambda(value)                                            \
    ((value)->data.u.function->u.lambda)


#define njs_object(value)                                                     \
    ((value)->data.u.object)


#define njs_object_hash(value)                                                \
    (&(value)->data.u.object->hash)


#define njs_object_hash_is_empty(value)                                       \
    (njs_lvlhsh_is_empty(njs_object_hash(value)))


#define njs_array(value)                                                      \
    ((value)->data.u.array)


#define njs_array_len(value)                                                  \
    ((value)->data.u.array->length)


#define njs_array_buffer(value)                                               \
    ((value)->data.u.array_buffer)


#define njs_array_start(value)                                                \
    ((value)->data.u.array->start)


#define njs_date(value)                                                       \
    ((value)->data.u.date)


#define njs_regexp(value)                                                     \
    ((value)->data.u.regexp)


#define njs_regexp_pattern(value)                                             \
    ((value)->data.u.regexp->pattern)


#define njs_object_value(_value)                                              \
    (&(_value)->data.u.object_value->value)


#define njs_set_undefined(value)                                              \
    *(value) = njs_value_undefined


#define njs_set_true(value)                                                   \
    *(value) = njs_value_true


#define njs_set_false(value)                                                  \
    *(value) = njs_value_false


#define njs_symbol_key(value)                                                 \
    ((value)->data.magic32)


#define njs_symbol_eq(value1, value2)                                         \
    (njs_symbol_key(value1) == njs_symbol_key(value2))


extern const njs_value_t  njs_value_null;
extern const njs_value_t  njs_value_undefined;
extern const njs_value_t  njs_value_false;
extern const njs_value_t  njs_value_true;
extern const njs_value_t  njs_value_zero;
extern const njs_value_t  njs_value_nan;
extern const njs_value_t  njs_value_invalid;

extern const njs_value_t  njs_string_empty;
extern const njs_value_t  njs_string_comma;
extern const njs_value_t  njs_string_null;
extern const njs_value_t  njs_string_undefined;
extern const njs_value_t  njs_string_boolean;
extern const njs_value_t  njs_string_false;
extern const njs_value_t  njs_string_true;
extern const njs_value_t  njs_string_number;
extern const njs_value_t  njs_string_minus_zero;
extern const njs_value_t  njs_string_minus_infinity;
extern const njs_value_t  njs_string_plus_infinity;
extern const njs_value_t  njs_string_nan;
extern const njs_value_t  njs_string_symbol;
extern const njs_value_t  njs_string_string;
extern const njs_value_t  njs_string_data;
extern const njs_value_t  njs_string_name;
extern const njs_value_t  njs_string_external;
extern const njs_value_t  njs_string_invalid;
extern const njs_value_t  njs_string_object;
extern const njs_value_t  njs_string_function;
extern const njs_value_t  njs_string_memory_error;


njs_inline void
njs_set_boolean(njs_value_t *value, unsigned yn)
{
    const njs_value_t  *retval;

    /* Using const retval generates a better code at least on i386/amd64. */
    retval = (yn) ? &njs_value_true : &njs_value_false;

    *value = *retval;
}


njs_inline void
njs_set_number(njs_value_t *value, double num)
{
    value->data.u.number = num;
    value->type = NJS_NUMBER;
    value->data.truth = njs_is_number_true(num);
}


njs_inline void
njs_set_int32(njs_value_t *value, int32_t num)
{
    value->data.u.number = num;
    value->type = NJS_NUMBER;
    value->data.truth = (num != 0);
}


njs_inline void
njs_set_uint32(njs_value_t *value, uint32_t num)
{
    value->data.u.number = num;
    value->type = NJS_NUMBER;
    value->data.truth = (num != 0);
}


njs_inline void
njs_set_data(njs_value_t *value, void *data)
{
    value->data.u.data = data;
    value->type = NJS_DATA;
    value->data.truth = 1;
}


njs_inline void
njs_set_object(njs_value_t *value, njs_object_t *object)
{
    value->data.u.object = object;
    value->type = NJS_OBJECT;
    value->data.truth = 1;
}


njs_inline void
njs_set_type_object(njs_value_t *value, njs_object_t *object,
    njs_uint_t type)
{
    value->data.u.object = object;
    value->type = type;
    value->data.truth = 1;
}


njs_inline void
njs_set_array(njs_value_t *value, njs_array_t *array)
{
    value->data.u.array = array;
    value->type = NJS_ARRAY;
    value->data.truth = 1;
}


njs_inline void
njs_set_array_buffer(njs_value_t *value, njs_array_buffer_t *array)
{
    value->data.u.array_buffer = array;
    value->type = NJS_ARRAY_BUFFER;
    value->data.truth = 1;
}


njs_inline void
njs_set_function(njs_value_t *value, njs_function_t *function)
{
    value->data.u.function = function;
    value->type = NJS_FUNCTION;
    value->data.truth = 1;
}


njs_inline void
njs_set_date(njs_value_t *value, njs_date_t *date)
{
    value->data.u.date = date;
    value->type = NJS_DATE;
    value->data.truth = 1;
}


njs_inline void
njs_set_regexp(njs_value_t *value, njs_regexp_t *regexp)
{
    value->data.u.regexp = regexp;
    value->type = NJS_REGEXP;
    value->data.truth = 1;
}


njs_inline void
njs_set_object_value(njs_value_t *value, njs_object_value_t *object_value)
{
    value->data.u.object_value = object_value;
    value->type = NJS_OBJECT_VALUE;
    value->data.truth = 1;
}


#define njs_set_invalid(value)                                                \
    (value)->type = NJS_INVALID


#if 0 /* GC: todo */

#define njs_retain(value)                                                     \
    do {                                                                      \
        if ((value)->data.truth == NJS_STRING_LONG) {                         \
            njs_value_retain(value);                                          \
        }                                                                     \
    } while (0)


#define njs_release(vm, value)                                                \
    do {                                                                      \
        if ((value)->data.truth == NJS_STRING_LONG) {                         \
            njs_value_release((vm), (value));                                 \
        }                                                                     \
    } while (0)

#else

#define njs_retain(value)
#define njs_release(vm, value)

#endif


#define njs_property_query_init(pq, _query, _own)                             \
    do {                                                                      \
        (pq)->lhq.key.length = 0;                                             \
        (pq)->lhq.key.start = NULL;                                           \
        (pq)->lhq.value = NULL;                                               \
        (pq)->own_whiteout = NULL;                                            \
        (pq)->query = _query;                                                 \
        (pq)->shared = 0;                                                     \
        (pq)->own = _own;                                                     \
    } while (0)


void njs_value_retain(njs_value_t *value);
void njs_value_release(njs_vm_t *vm, njs_value_t *value);
njs_int_t njs_value_to_primitive(njs_vm_t *vm, njs_value_t *dst,
    njs_value_t *value, njs_uint_t hint);
njs_array_t *njs_value_enumerate(njs_vm_t *vm, const njs_value_t *value,
    njs_object_enum_t kind, njs_object_enum_type_t type, njs_bool_t all);
njs_array_t *njs_value_own_enumerate(njs_vm_t *vm, const njs_value_t *value,
    njs_object_enum_t kind, njs_object_enum_type_t type, njs_bool_t all);
njs_int_t njs_value_length(njs_vm_t *vm, njs_value_t *value, uint32_t *dest);
const char *njs_type_string(njs_value_type_t type);

njs_int_t njs_primitive_value_to_string(njs_vm_t *vm, njs_value_t *dst,
    const njs_value_t *src);
njs_int_t njs_primitive_value_to_chain(njs_vm_t *vm, njs_chb_t *chain,
    const njs_value_t *src);
double njs_string_to_number(const njs_value_t *value, njs_bool_t parse_float);

njs_bool_t njs_string_eq(const njs_value_t *v1, const njs_value_t *v2);

njs_int_t njs_property_query(njs_vm_t *vm, njs_property_query_t *pq,
    njs_value_t *value, njs_value_t *key);
njs_int_t njs_value_property(njs_vm_t *vm, njs_value_t *value,
    njs_value_t *key, njs_value_t *retval);
njs_int_t njs_value_property_set(njs_vm_t *vm, njs_value_t *value,
    njs_value_t *key, njs_value_t *setval);
njs_int_t njs_value_property_delete(njs_vm_t *vm, njs_value_t *value,
    njs_value_t *key, njs_value_t *removed);
njs_int_t njs_value_to_object(njs_vm_t *vm, njs_value_t *value);

void njs_symbol_conversion_failed(njs_vm_t *vm, njs_bool_t to_string);

njs_int_t njs_value_species_constructor(njs_vm_t *vm, njs_value_t *object,
    njs_value_t *default_constructor, njs_value_t *dst);


njs_inline njs_bool_t
njs_values_same_non_numeric(const njs_value_t *val1, const njs_value_t *val2)
{
    if (njs_is_string(val1)) {
        return njs_string_eq(val1, val2);
    }

    if (njs_is_symbol(val1)) {
        return njs_symbol_eq(val1, val2);
    }

    return (njs_object(val1) == njs_object(val2));
}


njs_inline njs_bool_t
njs_values_strict_equal(const njs_value_t *val1, const njs_value_t *val2)
{
    if (val1->type != val2->type) {
        return 0;
    }

    if (njs_is_numeric(val1)) {

        if (njs_is_undefined(val1)) {
            return 1;
        }

        /* Infinities are handled correctly by comparision. */
        return (njs_number(val1) == njs_number(val2));
    }

    return njs_values_same_non_numeric(val1, val2);
}


njs_inline njs_bool_t
njs_values_same(const njs_value_t *val1, const njs_value_t *val2)
{
    double  num1, num2;

    if (val1->type != val2->type) {
        return 0;
    }

    if (njs_is_numeric(val1)) {

        if (njs_is_undefined(val1)) {
            return 1;
        }

        num1 = njs_number(val1);
        num2 = njs_number(val2);

        if (njs_slow_path(isnan(num1) && isnan(num2))) {
            return 1;
        }

        if (njs_slow_path(num1 == 0 && num2 == 0
                          && (signbit(num1) ^ signbit(num2))))
        {
            return 0;
        }

        /* Infinities are handled correctly by comparision. */
        return num1 == num2;
    }

    return njs_values_same_non_numeric(val1, val2);
}


njs_inline njs_bool_t
njs_values_same_zero(const njs_value_t *val1, const njs_value_t *val2)
{
    double  num1, num2;

    if (val1->type != val2->type) {
        return 0;
    }

    if (njs_is_numeric(val1)) {

        if (njs_is_undefined(val1)) {
            return 1;
        }

        num1 = njs_number(val1);
        num2 = njs_number(val2);

        if (njs_slow_path(isnan(num1) && isnan(num2))) {
            return 1;
        }

        /* Infinities are handled correctly by comparision. */
        return num1 == num2;
    }

    return njs_values_same_non_numeric(val1, val2);
}


#endif /* _NJS_VALUE_H_INCLUDED_ */
