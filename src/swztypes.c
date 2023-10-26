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
static Iterator _swz_type_iterator(SWZObject* obj);

static SWZType _swzType = {
    SWZ_TYPE_HEADER,
    "type",             // .name
    _swz_type_print,    // .print()
    _swz_type_create,   // .create()
    _simple_free,       // .destroy()
    _swz_type_iterator, // .iter()
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

// -*-
static bool _swz_type_compare(SWZObject *self, SWZObject *other){
    if(self->type != other->type || self->type != swzType){
        return false;
    }
    return self == other;
}

static Iterator _swz_type_iterator(SWZObject* obj){
    SWZ_UNUSED(obj);
    return (Iterator){0};
}

// -*----------*-
// -*- SWZEnv -*-
// -*----------*-
static void _swz_env_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_env_create(SWZLisp *swz);
static void _swz_env_destroy(SWZLisp *swz, void *arg);
static Iterator _swz_env_iter(SWZObject *obj);
static bool _swz_env_compare(SWZObject *self, SWZObject *other);

static SWZType _swzenv = {
    SWZ_TYPE_HEADER,
    "env",              // .name
    _swz_env_print,     // .print()
    _swz_env_create,    // .create()
    _swz_env_destroy,   // .destroy()
    _swz_env_iter,      // .iter()
    _swz_eval_error,    // .eval()
    _swz_call_error,    // .call()
    _swz_env_compare,   // .compare()
};

SWZType *swzEnv = &_swzenv;

// -*-
static uint32_t _swz_text_hash(void *arg){
    struct swztext **text = (struct swztext **)arg;
    return htable_string_hash(&(*text)->cstr);
}

// -*-
static bool _swz_text_compare(const void *lhs, const void *rhs){
    SWZSymbol *sym1 = *(SWZSymbol **)lhs;
    SWZSymbol *sym2 = *(SWZSymbol **)rhs;
    return strcmp(sym1->cstr, sym2->cstr) == 0;
}

// -*-
static SWZObject *_swz_env_create(SWZLisp *swz){
    SWZ_UNUSED(swz);
    SWZEnv *env = NULL;
    env = calloc(1, sizeof(*env));
    env->up = NULL;
    htable_init(
        &env->scope,
        _swz_text_hash,
        _swz_text_compare,
        sizeof(void *),
        sizeof(void *)
    );

    return (SWZObject *)env;
}

static void _swz_env_destroy(SWZLisp *swz, void *arg){
    SWZ_UNUSED(swz);
    SWZEnv *env = NULL;
    env = (SWZEnv *)arg;
    htable_destroy(&env->scope);
    free(env);
}

static void _swz_env_print(FILE *stream, SWZObject *obj);
static Iterator _swz_env_iter(SWZObject *obj);
static bool _swz_env_compare(SWZObject *self, SWZObject *other);