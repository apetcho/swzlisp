#include "swzlisp.h"

// -*------------------------------------------------------------*-
// -*- CharBuffer                                               -*-
// -*------------------------------------------------------------*-

void cbuffer_init(CharBuffer *cbuffer, uint32_t capacity){
    //! @todo
}

// -*-
CharBuffer* cbuffer_create(uint32_t capacity){
    //! @todo
    return NULL;
}

// -*-
void cbuffer_destroy(CharBuffer* cbuffer){
    //! @todo
}

// -*-
void cbuffer_delete(CharBuffer* cbuffer){
    //! @todo
}

// -*-
void cbuffer_concat(CharBuffer* cbuffer, char *cstr){
    //! @todo
}

// -*-
void cbuffer_append(CharBuffer* cbuffer, char c){
    //! @todo
}

// -*-
void cbuffer_trim(CharBuffer* cbuffer){
    //! @todo
}

// -*-
void cbuffer_clear(CharBuffer* cbuffer){
    //! @todo
}

// -*-
void cbuffer_printf(CharBuffer* cbuffer, const char* fmt, ...){
    //! @todo
}

// -*-
void cbuffer_vprintf(CharBuffer* cbuffer, const char* fmt, va_list args){
    //! @todo
}

// -*------------------------------------------------------------*-
// -*- RingBuffer                                               -*-
// -*------------------------------------------------------------*-
// -*-
void rbuffer_init(RingBuffer *rbuffer, uint32_t dsize, int init){
    //! @todo
}

// -*-
void rbuffer_destroy(RingBuffer *rbuffer){
    //! @todo
}

// -*-
void rbuffer_push_front(RingBuffer *rbuffer, void *src){
    //! @todo
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
    //! @todo
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