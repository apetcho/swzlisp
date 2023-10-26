#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<float.h>
#include<limits.h>
#include<math.h>
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
static void _swz_simple_destroy(SWZLisp *swz, void *arg){
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
static bool _swz_type_compare(const SWZObject *self, const SWZObject *other);
static Iterator _swz_empty_iterator(SWZObject* obj);

static SWZType _swzType = {
    SWZ_TYPE_HEADER,
    "type",                 // .name
    _swz_type_print,        // .print()
    _swz_type_create,       // .create()
    _swz_simple_destroy,    // .destroy()
    _swz_empty_iterator,    // .iter()
    _swz_eval_error,        // .eval()
    _swz_call_error,        // .call()
    _swz_type_compare,      // .compare()
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
static bool _swz_type_compare(const SWZObject *self, const SWZObject *other){
    if(self->type != other->type || self->type != swzType){
        return false;
    }
    return self == other;
}

static Iterator _swz_empty_iterator(SWZObject* obj){
    SWZ_UNUSED(obj);
    return iterator_empty();
}

// -*----------*-
// -*- SWZEnv -*-
// -*----------*-
static void _swz_env_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_env_create(SWZLisp *swz);
static void _swz_env_destroy(SWZLisp *swz, void *arg);
static Iterator _swz_env_iter(SWZObject *obj);
static bool _swz_env_compare(const SWZObject *self, const SWZObject *other);

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
    env->parent = NULL;
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

// -*-
static void _swz_env_print(FILE *stream, SWZObject *obj){
    SWZEnv *env = (SWZEnv *)obj;
    Iterator iterator = htable_iterator_keys_ptr(&env->scope);
    fprintf(stream, "(scope:");
    while(iterator.has_next(&iterator)){
        SWZObject *key = iterator.next(&iterator);
        SWZObject *value = htable_get_ptr(&env->scope, key);
        fprintf(stream, " ");
        swz_print(stream, key);
        fprintf(stream, ": ");
        swz_print(stream, value);
    }
    fprintf(stream, ")");
}

// -*-
static Iterator _swz_env_iter(SWZObject *obj){
    SWZEnv *env = (SWZEnv *)obj;
    if(env->parent){
        return iterator_concat3(
            iterator_single_value(env->parent),
            htable_iterator_keys_ptr(&env->scope),
            htable_iterator_values_ptr(&env->scope)
        );
    }else{
        return iterator_concat2(
            htable_iterator_keys_ptr(&env->scope),
            htable_iterator_values_ptr(&env->scope)
        );
    }
}

// -*-
static bool _swz_env_compare(const SWZObject *self, const SWZObject *other){
    SWZEnv *lhsEnv = NULL;
    SWZEnv *rhsEnv = NULL;
    SWZSymbol *key = NULL;
    SWZObject *value = NULL;
    SWZObject *rhs = NULL;
    Iterator iterator = {0};

    // - quick checks
    if(self == other){
        return true;
    }
    if(self->type != other->type || self->type != swzEnv){
        return false;
    }
    // -
    lhsEnv = (SWZEnv *)self;
    rhsEnv = (SWZEnv *)other;
    // - now actual equality
    if(lhsEnv->parent && rhsEnv->parent){
        // - both parent non-NULL
        if(!_swz_env_compare((SWZObject*)lhsEnv->parent, (SWZObject*)rhsEnv->parent)){
            return false;
        }
    }else if(lhsEnv->parent || rhsEnv->parent){
        return false;
    }

    // - now equality check of scope contents
    if(htable_length(&lhsEnv->scope) != htable_length(&rhsEnv->scope)){
        return false;
    }

    iterator = htable_iterator_keys_ptr(&lhsEnv->scope);
    while(iterator.has_next(&iterator)){
        key = (SWZSymbol *)iterator.next(&iterator);
        rhs = (SWZObject *)htable_get_ptr(&rhsEnv->scope, key);
        if(!rhs){
            iterator.close(&iterator);
            return false;
        }
        value = (SWZObject *)htable_get_ptr(&lhsEnv->scope, key);
        if(!swz_compare(value, rhs)){
            iterator.close(&iterator);
            return false;
        }
    }

    // -
    iterator.close(&iterator);
    return true;
}

// -*-----------*-
// -*- SWZList -*-
// -*-----------*-
static void _swz_list_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_list_create(SWZLisp *swz);
static SWZObject *_swz_list_eval(SWZLisp *swz, SWZEnv *env, SWZObject *obj);
static Iterator _swz_list_iter(SWZObject *obj);
static bool _swz_list_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzlist = {
    SWZ_TYPE_HEADER,
    "list",                 // .name
    _swz_list_print,        // .print()
    _swz_list_create,       // .create()
    _swz_simple_destroy,    // .destroy()
    _swz_list_iter,         // .iter()
    _swz_list_eval,         // .eval()
    _swz_call_error,        // .call()
    _swz_list_compare,      // .compare()
};

SWZType *swzList = &_swzlist;

// -*-
static SWZObject *_swz_list_eval(SWZLisp *swz, SWZEnv *env, SWZObject *obj){
    SWZObject *callable;
    SWZList *list = (SWZList *)obj;

    if(swz_nil_p(obj)){
        return swz_error(swz, SWZE_CALL, "Cannot call empty list");
    }
    if(list->cdr->type != swzList){
        return swz_error(swz, SWZE_SYNTAY, "unexpected cons cell");
    }
    callable = swz_eval(swz, env, list->car);
    return swz_call(swz, env, callable, (SWZList *)list->cdr);
}

// -*-
static void _swz_list_print_helper(FILE *stream, SWZList *list){
    if(swz_nil_p((SWZObject*)list)){
        return;
    }
    swz_print(stream, list->car);
    if(list->cdr->type != swzList){
        fprintf(stream, " . ");
        swz_print(stream, list->cdr);
        return;
    }else if(!swz_nil_p((SWZObject*)list)){
        fprintf(stream, " ");
        _swz_list_print_helper(stream, (SWZList *)list->cdr);
    }
}
// -*-
static void _swz_list_print(FILE *stream, SWZObject *obj){
    fprintf(stream, "(");
    _swz_list_print_helper(stream, (SWZList *)obj);
    fprintf(stream, ")");
}

// -*-
static SWZObject *_swz_list_create(SWZLisp *swz){
    SWZ_UNUSED(swz);
    SWZList *list = NULL;
    list = calloc(1, sizeof(*list));
    list->car = NULL;
    list->cdr = NULL;
    return (SWZObject *)list;
}

// -*-
bool swz_nil_p(SWZObject *obj){
    return (
        (obj->type == swzList) &&
        (((SWZList*)obj)->car == NULL) &&
        (((SWZList*)obj)->cdr == NULL)
    );
}

// -*-
static void* _swz_list_iter_next(Iterator *iterator){
    SWZList *list = (SWZList *)iterator->data;
    iterator->index++;
    SWZObject *result = NULL;
    switch (iterator->index){
    case 1:
        result = list->car;
        break;
    case 2:
        result = list->cdr;
        break;
    default:
        break;
    }
    return result;
}

// -*-
static bool _swz_list_has_next(Iterator *iterator){
    SWZObject *list = (SWZObject *)iterator->data;
    if(swz_nil_p(list)){
        return true;
    }
    return iterator->index < iterator->stateIdx;
}

// -*-
static Iterator _swz_list_iter(SWZObject *obj){
    Iterator iterator = {0};
    iterator.data = obj;
    iterator.stateIdx = 2;
    iterator.next = _swz_list_iter_next;
    iterator.close = iterator_close_noop;
    iterator.has_next = _swz_list_has_next;
    return iterator;
}

// -*-
static bool _swz_list_compare(const SWZObject *self, const SWZObject *other){
    if(self == other){
        return true;
    }
    if(self->type != other->type || self->type != swzList){
        return false;
    }
    SWZList *lhs = (SWZList *)self;
    SWZList *rhs = (SWZList *)other;
    SWZObject *x = (SWZObject *)self;
    SWZObject *y = (SWZObject *)other;
    if (swz_nil_p(x) && swz_nil_p(y)){
        return true;
    }
    if(swz_nil_p(x) || swz_nil_p(y)){
        return false;
    }
    return (
        swz_compare(lhs->car, rhs->car) && swz_compare(rhs->cdr, rhs->cdr)
    );
}

// -*-------------*-
// -*- SWZSymbol -*-
// -*-------------*-
static void _swz_txt_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_txt_create(SWZLisp *swz);
static SWZObject *_swz_symbol_eval(SWZLisp *swz, SWZEnv *env, SWZObject *obj);
static void _swz_txt_destroy(SWZLisp *swz, void *arg);
static bool _swz_txt_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzsymbol = {
    SWZ_TYPE_HEADER,
    "symbol",               // .name
    _swz_txt_print,         // .print()
    _swz_txt_create,        // .create()
    _swz_txt_destroy,       // .destroy()
    _swz_empty_iterator,    // .iter()
    _swz_symbol_eval,       // .eval()
    _swz_call_error,        // .call()
    _swz_txt_compare,       // .compare()
};

SWZType *swzSymbol = &_swzsymbol;

// -*-
static void _swz_txt_print(FILE *stream, SWZObject *obj){
    struct swztext *text = (struct swztext *)obj;
    fprintf(stream, "%s", text->cstr);
}

// -*-
static SWZObject *_swz_txt_create(SWZLisp *swz){
    struct swztext *text = NULL;
    SWZ_UNUSED(swz);
    text = calloc(1, sizeof(struct swztext));
    text->cstr = NULL;
    text->can_free = 1;
    return (SWZObject *)text;
}

// -*-
static void _swz_txt_destroy(SWZLisp *swz, void *arg){
    struct swztext *text = (struct swztext *)arg;
    if(text->type == swzString && swz->strcache){
        swz_textchach_remove(swz->strcache, text);
    }else if(text->type == swzSymbol && swz->symcache){
        swz_textchach_remove(swz->symcache, text);
    }

    // - deal with the ownership
    if(text->can_free){
        free(text->cstr);
    }
    free(text);
}

// -*-
static SWZObject *_swz_symbol_eval(SWZLisp *swz, SWZEnv *env, SWZObject *obj){
    SWZ_UNUSED(swz);
    SWZSymbol *symbol = NULL;
    symbol = (SWZSymbol *)obj;
    return swz_env_lookup(swz, env, symbol);
}

// -*-
static bool _swz_txt_compare(const SWZObject *self, const SWZObject *other){
    if(self == other){
        return true;
    }
    if(self->type != other->type){
        return false;
    }
    struct swztext *lhs = (struct swztext *)self;
    struct swztext *rhs = (struct swztext *)other;
    return strcmp(lhs->cstr, rhs->cstr) == 0;
}

// -*--------------*-
// -*- SWZInteger -*-
// -*--------------*-
static void _swz_integer_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_integer_create(SWZLisp *swz);
static bool _swz_integer_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzinteger = {
    SWZ_TYPE_HEADER,
    "integer",              // .name
    _swz_integer_print,     // .print()
    _swz_integer_create,    // .create()
    _swz_simple_destroy,    // .destroy()
    _swz_empty_iterator,    // .iter()
    _swz_eval_same,         // .eval()
    _swz_call_error,        // .call()
    _swz_integer_compare,   // .compare()
};

SWZType *swzInteger = &_swzinteger;

// -*-
static void _swz_integer_print(FILE *stream, SWZObject *obj){
    SWZInteger *self = (SWZInteger *)obj;
    fprintf(stream, "%ld", self->val);
}

// -*-
static SWZObject *_swz_integer_create(SWZLisp *swz){
    SWZ_UNUSED(swz);
    SWZInteger *self = calloc(1, sizeof(*self));
    self->val = 0;
    return (SWZObject *)self;
}

// -*-
static bool _swz_is_integer(SWZObject *obj){
    if(!obj){
        return false;
    }
    if(obj->type != swzInteger){
        return false;
    }
    return true;
}

// -*-
static bool _swz_is_float(SWZObject *obj){
    if(!obj){
        return false;
    }
    if(obj->type != swzFloat){
        return false;
    }
    return true;
}

// -*-
static bool _swz_is_number(SWZObject *obj){
    return (_swz_is_integer(obj) || _swz_is_float(obj));
}

// -
static bool _swz_almostEqual(double x, double y){
    static double eps = DBL_EPSILON;
    double diff = fabs(x - y);
    x = fabs(x);
    y = fabs(y);
    double xymax = (x > y) ? x : y;
    bool result = (diff <= xymax*eps) ? true: false;
    return result;
}

// -*-
static bool _swz_num_compare(SWZObject *self, SWZObject *other){
    if(self == other){
        return true;
    }
    if(!_swz_is_number(self)){
        return false;
    }
    if(!_swz_is_number(other)){
        return false;
    }
    if(_swz_is_integer(self)){
        SWZInteger *lhs = (SWZInteger *)self;
        if(_swz_is_integer(other)){
            SWZInteger *rhs = (SWZInteger *)other;
        }else{
            double x = (double)lhs->val;
            double y = ((SWZFloat *)other)->val;
            return _swz_almostEqual(x, y);
        }
    }
    // self is a SWZFloat
    if(_swz_is_integer(other)){
        double x = ((SWZFloat *)self)->val;
        double y = (double)((SWZInteger *)other)->val;
        return _swz_almostEqual(x, y);
    }
    // both are SWZFloat
    double x = ((SWZFloat *)self)->val;
    double y = ((SWZFloat *)other)->val;
    return _swz_almostEqual(x, y);
}

// -*-
static bool _swz_integer_compare(const SWZObject *self, const SWZObject *other){
    return _swz_num_compare((SWZObject*)self, (SWZObject*)other);
}

// -*------------*-
// -*- SWZFloat -*-
// -*------------*-
static void _swz_float_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_float_create(SWZLisp *swz);
static bool _swz_float_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzfloat = {
    SWZ_TYPE_HEADER,
    "float",                // .name
    _swz_float_print,       // .print()
    _swz_float_create,      // .create()
    _swz_simple_destroy,    // .destroy()
    _swz_empty_iterator,    // .iter()
    _swz_eval_same,         // .eval()
    _swz_call_error,        // .call()
    _swz_float_compare,     // .compare()
};

SWZType *swzFloat = &_swzfloat;