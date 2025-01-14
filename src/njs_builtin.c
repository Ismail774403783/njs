
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Dmitry Volyntsev
 * Copyright (C) NGINX, Inc.
 */


#include <njs_main.h>


typedef struct {
    enum {
       NJS_BUILTIN_TRAVERSE_KEYS,
       NJS_BUILTIN_TRAVERSE_MATCH,
    }                          type;

    njs_function_native_t      native;

    njs_lvlhsh_t               keys;
    njs_str_t                  match;
} njs_builtin_traverse_t;


static njs_arr_t *njs_vm_expression_completions(njs_vm_t *vm,
    njs_str_t *expression);
static njs_arr_t *njs_object_completions(njs_vm_t *vm, njs_object_t *object);
static njs_int_t njs_env_hash_init(njs_vm_t *vm, njs_lvlhsh_t *hash,
    char **environment);


static const njs_object_init_t  njs_global_this_init;
static const njs_object_init_t  njs_njs_object_init;
static const njs_object_init_t  njs_process_object_init;


static const njs_object_init_t  *njs_object_init[] = {
    &njs_global_this_init,
    &njs_njs_object_init,
    &njs_process_object_init,
    &njs_math_object_init,
    &njs_json_object_init,
    NULL
};


static const njs_object_init_t  *njs_module_init[] = {
    &njs_fs_object_init,
    &njs_crypto_object_init,
    NULL
};


static const njs_object_type_init_t *const
    njs_object_type_init[NJS_OBJ_TYPE_MAX] =
{
    /* Global types. */

    &njs_obj_type_init,
    &njs_array_type_init,
    &njs_array_buffer_type_init,
    &njs_boolean_type_init,
    &njs_number_type_init,
    &njs_symbol_type_init,
    &njs_string_type_init,
    &njs_function_type_init,
    &njs_regexp_type_init,
    &njs_date_type_init,

    /* Hidden types. */

    &njs_hash_type_init,
    &njs_hmac_type_init,

    /* Error types. */

    &njs_error_type_init,
    &njs_eval_error_type_init,
    &njs_internal_error_type_init,
    &njs_range_error_type_init,
    &njs_reference_error_type_init,
    &njs_syntax_error_type_init,
    &njs_type_error_type_init,
    &njs_uri_error_type_init,
    &njs_memory_error_type_init,
};


extern char  **environ;


njs_inline njs_int_t
njs_object_hash_init(njs_vm_t *vm, njs_lvlhsh_t *hash,
    const njs_object_init_t *init)
{
    return njs_object_hash_create(vm, hash, init->properties, init->items);
}


njs_int_t
njs_builtin_objects_create(njs_vm_t *vm)
{
    njs_int_t                  ret;
    njs_uint_t                 i;
    njs_module_t               *module;
    njs_object_t               *object, *string_object;
    njs_function_t             *constructor;
    njs_vm_shared_t            *shared;
    njs_lvlhsh_query_t         lhq;
    njs_regexp_pattern_t       *pattern;
    njs_object_prototype_t     *prototype;
    const njs_object_prop_t    *prop;
    const njs_object_init_t    *obj, **p;

    static const njs_str_t  sandbox_key = njs_str("sandbox");
    static const njs_str_t  name_key = njs_str("name");

    shared = njs_mp_zalloc(vm->mem_pool, sizeof(njs_vm_shared_t));
    if (njs_slow_path(shared == NULL)) {
        return NJS_ERROR;
    }

    njs_lvlhsh_init(&shared->keywords_hash);

    ret = njs_lexer_keywords_init(vm->mem_pool, &shared->keywords_hash);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    njs_lvlhsh_init(&shared->values_hash);

    pattern = njs_regexp_pattern_create(vm, (u_char *) "(?:)",
                                        njs_length("(?:)"), 0);
    if (njs_slow_path(pattern == NULL)) {
        return NJS_ERROR;
    }

    shared->empty_regexp_pattern = pattern;

    ret = njs_object_hash_init(vm, &shared->array_instance_hash,
                               &njs_array_instance_init);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    ret = njs_object_hash_init(vm, &shared->string_instance_hash,
                               &njs_string_instance_init);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    ret = njs_object_hash_init(vm, &shared->function_instance_hash,
                               &njs_function_instance_init);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    ret = njs_object_hash_init(vm, &shared->arrow_instance_hash,
                               &njs_arrow_instance_init);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    ret = njs_object_hash_init(vm, &shared->arguments_object_instance_hash,
                               &njs_arguments_object_instance_init);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    ret = njs_object_hash_init(vm, &shared->regexp_instance_hash,
                               &njs_regexp_instance_init);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    object = shared->objects;

    for (p = njs_object_init; *p != NULL; p++) {
        obj = *p;

        ret = njs_object_hash_init(vm, &object->shared_hash, obj);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }

        object->type = NJS_OBJECT;
        object->shared = 1;
        object->extensible = 1;

        object++;
    }

    ret = njs_env_hash_init(vm, &shared->env_hash, environ);
    if (njs_slow_path(ret != NJS_OK)) {
        return NJS_ERROR;
    }

    njs_lvlhsh_init(&vm->modules_hash);

    lhq.replace = 0;
    lhq.pool = vm->mem_pool;

    for (p = njs_module_init; *p != NULL; p++) {
        obj = *p;

        module = njs_mp_zalloc(vm->mem_pool, sizeof(njs_module_t));
        if (njs_slow_path(module == NULL)) {
            return NJS_ERROR;
        }

        module->function.native = 1;

        ret = njs_object_hash_init(vm, &module->object.shared_hash, obj);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }

        if (vm->options.sandbox) {
            lhq.key = sandbox_key;
            lhq.key_hash = njs_djb_hash(sandbox_key.start, sandbox_key.length);
            lhq.proto = &njs_object_hash_proto;

            ret = njs_lvlhsh_find(&module->object.shared_hash, &lhq);
            if (njs_fast_path(ret != NJS_OK)) {
                continue;
            }
        }

        lhq.key = name_key;
        lhq.key_hash = njs_djb_hash(name_key.start, name_key.length);
        lhq.proto = &njs_object_hash_proto;

        ret = njs_lvlhsh_find(&module->object.shared_hash, &lhq);
        if (njs_fast_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }

        prop = lhq.value;

        njs_string_get(&prop->value, &module->name);
        module->object.shared = 1;

        lhq.key = module->name;
        lhq.key_hash = njs_djb_hash(lhq.key.start, lhq.key.length);
        lhq.proto = &njs_modules_hash_proto;
        lhq.value = module;

        ret = njs_lvlhsh_insert(&vm->modules_hash, &lhq);
        if (njs_fast_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }
    }

    prototype = shared->prototypes;

    for (i = NJS_OBJ_TYPE_OBJECT; i < NJS_OBJ_TYPE_MAX; i++) {
        prototype[i] = njs_object_type_init[i]->prototype_value;

        ret = njs_object_hash_init(vm, &prototype[i].object.shared_hash,
                                   njs_object_type_init[i]->prototype_props);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }

        prototype[i].object.extensible = 1;
    }

    shared->prototypes[NJS_OBJ_TYPE_REGEXP].regexp.pattern =
                                              shared->empty_regexp_pattern;

    constructor = shared->constructors;

    for (i = NJS_OBJ_TYPE_OBJECT; i < NJS_OBJ_TYPE_MAX; i++) {
        constructor[i] = njs_object_type_init[i]->constructor;
        constructor[i].object.shared = 0;

        ret = njs_object_hash_init(vm, &constructor[i].object.shared_hash,
                                   njs_object_type_init[i]->constructor_props);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }
    }

    vm->global_object = shared->objects[0];
    vm->global_object.shared = 0;

    string_object = &shared->string_object;
    njs_lvlhsh_init(&string_object->hash);
    string_object->shared_hash = shared->string_instance_hash;
    string_object->type = NJS_OBJECT_STRING;
    string_object->shared = 1;
    string_object->extensible = 0;

    vm->shared = shared;

    return NJS_OK;
}


njs_int_t
njs_builtin_objects_clone(njs_vm_t *vm, njs_value_t *global)
{
    size_t        size;
    njs_uint_t    i;
    njs_object_t  *object_prototype, *function_prototype, *error_prototype,
                  *error_constructor;

    /*
     * Copy both prototypes and constructors arrays by one memcpy()
     * because they are stored together.
     */
    size = (sizeof(njs_object_prototype_t) + sizeof(njs_function_t))
           * NJS_OBJ_TYPE_MAX;

    memcpy(vm->prototypes, vm->shared->prototypes, size);

    object_prototype = &vm->prototypes[NJS_OBJ_TYPE_OBJECT].object;

    for (i = NJS_OBJ_TYPE_ARRAY; i < NJS_OBJ_TYPE_EVAL_ERROR; i++) {
        vm->prototypes[i].object.__proto__ = object_prototype;
    }

    error_prototype = &vm->prototypes[NJS_OBJ_TYPE_ERROR].object;

    for (i = NJS_OBJ_TYPE_EVAL_ERROR; i < NJS_OBJ_TYPE_MAX; i++) {
        vm->prototypes[i].object.__proto__ = error_prototype;
    }

    function_prototype = &vm->prototypes[NJS_OBJ_TYPE_FUNCTION].object;

    for (i = NJS_OBJ_TYPE_OBJECT; i < NJS_OBJ_TYPE_EVAL_ERROR; i++) {
        vm->constructors[i].object.__proto__ = function_prototype;
    }

    error_constructor = &vm->constructors[NJS_OBJ_TYPE_ERROR].object;

    for (i = NJS_OBJ_TYPE_EVAL_ERROR; i < NJS_OBJ_TYPE_MAX; i++) {
        vm->constructors[i].object.__proto__ = error_constructor;
    }

    vm->global_object.__proto__ = object_prototype;
    njs_set_object(global, &vm->global_object);

    vm->string_object = vm->shared->string_object;
    vm->string_object.__proto__ = &vm->prototypes[NJS_OBJ_TYPE_STRING].object;

    return NJS_OK;
}


static njs_int_t
njs_builtin_traverse(njs_vm_t *vm, njs_traverse_t *traverse, void *data)
{
    size_t                  len;
    u_char                  *p, *start, *end;
    njs_int_t               ret, n;
    njs_str_t               name;
    njs_object_prop_t       *prop;
    njs_lvlhsh_query_t      lhq;
    njs_builtin_traverse_t  *ctx;
    njs_traverse_t          *path[NJS_TRAVERSE_MAX_DEPTH];
    u_char                  buf[256];

    ctx = data;

    if (ctx->type == NJS_BUILTIN_TRAVERSE_MATCH) {
        prop = traverse->prop;

        if (!(njs_is_function(&prop->value)
              && njs_function(&prop->value)->native
              && njs_function(&prop->value)->u.native == ctx->native))
        {
            return NJS_OK;
        }
    }

    n = 0;

    while (traverse != NULL) {
        path[n++] = traverse;
        traverse = traverse->parent;
    }

    n--;

    p = buf;
    end = buf + sizeof(buf);

    do {
        njs_string_get(&path[n]->prop->name, &name);

        if (njs_slow_path((p + name.length + 1) > end)) {
            njs_type_error(vm, "njs_builtin_traverse() key is too long");
            return NJS_ERROR;
        }

        p = njs_cpymem(p, name.start, name.length);

        if (n != 0) {
            *p++ = '.';
        }

    } while (n-- > 0);

    if (ctx->type == NJS_BUILTIN_TRAVERSE_MATCH) {
        len = ctx->match.length;
        start = njs_mp_alloc(vm->mem_pool, len + (p - buf) + (len != 0));
        if (njs_slow_path(start == NULL)) {
            njs_memory_error(vm);
            return NJS_ERROR;
        }

        if (len != 0) {
            memcpy(start, ctx->match.start, len);
            start[len++] = '.';
        }

        memcpy(start + len, buf, p - buf);
        ctx->match.length = len + p - buf;
        ctx->match.start = start;

        return NJS_DONE;
    }

    /* NJS_BUILTIN_TRAVERSE_KEYS. */

    prop = njs_object_prop_alloc(vm, &njs_value_undefined, &njs_value_null, 0);
    if (njs_slow_path(prop == NULL)) {
        return NJS_ERROR;
    }

    ret = njs_string_new(vm, &prop->name, buf, p - buf, 0);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    lhq.value = prop;
    njs_string_get(&prop->name, &lhq.key);
    lhq.key_hash = njs_djb_hash(lhq.key.start, lhq.key.length);
    lhq.replace = 1;
    lhq.pool = vm->mem_pool;
    lhq.proto = &njs_object_hash_proto;

    ret = njs_lvlhsh_insert(&ctx->keys, &lhq);
    if (njs_slow_path(ret != NJS_OK)) {
        njs_internal_error(vm, "lvlhsh insert/replace failed");
        return NJS_ERROR;
    }

    return NJS_OK;
}


static njs_arr_t *
njs_builtin_completions(njs_vm_t *vm)
{
    njs_arr_t                *array;
    njs_str_t                *completion;
    njs_int_t                ret;
    njs_keyword_t            *keyword;
    njs_lvlhsh_each_t        lhe;
    njs_builtin_traverse_t   ctx;
    const njs_object_prop_t  *prop;

    array = njs_arr_create(vm->mem_pool, 64, sizeof(njs_str_t));
    if (njs_slow_path(array == NULL)) {
        return NULL;
    }

    /* Keywords completions. */

    njs_lvlhsh_each_init(&lhe, &njs_keyword_hash_proto);

    for ( ;; ) {
        keyword = njs_lvlhsh_each(&vm->shared->keywords_hash, &lhe);

        if (keyword == NULL) {
            break;
        }

        completion = njs_arr_add(array);
        if (njs_slow_path(completion == NULL)) {
            return NULL;
        }

        *completion = keyword->name;
    }

    /* Global object completions. */

    ctx.type = NJS_BUILTIN_TRAVERSE_KEYS;
    njs_lvlhsh_init(&ctx.keys);

    ret = njs_object_traverse(vm, &vm->global_object, &ctx,
                              njs_builtin_traverse);
    if (njs_slow_path(ret != NJS_OK)) {
        return NULL;
    }

    njs_lvlhsh_each_init(&lhe, &njs_object_hash_proto);

    for ( ;; ) {
        prop = njs_lvlhsh_each(&ctx.keys, &lhe);

        if (prop == NULL) {
            break;
        }

        completion = njs_arr_add(array);
        if (njs_slow_path(completion == NULL)) {
            return NULL;
        }

        njs_string_get(&prop->name, completion);
    }

    return array;
}


njs_arr_t *
njs_vm_completions(njs_vm_t *vm, njs_str_t *expression)
{
    if (expression == NULL) {
        return njs_builtin_completions(vm);
    }

    return njs_vm_expression_completions(vm, expression);
}


static njs_arr_t *
njs_vm_expression_completions(njs_vm_t *vm, njs_str_t *expression)
{
    u_char              *p, *end;
    njs_int_t           ret;
    njs_value_t         *value;
    njs_variable_t      *var;
    njs_object_prop_t   *prop;
    njs_lvlhsh_query_t  lhq;

    if (njs_slow_path(vm->parser == NULL)) {
        return NULL;
    }

    p = expression->start;
    end = p + expression->length;

    lhq.key.start = p;

    while (p < end && *p != '.') { p++; }

    lhq.proto = &njs_variables_hash_proto;
    lhq.key.length = p - lhq.key.start;
    lhq.key_hash = njs_djb_hash(lhq.key.start, lhq.key.length);

    ret = njs_lvlhsh_find(&vm->parser->scope->variables, &lhq);
    if (njs_slow_path(ret != NJS_OK)) {
        return NULL;
    }

    var = lhq.value;
    value = njs_vmcode_operand(vm, var->index);

    if (!njs_is_object(value)) {
        return NULL;
    }

    lhq.proto = &njs_object_hash_proto;

    for ( ;; ) {

        if (p == end) {
            break;
        }

        lhq.key.start = ++p;

        while (p < end && *p != '.') { p++; }

        lhq.key.length = p - lhq.key.start;
        lhq.key_hash = njs_djb_hash(lhq.key.start, lhq.key.length);

        ret = njs_lvlhsh_find(njs_object_hash(value), &lhq);
        if (njs_slow_path(ret != NJS_OK)) {
            return NULL;
        }

        prop = lhq.value;

        if (!njs_is_object(&prop->value)) {
            return NULL;
        }

        value = &prop->value;
    }

    return njs_object_completions(vm, njs_object(value));
}


static njs_arr_t *
njs_object_completions(njs_vm_t *vm, njs_object_t *object)
{
    size_t             size;
    njs_str_t          *compl;
    njs_arr_t          *completions;
    njs_uint_t         n, k;
    njs_object_t       *o;
    njs_object_prop_t  *prop;
    njs_lvlhsh_each_t  lhe;

    size = 0;
    o = object;

    do {
        njs_lvlhsh_each_init(&lhe, &njs_object_hash_proto);

        for ( ;; ) {
            prop = njs_lvlhsh_each(&o->hash, &lhe);
            if (prop == NULL) {
                break;
            }

            size++;
        }

        njs_lvlhsh_each_init(&lhe, &njs_object_hash_proto);

        for ( ;; ) {
            prop = njs_lvlhsh_each(&o->shared_hash, &lhe);
            if (prop == NULL) {
                break;
            }

            size++;
        }

        o = o->__proto__;

    } while (o != NULL);

    completions = njs_arr_create(vm->mem_pool, size, sizeof(njs_str_t));
    if (njs_slow_path(completions == NULL)) {
        return NULL;
    }

    n = 0;
    o = object;
    compl = completions->start;

    do {
        njs_lvlhsh_each_init(&lhe, &njs_object_hash_proto);

        for ( ;; ) {
            prop = njs_lvlhsh_each(&o->hash, &lhe);
            if (prop == NULL) {
                break;
            }

            njs_string_get(&prop->name, &compl[n]);

            for (k = 0; k < n; k++) {
                if (njs_strstr_eq(&compl[k], &compl[n])) {
                    break;
                }
            }

            if (k == n) {
                n++;
            }
        }

        njs_lvlhsh_each_init(&lhe, &njs_object_hash_proto);

        for ( ;; ) {
            prop = njs_lvlhsh_each(&o->shared_hash, &lhe);
            if (prop == NULL) {
                break;
            }

            njs_string_get(&prop->name, &compl[n]);

            for (k = 0; k < n; k++) {
                if (njs_strstr_eq(&compl[k], &compl[n])) {
                    break;
                }
            }

            if (k == n) {
                n++;
            }
        }

        o = o->__proto__;

    } while (o != NULL);

    completions->items = n;

    return completions;
}


njs_int_t
njs_builtin_match_native_function(njs_vm_t *vm, njs_function_native_t func,
    njs_str_t *name)
{
    njs_int_t               ret;
    njs_uint_t              i;
    njs_value_t             value;
    njs_module_t            *module;
    njs_lvlhsh_each_t       lhe;
    njs_builtin_traverse_t  ctx;

    ctx.type = NJS_BUILTIN_TRAVERSE_MATCH;
    ctx.native = func;

    /* Global object. */

    ctx.match = njs_str_value("");

    ret = njs_object_traverse(vm, &vm->global_object, &ctx,
                              njs_builtin_traverse);

    if (ret == NJS_DONE) {
        *name = ctx.match;
        return NJS_OK;
    }

    /* Constructor from built-in modules (not-mapped to global object). */

    for (i = NJS_OBJ_TYPE_HIDDEN_MIN; i < NJS_OBJ_TYPE_HIDDEN_MAX; i++) {
        njs_set_object(&value, &vm->constructors[i].object);

        ret = njs_value_property(vm, &value, njs_value_arg(&njs_string_name),
                                 &value);

        if (ret == NJS_OK && njs_is_string(&value)) {
            njs_string_get(&value, &ctx.match);
        }

        ret = njs_object_traverse(vm, &vm->constructors[i].object, &ctx,
                                  njs_builtin_traverse);

        if (ret == NJS_DONE) {
            *name = ctx.match;
            return NJS_OK;
        }
    }

    /* Modules. */

    njs_lvlhsh_each_init(&lhe, &njs_modules_hash_proto);

    for ( ;; ) {
        module = njs_lvlhsh_each(&vm->modules_hash, &lhe);

        if (module == NULL) {
            break;
        }

        ctx.match = module->name;

        ret = njs_object_traverse(vm, &module->object, &ctx,
                                  njs_builtin_traverse);

        if (ret == NJS_DONE) {
            *name = ctx.match;
            return NJS_OK;
        }
    }

    return NJS_DECLINED;
}


static njs_int_t
njs_dump_value(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    uint32_t     n;
    njs_int_t    ret;
    njs_str_t    str;
    njs_value_t  *value, *indent;

    value = njs_arg(args, nargs, 1);
    indent = njs_arg(args, nargs, 2);

    ret = njs_value_to_uint32(vm, indent, &n);
    if (njs_slow_path(ret != NJS_OK)) {
        return ret;
    }

    n = njs_min(n, 5);

    if (njs_vm_value_dump(vm, &str, value, 1, n) != NJS_OK) {
        return NJS_ERROR;
    }

    return njs_string_new(vm, &vm->retval, str.start, str.length, 0);
}


static njs_int_t
njs_top_level_object(njs_vm_t *vm, njs_object_prop_t *self,
    njs_value_t *global, njs_value_t *setval, njs_value_t *retval)
{
    njs_int_t           ret;
    njs_object_t        *object;
    njs_object_prop_t   *prop;
    njs_lvlhsh_query_t  lhq;

    if (njs_slow_path(setval != NULL)) {
        *retval = *setval;

    } else {
        njs_set_object(retval, &vm->shared->objects[self->value.data.magic16]);

        object = njs_object_value_copy(vm, retval);
        if (njs_slow_path(object == NULL)) {
            return NJS_ERROR;
        }
    }

    prop = njs_object_prop_alloc(vm, &self->name, retval, 1);
    if (njs_slow_path(prop == NULL)) {
        return NJS_ERROR;
    }

    /* GC */

    prop->value = *retval;
    prop->enumerable = self->enumerable;

    lhq.value = prop;
    njs_string_get(&self->name, &lhq.key);
    lhq.key_hash = self->value.data.magic32;
    lhq.replace = 1;
    lhq.pool = vm->mem_pool;
    lhq.proto = &njs_object_hash_proto;

    ret = njs_lvlhsh_insert(njs_object_hash(global), &lhq);
    if (njs_slow_path(ret != NJS_OK)) {
        njs_internal_error(vm, "lvlhsh insert/replace failed");
        return NJS_ERROR;
    }

    return NJS_OK;
}


static njs_int_t
njs_top_level_constructor(njs_vm_t *vm, njs_object_prop_t *self,
    njs_value_t *global, njs_value_t *setval, njs_value_t *retval)
{
    njs_int_t           ret;
    njs_function_t      *ctor;
    njs_object_prop_t   *prop;
    njs_lvlhsh_query_t  lhq;

    if (njs_slow_path(setval != NULL)) {
        *retval = *setval;

    } else {
        ctor = &vm->constructors[self->value.data.magic16];

        njs_set_function(retval, ctor);
    }

    prop = njs_object_prop_alloc(vm, &self->name, retval, 1);
    if (njs_slow_path(prop == NULL)) {
        return NJS_ERROR;
    }

    /* GC */

    prop->value = *retval;
    prop->enumerable = 0;

    lhq.value = prop;
    njs_string_get(&self->name, &lhq.key);
    lhq.key_hash = self->value.data.magic32;
    lhq.replace = 1;
    lhq.pool = vm->mem_pool;
    lhq.proto = &njs_object_hash_proto;

    ret = njs_lvlhsh_insert(njs_object_hash(global), &lhq);
    if (njs_slow_path(ret != NJS_OK)) {
        njs_internal_error(vm, "lvlhsh insert/replace failed");
        return NJS_ERROR;
    }

    return NJS_OK;
}


static const njs_object_prop_t  njs_global_this_object_properties[] =
{
    {
        .type = NJS_PROPERTY,
        .name = njs_wellknown_symbol(NJS_SYMBOL_TO_STRING_TAG),
        .value = njs_string("global"),
        .configurable = 1,
    },

    /* Global constants. */

    {
        .type = NJS_PROPERTY,
        .name = njs_string("NaN"),
        .value = njs_value(NJS_NUMBER, 0, NAN),
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("Infinity"),
        .value = njs_value(NJS_NUMBER, 1, INFINITY),
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("undefined"),
        .value = njs_value(NJS_UNDEFINED, 0, NAN),
    },

    /* Global functions. */

    {
        .type = NJS_PROPERTY,
        .name = njs_string("isFinite"),
        .value = njs_native_function(njs_number_global_is_finite, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("isNaN"),
        .value = njs_native_function(njs_number_global_is_nan, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("parseFloat"),
        .value = njs_native_function(njs_number_parse_float, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("parseInt"),
        .value = njs_native_function(njs_number_parse_int, 2),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("toString"),
        .value = njs_native_function(njs_object_prototype_to_string, 0),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("encodeURI"),
        .value = njs_native_function(njs_string_encode_uri, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_long_string("encodeURIComponent"),
        .value = njs_native_function(njs_string_encode_uri_component, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("decodeURI"),
        .value = njs_native_function(njs_string_decode_uri, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_long_string("decodeURIComponent"),
        .value = njs_native_function(njs_string_decode_uri_component, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("eval"),
        .value = njs_native_function(njs_eval_function, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("setTimeout"),
        .value = njs_native_function(njs_set_timeout, 2),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("setImmediate"),
        .value = njs_native_function(njs_set_immediate, 4),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("clearTimeout"),
        .value = njs_native_function(njs_clear_timeout, 1),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("require"),
        .value = njs_native_function(njs_module_require, 1),
        .writable = 1,
        .configurable = 1,
    },

    /* Global objects. */

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("njs"),
        .value = njs_prop_handler2(njs_top_level_object, NJS_OBJECT_NJS,
                                   NJS_NJS_HASH),
        .writable = 1,
        .enumerable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("process"),
        .value = njs_prop_handler2(njs_top_level_object, NJS_OBJECT_PROCESS,
                                   NJS_PROCESS_HASH),
        .writable = 1,
        .enumerable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Math"),
        .value = njs_prop_handler2(njs_top_level_object, NJS_OBJECT_MATH,
                                   NJS_MATH_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("JSON"),
        .value = njs_prop_handler2(njs_top_level_object, NJS_OBJECT_JSON,
                                   NJS_JSON_HASH),
        .writable = 1,
        .configurable = 1,
    },

    /* Global constructors. */

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Object"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_OBJECT, NJS_OBJECT_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Array"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_ARRAY, NJS_ARRAY_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("ArrayBuffer"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_ARRAY_BUFFER,
                                   NJS_ARRAY_BUFFER_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Boolean"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_BOOLEAN, NJS_BOOLEAN_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Number"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_NUMBER, NJS_NUMBER_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Symbol"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_SYMBOL, NJS_SYMBOL_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("String"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_STRING, NJS_STRING_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Function"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_FUNCTION, NJS_FUNCTION_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("RegExp"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_REGEXP, NJS_REGEXP_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Date"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_DATE, NJS_DATE_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("Error"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_ERROR, NJS_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("EvalError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_EVAL_ERROR,
                                   NJS_EVAL_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("InternalError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_INTERNAL_ERROR,
                                   NJS_INTERNAL_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("RangeError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_RANGE_ERROR,
                                   NJS_RANGE_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("ReferenceError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_REF_ERROR, NJS_REF_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("SyntaxError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_SYNTAX_ERROR,
                                   NJS_SYNTAX_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("TypeError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_TYPE_ERROR,
                                   NJS_TYPE_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("URIError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_URI_ERROR,
                                   NJS_URI_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("MemoryError"),
        .value = njs_prop_handler2(njs_top_level_constructor,
                                   NJS_OBJ_TYPE_MEMORY_ERROR,
                                   NJS_MEMORY_ERROR_HASH),
        .writable = 1,
        .configurable = 1,
    },
};


static const njs_object_init_t  njs_global_this_init = {
    njs_global_this_object_properties,
    njs_nitems(njs_global_this_object_properties)
};


static const njs_object_prop_t  njs_njs_object_properties[] =
{
    {
        .type = NJS_PROPERTY,
        .name = njs_wellknown_symbol(NJS_SYMBOL_TO_STRING_TAG),
        .value = njs_string("njs"),
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("version"),
        .value = njs_string(NJS_VERSION),
        .configurable = 1,
        .enumerable = 1,
    },

    {
        .type = NJS_PROPERTY,
        .name = njs_string("dump"),
        .value = njs_native_function(njs_dump_value, 0),
        .configurable = 1,
    },
};


static const njs_object_init_t  njs_njs_object_init = {
    njs_njs_object_properties,
    njs_nitems(njs_njs_object_properties),
};


static njs_int_t
njs_process_object_argv(njs_vm_t *vm, njs_object_prop_t *pr,
    njs_value_t *process, njs_value_t *unused, njs_value_t *retval)
{
    char                **arg;
    njs_int_t           ret;
    njs_uint_t          i;
    njs_array_t         *argv;
    njs_object_prop_t   *prop;
    njs_lvlhsh_query_t  lhq;

    static const njs_value_t  argv_string = njs_string("argv");

    argv = njs_array_alloc(vm, vm->options.argc, 0);
    if (njs_slow_path(argv == NULL)) {
        return NJS_ERROR;
    }

    i = 0;

    for (arg = vm->options.argv; i < vm->options.argc; arg++) {
        njs_string_set(vm, &argv->start[i++], (u_char *) *arg,
                       njs_strlen(*arg));
    }

    prop = njs_object_prop_alloc(vm, &argv_string, &njs_value_undefined, 1);
    if (njs_slow_path(prop == NULL)) {
        return NJS_ERROR;
    }

    njs_set_array(&prop->value, argv);

    lhq.value = prop;
    lhq.key_hash = NJS_ARGV_HASH;
    lhq.key = njs_str_value("argv");
    lhq.replace = 1;
    lhq.pool = vm->mem_pool;
    lhq.proto = &njs_object_hash_proto;

    ret = njs_lvlhsh_insert(njs_object_hash(process), &lhq);

    if (njs_fast_path(ret == NJS_OK)) {
        *retval = prop->value;
        return NJS_OK;
    }

    njs_internal_error(vm, "lvlhsh insert failed");

    return NJS_ERROR;
}


static njs_int_t
njs_env_hash_init(njs_vm_t *vm, njs_lvlhsh_t *hash, char **environment)
{
    char                **ep;
    u_char              *val, *entry;
    njs_int_t           ret;
    njs_object_prop_t   *prop;
    njs_lvlhsh_query_t  lhq;

    lhq.replace = 0;
    lhq.pool = vm->mem_pool;
    lhq.proto = &njs_object_hash_proto;

    ep = environment;

    while (*ep != NULL) {
        prop = njs_object_prop_alloc(vm, &njs_value_undefined,
                                     &njs_value_undefined, 1);
        if (njs_slow_path(prop == NULL)) {
            return NJS_ERROR;
        }

        entry = (u_char *) *ep++;

        val = njs_strchr(entry, '=');
        if (njs_slow_path(val == NULL)) {
            continue;
        }

        ret = njs_string_set(vm, &prop->name, entry, val - entry);
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }

        val++;

        ret = njs_string_set(vm, &prop->value, val, njs_strlen(val));
        if (njs_slow_path(ret != NJS_OK)) {
            return NJS_ERROR;
        }

        lhq.value = prop;
        njs_string_get(&prop->name, &lhq.key);
        lhq.key_hash = njs_djb_hash(lhq.key.start, lhq.key.length);

        ret = njs_lvlhsh_insert(hash, &lhq);
        if (njs_slow_path(ret != NJS_OK)) {
            njs_internal_error(vm, "lvlhsh insert failed");
            return NJS_ERROR;
        }
    }

    return NJS_OK;
}


static njs_int_t
njs_process_object_env(njs_vm_t *vm, njs_object_prop_t *pr,
    njs_value_t *process, njs_value_t *unused, njs_value_t *retval)
{
    njs_int_t           ret;
    njs_object_t        *env;
    njs_object_prop_t   *prop;
    njs_lvlhsh_query_t  lhq;

    static const njs_value_t  env_string = njs_string("env");

    env = njs_object_alloc(vm);
    if (njs_slow_path(env == NULL)) {
        return NJS_ERROR;
    }

    env->shared_hash = vm->shared->env_hash;

    prop = njs_object_prop_alloc(vm, &env_string, &njs_value_undefined, 1);
    if (njs_slow_path(prop == NULL)) {
        return NJS_ERROR;
    }

    njs_set_object(&prop->value, env);

    lhq.replace = 1;
    lhq.pool = vm->mem_pool;
    lhq.proto = &njs_object_hash_proto;
    lhq.value = prop;
    lhq.key = njs_str_value("env");
    lhq.key_hash = NJS_ENV_HASH;

    ret = njs_lvlhsh_insert(njs_object_hash(process), &lhq);

    if (njs_fast_path(ret == NJS_OK)) {
        *retval = prop->value;
        return NJS_OK;
    }

    njs_internal_error(vm, "lvlhsh insert failed");

    return NJS_ERROR;
}


static njs_int_t
njs_process_object_pid(njs_vm_t *vm, njs_object_prop_t *prop,
    njs_value_t *unused, njs_value_t *unused2, njs_value_t *retval)
{
    njs_set_number(retval, getpid());

    return NJS_OK;
}


static njs_int_t
njs_process_object_ppid(njs_vm_t *vm, njs_object_prop_t *prop,
    njs_value_t *unused, njs_value_t *unused2, njs_value_t *retval)
{
    njs_set_number(retval, getppid());

    return NJS_OK;
}


static const njs_object_prop_t  njs_process_object_properties[] =
{
    {
        .type = NJS_PROPERTY,
        .name = njs_wellknown_symbol(NJS_SYMBOL_TO_STRING_TAG),
        .value = njs_string("process"),
        .configurable = 1,
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("argv"),
        .value = njs_prop_handler(njs_process_object_argv),
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("env"),
        .value = njs_prop_handler(njs_process_object_env),
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("pid"),
        .value = njs_prop_handler(njs_process_object_pid),
    },

    {
        .type = NJS_PROPERTY_HANDLER,
        .name = njs_string("ppid"),
        .value = njs_prop_handler(njs_process_object_ppid),
    },

};


static const njs_object_init_t  njs_process_object_init = {
    njs_process_object_properties,
    njs_nitems(njs_process_object_properties),
};
