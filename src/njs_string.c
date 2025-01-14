
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) NGINX, Inc.
 */


#include <njs_main.h>


#define NJS_TRIM_START  1
#define NJS_TRIM_END    2


typedef struct {
    u_char                     *start;
    size_t                     size;
    njs_value_t                value;
} njs_string_replace_part_t;


#define NJS_SUBST_COPY        255
#define NJS_SUBST_PRECEDING   254
#define NJS_SUBST_FOLLOWING   253


typedef struct {
     uint32_t  type;
     uint32_t  size;
     u_char    *start;
} njs_string_subst_t;


typedef struct {
    njs_value_t                retval;

    njs_arr_t                  parts;
    njs_string_replace_part_t  array[3];
    njs_string_replace_part_t  *part;

    njs_arr_t                  *substitutions;
    njs_function_t             *function;

    njs_regex_match_data_t     *match_data;

    njs_bool_t                 empty;

    njs_utf8_t                 utf8:8;
    njs_regexp_utf8_t          type:8;
} njs_string_replace_t;


static void njs_encode_base64_core(njs_str_t *dst, const njs_str_t *src,
    const u_char *basis, njs_uint_t padding);
static njs_int_t njs_decode_base64_core(njs_vm_t *vm,
    njs_value_t *value, const njs_str_t *src, const u_char *basis);
static njs_int_t njs_string_slice_prop(njs_vm_t *vm, njs_string_prop_t *string,
    njs_slice_prop_t *slice, njs_value_t *args, njs_uint_t nargs);
static njs_int_t njs_string_slice_args(njs_vm_t *vm, njs_slice_prop_t *slice,
    njs_value_t *args, njs_uint_t nargs);
static njs_int_t njs_string_from_char_code(njs_vm_t *vm,
    njs_value_t *args, njs_uint_t nargs, njs_index_t unused);
static njs_int_t njs_string_from_code_point(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused);
static njs_int_t njs_string_bytes_from(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused);
static njs_int_t njs_string_bytes_from_array(njs_vm_t *vm,
    const njs_value_t *value);
static njs_int_t njs_string_bytes_from_string(njs_vm_t *vm,
    const njs_value_t *args, njs_uint_t nargs);
static njs_int_t njs_string_starts_or_ends_with(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_bool_t starts);
static njs_int_t njs_string_trim(njs_vm_t *vm, njs_value_t *value,
    njs_uint_t mode);
static njs_int_t njs_string_prototype_pad(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_bool_t pad_start);
static njs_int_t njs_string_match_multiple(njs_vm_t *vm, njs_value_t *args,
    njs_regexp_pattern_t *pattern);
static njs_int_t njs_string_split_part_add(njs_vm_t *vm, njs_array_t *array,
    njs_utf8_t utf8, const u_char *start, size_t size);
static njs_int_t njs_string_replace_regexp(njs_vm_t *vm, njs_value_t *this,
    njs_value_t *regex, njs_string_replace_t *r);
static njs_int_t njs_string_replace_regexp_function(njs_vm_t *vm,
    njs_value_t *this, njs_value_t *regex, njs_string_replace_t *r,
    int *captures, njs_uint_t n);
static njs_int_t njs_string_replace_regexp_join(njs_vm_t *vm,
    njs_string_replace_t *r);
static njs_int_t njs_string_replace_search(njs_vm_t *vm, njs_value_t *this,
    njs_value_t *search, njs_string_replace_t *r);
static njs_int_t njs_string_replace_search_function(njs_vm_t *vm,
        njs_value_t *this, njs_value_t *search, njs_string_replace_t *r);
static njs_int_t njs_string_replace_parse(njs_vm_t *vm,
    njs_string_replace_t *r, u_char *p, u_char *end, size_t size,
    njs_uint_t ncaptures);
static njs_int_t njs_string_replace_substitute(njs_vm_t *vm,
    njs_string_replace_t *r, int *captures);
static njs_int_t njs_string_replace_join(njs_vm_t *vm, njs_string_replace_t *r);
static void njs_string_replacement_copy(njs_string_replace_part_t *string,
    const njs_value_t *value);
static njs_int_t njs_string_encode(njs_vm_t *vm, njs_value_t *value,
    const uint32_t *escape);
static njs_int_t njs_string_decode(njs_vm_t *vm, njs_value_t *value,
    const uint32_t *reserve);


#define njs_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define njs_base64_decoded_length(len)  (((len + 3) / 4) * 3)


njs_int_t
njs_string_set(njs_vm_t *vm, njs_value_t *value, const u_char *start,
    uint32_t size)
{
    u_char        *dst;
    const u_char  *src;
    njs_string_t  *string;

    value->type = NJS_STRING;
    njs_string_truth(value, size);

    if (size <= NJS_STRING_SHORT) {
        value->short_string.size = size;
        value->short_string.length = 0;

        dst = value->short_string.start;
        src = start;

        while (size != 0) {
            /* The maximum size is just 14 bytes. */
            njs_pragma_loop_disable_vectorization;

            *dst++ = *src++;
            size--;
        }

    } else {
        /*
         * Setting UTF-8 length is not required here, it just allows
         * to store the constant in whole byte instead of bit twiddling.
         */
        value->short_string.size = NJS_STRING_LONG;
        value->short_string.length = 0;
        value->long_string.external = 0xff;
        value->long_string.size = size;

        string = njs_mp_alloc(vm->mem_pool, sizeof(njs_string_t));
        if (njs_slow_path(string == NULL)) {
            njs_memory_error(vm);
            return NJS_ERROR;
        }

        value->long_string.data = string;

        string->start = (u_char *) start;
        string->length = 0;
        string->retain = 1;
    }

    return NJS_OK;
}


njs_int_t
njs_string_new(njs_vm_t *vm, njs_value_t *value, const u_char *start,
    uint32_t size, uint32_t length)
{
    u_char  *p;

    p = njs_string_alloc(vm, value, size, length);

    if (njs_fast_path(p != NULL)) {
        memcpy(p, start, size);
        return NJS_OK;
    }

    return NJS_ERROR;
}


u_char *
njs_string_alloc(njs_vm_t *vm, njs_value_t *value, uint64_t size,
    uint64_t length)
{
    uint32_t      total, map_offset, *map;
    njs_string_t  *string;

    if (njs_slow_path(size > NJS_STRING_MAX_LENGTH)) {
        njs_range_error(vm, "invalid string length");
        return NULL;
    }

    value->type = NJS_STRING;
    njs_string_truth(value, size);

    if (size <= NJS_STRING_SHORT) {
        value->short_string.size = size;
        value->short_string.length = length;

        return value->short_string.start;
    }

    /*
     * Setting UTF-8 length is not required here, it just allows
     * to store the constant in whole byte instead of bit twiddling.
     */
    value->short_string.size = NJS_STRING_LONG;
    value->short_string.length = 0;
    value->long_string.external = 0;
    value->long_string.size = size;

    if (size != length && length > NJS_STRING_MAP_STRIDE) {
        map_offset = njs_string_map_offset(size);
        total = map_offset + njs_string_map_size(length);

    } else {
        map_offset = 0;
        total = size;
    }

    string = njs_mp_alloc(vm->mem_pool, sizeof(njs_string_t) + total);

    if (njs_fast_path(string != NULL)) {
        value->long_string.data = string;

        string->start = (u_char *) string + sizeof(njs_string_t);
        string->length = length;
        string->retain = 1;

        if (map_offset != 0) {
            map = (uint32_t *) (string->start + map_offset);
            map[0] = 0;
        }

        return string->start;
    }

    njs_memory_error(vm);

    return NULL;
}


void
njs_string_truncate(njs_value_t *value, uint32_t size)
{
    u_char  *dst, *src;

    if (size <= NJS_STRING_SHORT) {
        if (value->short_string.size != NJS_STRING_LONG) {
            value->short_string.size = size;

        } else {
            value->short_string.size = size;
            dst = value->short_string.start;
            src = value->long_string.data->start;

            while (size != 0) {
                /* The maximum size is just 14 bytes. */
                njs_pragma_loop_disable_vectorization;

                *dst++ = *src++;
                size--;
            }
        }

    } else {
        value->long_string.size = size;
    }
}


njs_int_t
njs_string_hex(njs_vm_t *vm, njs_value_t *value, const njs_str_t *src)
{
    u_char        *p, c;
    size_t        len;
    njs_uint_t    i;
    const u_char  *start;

    static const u_char  hex[16] = "0123456789abcdef";

    len = src->length;
    start = src->start;

    p = njs_string_alloc(vm, value, len * 2, len * 2);

    if (njs_fast_path(p != NULL)) {
        for (i = 0; i < len; i++) {
            c = start[i];
            *p++ = hex[c >> 4];
            *p++ = hex[c & 0x0f];
        }

        return NJS_OK;
    }

    return NJS_ERROR;
}


static void
njs_encode_base64(njs_str_t *dst, const njs_str_t *src)
{
    static u_char   basis64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    njs_encode_base64_core(dst, src, basis64, 1);
}


static void
njs_encode_base64url(njs_str_t *dst, const njs_str_t *src)
{
    static u_char   basis64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    njs_encode_base64_core(dst, src, basis64, 0);
}


static void
njs_encode_base64_core(njs_str_t *dst, const njs_str_t *src,
    const u_char *basis, njs_bool_t padding)
{
   u_char  *d, *s, c0, c1, c2;
   size_t  len;

    len = src->length;
    s = src->start;
    d = dst->start;

    while (len > 2) {
        c0 = s[0];
        c1 = s[1];
        c2 = s[2];

        *d++ = basis[c0 >> 2];
        *d++ = basis[((c0 & 0x03) << 4) | (c1 >> 4)];
        *d++ = basis[((c1 & 0x0f) << 2) | (c2 >> 6)];
        *d++ = basis[c2 & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len > 0) {
        c0 = s[0];
        *d++ = basis[c0 >> 2];

        if (len == 1) {
            *d++ = basis[(c0 & 0x03) << 4];
            if (padding) {
                *d++ = '=';
                *d++ = '=';
            }

        } else {
            c1 = s[1];

            *d++ = basis[((c0 & 0x03) << 4) | (c1 >> 4)];
            *d++ = basis[(c1 & 0x0f) << 2];

            if (padding) {
                *d++ = '=';
            }
        }

    }

    dst->length = d - dst->start;
}


njs_int_t
njs_string_base64(njs_vm_t *vm, njs_value_t *value, const njs_str_t *src)
{
    njs_str_t  dst;

    if (njs_slow_path(src->length == 0)) {
        vm->retval = njs_string_empty;
        return NJS_OK;
    }

    dst.length = njs_base64_encoded_length(src->length);

    dst.start = njs_string_alloc(vm, &vm->retval, dst.length, dst.length);
    if (njs_slow_path(dst.start == NULL)) {
        return NJS_ERROR;
    }

    njs_encode_base64(&dst, src);

    return NJS_OK;
}


njs_int_t
njs_string_base64url(njs_vm_t *vm, njs_value_t *value, const njs_str_t *src)
{
    size_t     padding;
    njs_str_t  dst;

    if (njs_slow_path(src->length == 0)) {
        vm->retval = njs_string_empty;
        return NJS_OK;
    }

    padding = src->length % 3;

    /*
     * Calculating the padding length: 0 -> 0, 1 -> 2, 2 -> 1.
     */
    padding = (4 >> padding) & 0x03;

    dst.length = njs_base64_encoded_length(src->length) - padding;

    dst.start = njs_string_alloc(vm, &vm->retval, dst.length, dst.length);
    if (njs_slow_path(dst.start == NULL)) {
        return NJS_ERROR;
    }

    njs_encode_base64url(&dst, src);

    return NJS_OK;
}


void
njs_string_copy(njs_value_t *dst, njs_value_t *src)
{
    *dst = *src;

    /* GC: long string retain */
}


/*
 * njs_string_validate() validates an UTF-8 string, evaluates its length,
 * sets njs_string_prop_t struct.
 */

njs_int_t
njs_string_validate(njs_vm_t *vm, njs_string_prop_t *string, njs_value_t *value)
{
    u_char    *start;
    size_t    new_size, map_offset;
    ssize_t   size, length;
    uint32_t  *map;

    size = value->short_string.size;

    if (size != NJS_STRING_LONG) {
        string->start = value->short_string.start;
        length = value->short_string.length;

        if (length == 0 && length != size) {
            length = njs_utf8_length(value->short_string.start, size);

            if (njs_slow_path(length < 0)) {
                /* Invalid UTF-8 string. */
                return length;
            }

            value->short_string.length = length;
        }

    } else {
        string->start = value->long_string.data->start;
        size = value->long_string.size;
        length = value->long_string.data->length;

        if (length == 0 && length != size) {
            length = njs_utf8_length(string->start, size);

            if (length != size) {
                if (njs_slow_path(length < 0)) {
                    /* Invalid UTF-8 string. */
                    return length;
                }

                if (length > NJS_STRING_MAP_STRIDE) {
                    /*
                     * Reallocate the long string with offset map
                     * after the string.
                     */
                    map_offset = njs_string_map_offset(size);
                    new_size = map_offset + njs_string_map_size(length);

                    start = njs_mp_alloc(vm->mem_pool, new_size);
                    if (njs_slow_path(start == NULL)) {
                        njs_memory_error(vm);
                        return NJS_ERROR;
                    }

                    memcpy(start, string->start, size);
                    string->start = start;
                    value->long_string.data->start = start;

                    map = (uint32_t *) (start + map_offset);
                    map[0] = 0;
                }
            }

            value->long_string.data->length = length;
        }
    }

    string->size = size;
    string->length = length;

    return length;
}


size_t
njs_string_prop(njs_string_prop_t *string, const njs_value_t *value)
{
    size_t     size;
    uintptr_t  length;

    size = value->short_string.size;

    if (size != NJS_STRING_LONG) {
        string->start = (u_char *) value->short_string.start;
        length = value->short_string.length;

    } else {
        string->start = (u_char *) value->long_string.data->start;
        size = value->long_string.size;
        length = value->long_string.data->length;
    }

    string->size = size;
    string->length = length;

    return (length == 0) ? size : length;
}


static njs_int_t
njs_string_constructor(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t     ret;
    njs_value_t   *value;
    njs_object_t  *object;

    if (nargs == 1) {
        value = njs_value_arg(&njs_string_empty);

    } else {
        value = &args[1];

        if (njs_slow_path(!njs_is_string(value))) {
            if (!vm->top_frame->ctor && njs_is_symbol(value)) {
                return njs_symbol_to_string(vm, &vm->retval, value);
            }

            ret = njs_value_to_string(vm, value, value);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }
        }
    }

    if (vm->top_frame->ctor) {
        object = njs_object_value_alloc(vm, value, value->type);
        if (njs_slow_path(object == NULL)) {
            return NJS_ERROR;
        }

        njs_set_type_object(&vm->retval, object, NJS_OBJECT_STRING);

    } else {
        vm->retval = *value;
    }

    return NJS_OK;
}


static const njs_object_prop_t  njs_string_constructor_properties[] =
{
    /* String.name == "String". */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("name"),
        .value = njs_string("String"),
        .configurable = 1,
    },

    /* String.length == 1. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("length"),
        .value = njs_value(NJS_NUMBER, 1, 1.0),
        .configurable = 1,
    },

    /* String.prototype. */
    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("prototype"),
        .value = njs_prop_handler(njs_object_prototype_create),
    },

    /* String.bytesFrom(). */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("bytesFrom"),
        .value = njs_native_function(njs_string_bytes_from, 0),
        .writable = 1,
        .configurable = 1,
    },

    /* String.fromCharCode(). */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("fromCharCode"),
        .value = njs_native_function(njs_string_from_char_code, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* String.fromCodePoint(), ECMAScript 6. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("fromCodePoint"),
        .value = njs_native_function(njs_string_from_code_point, 1),
        .writable = 1,
        .configurable = 1,
    },
};


const njs_object_init_t  njs_string_constructor_init = {
    njs_string_constructor_properties,
    njs_nitems(njs_string_constructor_properties),
};


static njs_int_t
njs_string_instance_length(njs_vm_t *vm, njs_object_prop_t *prop,
    njs_value_t *value, njs_value_t *setval, njs_value_t *retval)
{
    size_t              size;
    uintptr_t           length;
    njs_object_t        *proto;
    njs_object_value_t  *ov;

    /*
     * This getter can be called for string primitive, String object,
     * String.prototype.  The zero should be returned for the latter case.
     */
    length = 0;

    if (njs_slow_path(njs_is_object(value))) {
        proto = njs_object(value);

        do {
            if (njs_fast_path(proto->type == NJS_OBJECT_STRING)) {
                break;
            }

            proto = proto->__proto__;
        } while (proto != NULL);

        if (proto != NULL) {
            ov = (njs_object_value_t *) proto;
            value = &ov->value;
        }
    }

    if (njs_is_string(value)) {
        size = value->short_string.size;
        length = value->short_string.length;

        if (size == NJS_STRING_LONG) {
            size = value->long_string.size;
            length = value->long_string.data->length;
        }

        length = (length == 0) ? size : length;
    }

    njs_set_number(retval, length);

    njs_release(vm, value);

    return NJS_OK;
}


njs_bool_t
njs_string_eq(const njs_value_t *v1, const njs_value_t *v2)
{
    size_t        size, length1, length2;
    const u_char  *start1, *start2;

    size = v1->short_string.size;

    if (size != v2->short_string.size) {
        return 0;
    }

    if (size != NJS_STRING_LONG) {
        length1 = v1->short_string.length;
        length2 = v2->short_string.length;

        /*
         * Using full memcmp() comparison if at least one string
         * is a Byte string.
         */
        if (length1 != 0 && length2 != 0 && length1 != length2) {
            return 0;
        }

        start1 = v1->short_string.start;
        start2 = v2->short_string.start;

    } else {
        size = v1->long_string.size;

        if (size != v2->long_string.size) {
            return 0;
        }

        length1 = v1->long_string.data->length;
        length2 = v2->long_string.data->length;

        /*
         * Using full memcmp() comparison if at least one string
         * is a Byte string.
         */
        if (length1 != 0 && length2 != 0 && length1 != length2) {
            return 0;
        }

        start1 = v1->long_string.data->start;
        start2 = v2->long_string.data->start;
    }

    return (memcmp(start1, start2, size) == 0);
}


njs_int_t
njs_string_cmp(const njs_value_t *v1, const njs_value_t *v2)
{
    size_t        size, size1, size2;
    njs_int_t     ret;
    const u_char  *start1, *start2;

    size1 = v1->short_string.size;

    if (size1 != NJS_STRING_LONG) {
        start1 = v1->short_string.start;

    } else {
        size1 = v1->long_string.size;
        start1 = v1->long_string.data->start;
    }

    size2 = v2->short_string.size;

    if (size2 != NJS_STRING_LONG) {
        start2 = v2->short_string.start;

    } else {
        size2 = v2->long_string.size;
        start2 = v2->long_string.data->start;
    }

    size = njs_min(size1, size2);

    ret = memcmp(start1, start2, size);

    if (ret != 0) {
        return ret;
    }

    return (size1 - size2);
}


static njs_int_t
njs_string_prototype_value_of(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_value_t  *value;

    value = &args[0];

    if (value->type != NJS_STRING) {

        if (value->type == NJS_OBJECT_STRING) {
            value = njs_object_value(value);

        } else {
            njs_type_error(vm, "unexpected value type:%s",
                           njs_type_string(value->type));
            return NJS_ERROR;
        }
    }

    vm->retval = *value;

    return NJS_OK;
}


/*
 * String.toString([encoding]).
 * Returns the string as is if no additional argument is provided,
 * otherwise converts a byte string into an encoded string: hex, base64,
 * base64url.
 */

static njs_int_t
njs_string_prototype_to_string(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    njs_int_t          ret;
    njs_str_t          enc, str;
    njs_value_t        value;
    njs_string_prop_t  string;

    ret = njs_string_prototype_value_of(vm, args, nargs, unused);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (nargs < 2) {
        return NJS_OK;
    }

    if (njs_slow_path(!njs_is_string(&args[1]))) {
        njs_type_error(vm, "encoding must be a string");
        return NJS_ERROR;
    }

    value = vm->retval;

    (void) njs_string_prop(&string, &value);

    if (njs_slow_path(string.length != 0)) {
        njs_type_error(vm, "argument must be a byte string");
        return NJS_ERROR;
    }

    njs_string_get(&args[1], &enc);

    str.length = string.size;
    str.start = string.start;

    if (enc.length == 3 && memcmp(enc.start, "hex", 3) == 0) {
        return njs_string_hex(vm, &vm->retval, &str);

    } else if (enc.length == 6 && memcmp(enc.start, "base64", 6) == 0) {
        return njs_string_base64(vm, &vm->retval, &str);

    } else if (enc.length == 9 && memcmp(enc.start, "base64url", 9) == 0) {
        return njs_string_base64url(vm, &vm->retval, &str);
    }

    njs_type_error(vm, "Unknown encoding: \"%V\"", &enc);

    return NJS_ERROR;
}


/*
 * String.concat(string2[, ..., stringN]).
 * JavaScript 1.2, ECMAScript 3.
 */

njs_int_t
njs_string_prototype_concat(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    u_char             *p, *start;
    uint64_t           size, length, mask;
    njs_int_t          ret;
    njs_uint_t         i;
    njs_string_prop_t  string;

    if (njs_is_null_or_undefined(&args[0])) {
        njs_type_error(vm, "\"this\" argument is null or undefined");
        return NJS_ERROR;
    }

    for (i = 0; i < nargs; i++) {
        if (!njs_is_string(&args[i])) {
            ret = njs_value_to_string(vm, &args[i], &args[i]);
            if (ret != NJS_OK) {
                return ret;
            }
        }
    }

    if (nargs == 1) {
        njs_string_copy(&vm->retval, &args[0]);
        return NJS_OK;
    }

    size = 0;
    length = 0;
    mask = -1;

    for (i = 0; i < nargs; i++) {
        (void) njs_string_prop(&string, &args[i]);

        size += string.size;
        length += string.length;

        if (string.length == 0 && string.size != 0) {
            mask = 0;
        }
    }

    length &= mask;

    start = njs_string_alloc(vm, &vm->retval, size, length);
    if (njs_slow_path(start == NULL)) {
        return NJS_ERROR;
    }

    p = start;

    for (i = 0; i < nargs; i++) {
        (void) njs_string_prop(&string, &args[i]);

        p = memcpy(p, string.start, string.size);
        p += string.size;
    }

    return NJS_OK;
}


njs_inline njs_int_t
njs_string_object_validate(njs_vm_t *vm, njs_value_t *object)
{
    njs_int_t  ret;

    if (njs_slow_path(njs_is_null_or_undefined(object))) {
        njs_type_error(vm, "cannot convert undefined to object");
        return NJS_ERROR;
    }

    if (njs_slow_path(!njs_is_string(object))) {
        ret = njs_value_to_string(vm, object, object);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    return NJS_OK;
}


/*
 * String.fromUTF8(start[, end]).
 * The method converts an UTF-8 encoded byte string to an Unicode string.
 */

static njs_int_t
njs_string_prototype_from_utf8(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    ssize_t            length;
    njs_int_t          ret;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_string_slice_prop(vm, &string, &slice, args, nargs);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (string.length != 0) {
        /* ASCII or UTF8 string. */
        return njs_string_slice(vm, &vm->retval, &string, &slice);
    }

    string.start += slice.start;

    length = njs_utf8_length(string.start, slice.length);

    if (length >= 0) {
        return njs_string_new(vm, &vm->retval, string.start, slice.length,
                              length);
    }

    vm->retval = njs_value_null;

    return NJS_OK;
}


/*
 * String.toUTF8(start[, end]).
 * The method serializes Unicode string to an UTF-8 encoded byte string.
 */

static njs_int_t
njs_string_prototype_to_utf8(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t          ret;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    (void) njs_string_prop(&string, njs_argument(args, 0));

    string.length = 0;
    slice.string_length = string.size;

    ret = njs_string_slice_args(vm, &slice, args, nargs);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return njs_string_slice(vm, &vm->retval, &string, &slice);
}


/*
 * String.fromBytes(start[, end]).
 * The method converts a byte string to an Unicode string.
 */

static njs_int_t
njs_string_prototype_from_bytes(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    u_char             *p, *s, *start, *end;
    size_t             size;
    njs_int_t          ret;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_string_slice_prop(vm, &string, &slice, args, nargs);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (string.length != 0) {
        /* ASCII or UTF8 string. */
        return njs_string_slice(vm, &vm->retval, &string, &slice);
    }

    size = 0;
    string.start += slice.start;
    end = string.start + slice.length;

    for (p = string.start; p < end; p++) {
        size += (*p < 0x80) ? 1 : 2;
    }

    start = njs_string_alloc(vm, &vm->retval, size, slice.length);

    if (njs_fast_path(start != NULL)) {

        if (size == slice.length) {
            memcpy(start, string.start, size);

        } else {
            s = start;
            end = string.start + slice.length;

            for (p = string.start; p < end; p++) {
                s = njs_utf8_encode(s, *p);
            }
        }

        return NJS_OK;
    }

    return NJS_ERROR;
}


/*
 * String.toBytes(start[, end]).
 * The method serializes an Unicode string to a byte string.
 * The method returns null if a character larger than 255 is
 * encountered in the Unicode string.
 */

static njs_int_t
njs_string_prototype_to_bytes(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    u_char             *p;
    size_t             length;
    uint32_t           byte;
    njs_int_t          ret;
    const u_char       *s, *end;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_string_slice_prop(vm, &string, &slice, args, nargs);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (string.length == 0) {
        /* Byte string. */
        return njs_string_slice(vm, &vm->retval, &string, &slice);
    }

    p = njs_string_alloc(vm, &vm->retval, slice.length, 0);

    if (njs_fast_path(p != NULL)) {

        if (string.length != string.size) {
            /* UTF-8 string. */
            end = string.start + string.size;

            s = njs_string_offset(string.start, end, slice.start);

            length = slice.length;

            while (length != 0 && s < end) {
                byte = njs_utf8_decode(&s, end);

                if (njs_slow_path(byte > 0xFF)) {
                    njs_release(vm, &vm->retval);
                    vm->retval = njs_value_null;

                    return NJS_OK;
                }

                *p++ = (u_char) byte;
                length--;
            }

        } else {
            /* ASCII string. */
            memcpy(p, string.start + slice.start, slice.length);
        }

        return NJS_OK;
    }

    return NJS_ERROR;
}


/*
 * String.slice(start[, end]).
 * JavaScript 1.2, ECMAScript 3.
 */

static njs_int_t
njs_string_prototype_slice(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t          ret;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_string_slice_prop(vm, &string, &slice, args, nargs);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return njs_string_slice(vm, &vm->retval, &string, &slice);
}


/*
 * String.substring(start[, end]).
 * JavaScript 1.0, ECMAScript 1.
 */

static njs_int_t
njs_string_prototype_substring(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    int64_t            start, end, length;
    njs_int_t          ret;
    njs_value_t        *value;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    length = njs_string_prop(&string, njs_argument(args, 0));

    slice.string_length = length;
    start = 0;

    if (nargs > 1) {
        value = njs_argument(args, 1);

        if (njs_slow_path(!njs_is_number(value))) {
            ret = njs_value_to_integer(vm, value, &start);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

        } else {
            start = njs_number_to_integer(njs_number(value));
        }

        if (start < 0) {
            start = 0;

        } else if (start > length) {
            start = length;
        }

        end = length;

        if (nargs > 2) {
            value = njs_arg(args, nargs, 2);

            if (njs_slow_path(!njs_is_number(value))) {
                ret = njs_value_to_integer(vm, value, &end);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

            } else {
                end = njs_number_to_integer(njs_number(value));
            }

            if (end < 0) {
                end = 0;

            } else if (end >= length) {
                end = length;
            }
        }

        length = end - start;

        if (length < 0) {
            length = -length;
            start = end;
        }
    }

    slice.start = start;
    slice.length = length;

    return njs_string_slice(vm, &vm->retval, &string, &slice);
}


/*
 * String.substr(start[, length]).
 * JavaScript 1.0, ECMAScript 3.
 */

static njs_int_t
njs_string_prototype_substr(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    int64_t            start, length, n;
    njs_int_t          ret;
    njs_value_t        *value;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    length = njs_string_prop(&string, njs_argument(args, 0));

    slice.string_length = length;
    start = 0;

    if (nargs > 1) {
        value = njs_arg(args, nargs, 1);

        if (njs_slow_path(!njs_is_number(value))) {
            ret = njs_value_to_integer(vm, value, &start);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

        } else {
            start = njs_number_to_integer(njs_number(value));
        }

        if (start < length) {
            if (start < 0) {
                start += length;

                if (start < 0) {
                    start = 0;
                }
            }

            length -= start;

            if (nargs > 2) {
                value = njs_arg(args, nargs, 2);

                if (njs_slow_path(!njs_is_number(value))) {
                    ret = njs_value_to_integer(vm, value, &n);
                    if (njs_slow_path(ret != NJS_OK)) {
                        return ret;
                    }

                } else {
                    n = njs_number_to_integer(njs_number(value));
                }

                if (n < 0) {
                    length = 0;

                } else if (n < length) {
                    length = n;
                }
            }

        } else {
            start = 0;
            length = 0;
        }
    }

    slice.start = start;
    slice.length = length;

    return njs_string_slice(vm, &vm->retval, &string, &slice);
}


static njs_int_t
njs_string_prototype_char_at(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    size_t             length;
    int64_t            start;
    njs_int_t          ret;
    njs_slice_prop_t   slice;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    slice.string_length = njs_string_prop(&string, njs_argument(args, 0));

    ret = njs_value_to_integer(vm, njs_arg(args, nargs, 1), &start);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    length = 1;

    if (start < 0 || start >= (int64_t) slice.string_length) {
        start = 0;
        length = 0;
    }

    slice.start = start;
    slice.length = length;

    return njs_string_slice(vm, &vm->retval, &string, &slice);
}


static njs_int_t
njs_string_slice_prop(njs_vm_t *vm, njs_string_prop_t *string,
    njs_slice_prop_t *slice, njs_value_t *args, njs_uint_t nargs)
{
    slice->string_length = njs_string_prop(string, &args[0]);

    return njs_string_slice_args(vm, slice, args, nargs);
}


static njs_int_t
njs_string_slice_args(njs_vm_t *vm, njs_slice_prop_t *slice, njs_value_t *args,
    njs_uint_t nargs)
{
    int64_t      start, end, length;
    njs_int_t    ret;
    njs_value_t  *value;

    length = slice->string_length;

    value = njs_arg(args, nargs, 1);

    if (njs_slow_path(!njs_is_number(value))) {
        ret = njs_value_to_integer(vm, value, &start);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }

    } else {
        start = njs_number_to_integer(njs_number(value));
    }

    if (start < 0) {
        start += length;

        if (start < 0) {
            start = 0;
        }
    }

    if (start >= length) {
        start = 0;
        length = 0;

    } else {
        value = njs_arg(args, nargs, 2);

        if (njs_slow_path(!njs_is_number(value))) {
            if (njs_is_defined(value)) {
                ret = njs_value_to_integer(vm, value, &end);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

            } else {
                end = length;
            }

        } else {
            end = njs_number_to_integer(njs_number(value));
        }

        if (end < 0) {
            end += length;
        }

        if (length >= end) {
            length = end - start;

            if (length < 0) {
                start = 0;
                length = 0;
            }

        } else {
            length -= start;
        }
    }

    slice->start = start;
    slice->length = length;

    return NJS_OK;
}


void
njs_string_slice_string_prop(njs_string_prop_t *dst,
    const njs_string_prop_t *string, const njs_slice_prop_t *slice)
{
    size_t        size, n, length;
    const u_char  *p, *start, *end;

    length = slice->length;
    start = string->start;

    if (string->size == slice->string_length) {
        /* Byte or ASCII string. */
        start += slice->start;
        size = slice->length;

        if (string->length == 0) {
            /* Byte string. */
            length = 0;
        }

    } else {
        /* UTF-8 string. */
        end = start + string->size;

        if (slice->start < slice->string_length) {
            start = njs_string_offset(start, end, slice->start);

            /* Evaluate size of the slice in bytes and adjust length. */
            p = start;
            n = length;

            while (n != 0 && p < end) {
                p = njs_utf8_next(p, end);
                n--;
            }

            size = p - start;
            length -= n;

        } else {
            length = 0;
            size = 0;
        }
    }

    dst->start = (u_char *) start;
    dst->length = length;
    dst->size = size;
}


njs_int_t
njs_string_slice(njs_vm_t *vm, njs_value_t *dst,
    const njs_string_prop_t *string, const njs_slice_prop_t *slice)
{
    njs_string_prop_t  prop;

    njs_string_slice_string_prop(&prop, string, slice);

    if (njs_fast_path(prop.size != 0)) {
        return njs_string_new(vm, dst, prop.start, prop.size, prop.length);
    }

    *dst = njs_string_empty;

    return NJS_OK;
}


static njs_int_t
njs_string_prototype_char_code_at(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    double             num;
    size_t             length;
    int64_t            index;
    uint32_t           code;
    njs_int_t          ret;
    const u_char       *start, *end;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    length = njs_string_prop(&string, njs_argument(args, 0));

    ret = njs_value_to_integer(vm, njs_arg(args, nargs, 1), &index);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (njs_slow_path(index < 0 || index >= (int64_t) length)) {
        num = NAN;
        goto done;
    }

    if (length == string.size) {
        /* Byte or ASCII string. */
        code = string.start[index];

    } else {
        /* UTF-8 string. */
        end = string.start + string.size;
        start = njs_string_offset(string.start, end, index);
        code = njs_utf8_decode(&start, end);
    }

    num = code;

done:

    njs_set_number(&vm->retval, num);

    return NJS_OK;
}


/*
 * String.bytesFrom(array).
 * Converts an array containing octets into a byte string.
 *
 * String.bytesFrom(string[, encoding]).
 * Converts a string using provided encoding: hex, base64, base64url to
 * a byte string.
 */

static njs_int_t
njs_string_bytes_from(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_value_t  *value;

    value = njs_arg(args, nargs, 1);

    if (njs_is_string(value)) {
        return njs_string_bytes_from_string(vm, args, nargs);
    }

    if (njs_is_array(value)) {
        return njs_string_bytes_from_array(vm, njs_arg(args, nargs, 1));
    }

    njs_type_error(vm, "value must be a string or array");

    return NJS_ERROR;
}


static njs_int_t
njs_string_bytes_from_array(njs_vm_t *vm, const njs_value_t *value)
{
    u_char       *p;
    uint32_t     i, length;
    njs_int_t    ret;
    njs_array_t  *array;
    njs_value_t  *octet;

    array = njs_array(value);
    length = array->length;

    for (i = 0; i < length; i++) {
        if (!njs_is_numeric(&array->start[i])) {
            ret = njs_value_to_numeric(vm, &array->start[i], &array->start[i]);
            if (ret != NJS_OK) {
                return ret;
            }
        }
    }

    p = njs_string_alloc(vm, &vm->retval, length, 0);
    if (njs_slow_path(p == NULL)) {
        return NJS_ERROR;
    }

    octet = array->start;

    while (length != 0) {
        *p++ = (u_char) njs_number_to_uint32(njs_number(octet));
        octet++;
        length--;
    }

    return NJS_OK;
}


static njs_int_t
njs_string_bytes_from_string(njs_vm_t *vm, const njs_value_t *args,
    njs_uint_t nargs)
{
    njs_str_t    enc, str;
    njs_value_t  *enc_val;

    enc_val = njs_arg(args, nargs, 2);

    if (njs_slow_path(nargs > 1 && !njs_is_string(enc_val))) {
        njs_type_error(vm, "encoding must be a string");
        return NJS_ERROR;
    }

    njs_string_get(enc_val, &enc);
    njs_string_get(&args[1], &str);

    if (enc.length == 3 && memcmp(enc.start, "hex", 3) == 0) {
        return njs_string_decode_hex(vm, &vm->retval, &str);

    } else if (enc.length == 6 && memcmp(enc.start, "base64", 6) == 0) {
        return njs_string_decode_base64(vm, &vm->retval, &str);

    } else if (enc.length == 9 && memcmp(enc.start, "base64url", 6) == 0) {
        return njs_string_decode_base64url(vm, &vm->retval, &str);
    }

    njs_type_error(vm, "Unknown encoding: \"%V\"", &enc);

    return NJS_ERROR;
}


njs_int_t
njs_string_decode_hex(njs_vm_t *vm, njs_value_t *value, const njs_str_t *src)
{
    u_char        *p, *dst;
    size_t        len;
    njs_int_t     c;
    njs_uint_t    i, n;
    const u_char  *start;

    len = src->length;
    start = src->start;

    if (njs_slow_path(len == 0)) {
        vm->retval = njs_string_empty;
        return NJS_OK;
    }

    dst = njs_string_alloc(vm, value, len / 2, 0);
    if (njs_slow_path(dst == NULL)) {
        return NJS_ERROR;
    }

    n = 0;
    p = dst;

    for (i = 0; i < len; i++) {
        c = njs_char_to_hex(start[i]);
        if (njs_slow_path(c < 0)) {
            break;
        }

        n = n * 16 + c;

        if ((i & 1) != 0) {
            *p++ = (u_char) n;
            n = 0;
        }
    }

    if (njs_slow_path((size_t) (p - dst) != (len / 2))) {
        njs_string_truncate(value, p - dst);
    }

    return NJS_OK;
}


njs_int_t
njs_string_decode_base64(njs_vm_t *vm, njs_value_t *value, const njs_str_t *src)
{
    static u_char   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77, 77, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 77,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return njs_decode_base64_core(vm, value, src, basis64);
}


njs_int_t
njs_string_decode_base64url(njs_vm_t *vm, njs_value_t *value,
    const njs_str_t *src)
{
    static u_char   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 63,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return njs_decode_base64_core(vm, value, src, basis64);
}


static njs_int_t
njs_decode_base64_core(njs_vm_t *vm, njs_value_t *value, const njs_str_t *src,
    const u_char *basis)
{
    size_t  len, dst_len;
    u_char  *d, *s, *dst;

    if (njs_slow_path(src->length == 0)) {
        vm->retval = njs_string_empty;
        return NJS_OK;
    }

    for (len = 0; len < src->length; len++) {
        if (src->start[len] == '=') {
            break;
        }

        if (basis[src->start[len]] == 77) {
            break;
        }
    }

    if (len % 4 == 1) {
        /* Rounding down to integer multiple of 4. */
        len -= 1;
    }

    dst_len = njs_base64_decoded_length(len);

    dst = njs_string_alloc(vm, value, dst_len, 0);
    if (njs_slow_path(dst == NULL)) {
        return NJS_ERROR;
    }

    s = src->start;
    d = dst;

    while (len > 3) {
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
        *d++ = (u_char) (basis[s[2]] << 6 | basis[s[3]]);

        s += 4;
        len -= 4;
    }

    if (len > 1) {
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
    }

    if (len > 2) {
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
    }

    if (njs_slow_path((size_t) (d - dst) != dst_len)) {
        njs_string_truncate(value, d - dst);
    }

    return NJS_OK;
}


static njs_int_t
njs_string_from_char_code(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    u_char      *p;
    size_t      size;
    uint16_t    code;
    njs_int_t   ret;
    njs_uint_t  i;

    for (i = 1; i < nargs; i++) {
        if (!njs_is_numeric(&args[i])) {
            ret = njs_value_to_numeric(vm, &args[i], &args[i]);
            if (ret != NJS_OK) {
                return ret;
            }
        }
    }

    size = 0;

    for (i = 1; i < nargs; i++) {
        code = njs_number_to_uint16(njs_number(&args[i]));
        size += njs_utf8_size_uint16(code);
    }

    p = njs_string_alloc(vm, &vm->retval, size, nargs - 1);
    if (njs_slow_path(p == NULL)) {
        return NJS_ERROR;
    }

    for (i = 1; i < nargs; i++) {
        code = njs_number_to_uint16(njs_number(&args[i]));
        p = njs_utf8_encode(p, code);
    }

    return NJS_OK;
}


static njs_int_t
njs_string_from_code_point(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    u_char      *p;
    double      num;
    size_t      size;
    int32_t     code;
    njs_int_t   ret;
    njs_uint_t  i;

    for (i = 1; i < nargs; i++) {
        if (!njs_is_numeric(&args[i])) {
            ret = njs_value_to_numeric(vm, &args[i], &args[i]);
            if (ret != NJS_OK) {
                return ret;
            }
        }
    }

    size = 0;

    for (i = 1; i < nargs; i++) {
        num = njs_number(&args[i]);
        if (isnan(num)) {
            goto range_error;
        }

        code = num;

        if (code != num || code < 0 || code >= 0x110000) {
            goto range_error;
        }

        size += njs_utf8_size(code);
    }

    p = njs_string_alloc(vm, &vm->retval, size, nargs - 1);
    if (njs_slow_path(p == NULL)) {
        return NJS_ERROR;
    }

    for (i = 1; i < nargs; i++) {
        p = njs_utf8_encode(p, njs_number(&args[i]));
    }

    return NJS_OK;

range_error:

    njs_range_error(vm, NULL);

    return NJS_ERROR;
}


static njs_int_t
njs_string_prototype_index_of(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    int64_t            index, length, search_length;
    njs_int_t          ret;
    njs_value_t        *value;
    const u_char       *p, *end;
    njs_string_prop_t  string, search;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (nargs > 1) {
        length = njs_string_prop(&string, njs_argument(args, 0));

        value = njs_argument(args, 1);

        if (njs_slow_path(!njs_is_string(value))) {
            ret = njs_value_to_string(vm, value, value);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }
        }

        search_length = njs_string_prop(&search, value);

        index = 0;

        if (nargs > 2) {
            value = njs_argument(args, 2);

            if (njs_slow_path(!njs_is_number(value))) {
                ret = njs_value_to_integer(vm, value, &index);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

            } else {
                index = njs_number_to_integer(njs_number(value));
            }

            if (index < 0) {
                index = 0;
            }
        }

        if (length - index >= search_length) {
            end = string.start + string.size;

            if (string.size == (size_t) length) {
                /* Byte or ASCII string. */

                end -= (search.size - 1);

                for (p = string.start + index; p < end; p++) {
                    if (memcmp(p, search.start, search.size) == 0) {
                        goto done;
                    }

                    index++;
                }

            } else {
                /* UTF-8 string. */

                p = njs_string_offset(string.start, end, index);
                end -= search.size - 1;

                while (p < end) {
                    if (memcmp(p, search.start, search.size) == 0) {
                        goto done;
                    }

                    index++;
                    p = njs_utf8_next(p, end);
                }
            }

        } else if (search.size == 0) {
            index = length;
            goto done;
        }
    }

    index = -1;

done:

    njs_set_number(&vm->retval, index);

    return NJS_OK;
}


static njs_int_t
njs_string_prototype_last_index_of(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    double             pos;
    ssize_t            index, start, length, search_length;
    njs_int_t          ret;
    njs_value_t        *value, *search_string, lvalue;
    const u_char       *p, *end;
    njs_string_prop_t  string, search;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    index = -1;

    length = njs_string_prop(&string, njs_argument(args, 0));

    search_string = njs_lvalue_arg(&lvalue, args, nargs, 1);

    if (njs_slow_path(!njs_is_string(search_string))) {
        ret = njs_value_to_string(vm, search_string, search_string);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    search_length = njs_string_prop(&search, search_string);

    if (length < search_length) {
        goto done;
    }

    value = njs_arg(args, nargs, 2);

    if (njs_slow_path(!njs_is_number(value))) {
        ret = njs_value_to_number(vm, value, &pos);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }

    } else {
        pos = njs_number(value);
    }

    if (isnan(pos)) {
        index = NJS_STRING_MAX_LENGTH;

    } else {
        index = njs_number_to_integer(pos);

        if (index < 0) {
            index = 0;
        }
    }

    if (search_length == 0) {
        index = njs_min(index, length);
        goto done;
    }

    if (index >= length) {
        index = length - 1;
    }

    if (string.size == (size_t) length) {
        /* Byte or ASCII string. */

        start = length - search.size;

        if (index > start) {
            index = start;
        }

        p = string.start + index;

        do {
            if (memcmp(p, search.start, search.size) == 0) {
                goto done;
            }

            index--;
            p--;

        } while (p >= string.start);

    } else {
        /* UTF-8 string. */

        end = string.start + string.size;
        p = njs_string_offset(string.start, end, index);
        end -= search.size;

        while (p > end) {
            index--;
            p = njs_utf8_prev(p);
        }

        for ( ;; ) {
            if (memcmp(p, search.start, search.size) == 0) {
                goto done;
            }

            index--;

            if (p <= string.start) {
                break;
            }

            p = njs_utf8_prev(p);
        }
    }

done:

    njs_set_number(&vm->retval, index);

    return NJS_OK;
}


static njs_int_t
njs_string_prototype_includes(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    int64_t            index, length, search_length;
    njs_int_t          ret;
    njs_value_t        *value;
    const u_char       *p, *end;
    const njs_value_t  *retval;
    njs_string_prop_t  string, search;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    retval = &njs_value_true;

    if (nargs > 1) {
        value = njs_argument(args, 1);

        if (njs_slow_path(!njs_is_string(value))) {
            ret = njs_value_to_string(vm, value, value);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }
        }

        search_length = njs_string_prop(&search, value);

        if (nargs > 2) {
            value = njs_argument(args, 2);

            if (njs_slow_path(!njs_is_number(value))) {
                ret = njs_value_to_integer(vm, value, &index);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

            } else {
                index = njs_number_to_integer(njs_number(value));
            }

            if (index < 0) {
                index = 0;
            }

        } else {
            index = 0;
        }

        if (search_length == 0) {
            goto done;
        }

        length = njs_string_prop(&string, &args[0]);

        if (length - index >= search_length) {
            end = string.start + string.size;

            if (string.size == (size_t) length) {
                /* Byte or ASCII string. */
                p = string.start + index;

            } else {
                /* UTF-8 string. */
                p = njs_string_offset(string.start, end, index);
            }

            end -= search.size - 1;

            while (p < end) {
                if (memcmp(p, search.start, search.size) == 0) {
                    goto done;
                }

                p++;
            }
        }
    }

    retval = &njs_value_false;

done:

    vm->retval = *retval;

    return NJS_OK;
}


static njs_int_t
njs_string_prototype_starts_with(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    return njs_string_starts_or_ends_with(vm, args, nargs, 1);
}


static njs_int_t
njs_string_prototype_ends_with(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    return njs_string_starts_or_ends_with(vm, args, nargs, 0);
}


static njs_int_t
njs_string_starts_or_ends_with(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_bool_t starts)
{
    int64_t            index, length, search_length;
    njs_int_t          ret;
    njs_value_t        *value, lvalue;
    const u_char       *p, *end;
    const njs_value_t  *retval;
    njs_string_prop_t  string, search;

    retval = &njs_value_true;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    value = njs_lvalue_arg(&lvalue, args, nargs, 1);

    if (njs_slow_path(!njs_is_string(value))) {
        ret = njs_value_to_string(vm, value, value);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    search_length = njs_string_prop(&search, value);

    value = njs_arg(args, nargs, 2);

    if (njs_slow_path(!njs_is_number(value))) {
        index = -1;

        if (!njs_is_undefined(value)) {
            ret = njs_value_to_integer(vm, value, &index);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }
        }

    } else {
        index = njs_number_to_integer(njs_number(value));
    }

    if (search_length == 0) {
        goto done;
    }

    if (nargs > 1) {
        length = njs_string_prop(&string, &args[0]);

        if (starts) {
            if (index < 0) {
                index = 0;
            }

            if (length - index < search_length) {
                goto small;
            }

        } else {
            if (index < 0 || index > length) {
                index = length;
            }

            index -= search_length;

            if (index < 0) {
                goto small;
            }
        }

        end = string.start + string.size;

        if (string.size == (size_t) length) {
            /* Byte or ASCII string. */
            p = string.start + index;

        } else {
            /* UTF-8 string. */
            p = njs_string_offset(string.start, end, index);
        }

        if ((size_t) (end - p) >= search.size
            && memcmp(p, search.start, search.size) == 0)
        {
            goto done;
        }
    }

small:

    retval = &njs_value_false;

done:

    vm->retval = *retval;

    return NJS_OK;
}


/*
 * njs_string_offset() assumes that index is correct.
 */

const u_char *
njs_string_offset(const u_char *start, const u_char *end, size_t index)
{
    uint32_t    *map;
    njs_uint_t  skip;

    if (index >= NJS_STRING_MAP_STRIDE) {
        map = njs_string_map_start(end);

        if (map[0] == 0) {
            njs_string_offset_map_init(start, end - start);
        }

        start += map[index / NJS_STRING_MAP_STRIDE - 1];
    }

    for (skip = index % NJS_STRING_MAP_STRIDE; skip != 0; skip--) {
        start = njs_utf8_next(start, end);
    }

    return start;
}


/*
 * njs_string_index() assumes that offset is correct.
 */

uint32_t
njs_string_index(njs_string_prop_t *string, uint32_t offset)
{
    uint32_t      *map, last, index;
    const u_char  *p, *start, *end;

    if (string->size == string->length) {
        return offset;
    }

    last = 0;
    index = 0;

    if (string->length >= NJS_STRING_MAP_STRIDE) {

        end = string->start + string->size;
        map = njs_string_map_start(end);

        if (map[0] == 0) {
            njs_string_offset_map_init(string->start, string->size);
        }

        while (index + NJS_STRING_MAP_STRIDE < string->length
               && *map <= offset)
        {
            last = *map++;
            index += NJS_STRING_MAP_STRIDE;
        }
    }

    p = string->start + last;
    start = string->start + offset;
    end = string->start + string->size;

    while (p < start) {
        index++;
        p = njs_utf8_next(p, end);
    }

    return index;
}


void
njs_string_offset_map_init(const u_char *start, size_t size)
{
    size_t        offset;
    uint32_t      *map;
    njs_uint_t    n;
    const u_char  *p, *end;

    end = start + size;
    map = njs_string_map_start(end);
    p = start;
    n = 0;
    offset = NJS_STRING_MAP_STRIDE;

    do {
        if (offset == 0) {
            map[n++] = p - start;
            offset = NJS_STRING_MAP_STRIDE;
        }

        /* The UTF-8 string should be valid since its length is known. */
        p = njs_utf8_next(p, end);

        offset--;

    } while (p < end);
}


/*
 * String.toLowerCase().
 * The method supports only simple folding.  For example, Turkish "İ"
 * folding "\u0130" to "\u0069\u0307" is not supported.
 */

static njs_int_t
njs_string_prototype_to_lower_case(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    size_t             size, length;
    u_char             *p;
    uint32_t           code;
    njs_int_t          ret;
    const u_char       *s, *end;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    (void) njs_string_prop(&string, njs_argument(args, 0));

    if (string.length == 0 || string.length == string.size) {
        /* Byte or ASCII string. */

        p = njs_string_alloc(vm, &vm->retval, string.size, string.length);
        if (njs_slow_path(p == NULL)) {
            return NJS_ERROR;
        }

        s = string.start;
        size = string.size;

        while (size != 0) {
            *p++ = njs_lower_case(*s++);
            size--;
        }

    } else {
        /* UTF-8 string. */
        s = string.start;
        end = s + string.size;
        length = string.length;

        size = 0;

        while (length != 0) {
            code = njs_utf8_lower_case(&s, end);
            size += njs_utf8_size(code);
            length--;
        }

        p = njs_string_alloc(vm, &vm->retval, size, string.length);
        if (njs_slow_path(p == NULL)) {
            return NJS_ERROR;
        }

        s = string.start;
        length = string.length;

        while (length != 0) {
            code = njs_utf8_lower_case(&s, end);
            p = njs_utf8_encode(p, code);
            length--;
        }
    }

    return NJS_OK;
}


/*
 * String.toUpperCase().
 * The method supports only simple folding.  For example, German "ß"
 * folding "\u00DF" to "\u0053\u0053" is not supported.
 */

static njs_int_t
njs_string_prototype_to_upper_case(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    size_t             size, length;
    u_char             *p;
    uint32_t           code;
    njs_int_t          ret;
    const u_char       *s, *end;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    (void) njs_string_prop(&string, njs_argument(args, 0));

    if (string.length == 0 || string.length == string.size) {
        /* Byte or ASCII string. */

        p = njs_string_alloc(vm, &vm->retval, string.size, string.length);
        if (njs_slow_path(p == NULL)) {
            return NJS_ERROR;
        }

        s = string.start;
        size = string.size;

        while (size != 0) {
            *p++ = njs_upper_case(*s++);
            size--;
        }

    } else {
        /* UTF-8 string. */
        s = string.start;
        end = s + string.size;
        length = string.length;

        size = 0;

        while (length != 0) {
            code = njs_utf8_upper_case(&s, end);
            size += njs_utf8_size(code);
            length--;
        }

        p = njs_string_alloc(vm, &vm->retval, size, string.length);
        if (njs_slow_path(p == NULL)) {
            return NJS_ERROR;
        }

        s = string.start;
        length = string.length;

        while (length != 0) {
            code = njs_utf8_upper_case(&s, end);
            p = njs_utf8_encode(p, code);
            length--;
        }
    }

    return NJS_OK;
}


static njs_int_t
njs_string_prototype_trim(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t  ret;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return njs_string_trim(vm, njs_argument(args, 0),
                           NJS_TRIM_START|NJS_TRIM_END);
}


static njs_int_t
njs_string_prototype_trim_start(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    njs_int_t  ret;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return njs_string_trim(vm, njs_argument(args, 0), NJS_TRIM_START);
}


static njs_int_t
njs_string_prototype_trim_end(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    njs_int_t  ret;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    return njs_string_trim(vm, njs_argument(args, 0), NJS_TRIM_END);
}


static njs_int_t
njs_string_trim(njs_vm_t *vm, njs_value_t *value, njs_uint_t mode)
{
    uint32_t           u, trim, length;
    const u_char       *p, *prev, *start, *end;
    njs_string_prop_t  string;

    trim = 0;

    njs_string_prop(&string, value);

    start = string.start;
    end = string.start + string.size;

    if (string.length == 0 || string.length == string.size) {
        /* Byte or ASCII string. */

        if (mode & NJS_TRIM_START) {
            for ( ;; ) {
                if (start == end) {
                    goto empty;
                }

                if (njs_is_whitespace(*start)) {
                    start++;
                    trim++;
                    continue;
                }

                break;
            }
        }

        if (mode & NJS_TRIM_END) {
            for ( ;; ) {
                if (start == end) {
                    goto empty;
                }

                end--;

                if (njs_is_whitespace(*end)) {
                    trim++;
                    continue;
                }

                end++;
                break;
            }
        }

    } else {
        /* UTF-8 string. */

        if (mode & NJS_TRIM_START) {
            for ( ;; ) {
                if (start == end) {
                    goto empty;
                }

                p = start;
                u = njs_utf8_decode(&start, end);

                if (njs_utf8_is_whitespace(u)) {
                    trim++;
                    continue;
                }

                start = p;
                break;
            }
        }

        if (mode & NJS_TRIM_END) {
            prev = end;

            for ( ;; ) {
                if (start == prev) {
                    goto empty;
                }

                prev = njs_utf8_prev(prev);
                p = prev;
                u = njs_utf8_decode(&p, end);

                if (njs_utf8_is_whitespace(u)) {
                    trim++;
                    continue;
                }

                end = p;
                break;
            }
        }
    }

    if (trim == 0) {
        /* GC: retain. */
        vm->retval = *value;

        return NJS_OK;
    }

    length = (string.length != 0) ? string.length - trim : 0;

    return njs_string_new(vm, &vm->retval, start, end - start, length);

empty:

    vm->retval = njs_string_empty;

    return NJS_OK;
}


static njs_int_t
njs_string_prototype_repeat(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    u_char             *p, *start;
    int64_t            n, max;
    uint64_t           size, length;
    njs_int_t          ret;
    njs_string_prop_t  string;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    ret = njs_value_to_integer(vm, njs_arg(args, nargs, 1), &n);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    (void) njs_string_prop(&string, njs_argument(args, 0));

    max = (string.size > 1) ? NJS_STRING_MAX_LENGTH / string.size
                            : NJS_STRING_MAX_LENGTH;

    if (njs_slow_path(n < 0 || n >= max)) {
        njs_range_error(vm, NULL);
        return NJS_ERROR;
    }

    if (string.size == 0) {
        vm->retval = njs_string_empty;
        return NJS_OK;
    }

    size = string.size * n;
    length = string.length * n;

    start = njs_string_alloc(vm, &vm->retval, size, length);
    if (njs_slow_path(start == NULL)) {
        return NJS_ERROR;
    }

    p = start;

    while (n != 0) {
        p = memcpy(p, string.start, string.size);
        p += string.size;
        n--;
    }

    return NJS_OK;
}


static njs_int_t
njs_string_prototype_pad_start(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    return njs_string_prototype_pad(vm, args, nargs, 1);
}


static njs_int_t
njs_string_prototype_pad_end(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    return njs_string_prototype_pad(vm, args, nargs, 0);
}


static njs_int_t
njs_string_prototype_pad(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_bool_t pad_start)
{
    u_char             *p, *start;
    size_t             padding, trunc, new_size;
    int64_t            length, new_length;
    uint32_t           n, pad_length;
    njs_int_t          ret;
    njs_value_t        *value, *pad;
    const u_char       *end;
    njs_string_prop_t  string, pad_string;

    static const njs_value_t  string_space = njs_string(" ");

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    length = njs_string_prop(&string, njs_argument(args, 0));

    new_length = 0;

    if (nargs > 1) {
        value = njs_argument(args, 1);

        if (njs_slow_path(!njs_is_number(value))) {
            ret = njs_value_to_integer(vm, value, &new_length);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

        } else {
            new_length = njs_number_to_integer(njs_number(value));
        }
    }

    if (new_length <= length) {
        vm->retval = args[0];
        return NJS_OK;
    }

    if (njs_slow_path(new_length >= NJS_STRING_MAX_LENGTH)) {
        njs_range_error(vm, NULL);
        return NJS_ERROR;
    }

    padding = new_length - length;

    /* GCC and Clang complain about uninitialized n and trunc. */
    n = 0;
    trunc = 0;

    pad = njs_arg(args, nargs, 2);

    if (njs_slow_path(!njs_is_string(pad))) {
        if (njs_is_undefined(pad)) {
            pad = njs_value_arg(&string_space);

        } else {
            ret = njs_value_to_string(vm, pad, pad);
            if (njs_slow_path(ret != NJS_OK)) {
                return NJS_ERROR;
            }
        }
    }

    pad_length = njs_string_prop(&pad_string, pad);

    if (pad_string.size == 0) {
        vm->retval = args[0];
        return NJS_OK;
    }

    if (pad_string.size > 1) {
        n = padding / pad_length;
        trunc = padding % pad_length;

        if (pad_string.size != (size_t) pad_length) {
            /* UTF-8 string. */
            end = pad_string.start + pad_string.size;
            end = njs_string_offset(pad_string.start, end, trunc);

            trunc = end - pad_string.start;
            padding = pad_string.size * n + trunc;
        }
    }

    new_size = string.size + padding;

    start = njs_string_alloc(vm, &vm->retval, new_size, new_length);
    if (njs_slow_path(start == NULL)) {
        return NJS_ERROR;
    }

    p = start;

    if (pad_start) {
        start += padding;

    } else {
        p += string.size;
    }

    memcpy(start, string.start, string.size);

    if (pad_string.size == 1) {
        njs_memset(p, pad_string.start[0], padding);

    } else {
        while (n != 0) {
            memcpy(p, pad_string.start, pad_string.size);
            p += pad_string.size;
            n--;
        }

        memcpy(p, pad_string.start, trunc);
    }

    return NJS_OK;
}


/*
 * String.search([regexp])
 */

static njs_int_t
njs_string_prototype_search(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    int                   *captures;
    njs_int_t             ret, index;
    njs_uint_t            n;
    njs_value_t           *value;
    njs_string_prop_t     string;
    njs_regexp_pattern_t  *pattern;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    index = 0;

    if (nargs > 1) {
        value = njs_argument(args, 1);

        switch (value->type) {

        case NJS_REGEXP:
            pattern = njs_regexp_pattern(value);
            break;

        case NJS_UNDEFINED:
            goto done;

        default:
            if (njs_slow_path(!njs_is_string(value))) {
                ret = njs_value_to_string(vm, value, value);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }
            }

            (void) njs_string_prop(&string, value);

            if (string.size != 0) {
                pattern = njs_regexp_pattern_create(vm, string.start,
                                                    string.size, 0);
                if (njs_slow_path(pattern == NULL)) {
                    return NJS_ERROR;
                }

                break;
            }

            goto done;
        }

        index = -1;

        (void) njs_string_prop(&string, &args[0]);

        n = (string.length != 0);

        if (njs_regex_is_valid(&pattern->regex[n])) {
            ret = njs_regexp_match(vm, &pattern->regex[n], string.start,
                                   string.size, vm->single_match_data);
            if (ret >= 0) {
                captures = njs_regex_captures(vm->single_match_data);
                index = njs_string_index(&string, captures[0]);

            } else if (ret != NJS_REGEX_NOMATCH) {
                return NJS_ERROR;
            }
        }
    }

done:

    njs_set_number(&vm->retval, index);

    return NJS_OK;
}


/*
 * String.match([regexp])
 */

static njs_int_t
njs_string_prototype_match(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_str_t             string;
    njs_int_t             ret;
    njs_value_t           arguments[2];
    njs_regexp_pattern_t  *pattern;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    arguments[1] = args[0];

    string.start = NULL;
    string.length = 0;

    if (nargs > 1) {

        if (njs_is_regexp(&args[1])) {
            pattern = njs_regexp_pattern(&args[1]);

            if (pattern->global) {
                return njs_string_match_multiple(vm, args, pattern);
            }

            /*
             * string.match(regexp) is the same as regexp.exec(string)
             * if the regexp has no global flag.
             */
            arguments[0] = args[1];

            goto match;
        }

        if (!njs_is_string(&args[1])) {
            if (!njs_is_undefined(&args[1])) {
                ret = njs_value_to_string(vm, &args[1], &args[1]);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

                njs_string_get(&args[1], &string);
            }

        } else {
            njs_string_get(&args[1], &string);
        }

        /* A void value. */
    }

    ret = njs_regexp_create(vm, &arguments[0], string.start, string.length, 0);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

match:

    return njs_regexp_prototype_exec(vm, arguments, nargs, unused);
}


static njs_int_t
njs_string_match_multiple(njs_vm_t *vm, njs_value_t *args,
    njs_regexp_pattern_t *pattern)
{
    int                *captures;
    int32_t            size, length;
    njs_int_t          ret;
    njs_utf8_t         utf8;
    njs_array_t        *array;
    const u_char       *p, *start, *end;
    njs_regexp_utf8_t  type;
    njs_string_prop_t  string;

    njs_set_number(&args[1].data.u.regexp->last_index, 0);
    vm->retval = njs_value_null;

    (void) njs_string_prop(&string, &args[0]);

    utf8 = NJS_STRING_BYTE;
    type = NJS_REGEXP_BYTE;

    if (string.length != 0) {
        utf8 = NJS_STRING_ASCII;
        type = NJS_REGEXP_UTF8;

        if (string.length != string.size) {
            utf8 = NJS_STRING_UTF8;
        }
    }

    if (njs_regex_is_valid(&pattern->regex[type])) {

        array = njs_array_alloc(vm, 0, NJS_ARRAY_SPARE);
        if (njs_slow_path(array == NULL)) {
            return NJS_ERROR;
        }

        p = string.start;
        end = p + string.size;

        do {
            ret = njs_regexp_match(vm, &pattern->regex[type], p, string.size,
                                   vm->single_match_data);
            if (ret < 0) {
                if (njs_fast_path(ret == NJS_REGEX_NOMATCH)) {
                    break;
                }

                njs_internal_error(vm, "njs_regexp_match() failed");

                return NJS_ERROR;
            }

            ret = njs_array_expand(vm, array, 0, 1);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

            captures = njs_regex_captures(vm->single_match_data);
            start = p + captures[0];

            if (captures[1] == 0) {
                if (start < end) {
                    p = (utf8 != NJS_STRING_BYTE) ? njs_utf8_next(start, end)
                                                  : start + 1;
                    string.size = end - p;

                } else {
                    /* To exit the loop. */
                    p++;
                }

                size = 0;
                length = 0;

            } else {
                p += captures[1];
                string.size -= captures[1];

                size = captures[1] - captures[0];
                length = njs_string_calc_length(utf8, start, size);
            }

            ret = njs_string_new(vm, &array->start[array->length],
                                 start, size, length);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

            array->length++;

        } while (p <= end);

        njs_set_array(&vm->retval, array);
    }

    return NJS_OK;
}


/*
 * String.prototype.split([string|regexp[, limit]])
 */

static njs_int_t
njs_string_prototype_split(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    int                   *captures;
    size_t                size;
    uint32_t              limit;
    njs_int_t             ret;
    njs_utf8_t            utf8;
    njs_value_t           *value;
    njs_array_t           *array;
    const u_char          *p, *start, *next, *last, *end;
    njs_regexp_utf8_t     type;
    njs_string_prop_t     string, split;
    njs_regexp_pattern_t  *pattern;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    array = njs_array_alloc(vm, 0, NJS_ARRAY_SPARE);
    if (njs_slow_path(array == NULL)) {
        return NJS_ERROR;
    }

    if (nargs > 1) {

        if (nargs > 2) {
            value = njs_argument(args, 2);

            if (njs_slow_path(!njs_is_number(value))) {
                ret = njs_value_to_uint32(vm, value, &limit);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

            } else {
                limit = njs_number_to_uint32(njs_number(value));
            }

            if (limit == 0) {
                goto done;
            }

        } else {
            limit = (uint32_t) -1;
        }

        (void) njs_string_prop(&string, &args[0]);

        if (string.size == 0) {
            goto single;
        }

        utf8 = NJS_STRING_BYTE;
        type = NJS_REGEXP_BYTE;

        if (string.length != 0) {
            utf8 = NJS_STRING_ASCII;
            type = NJS_REGEXP_UTF8;

            if (string.length != string.size) {
                utf8 = NJS_STRING_UTF8;
            }
        }

        switch (args[1].type) {

        case NJS_REGEXP:
            pattern = njs_regexp_pattern(&args[1]);

            if (!njs_regex_is_valid(&pattern->regex[type])) {
                goto single;
            }

            start = string.start;
            end = string.start + string.size;

            do {
                ret = njs_regexp_match(vm, &pattern->regex[type], start,
                                       end - start, vm->single_match_data);
                if (ret >= 0) {
                    captures = njs_regex_captures(vm->single_match_data);

                    p = start + captures[0];
                    next = start + captures[1];

                } else if (ret == NJS_REGEX_NOMATCH) {
                    p = (u_char *) end;
                    next = (u_char *) end + 1;

                } else {
                    return NJS_ERROR;
                }

                /* Empty split regexp. */
                if (p == next) {
                    p = (utf8 != NJS_STRING_BYTE) ? njs_utf8_next(p, end)
                                                  : p + 1;
                    next = p;
                }

                size = p - start;

                ret = njs_string_split_part_add(vm, array, utf8, start, size);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

                start = next;
                limit--;

            } while (limit != 0 && p < end);

            goto done;

        case NJS_UNDEFINED:
            break;

        default:
            if (njs_slow_path(!njs_is_string(&args[1]))) {
                ret = njs_value_to_string(vm, &args[1], &args[1]);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }
            }

            (void) njs_string_prop(&split, &args[1]);

            if (string.size < split.size) {
                goto single;
            }

            start = string.start;
            end = string.start + string.size;
            last = end - split.size;

            do {
                for (p = start; p <= last; p++) {
                    if (memcmp(p, split.start, split.size) == 0) {
                        goto found;
                    }
                }

                p = end;

found:

                next = p + split.size;

                /* Empty split string. */
                if (p == next) {
                    p = (utf8 != NJS_STRING_BYTE) ? njs_utf8_next(p, end)
                                                  : p + 1;
                    next = p;
                }

                size = p - start;

                ret = njs_string_split_part_add(vm, array, utf8, start, size);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

                start = next;
                limit--;

            } while (limit != 0 && p < end);

            goto done;
        }
    }

single:

    /* GC: retain. */
    array->start[0] = args[0];
    array->length = 1;

done:

    njs_set_array(&vm->retval, array);

    return NJS_OK;
}


static njs_int_t
njs_string_split_part_add(njs_vm_t *vm, njs_array_t *array, njs_utf8_t utf8,
    const u_char *start, size_t size)
{
    ssize_t  length;

    length = njs_string_calc_length(utf8, start, size);

    return njs_array_string_add(vm, array, start, size, length);
}


/*
 * String.replace([regexp|string[, string|function]])
 */

static njs_int_t
njs_string_prototype_replace(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    u_char                *p, *start, *end;
    njs_int_t             ret;
    njs_uint_t            ncaptures;
    njs_value_t           *this, *search, *replace;
    njs_value_t           search_lvalue, replace_lvalue;
    njs_regex_t           *regex;
    njs_string_prop_t     string;
    njs_string_replace_t  *r, string_replace;

    ret = njs_string_object_validate(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    this = njs_argument(args, 0);

    if (nargs == 1) {
        goto original;
    }

    search = njs_lvalue_arg(&search_lvalue, args, nargs, 1);
    replace = njs_lvalue_arg(&replace_lvalue, args, nargs, 2);

    (void) njs_string_prop(&string, this);

    if (string.size == 0) {
        goto original;
    }

    r = &string_replace;

    r->utf8 = NJS_STRING_BYTE;
    r->type = NJS_REGEXP_BYTE;

    if (string.length != 0) {
        r->utf8 = NJS_STRING_ASCII;
        r->type = NJS_REGEXP_UTF8;

        if (string.length != string.size) {
            r->utf8 = NJS_STRING_UTF8;
        }
    }

    if (njs_is_regexp(search)) {
        regex = &njs_regexp_pattern(search)->regex[r->type];

        if (!njs_regex_is_valid(regex)) {
            goto original;
        }

        ncaptures = njs_regex_ncaptures(regex);

    } else {
        regex = NULL;
        ncaptures = 1;

        if (!njs_is_string(search)) {
            ret = njs_value_to_string(vm, search, search);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }
        }
    }

    /* This cannot fail. */
    r->part = njs_arr_init(vm->mem_pool, &r->parts, &r->array,
                           3, sizeof(njs_string_replace_part_t));

    r->substitutions = NULL;
    r->function = NULL;

    /* A literal replacement is stored in the second part. */

    if (nargs == 2) {
        njs_string_replacement_copy(&r->part[1], &njs_string_undefined);

    } else if (njs_is_function(replace)) {
        r->function = njs_function(replace);

    } else {
        if (njs_slow_path(!njs_is_string(replace))) {
            ret = njs_value_to_string(vm, replace, replace);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }
        }

        njs_string_replacement_copy(&r->part[1], replace);

        start = r->part[1].start;

        if (start == NULL) {
            start = r->part[1].value.short_string.start;
        }

        end = start + r->part[1].size;

        for (p = start; p < end; p++) {
            if (*p == '$') {
                ret = njs_string_replace_parse(vm, r, p, end, p - start,
                                               ncaptures);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

                /* Reset parts array to the subject string only. */
                r->parts.items = 1;

                break;
            }
        }
    }

    r->part[0].start = string.start;
    r->part[0].size = string.size;
    njs_set_invalid(&r->part[0].value);

    if (regex != NULL) {
        r->match_data = njs_regex_match_data(regex, vm->regex_context);
        if (njs_slow_path(r->match_data == NULL)) {
            return NJS_ERROR;
        }

        return njs_string_replace_regexp(vm, this, search, r);
    }

    return njs_string_replace_search(vm, this, search, r);

original:

    njs_string_copy(&vm->retval, this);

    return NJS_OK;
}


static njs_int_t
njs_string_replace_regexp(njs_vm_t *vm, njs_value_t *this, njs_value_t *regex,
    njs_string_replace_t *r)
{
    int                        *captures;
    u_char                     *p, *start;
    njs_int_t                  ret;
    const u_char               *end;
    njs_regexp_pattern_t       *pattern;
    njs_string_replace_part_t  replace;

    pattern = njs_regexp_pattern(regex);
    end = r->part[0].start + r->part[0].size;

    replace = r->part[1];

    do {
        ret = njs_regexp_match(vm, &pattern->regex[r->type],
                               r->part[0].start, r->part[0].size,
                               r->match_data);

        if (ret < 0) {
            if (njs_slow_path(ret != NJS_REGEX_NOMATCH)) {
                return NJS_ERROR;
            }

            break;
        }

        captures = njs_regex_captures(r->match_data);

        if (r->substitutions != NULL) {
            ret = njs_string_replace_substitute(vm, r, captures);
            if (njs_slow_path(ret != NJS_OK)) {
                return ret;
            }

            if (!pattern->global) {
                return njs_string_replace_regexp_join(vm, r);
            }

            continue;
        }

        if (r->part != r->parts.start) {
            r->part = njs_arr_add(&r->parts);
            if (njs_slow_path(r->part == NULL)) {
                return NJS_ERROR;
            }

            r->part = njs_arr_add(&r->parts);
            if (njs_slow_path(r->part == NULL)) {
                return NJS_ERROR;
            }

            r->part -= 2;
        }

        if (captures[1] == 0) {

            /* Empty match. */

            start = r->part[0].start;

            if (start < end) {
                p = (r->utf8 != NJS_STRING_BYTE)
                    ? (u_char *) njs_utf8_next(start, end) : start + 1;

                r->part[1].start = start;
                r->part[1].size = p - start;

                r->part[2].start = p;
                r->part[2].size = end - p;

            } else {
                r->part[1].size = 0;
                r->part[2].size = 0;

                /* To exit the loop. */
                r->part[2].start = start + 1;
            }

            if (r->function != NULL) {
                return njs_string_replace_regexp_function(vm, this, regex, r,
                                                          captures, ret);
            }

            r->part[0] = replace;

        } else {
            r->part[2].start = r->part[0].start + captures[1];
            r->part[2].size = r->part[0].size - captures[1];
            njs_set_invalid(&r->part[2].value);

            if (r->function != NULL) {
                return njs_string_replace_regexp_function(vm, this, regex, r,
                                                          captures, ret);
            }

            r->part[0].size = captures[0];

            r->part[1] = replace;
        }

        if (!pattern->global) {
            return njs_string_replace_regexp_join(vm, r);
        }

        r->part += 2;

    } while (r->part[0].start <= end);

    if (r->part != r->parts.start) {
        return njs_string_replace_regexp_join(vm, r);
    }

    njs_regex_match_data_free(r->match_data, vm->regex_context);

    njs_arr_destroy(&r->parts);

    njs_string_copy(&vm->retval, this);

    return NJS_OK;
}


static njs_int_t
njs_string_replace_regexp_function(njs_vm_t *vm, njs_value_t *this,
    njs_value_t *regex, njs_string_replace_t *r, int *captures, njs_uint_t n)
{
    u_char             *start;
    size_t             size, length;
    njs_int_t          ret;
    njs_uint_t         i, k;
    njs_value_t        *arguments;
    njs_string_prop_t  string;

    njs_set_invalid(&r->retval);

    arguments = njs_mp_alloc(vm->mem_pool, (n + 3) * sizeof(njs_value_t));
    if (njs_slow_path(arguments == NULL)) {
        return NJS_ERROR;
    }

    njs_set_undefined(&arguments[0]);

    /* Matched substring and parenthesized submatch strings. */
    for (k = 0, i = 1; i <= n; i++) {

        start = r->part[0].start + captures[k];
        size = captures[k + 1] - captures[k];
        k += 2;

        length = njs_string_calc_length(r->utf8, start, size);

        ret = njs_string_new(vm, &arguments[i], start, size, length);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }
    }

    r->empty = (captures[0] == captures[1]);

    /* The offset of the matched substring. */
    njs_set_number(&arguments[n + 1], captures[0]);

    /* The whole string being examined. */
    length = njs_string_calc_length(r->utf8, r->part[0].start, r->part[0].size);

    (void) njs_string_prop(&string, this);

    ret = njs_string_new(vm, &arguments[n + 2], string.start, string.size,
                         length);

    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    r->part[0].size = captures[0];

    ret = njs_function_apply(vm, r->function, arguments, n + 3, &r->retval);

    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    (void) njs_string_prop(&string, this);

    if (njs_is_string(&r->retval)) {
        njs_string_replacement_copy(&r->part[r->empty ? 0 : 1], &r->retval);

        if (njs_regexp_pattern(regex)->global) {
            r->part += 2;

            if (r->part[0].start > (string.start + string.size)) {
                return njs_string_replace_regexp_join(vm, r);
            }

            return njs_string_replace_regexp(vm, this, regex, r);
        }

        return njs_string_replace_regexp_join(vm, r);
    }

    njs_regex_match_data_free(r->match_data, vm->regex_context);

    njs_internal_error(vm, "unexpected retval type:%s",
                       njs_type_string(r->retval.type));

    return NJS_ERROR;
}


static njs_int_t
njs_string_replace_regexp_join(njs_vm_t *vm, njs_string_replace_t *r)
{
    njs_regex_match_data_free(r->match_data, vm->regex_context);

    return njs_string_replace_join(vm, r);
}


static njs_int_t
njs_string_replace_search(njs_vm_t *vm, njs_value_t *this, njs_value_t *search,
    njs_string_replace_t *r)
{
    int        captures[2];
    u_char     *p, *end;
    size_t     size;
    njs_int_t  ret;
    njs_str_t  string;

    njs_string_get(search, &string);

    p = r->part[0].start;
    end = (p + r->part[0].size) - (string.length - 1);

    while (p < end) {
        if (memcmp(p, string.start, string.length) == 0) {

            if (r->substitutions != NULL) {
                captures[0] = p - r->part[0].start;
                captures[1] = captures[0] + string.length;

                ret = njs_string_replace_substitute(vm, r, captures);
                if (njs_slow_path(ret != NJS_OK)) {
                    return ret;
                }

            } else {
                r->part[2].start = p + string.length;
                size = p - r->part[0].start;
                r->part[2].size = r->part[0].size - size - string.length;
                r->part[0].size = size;
                njs_set_invalid(&r->part[2].value);

                if (r->function != NULL) {
                    return njs_string_replace_search_function(vm, this, search,
                                                              r);
                }
            }

            return njs_string_replace_join(vm, r);
        }

        if (r->utf8 < 2) {
            p++;

        } else {
            p = (u_char *) njs_utf8_next(p, end);
        }
    }

    njs_string_copy(&vm->retval, this);

    return NJS_OK;
}


static njs_int_t
njs_string_replace_search_function(njs_vm_t *vm, njs_value_t *this,
    njs_value_t *search, njs_string_replace_t *r)
{
    njs_int_t    ret;
    njs_value_t  string;
    njs_value_t  arguments[4];

    njs_set_undefined(&arguments[0]);

    /* GC, args[0], args[1] */

    /* Matched substring, it is the same as the args[1]. */
    arguments[1] = *search;

    /* The offset of the matched substring. */
    njs_set_number(&arguments[2], r->part[0].size);

    /* The whole string being examined. */
    arguments[3] = *this;

    ret = njs_function_apply(vm, r->function, arguments, 4, &r->retval);

    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    if (!njs_is_primitive(&r->retval)) {
        ret = njs_value_to_string(vm, &r->retval, &r->retval);
        if (ret != NJS_OK) {
            return ret;
        }
    }

    ret = njs_primitive_value_to_string(vm, &string, &r->retval);
    if (njs_slow_path(ret != NJS_OK)) {
        njs_type_error(vm, "cannot convert primitive value to string: %s",
                       njs_type_string(r->retval.type));

        return NJS_ERROR;
    }

    njs_string_replacement_copy(&r->part[1], &string);

    return njs_string_replace_join(vm, r);
}


static njs_int_t
njs_string_replace_parse(njs_vm_t *vm, njs_string_replace_t *r, u_char *p,
    u_char *end, size_t size, njs_uint_t ncaptures)
{
    u_char              c;
    uint32_t            type;
    njs_string_subst_t  *s;

    r->substitutions = njs_arr_create(vm->mem_pool, 4,
                                      sizeof(njs_string_subst_t));

    if (njs_slow_path(r->substitutions == NULL)) {
        return NJS_ERROR;
    }

    s = NULL;

    if (size == 0) {
        goto skip;
    }

copy:

    if (s == NULL) {
        s = njs_arr_add(r->substitutions);
        if (njs_slow_path(s == NULL)) {
            return NJS_ERROR;
        }

        s->type = NJS_SUBST_COPY;
        s->size = size;
        s->start = p - size;

    } else {
        s->size += size;
    }

skip:

    while (p < end) {
        size = 1;
        c = *p++;

        if (c != '$' || p == end) {
            goto copy;
        }

        c = *p++;

        if (c == '$') {
            s = NULL;
            goto copy;
        }

        size = 2;

        if (c >= '1' && c <= '9') {
            type = c - '0';

            if (p < end) {
                c = *p;

                if (c >= '0' && c <= '9') {
                    type = type * 10 + (c - '0');
                    p++;
                    size = 3;
                }
            }

            if (type >= ncaptures) {
                goto copy;
            }

            type *= 2;

        } else if (c == '`') {
            type = NJS_SUBST_PRECEDING;

        } else if (c == '&') {
            type = 0;

        } else if (c == '\'') {
            type = NJS_SUBST_FOLLOWING;

        } else {
            goto copy;
        }

        s = njs_arr_add(r->substitutions);
        if (njs_slow_path(s == NULL)) {
            return NJS_ERROR;
        }

        s->type = type;
        s = NULL;
    }

    return NJS_OK;
}


static njs_int_t
njs_string_replace_substitute(njs_vm_t *vm, njs_string_replace_t *r,
    int *captures)
{
    int                        *capture;
    uint32_t                   i, n, last;
    const u_char               *end;
    njs_string_subst_t         *s;
    njs_string_replace_part_t  *part, *subject;

    capture = NULL;

    last = r->substitutions->items;
    end = r->part[0].start + r->part[0].size;

    part = njs_arr_add_multiple(&r->parts, last + 1);
    if (njs_slow_path(part == NULL)) {
        return NJS_ERROR;
    }

    r->part = &part[-1];

    part[last].start = r->part[0].start + captures[1];

    if (captures[1] == 0) {

        /* Empty match. */

        if (r->part[0].start < end) {
            captures[1] = njs_utf8_next(r->part[0].start, end)
                          - r->part[0].start;
            part[last].start = r->part[0].start + captures[1];

        } else {
            /* To exit the loop. */
            part[last].start = r->part[0].start + 1;
        }
    }

    part[last].size = r->part[0].size - captures[1];
    njs_set_invalid(&part[last].value);

    r->part[0].size = captures[0];

    s = r->substitutions->start;

    for (i = 0; i < last; i++) {
        n = s[i].type;

        switch (n) {

        /* Literal text, "$$", and out of range "$n" substitutions. */
        case NJS_SUBST_COPY:
            part->start = s[i].start;
            part->size = s[i].size;
            break;

        /* "$`" substitution. */
        case NJS_SUBST_PRECEDING:
            subject = r->parts.start;
            part->start = subject->start;
            part->size = (r->part[0].start - subject->start) + r->part[0].size;
            break;

        /* "$'" substitution. */
        case NJS_SUBST_FOLLOWING:
            part->start = r->part[last + 1].start;
            part->size = r->part[last + 1].size;
            break;

        /*
         * "$n" and "$&" substitutions.
         */
        default:
            if (captures[n] == captures[n + 1]) {

                /* Empty match. */

                if (n > 0 && captures[n - 1] == captures[n]) {

                    /*
                     * Consecutive empty matches as in
                     * 'ab'.replace(/(z*)(h*)/g, 'x')
                     */

                    part->size = 0;
                    break;
                }

                capture = &captures[n];
                continue;
            }

            if (capture != NULL) {

                /*
                 * Inserting a single character after a series of
                 * (possibly several) empty matches.
                 */

                if (part->start < end) {
                    part->start = r->part[0].start + *capture;
                    part->size = njs_utf8_next(part->start, end) - part->start;

                } else {
                    part->size = 0;
                }

                capture = NULL;
                break;
            }

            part->start = r->part[0].start + captures[n];
            part->size = captures[n + 1] - captures[n];
            break;
        }

        njs_set_invalid(&part->value);
        part++;
    }

    if (capture != NULL) {
        part->start = r->part[0].start + *capture;

        if (part->start < end) {
            part->size = njs_utf8_next(part->start, end) - part->start;

        } else {
            part->size = 0;
        }

        njs_set_invalid(&part->value);
        part++;
    }

    r->part = part;

    return NJS_OK;
}


static njs_int_t
njs_string_replace_join(njs_vm_t *vm, njs_string_replace_t *r)
{
    u_char                     *p, *string;
    size_t                     size, length, mask;
    ssize_t                    len;
    njs_uint_t                 i, n;
    njs_string_replace_part_t  *part;

    size = 0;
    length = 0;
    mask = -1;

    part = r->parts.start;
    n = r->parts.items;

    for (i = 0; i < n; i++) {
        if (part[i].size == 0) {
            continue;
        }

        size += part[i].size;

        if (part[i].start == NULL) {
            part[i].start = part[i].value.short_string.start;
        }

        len = njs_utf8_length(part[i].start, part[i].size);

        if (len >= 0) {
            length += len;

        } else {
            mask = 0;
        }
    }

    length &= mask;

    string = njs_string_alloc(vm, &vm->retval, size, length);
    if (njs_slow_path(string == NULL)) {
        return NJS_ERROR;
    }

    p = string;

    for (i = 0; i < n; i++) {
        size = part[i].size;

        if (size != 0) {
            p = njs_cpymem(p, part[i].start, size);
        }

        /* GC: release valid values. */
    }

    njs_arr_destroy(&r->parts);

    return NJS_OK;
}


static void
njs_string_replacement_copy(njs_string_replace_part_t *string,
    const njs_value_t *value)
{
    size_t  size;

    string->value = *value;

    size = value->short_string.size;

    if (size != NJS_STRING_LONG) {
        string->start = NULL;

    } else {
        string->start = value->long_string.data->start;
        size = value->long_string.size;
    }

    string->size = size;
}


double
njs_string_to_number(const njs_value_t *value, njs_bool_t parse_float)
{
    double        num;
    size_t        size;
    uint32_t      u;
    njs_bool_t    minus;
    const u_char  *p, *start, *end;

    const size_t  infinity = njs_length("Infinity");

    size = value->short_string.size;

    if (size != NJS_STRING_LONG) {
        p = value->short_string.start;

    } else {
        size = value->long_string.size;
        p = value->long_string.data->start;
    }

    end = p + size;

    while (p < end) {
        start = p;
        u = njs_utf8_decode(&p, end);

        if (!njs_utf8_is_whitespace(u)) {
            p = start;
            break;
        }
    }

    if (p == end) {
        return parse_float ? NAN : 0.0;
    }

    minus = 0;

    if (*p == '+') {
        p++;

    } else if (*p == '-') {
        p++;
        minus = 1;
    }

    if (p == end) {
        return NAN;
    }

    if (!parse_float
        && p + 2 < end && p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        p += 2;
        num = njs_number_hex_parse(&p, end);

    } else {
        start = p;
        num = njs_number_dec_parse(&p, end);

        if (p == start) {
            if (p + infinity > end || memcmp(p, "Infinity", infinity) != 0) {
                return NAN;
            }

            num = INFINITY;
            p += infinity;
        }
    }

    if (!parse_float) {
        while (p < end) {
            if (*p != ' ' && *p != '\t') {
                return NAN;
            }

            p++;
        }
    }

    return minus ? -num : num;
}


double
njs_string_to_index(const njs_value_t *value)
{
    double        num;
    size_t        size;
    const u_char  *p, *end;

    size = value->short_string.size;

    if (size != NJS_STRING_LONG) {
        p = value->short_string.start;

    } else {
        size = value->long_string.size;
        p = value->long_string.data->start;
    }

    if (size == 0) {
        return NAN;
    }

    if (*p == '0' && size > 1) {
        return NAN;
    }

    end = p + size;
    num = njs_number_dec_parse(&p, end);

    if (p != end) {
        return NAN;
    }

    return num;
}


/*
 * If string value is null-terminated the corresponding C string
 * is returned as is, otherwise the new copy is allocated with
 * the terminating zero byte.
 */
const char *
njs_string_to_c_string(njs_vm_t *vm, njs_value_t *value)
{
    u_char  *p, *data, *start;
    size_t  size;

    if (value->short_string.size != NJS_STRING_LONG) {
        start = value->short_string.start;
        size = value->short_string.size;

        if (size < NJS_STRING_SHORT) {
            start[size] = '\0';
            return (const char *) start;
        }

    } else {
        start = value->long_string.data->start;
        size = value->long_string.size;
    }

    data = njs_mp_alloc(vm->mem_pool, size + 1);
    if (njs_slow_path(data == NULL)) {
        njs_memory_error(vm);
        return NULL;
    }

    p = njs_cpymem(data, start, size);
    *p++ = '\0';

    return (const char *) data;
}


static const njs_object_prop_t  njs_string_prototype_properties[] =
{
    {
        .type = NJS_PROPERTY,
        .name = njs_string("length"),
        .value = njs_value(NJS_NUMBER, 0, 0.0),
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("__proto__"),
        .value = njs_prop_handler(njs_primitive_prototype_get_proto),
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("constructor"),
        .value = njs_prop_handler(njs_object_prototype_create_constructor),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("valueOf"),
        .value = njs_native_function(njs_string_prototype_value_of, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("toString"),
        .value = njs_native_function(njs_string_prototype_to_string, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("concat"),
        .value = njs_native_function(njs_string_prototype_concat, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("fromUTF8"),
        .value = njs_native_function(njs_string_prototype_from_utf8, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("toUTF8"),
        .value = njs_native_function(njs_string_prototype_to_utf8, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("fromBytes"),
        .value = njs_native_function(njs_string_prototype_from_bytes, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("toBytes"),
        .value = njs_native_function(njs_string_prototype_to_bytes, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("slice"),
        .value = njs_native_function(njs_string_prototype_slice, 2),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("substring"),
        .value = njs_native_function(njs_string_prototype_substring, 2),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("substr"),
        .value = njs_native_function(njs_string_prototype_substr, 2),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("charAt"),
        .value = njs_native_function(njs_string_prototype_char_at, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("charCodeAt"),
        .value = njs_native_function(njs_string_prototype_char_code_at, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* String.codePointAt(), ECMAScript 6. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("codePointAt"),
        .value = njs_native_function(njs_string_prototype_char_code_at, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("indexOf"),
        .value = njs_native_function(njs_string_prototype_index_of, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("lastIndexOf"),
        .value = njs_native_function(njs_string_prototype_last_index_of, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* ES6. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("includes"),
        .value = njs_native_function(njs_string_prototype_includes, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* ES6. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("startsWith"),
        .value = njs_native_function(njs_string_prototype_starts_with, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* ES6. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("endsWith"),
        .value = njs_native_function(njs_string_prototype_ends_with, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("toLowerCase"),
        .value = njs_native_function(njs_string_prototype_to_lower_case, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("toUpperCase"),
        .value = njs_native_function(njs_string_prototype_to_upper_case, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("trim"),
        .value = njs_native_function(njs_string_prototype_trim, 0),
        .writable = 1,
        .configurable = 1,
    },

    /* ES10. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("trimStart"),
        .value = njs_native_function(njs_string_prototype_trim_start, 0),
        .writable = 1,
        .configurable = 1,
    },

    /* ES10. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("trimEnd"),
        .value = njs_native_function(njs_string_prototype_trim_end, 0),
        .writable = 1,
        .configurable = 1,
    },

    /* ES6. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("repeat"),
        .value = njs_native_function(njs_string_prototype_repeat, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* ES8. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("padStart"),
        .value = njs_native_function(njs_string_prototype_pad_start, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* ES8. */
    {
        .type = NJS_PROPERTY,
        .name = njs_string("padEnd"),
        .value = njs_native_function(njs_string_prototype_pad_end, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("search"),
        .value = njs_native_function(njs_string_prototype_search, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("match"),
        .value = njs_native_function(njs_string_prototype_match, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("split"),
        .value = njs_native_function(njs_string_prototype_split, 2),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("replace"),
        .value = njs_native_function(njs_string_prototype_replace, 2),
        .writable = 1,
        .configurable = 1,
    },
};


const njs_object_init_t  njs_string_prototype_init = {
    njs_string_prototype_properties,
    njs_nitems(njs_string_prototype_properties),
};


const njs_object_prop_t  njs_string_instance_properties[] =
{
    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("length"),
        .value = njs_prop_handler(njs_string_instance_length),
    },
};


const njs_object_init_t  njs_string_instance_init = {
    njs_string_instance_properties,
    njs_nitems(njs_string_instance_properties),
};


/*
 * encodeURI(string)
 */

njs_int_t
njs_string_encode_uri(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t    ret;
    njs_value_t  *value;

    static const uint32_t  escape[] = {
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                     /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x50000025,  /* 0101 0000 0000 0000  0000 0000 0010 0101 */

                     /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x78000000,  /* 0111 1000 0000 0000  0000 0000 0000 0000 */

                     /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001,  /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    if (nargs < 2) {
        njs_set_undefined(&vm->retval);

        return NJS_OK;
    }

    value = njs_argument(args, 1);

    if (!njs_is_string(value)) {
        ret = njs_value_to_string(vm, value, value);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    return njs_string_encode(vm, value, escape);
}


/*
 * encodeURIComponent(string)
 */

njs_int_t
njs_string_encode_uri_component(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    njs_int_t    ret;
    njs_value_t  *value;

    static const uint32_t  escape[] = {
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                     /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xfc00987d,  /* 1111 1100 0000 0000  1001 1000 0111 1101 */

                     /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x78000001,  /* 0111 1000 0000 0000  0000 0000 0000 0001 */

                     /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001,  /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff,  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

    if (nargs < 2) {
        njs_set_undefined(&vm->retval);

        return NJS_OK;
    }

    value = njs_argument(args, 1);

    if (!njs_is_string(value)) {
        ret = njs_value_to_string(vm, value, value);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    return njs_string_encode(vm, value, escape);
}


static njs_int_t
njs_string_encode(njs_vm_t *vm, njs_value_t *value, const uint32_t *escape)
{
    u_char               byte, *src, *dst;
    size_t               n, size;
    njs_str_t            string;
    static const u_char  hex[16] = "0123456789ABCDEF";

    njs_prefetch(escape);

    njs_string_get(value, &string);

    src = string.start;
    n = 0;

    for (size = string.length; size != 0; size--) {
        byte = *src++;

        if ((escape[byte >> 5] & ((uint32_t) 1 << (byte & 0x1f))) != 0) {
            n += 2;
        }
    }

    if (n == 0) {
        /* GC: retain src. */
        vm->retval = *value;
        return NJS_OK;
    }

    size = string.length + n;

    dst = njs_string_alloc(vm, &vm->retval, size, size);
    if (njs_slow_path(dst == NULL)) {
        return NJS_ERROR;
    }

    size = string.length;
    src = string.start;

    do {
        byte = *src++;

        if ((escape[byte >> 5] & ((uint32_t) 1 << (byte & 0x1f))) != 0) {
            *dst++ = '%';
            *dst++ = hex[byte >> 4];
            *dst++ = hex[byte & 0xf];

        } else {
            *dst++ = byte;
        }

        size--;

    } while (size != 0);

    return NJS_OK;
}


/*
 * decodeURI(string)
 */

njs_int_t
njs_string_decode_uri(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t    ret;
    njs_value_t  *value;

    static const uint32_t  reserve[] = {
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                     /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xac009858,  /* 1010 1100 0000 0000  1001 1000 0101 1000 */

                     /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000001,  /* 0000 0000 0000 0000  0000 0000 0000 0001 */

                     /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };

    if (nargs < 2) {
        njs_set_undefined(&vm->retval);

        return NJS_OK;
    }

    value = njs_argument(args, 1);

    if (njs_slow_path(!njs_is_string(value))) {
        ret = njs_value_to_string(vm, value, value);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    return njs_string_decode(vm, value, reserve);
}


/*
 * decodeURIComponent(string)
 */

njs_int_t
njs_string_decode_uri_component(njs_vm_t *vm, njs_value_t *args,
    njs_uint_t nargs, njs_index_t unused)
{
    njs_int_t    ret;
    njs_value_t  *value;

    static const uint32_t  reserve[] = {
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                     /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                     /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                     /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000,  /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };

    if (nargs < 2) {
        njs_set_undefined(&vm->retval);

        return NJS_OK;
    }

    value = njs_argument(args, 1);

    if (njs_slow_path(!njs_is_string(value))) {
        ret = njs_value_to_string(vm, value, value);
        if (njs_slow_path(ret != NJS_OK)) {
            return ret;
        }
    }

    return njs_string_decode(vm, &args[1], reserve);
}


static njs_int_t
njs_string_decode(njs_vm_t *vm, njs_value_t *value, const uint32_t *reserve)
{
    int8_t               d0, d1;
    u_char               byte, *start, *src, *dst;
    size_t               n;
    ssize_t              size, length;
    njs_str_t            string;
    njs_bool_t           utf8;

    static const int8_t  hex[256]
        njs_aligned(32) =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
        -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };

    njs_prefetch(&hex['0']);
    njs_prefetch(reserve);

    njs_string_get(value, &string);

    src = string.start;
    n = 0;

    for (size = string.length; size != 0; size--) {
        byte = *src++;

        if (byte == '%') {
            size -= 2;

            if (size <= 0) {
                goto uri_error;
            }

            d0 = hex[*src++];
            if (d0 < 0) {
                goto uri_error;
            }

            d1 = hex[*src++];
            if (d1 < 0) {
                goto uri_error;
            }

            byte = (d0 << 4) + d1;

            if ((reserve[byte >> 5] & ((uint32_t) 1 << (byte & 0x1f))) == 0) {
                n += 2;
            }
        }
    }

    if (n == 0) {
        /* GC: retain src. */
        vm->retval = *value;
        return NJS_OK;
    }

    n = string.length - n;

    start = njs_string_alloc(vm, &vm->retval, n, n);
    if (njs_slow_path(start == NULL)) {
        return NJS_ERROR;
    }

    utf8 = 0;
    dst = start;
    size = string.length;
    src = string.start;

    do {
        byte = *src++;

        if (byte == '%') {
            size -= 2;

            d0 = hex[*src++];
            d1 = hex[*src++];
            byte = (d0 << 4) + d1;

            utf8 |= (byte >= 0x80);

            if ((reserve[byte >> 5] & ((uint32_t) 1 << (byte & 0x1f))) != 0) {
                *dst++ = '%';
                *dst++ = src[-2];
                byte = src[-1];
            }
        }

        *dst++ = byte;
        size--;

    } while (size != 0);

    if (utf8) {
        length = njs_utf8_length(start, n);

        if (length < 0) {
            length = 0;
        }

        if (vm->retval.short_string.size != NJS_STRING_LONG) {
            vm->retval.short_string.length = length;

        } else {
            vm->retval.long_string.data->length = length;
        }
    }

    return NJS_OK;

uri_error:

    njs_uri_error(vm, NULL);

    return NJS_ERROR;
}


static njs_int_t
njs_values_hash_test(njs_lvlhsh_query_t *lhq, void *data)
{
    njs_str_t    string;
    njs_value_t  *value;

    value = data;

    if (njs_is_string(value)) {
        njs_string_get(value, &string);

    } else {
        string.start = (u_char *) value;
        string.length = sizeof(njs_value_t);
    }

    if (lhq->key.length == string.length
        && memcmp(lhq->key.start, string.start, string.length) == 0)
    {
        return NJS_OK;
    }

    return NJS_DECLINED;
}


static const njs_lvlhsh_proto_t  njs_values_hash_proto
    njs_aligned(64) =
{
    NJS_LVLHSH_DEFAULT,
    njs_values_hash_test,
    njs_lvlhsh_alloc,
    njs_lvlhsh_free,
};


/*
 * Constant values such as njs_value_true are copied to values_hash during
 * code generation when they are used as operands to guarantee aligned value.
 */

njs_index_t
njs_value_index(njs_vm_t *vm, const njs_value_t *src, njs_uint_t runtime)
{
    u_char              *start;
    uint32_t            value_size, size, length;
    njs_int_t           ret;
    njs_str_t           str;
    njs_bool_t          long_string;
    njs_value_t         *value;
    njs_string_t        *string;
    njs_lvlhsh_t        *values_hash;
    njs_lvlhsh_query_t  lhq;

    long_string = 0;
    value_size = sizeof(njs_value_t);

    if (njs_is_string(src)) {
        njs_string_get(src, &str);

        size = (uint32_t) str.length;
        start = str.start;

        if (src->short_string.size == NJS_STRING_LONG) {
            long_string = 1;
        }

    } else {
        size = value_size;
        start = (u_char *) src;
    }

    lhq.key_hash = njs_djb_hash(start, size);
    lhq.key.length = size;
    lhq.key.start = start;
    lhq.proto = &njs_values_hash_proto;

    if (njs_lvlhsh_find(&vm->shared->values_hash, &lhq) == NJS_OK) {
        value = lhq.value;

    } else if (runtime && njs_lvlhsh_find(&vm->values_hash, &lhq) == NJS_OK) {
        value = lhq.value;

    } else {
        if (long_string) {
            length = src->long_string.data->length;

            if (size != length && length > NJS_STRING_MAP_STRIDE) {
                size = njs_string_map_offset(size)
                       + njs_string_map_size(length);
            }

            value_size += sizeof(njs_string_t) + size;
        }

        value = njs_mp_align(vm->mem_pool, sizeof(njs_value_t), value_size);
        if (njs_slow_path(value == NULL)) {
            return NJS_INDEX_NONE;
        }

        *value = *src;

        if (long_string) {
            string = (njs_string_t *) ((u_char *) value + sizeof(njs_value_t));
            value->long_string.data = string;

            string->start = (u_char *) string + sizeof(njs_string_t);
            string->length = src->long_string.data->length;
            string->retain = 0xffff;

            memcpy(string->start, start, size);
        }

        lhq.replace = 0;
        lhq.value = value;
        lhq.pool = vm->mem_pool;

        values_hash = runtime ? &vm->values_hash : &vm->shared->values_hash;

        ret = njs_lvlhsh_insert(values_hash, &lhq);

        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_INDEX_NONE;
        }
    }

    if (start != (u_char *) src) {
        /*
         * The source node value must be updated with the shared value
         * allocated from the permanent memory pool because the node
         * value can be used as a variable initial value.
         */
        *(njs_value_t *) src = *value;
    }

    return (njs_index_t) value;
}


const njs_object_type_init_t  njs_string_type_init = {
    .constructor = njs_native_ctor(njs_string_constructor, 1, 0),
    .constructor_props = &njs_string_constructor_init,
    .prototype_props = &njs_string_prototype_init,
    .prototype_value = { .object_value = {
                            .value = njs_string(""),
                            .object = { .type = NJS_OBJECT_STRING } }
                       },
};
