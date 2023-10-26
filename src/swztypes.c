#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<stdio.h>

#include "swzlisp.h"

#define SWZ_TYPE_HEADER     \
    &_swzType,              \
    NULL,                   \
    'w'

// -*-
static SWZObject* _swz_eval_error(SWZLisp *swz, SWZEnv *env, SWZObject *obj){
    (void)env;
    (void)obj;
    return swz_error(swz, SWZE_EVAL, "cannot evaluate this object");
}

// -*-
static SWZObject* _swz_eval_same(SWZLisp *swz, SWZEnv *env, SWZObject *obj){
    (void)swz;
    (void)env;
    return obj;
}

// -*-
static SWZObject* _swz_call_error(SWZLisp *swz, SWZEnv *env, SWZObject *obj, SWZList *list){
    (void)env;
    (void)obj;
    (void)list;
    return swz_error(swz, SWZE_CALL, "not callable!");
}

// -*-
static void _simple_free(SWZLisp *swz, void *arg){
    (void)swz;
    free(arg);
}

// -*-
static bool _hash_next_index_lt_state(Iterator *iter){
    return iter->index < iter->stateIdx;
}

// -*-----------*-
// -*- SWZType -*-
// -*-----------*-
static void _swz_type_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_type_create(SWZLisp *swz);
static bool _swz_type_compare(SWZObject *self, SWZObject *other);

static SWZType _swzType = {
    SWZ_TYPE_HEADER,
    "type",             // .name
    _swz_type_print,    // .print()
    _swz_type_create,   // .create()
    _simple_free,       // .destroy()
    iterator_empty,     // .expand()
    _swz_eval_error,    // .eval()
    _swz_call_error,    // .call()
    _swz_type_compare,  // .compare()
};

SWZType *swzType = &_swzType;

// -*-
static void _swz_type_print(FILE *stream, SWZObject *obj){
    SWZType *self = (SWZType *)obj;
    fprintf(stream, "%s", self->name);
}

// -*-
static SWZObject *_swz_type_create(SWZLisp *swz){
    SWZType *self = NULL;
    SWZ_UNUSED(swz);
    self = calloc(1, sizeof(*self));
    return (SWZObject *)self;
}

//static bool _swz_type_compare(SWZObject *self, SWZObject *other);