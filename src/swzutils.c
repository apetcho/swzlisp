#include "swzlisp.h"
#include<stdlib.h>
#include<string.h>

// -*------------------------------------------------------------*-
// -*- CharBuffer                                               -*-
// -*------------------------------------------------------------*-
/**
 * @brief Initialize a new character buffer.
 * 
 * @param cbuffer  CharBuffer data structure
 * @param capacity Initial buffer capacity
 */
void cbuffer_init(CharBuffer *cbuffer, uint32_t capacity){
    if(!cbuffer){
        fprintf(stderr, "Invalid character buffer argument.\n");
        abort();
    }
    cbuffer->buffer = calloc(capacity, sizeof(char));
    cbuffer->buffer[0] = '\0';
    cbuffer->capacity = capacity;
    cbuffer->len = 0;
}

/**
 * @brief Allocate and initialize a fresh character buffer.
 * 
 * @param capacity CharBuffer capacity.
 * @return CharBuffer* 
 */
CharBuffer* cbuffer_create(uint32_t capacity){
    CharBuffer *cbuffer = calloc(1, sizeof(CharBuffer));
    cbuffer_init(cbuffer, capacity);
    return cbuffer;
}

/**
 * @brief Deallocate CharBuffer
 * 
 * @param cbuffer 
 */
void cbuffer_destroy(CharBuffer* cbuffer){
    free(cbuffer->buffer);
    cbuffer->buffer = NULL;
}

/**
 * @brief Deallocate character buffer and its wrapper structure.
 * 
 * @param cbuffer 
 */
void cbuffer_delete(CharBuffer* cbuffer){
    cbuffer_destroy(cbuffer);
    free(cbuffer);
}

/**
 * @brief Resize character buffer to fit a given amount of characters.
 * 
 * @param cbuffer 
 * @param minsize 
 */
static void _cbuffer_expand_to_fit(CharBuffer *cbuffer, uint32_t minsize){
    uint32_t capacity = cbuffer->capacity;
    while(capacity < minsize){
        capacity *= 2;
    }
    if(capacity != cbuffer->capacity){
        cbuffer->buffer = realloc(cbuffer->buffer, sizeof(char) * capacity);
        cbuffer->capacity = capacity;
    }
}

// -*-
void cbuffer_concat(CharBuffer* cbuffer, const char *cstr){
    size_t len = strlen(cstr);
    _cbuffer_expand_to_fit(cbuffer, (cbuffer->len + len + 1));
    strcpy(cbuffer->buffer + cbuffer->len, cstr);
    cbuffer->len += len;
}

// -*-
void cbuffer_append(CharBuffer* cbuffer, char c){
    _cbuffer_expand_to_fit(cbuffer, cbuffer->len + 2);
    cbuffer->buffer[cbuffer->len] = c;
    cbuffer->len++;
    cbuffer->buffer[cbuffer->len] = '\0';
}

// -*-
void cbuffer_trim(CharBuffer* cbuffer){
    cbuffer->buffer = realloc(cbuffer->buffer, sizeof(char) * cbuffer->len + 1);
    cbuffer->capacity = cbuffer->len + 1;
}

// -*-
void cbuffer_clear(CharBuffer* cbuffer){
    cbuffer->buffer[0] = '\0';
    cbuffer->len = 0;
}

// -*-
void cbuffer_printf(CharBuffer* cbuffer, const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    cbuffer_vprintf(cbuffer, fmt, args);
    va_end(args);
}

// -*-
void cbuffer_vprintf(CharBuffer* cbuffer, const char* fmt, va_list args){
    va_list argp;
    int len;
    va_copy(argp, args);

    len = vsnprintf(NULL, 0, fmt, args);
    _cbuffer_expand_to_fit(cbuffer, cbuffer->len + len + 1);
    vsnprintf(cbuffer->buffer + cbuffer->len, len + 1, fmt, argp);
    va_end(argp);
}

// -*------------------------------------------------------------*-
// -*- RingBuffer                                               -*-
// -*------------------------------------------------------------*-
// -*-
void rbuffer_init(RingBuffer *rbuffer, uint32_t dsize, int init){
    rbuffer->dsize = dsize;
    rbuffer->nalloc = init;
    rbuffer->start = 0;
    rbuffer->count = 0;
    rbuffer->buffer = calloc(dsize, init);
}

// -*-
void rbuffer_destroy(RingBuffer *rbuffer){
    free(rbuffer->buffer);
}

// -*-
void rbuffer_push_front(RingBuffer *rbuffer, void *src){
    int neostart;
    if(rbuffer->count >= rbuffer->nalloc){
        rbuffer_grow(rbuffer);
    }
    neostart = (rbuffer->start + rbuffer->nalloc - 1) % rbuffer->nalloc;
    rbuffer->start = neostart;
    memcpy(
        (char *)rbuffer->buffer + rbuffer->start * rbuffer->dsize,
        src, rbuffer->dsize
    );
    rbuffer->count++;
}

// -*-
void rbuffer_pop_front(RingBuffer *rbuffer, void *dst){
    //! @todo
}

// -*-
void rbuffer_push_back(RingBuffer *rbuffer, void *src){
    //! @todo
}

// -*-
void rbuffer_pop_back(RingBuffer *rbuffer, void *dst){
    //! @todo
}

// -*-
void rbuffer_grow(RingBuffer *rbuffer){
    int i, oldalloc;
    oldalloc = rbuffer->nalloc;
    rbuffer->nalloc *= 2;
    rbuffer->buffer = realloc(rbuffer->buffer, rbuffer->nalloc * rbuffer->dsize);
    for (i = 0; i < rbuffer->count; i++){
        int oldIdx, neoIdx;
        oldIdx = (rbuffer->start + i) % oldalloc;
        neoIdx = (rbuffer->start + i) % rbuffer->nalloc;
        if(oldIdx != neoIdx){
            memcpy(
                (char *)rbuffer->buffer + neoIdx * rbuffer->dsize,
                (char *)rbuffer->buffer + oldIdx * rbuffer->dsize,
                rbuffer->nalloc
            );
        }
    }
}

// -*------------------------------------------------------------*-
// -*- Iterator                                                 -*-
// -*------------------------------------------------------------*-

void iterator_close_noop(Iterator* iter){
    //! @todo
}

// -*-
Iterator iterator_empty(){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator iterator_single_value(void *value){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator iterator_concat(Iterator *iter, size_t n){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator iterator_concat2(Iterator left, Iterator right){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator iterator_concat3(Iterator a, Iterator b, Iterator c){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator iterator_array(void **array, uint32_t len, bool own){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator iterator_from_args(int n, ...){
    //! @todo
    return (Iterator){0};
}

// -*------------------------------------------------------------*-
// -*- HTable                                                   -*-
// -*------------------------------------------------------------*-
// -*-
void htable_init(HTable *htable, HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize){
    //! @todo
}

// -*-
HTable* htable_create(HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize){
    //! @todo
    return NULL;
}

// -*-
void htable_destroy(HTable *htable){
    //! @todo
}

// -*-
void htable_delete(HTable *htable){
    //! @todo
}

// -*-
void htable_insert(HTable *htable, void *key, void *value){
    //! @todo
}

// -*-
void htable_insert_ptr(HTable *htable, void *key, void *value){
    //! @todo
}

// -*-
bool htable_remove(HTable *htable, void *key){
    //! @todo
    return false;
}

// -*-
bool htable_remove_ptr(HTable *htable, void *key){
    //! @todo
    return false;
}

// -*-
void* htable_get(const HTable *htable, void *key){
    //! @todo
    return NULL;
}

// -*-
void* htable_get_ptr(const HTable *htable, void *key){
    //! @todo
    return NULL;
}

// -*-
void* htable_get_key(const HTable *htable, void *key){
    //! @todo
    return NULL;
}

// -*-
void* htable_get_key_ptr(const HTable *htable, void *key){
    //! @todo
    return NULL;
}

// -*-
bool htable_contains(const HTable *htable, void *key){
    //! @todo
    return false;
}

// -*-
bool htable_contains_ptr(const HTable *htable, void *key){
    //! @todo
    return false;
}

// -*-
uint32_t htable_length(const HTable *htable){
    //! @todo
    return 0;
}

//! @note: maybe, the next four (4) function should be private
uint32_t htable_string_hash(void *data){
    //! @todo
    return 0;
}

// -*-
bool htable_string_equal(const void *lhs, const void *rhs){
    //! @todo
    return false;
}

// -*-
bool htable_int_equal(const void *lhs, const void *rhs){
    //! @todo
    return false;
}

//! @note: extension to floating-point numbers
bool htable_float_equal(const void *lhs, const void *rhs){
    //! @todo
    return false;
}

// -*-
Iterator htable_iterator_keys(HTable *htable){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator htable_iterator_keys_ptr(HTable *htable){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator htable_iterator_values(HTable *htable){
    //! @todo
    return (Iterator){0};
}

// -*-
Iterator htable_iterator_values_ptr(HTable *htable){
    //! @todo
    return (Iterator){0};
}