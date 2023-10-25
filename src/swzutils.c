#include "swzlisp.h"
#include<stdlib.h>
#include<string.h>
#include<assert.h>

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
    int start;
    start = (rbuffer->start + 1) % rbuffer->nalloc;
    memcpy(dst, (char *)rbuffer->buffer + rbuffer->start * rbuffer->dsize, rbuffer->dsize);
    rbuffer->start = start;
    rbuffer->count--;
}

// -*-
void rbuffer_push_back(RingBuffer *rbuffer, void *src){
    if(rbuffer->count >= rbuffer->nalloc){
        rbuffer_grow(rbuffer);
    }
    int index = (rbuffer->start + rbuffer->count) % rbuffer->nalloc;
    memcpy((char*)rbuffer->buffer + index * rbuffer->dsize, src, rbuffer->dsize);
    rbuffer->count++;
}

// -*-
void rbuffer_pop_back(RingBuffer *rbuffer, void *dst){
    int index = (rbuffer->start + rbuffer->count - 1) % rbuffer->nalloc;
    memcpy(dst, (char *)rbuffer->buffer + index * rbuffer->dsize, rbuffer->dsize);
    rbuffer->count--;
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
    (void)iter;
}

// -*-
static bool _single_value_has_next(Iterator *iter){
    return iter->index == 0;
}

// -*-
static void *_single_value_next(Iterator *iter){
    iter->index++;
    return iter->data;
}


// -*-
Iterator iterator_single_value(void *value){
    Iterator iter = {0};
    iter.data = value;
    iter.index = 0;
    iter.has_next = _single_value_has_next;
    iter.next = _single_value_next;
    iter.close = iterator_close_noop;
    return iter;
}

// -*-
static bool _cc_has_next(Iterator *iter){
    Iterator *iterator = iter->data;
    intptr_t max_iterators = (intptr_t)iter->statePtr;
    bool hasNext;
    while(iter->stateIdx < max_iterators){
        hasNext = iterator[iter->stateIdx].has_next(&iterator[iter->stateIdx]);
        if(hasNext){
            return true;
        }
        iterator[iterator->stateIdx].close(&iterator[iter->stateIdx]);
        iter->stateIdx++;
    }
    return false;
};

// -*-
static void* _cc_next(Iterator *iter){
    Iterator *iterator = iter->data;
    void *result = iterator[iter->stateIdx].next(&iterator[iter->stateIdx]);
    if(result){
        iter->index++;
    }
    return result;
}

// -*-
static void _cc_close(Iterator *iter){
    free(iter->data);
}

// -*-
Iterator iterator_concat(Iterator *iter, size_t n){
    Iterator iterator = {0};
    iterator.data = iter;
    iterator.index = 0;
    iterator.stateIdx = 0;
    iterator.statePtr = (void*)n;
    iterator.has_next = _cc_has_next;
    iterator.next = _cc_next;
    iterator.close = _cc_close;
    return iterator;
}

// -*-
Iterator iterator_concat2(Iterator left, Iterator right){
    Iterator *arr = calloc(2, sizeof(Iterator));
    if(!arr){
        abort();
    }
    arr[0] = left;
    arr[1] = right;
    return iterator_concat(arr, 2);
}

// -*-
Iterator iterator_concat3(Iterator a, Iterator b, Iterator c){
    Iterator *arr = calloc(3, sizeof(*arr));
    arr[0] = a;
    arr[1] = b;
    arr[2] = c;
    return iterator_concat(arr, 3);
}

// -*-
static void* _empty_next(Iterator *iter){
    (void)iter;
    return NULL;
}

// -*-
static bool _empty_has_next(Iterator *iter){
    (void)iter;
    return false;
}

// -*-
Iterator iterator_empty(){
    Iterator iter = {0};
    iter.index = 0;
    iter.next = _empty_next;
    iter.has_next = _empty_has_next;
    iter.close = iterator_close_noop;
    return iter;
}


// -*-
static bool _array_has_next(Iterator *iter){
    return iter->index < iter->stateIdx;
}

// -*-
static void* _array_next(Iterator *iter){
    return ((void **)iter->data)[iter->index++];
}

// -*-
static void _array_close(Iterator *iter){
    if((bool)iter->statePtr){
        free(iter->data);
    }
}

// -*-
Iterator iterator_array(void **array, uint32_t len, bool own){
    Iterator iter = {0};
    iter.index = 0;
    iter.stateIdx = len;
    iter.statePtr = (void *)own;
    iter.has_next = (void *)own;
    iter.next = _array_next;
    iter.has_next = _array_has_next;
    iter.close = _array_close;
    return iter;
}

// -*-
Iterator iterator_from_args(int n, ...){
    void **array = calloc(n, sizeof(void*));
    assert(array);
    va_list args;

    va_start(args, n);
    for (int i = 0; i < n; i++){
        array[i] = va_arg(args, void *);
    }
    va_end(args);

    return iterator_array(array, n, true);
}

// -*------------------------------------------------------------*-
// -*- HTable                                                   -*-
// -*------------------------------------------------------------*-
#define SWZ_HTABLE_KEY_OFFSET       1
#define SWZ_HTABLE_INITIAL_SIZE     31
#define SWZ_HTABLE_MAX_LOAD_FACTOR  0.5
#define SWZ_HTABLE_EMPTY            0
#define SWZ_HTABLE_FULL             1
#define SWZ_HTABLE_GRAVE            2
#define SWZ_NELEM(arg)              (sizeof(arg)/sizeof((arg)[0]))

// --
static const uint64_t htablePrimes[] = {
           31UL,         61UL,        127UL,        257UL,
          509UL,       1021UL,       2053UL,       4093UL,
         8191UL,      16381UL,      32771UL,      65537UL,
       131071UL,     262147UL,     524287UL,    1048573UL, 
      2097143UL,    4194301UL,    8388617UL,   16777213UL,
     33554467UL,   67108859UL,  134217757UL,  268435459UL, 
    536870909UL, 1073741827UL, 2147483647UL, 4294967291UL,
};

// -*-
static uint64_t _binary_search(const uint64_t *array, size_t len, uint64_t value){
    size_t lo = 0;
    size_t hi = len;
    size_t mid;
    while(lo < hi){
        mid = (lo + hi) / 2;
        if(value <= array[mid]){
            hi = mid;
        }else{
            lo = mid + 1;
        }
    }
    return lo;
}

// -*-
static uint64_t _htable_next_size(uint64_t current){
    uint64_t idx = _binary_search(htablePrimes, SWZ_NELEM(htablePrimes), current);
    return htablePrimes[idx + 1];
}

#define SWZ_ITEMSIZE(htable) (SWZ_HTABLE_KEY_OFFSET + (htable)->ksize + (htable)->vsize)
#define SWZ_MARK_AT_BUF(htable, buf, i)     \
    (*(int8_t*)(((char*)buf) + i * SWZ_ITEMSIZE(htable)))
#define SWZ_MARK_AT(htable, i) SWZ_MARK_AT_BUF(htable, htable->table, i)

#define SWZ_KEY_PTR_BUF(htable, buf, i)     \
    (void*)(((char*)buf) + i *SWZ_ITEMSIZE(htable) + SWZ_HTABLE_KEY_OFFSET)
#define SWZ_KEY_PTR(htable, i) SWZ_KEY_PTR_BUF(htable, (htable)->table, i)

#define SWZ_VALUE_PTR_BUF(htable, buf, i)               \
    (void*)(((char*)buf) + i * SWZ_ITEMSIZE(htable) +   \
    SWZ_HTABLE_KEY_OFFSET + (htable)->ksize)

#define SWZ_VALUE_PTR(htable, i)    SWZ_VALUE_PTR_BUF(htable, (htable)->table, i)


// -*-
static uint32_t _htable_find_insert(const HTable *htable, void *key){
    uint64_t index = htable->hashfn(key) % htable->allocated;
    uint32_t j = 1;
    /*
    Continue searching until we either find a non-nil full slot, o we find the
    key we're try to insert:
    unitl (cell.mark != full || cell.key == key)
    while (cell.mark == full && cell.key != key)
    */
    while(SWZ_MARK_AT(htable, index)==SWZ_HTABLE_FULL &&
        htable->equalfn(key, SWZ_KEY_PTR(htable, index)) != 0){
        // -
        index = (index + j) % htable->allocated;
        j += 2;
    }

    return index;
}

// -*-
static uint32_t _htable_find_retrieve(const HTable *htable, void *key){
    uint64_t index = htable->hashfn(key) % htable->allocated;
    uint32_t j = 1;

    while(SWZ_MARK_AT(htable, index) != SWZ_HTABLE_EMPTY){
        if(SWZ_MARK_AT(htable, index)==SWZ_HTABLE_FULL &&
            htable->equalfn(key, SWZ_KEY_PTR(htable, index)) == 0){
            // -
            return index;
        }
        index = (index + j) % htable->allocated;
        j += 2;
    }
    return index;
}

// -*-
static void _htable_resize(HTable *htable){
    void *table;
    uint64_t index, allocated;
    table = htable->table;
    allocated = htable->allocated;
    htable->len = 0;
    htable->allocated = _htable_next_size(allocated);
    htable->table = calloc(htable->allocated, SWZ_ITEMSIZE(htable));
    for (index = 0; index < allocated; index++){
        if(SWZ_MARK_AT_BUF(htable, table, index)==SWZ_HTABLE_FULL){
            htable_insert(
                htable,
                SWZ_KEY_PTR_BUF(htable, table, index),
                SWZ_VALUE_PTR_BUF(htable, table, index)
            );
        }
    }
    free(table);
}

// -*-
static double _htable_load_factor(HTable *htable){
    return ((double)htable->len) / ((double)htable->allocated);
}

// -*- Find the proper index for insertion into the table
void htable_init(HTable *htable, HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize){
    // - Initialize values
    htable->len = 0;
    htable->allocated = SWZ_HTABLE_INITIAL_SIZE;
    htable->ksize = ksize;
    htable->vsize = vsize;
    htable->hashfn = hashfn;
    htable->equalfn = equalfn;
    // -*- allocate table
    htable->table = calloc(SWZ_HTABLE_INITIAL_SIZE, SWZ_ITEMSIZE(htable));
}

// -*-
HTable* htable_create(HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize){
    HTable *htable = NULL;
    htable = calloc(1, sizeof(*htable));
    htable_init(htable, hashfn, equalfn, ksize, vsize);
    return htable;
}

// -*-
void htable_destroy(HTable *htable){
    free(htable->table);
}

// -*-
void htable_delete(HTable *htable){
    if(!htable){
        return;
    }
    htable_destroy(htable);
    free(htable);
    htable = NULL;
}

// -*-
void htable_insert(HTable *htable, void *key, void *value){
    uint64_t index;
    if(_htable_load_factor(htable) > SWZ_HTABLE_MAX_LOAD_FACTOR){
        _htable_resize(htable);
    }
    index = _htable_find_retrieve(htable, key);
    if(SWZ_MARK_AT(htable, index)==SWZ_HTABLE_FULL){
        memcpy(SWZ_VALUE_PTR(htable, index), value, htable->vsize);
        return;
    }
    index = _htable_find_insert(htable, key);
    SWZ_MARK_AT(htable, index) = SWZ_HTABLE_FULL;
    memcpy(SWZ_KEY_PTR(htable, index), key, htable->ksize);
    memcpy(SWZ_VALUE_PTR(htable, index), value, htable->vsize);
    htable->len++;
}

// -*-
void htable_insert_ptr(HTable *htable, void *key, void *value){
    htable_insert(htable, &key, &value);
}

// -*-
int htable_remove(HTable *htable, void *key){
    uint64_t index = _htable_find_retrieve(htable, key);
    if(SWZ_MARK_AT(htable, index) != SWZ_HTABLE_FULL){
        return -1;
    }
    SWZ_MARK_AT(htable, index) = SWZ_HTABLE_GRAVE;
    htable->len--;
    return 0;
}

// -*-
int htable_remove_ptr(HTable *htable, void *key){
    return htable_remove(htable, key);
}

// -*-
void* htable_get(const HTable *htable, void *key){
    uint64_t index = _htable_find_retrieve(htable, key);
    if(SWZ_MARK_AT(htable, index) != SWZ_HTABLE_FULL){
        return NULL;
    }
    return SWZ_VALUE_PTR(htable, index);
}

// -*-
void* htable_get_ptr(const HTable *htable, void *key){
    void **result = htable_get(htable, &key);
    if(!result){
        return NULL;
    }
    return *result;
}

// -*-
void* htable_get_key(const HTable *htable, void *key){
    uint64_t index = _htable_find_retrieve(htable, key);
    if(SWZ_MARK_AT(htable, index) != SWZ_HTABLE_FULL){
        return NULL;
    }
    return SWZ_KEY_PTR(htable, index);
}

// -*-
void* htable_get_key_ptr(const HTable *htable, void *key){
    void **result = htable_get_key(htable, &key);
    if(!result){
        return NULL;
    }
    return *result;
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