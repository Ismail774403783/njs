
/*
 * Copyright (C) Dmitry Volyntsev
 * Copyright (C) NGINX, Inc.
 */

#ifndef _NJS_CHB_H_INCLUDED_
#define _NJS_CHB_H_INCLUDED_


typedef struct njs_chb_node_s njs_chb_node_t;

struct njs_chb_node_s {
    njs_chb_node_t          *next;
    u_char                  *start;
    u_char                  *pos;
    u_char                  *end;
};

typedef struct {
    njs_bool_t              error;
    njs_mp_t                *pool;
    njs_chb_node_t          *nodes;
    njs_chb_node_t          *last;
} njs_chb_t;


void njs_chb_append0(njs_chb_t *chain, const char *msg, size_t len);
void njs_chb_vsprintf(njs_chb_t *chain, size_t size, const char *fmt,
    va_list args);
void njs_chb_sprintf(njs_chb_t *chain, size_t size, const char* fmt, ...);
u_char *njs_chb_reserve(njs_chb_t *chain, size_t size);
void njs_chb_drain(njs_chb_t *chain, size_t drop);
void njs_chb_drop(njs_chb_t *chain, size_t drop);
njs_int_t njs_chb_join(njs_chb_t *chain, njs_str_t *str);
void njs_chb_join_to(njs_chb_t *chain, u_char *dst);
void njs_chb_destroy(njs_chb_t *chain);


#define njs_chb_append(chain, msg, len)                                      \
    njs_chb_append0(chain, (const char *) msg, len)

#define njs_chb_append_literal(chain, literal)                               \
    njs_chb_append0(chain, literal, njs_length(literal))

#define njs_chb_append_str(chain, str)                                       \
    njs_chb_append0(chain, (const char *) (str)->start, (str)->length)


#define njs_chb_node_size(n) (size_t) ((n)->pos - (n)->start)
#define njs_chb_node_room(n) (size_t) ((n)->end - (n)->pos)


njs_inline void
njs_chb_init(njs_chb_t *chain, njs_mp_t *pool)
{
    chain->error = 0;
    chain->pool = pool;
    chain->nodes = NULL;
    chain->last = NULL;
}


njs_inline size_t
njs_chb_size(njs_chb_t *chain)
{
    size_t          size;
    njs_chb_node_t  *n;

    n = chain->nodes;

    size = 0;

    while (n != NULL) {
        size += njs_chb_node_size(n);
        n = n->next;
    }

    return size;
}


njs_inline ssize_t
njs_chb_utf8_length(njs_chb_t *chain)
{
    ssize_t         len, length;
    njs_chb_node_t  *n;

    n = chain->nodes;

    length = 0;

    while (n != NULL) {
        len = njs_utf8_length(n->start, njs_chb_node_size(n));
        if (njs_slow_path(len < 0)) {
            return len;
        }

        length += len;
        n = n->next;
    }

    return length;
}


njs_inline void
njs_chb_written(njs_chb_t *chain, size_t bytes)
{
    chain->last->pos += bytes;
}


#endif /* _NJS_JSON_H_INCLUDED_ */
